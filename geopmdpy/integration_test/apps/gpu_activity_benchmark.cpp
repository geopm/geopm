//
//  Copyright (c) 2015 - 2026 Intel Corporation
//  SPDX-License-Identifier: BSD-3-Clause
//
// GPU activity benchmark: a small SYCL workload generator used by the GPU
// activity agent integration test.  It emits a single "FOM (<unit>): <n>"
// line so the test can compare throughput across agent phi values.  Three
// profiles exercise different GPU activity regimes:
//   * steady  - back-to-back compute kernels that keep the GPU saturated
//               (control case: the agent has no stalls to exploit).
//   * serving - the same compute kernel but with an idle gap after each
//               request to hit a target duty cycle (activity oscillates).
//   * decode  - a memory-bandwidth-bound autoregressive proxy that looks
//               busy but leaves the compute engine only partially active.
//
// The default sizes below are generic tuning knobs chosen to produce
// measurable, steady activity on a data-center GPU; they are NOT tied to any
// specific device's memory capacity or PVC in particular.  Every one of them
// is overridable via CLI flags and the GEOPM_GPU_* environment variables, so
// no source change is required to retarget different hardware.

#include <sycl/sycl.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
    // Workload configuration.  Defaults are generic starting points; override
    // via CLI flags or GEOPM_GPU_* environment variables for other hardware.
    struct Args {
        std::string profile = "steady";
        double duration = 60.0;
        double duty_cycle = 0.5;     // serving profile active fraction
        int batch_size = 64;         // FoM scaling factor for image profiles
        std::size_t hidden = 8192;   // decode hidden dimension (per-layer width)
        std::size_t layers = 8;      // decode layer count (working-set depth)
        std::size_t elements = 1 << 20; // steady/serving vector length
        int compute_intensity = 2048;   // FMAs per element per compute kernel
        int decode_intensity = 1;       // memory passes per decode token
    };

    double now_sec(void)
    {
        using clock = std::chrono::steady_clock;
        return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
    }

    std::size_t env_size(const char *name, std::size_t default_value)
    {
        const char *value = std::getenv(name);
        return value == nullptr ? default_value : std::stoull(value);
    }

    int env_int(const char *name, int default_value)
    {
        const char *value = std::getenv(name);
        return value == nullptr ? default_value : std::stoi(value);
    }

    Args parse_args(int argc, char **argv)
    {
        Args result;
        result.elements = env_size("GEOPM_GPU_BENCH_ELEMENTS", result.elements);
        result.hidden = env_size("GEOPM_GPU_DECODE_HIDDEN", result.hidden);
        result.layers = env_size("GEOPM_GPU_DECODE_LAYERS", result.layers);
        result.compute_intensity = env_int("GEOPM_GPU_COMPUTE_INTENSITY",
                                           result.compute_intensity);
        result.decode_intensity = env_int("GEOPM_GPU_DECODE_INTENSITY",
                                          result.decode_intensity);

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            auto need_value = [&](const std::string &name) -> const char * {
                if (i + 1 >= argc) {
                    throw std::runtime_error(name + " requires a value");
                }
                return argv[++i];
            };
            if (arg == "--profile" || arg == "--mode") {
                result.profile = need_value(arg);
            }
            else if (arg == "--duration") {
                result.duration = std::stod(need_value(arg));
            }
            else if (arg == "--duty-cycle") {
                result.duty_cycle = std::stod(need_value(arg));
            }
            else if (arg == "--batch-size") {
                result.batch_size = std::stoi(need_value(arg));
            }
            else if (arg == "--hidden") {
                result.hidden = std::stoull(need_value(arg));
            }
            else if (arg == "--layers") {
                result.layers = std::stoull(need_value(arg));
            }
            else if (arg == "--elements") {
                result.elements = std::stoull(need_value(arg));
            }
            else if (arg == "--compute-intensity") {
                result.compute_intensity = std::stoi(need_value(arg));
            }
            else if (arg == "--decode-intensity") {
                result.decode_intensity = std::stoi(need_value(arg));
            }
            else if (arg == "--device") {
                // Accepted for CLI compatibility with the Python drivers.  The
                // SYCL queue below always selects a GPU.
                (void) need_value(arg);
            }
            else if (arg == "--help" || arg == "-h") {
                std::cout
                    << "usage: gpu_activity_benchmark [options]\n\n"
                    << "  --profile steady|serving|decode   workload profile\n"
                    << "  --duration SEC                    timed run length\n"
                    << "  --duty-cycle FRACTION             serving active fraction\n"
                    << "  --batch-size N                    FoM scaling for image profiles\n"
                    << "  --hidden N --layers N             decode working-set controls\n"
                    << "  --elements N                      steady/serving vector length\n"
                    << "  --compute-intensity N             FMAs per element\n"
                    << "  --decode-intensity N              memory passes per decode token\n";
                std::exit(0);
            }
            else {
                throw std::runtime_error("unknown argument: " + arg);
            }
        }

        if (result.profile != "steady" && result.profile != "serving" &&
            result.profile != "decode") {
            throw std::runtime_error("--profile must be steady, serving, or decode");
        }
        if (result.duration <= 0.0) {
            throw std::runtime_error("--duration must be positive");
        }
        if (!(result.duty_cycle > 0.0 && result.duty_cycle <= 1.0)) {
            throw std::runtime_error("--duty-cycle must be in (0, 1]");
        }
        if (result.compute_intensity < 1 || result.decode_intensity < 1) {
            throw std::runtime_error("kernel intensities must be positive");
        }
        return result;
    }

    sycl::event submit_compute(sycl::queue &queue, float *data,
                               std::size_t elements, int intensity)
    {
        return queue.submit([&](sycl::handler &handler) {
            handler.parallel_for(sycl::range<1>(elements), [=](sycl::id<1> idx) {
                std::size_t i = idx[0];
                float x = data[i];
                for (int repeat = 0; repeat < intensity; ++repeat) {
                    x = sycl::fma(x, 1.0000001f, 0.0000001f);
                    x = sycl::fma(x, 0.9999999f, 0.0000002f);
                }
                data[i] = x;
            });
        });
    }

    sycl::event submit_decode(sycl::queue &queue, const float *weights,
                              float *state, std::size_t elements,
                              std::size_t state_elements, int intensity,
                              std::uint64_t token)
    {
        return queue.submit([&](sycl::handler &handler) {
            handler.parallel_for(sycl::range<1>(elements), [=](sycl::id<1> idx) {
                std::size_t i = idx[0];
                float x = weights[(i + token) % elements];
                for (int repeat = 0; repeat < intensity; ++repeat) {
                    x = sycl::fma(x, 1.0001f, state[(i + repeat) % state_elements]);
                }
                state[i % state_elements] = x * 0.000001f;
            });
        });
    }

    void initialize(sycl::queue &queue, float *data, std::size_t elements)
    {
        queue.submit([&](sycl::handler &handler) {
            handler.parallel_for(sycl::range<1>(elements), [=](sycl::id<1> idx) {
                std::size_t i = idx[0];
                data[i] = static_cast<float>((i % 1024) + 1) * 0.0001f;
            });
        }).wait();
    }

    int run_compute_profile(sycl::queue &queue, const Args &args)
    {
        // steady/serving: repeatedly launch a compute-bound kernel over a
        // single device vector.  "serving" sleeps after each request to hold a
        // target duty cycle, creating the idle gaps the agent exploits;
        // "steady" runs back-to-back to keep the GPU saturated.
        float *data = sycl::malloc_device<float>(args.elements, queue);
        if (data == nullptr) {
            throw std::runtime_error("failed to allocate device buffer");
        }
        initialize(queue, data, args.elements);

        std::uint64_t iterations = 0;
        double start = now_sec();
        if (args.profile == "serving") {
            double idle_ratio = (1.0 - args.duty_cycle) / args.duty_cycle;
            while (now_sec() - start < args.duration) {
                double request_start = now_sec();
                submit_compute(queue, data, args.elements, args.compute_intensity).wait();
                double latency = now_sec() - request_start;
                ++iterations;
                if (idle_ratio > 0.0) {
                    std::this_thread::sleep_for(
                        std::chrono::duration<double>(latency * idle_ratio));
                }
            }
        }
        else {
            while (now_sec() - start < args.duration) {
                submit_compute(queue, data, args.elements, args.compute_intensity).wait();
                ++iterations;
            }
        }
        double elapsed = now_sec() - start;
        sycl::free(data, queue);

        double fom = (static_cast<double>(iterations) * args.batch_size) / elapsed;
        std::cout << "FOM (images/sec): " << fom << "\n";
        return 0;
    }

    int run_decode_profile(sycl::queue &queue, const Args &args)
    {
        // decode: an autoregressive proxy.  Each token reads a large weight
        // buffer (hidden x layers) and mutates a small recurrent state, so the
        // kernel is memory-bandwidth bound and the compute engine is only
        // partially active -- the regime where the agent lowers frequency with
        // little throughput loss.
        std::size_t elements = args.hidden * args.layers;
        std::size_t state_elements = args.hidden;
        float *weights = sycl::malloc_device<float>(elements, queue);
        float *state = sycl::malloc_device<float>(state_elements, queue);
        if (weights == nullptr || state == nullptr) {
            throw std::runtime_error("failed to allocate decode device buffers");
        }
        initialize(queue, weights, elements);
        initialize(queue, state, state_elements);

        std::uint64_t tokens = 0;
        double start = now_sec();
        while (now_sec() - start < args.duration) {
            submit_decode(queue, weights, state, elements, state_elements,
                          args.decode_intensity, tokens).wait();
            ++tokens;
        }
        double elapsed = now_sec() - start;
        sycl::free(weights, queue);
        sycl::free(state, queue);

        double fom = static_cast<double>(tokens) / elapsed;
        std::cout << "FOM (tokens/sec): " << fom << "\n";
        return 0;
    }
}

int main(int argc, char **argv)
{
    try {
        Args args = parse_args(argc, argv);
        sycl::queue queue{sycl::gpu_selector_v};
        std::cerr << "GPU activity benchmark device: "
                  << queue.get_device().get_info<sycl::info::device::name>()
                  << "\n";
        if (args.profile == "decode") {
            return run_decode_profile(queue, args);
        }
        return run_compute_profile(queue, args);
    }
    catch (const std::exception &ex) {
        std::cerr << "gpu_activity_benchmark error: " << ex.what() << "\n";
        return 1;
    }
}
