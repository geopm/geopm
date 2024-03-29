/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>

using testing::EmptyTestEventListener;
using testing::UnitTest;
using testing::TestInfo;

class TapListener : public EmptyTestEventListener
{
    public:
        TapListener(std::shared_ptr<std::ostream> tap_stream, bool do_print_yaml)
            : m_tap_stream_ptr(std::move(tap_stream))
            , m_tap_stream(m_tap_stream_ptr ? *m_tap_stream_ptr : std::cout)
            , m_do_print_yaml(do_print_yaml)
        {
        }

        void OnTestProgramStart(const UnitTest &unit_test) override
        {
            m_tap_stream << "TAP version 14\n"
                         << "1.." << unit_test.test_to_run_count() << std::endl;
        }

        void OnTestEnd(const TestInfo &test_info) override
        {
            const auto& result = *test_info.result();

            // Test status. I.e., pass/fail state.
            if (result.Failed()) {
                m_tap_stream << "not ok";
            }
            else {
                m_tap_stream << "ok";
            }

            // Test description. Brief context about pass/fail state, such as name.
            m_tap_stream << " - " << test_info.test_suite_name() << "::" << test_info.name()
                         << " (" << result.elapsed_time() << " ms)";

            // Test directive. Special notes (only TODO and SKIP as of TAP v14)
            if (result.Skipped()) {
                m_tap_stream << " # SKIP";
            }
            m_tap_stream << "\n";

            if (result.Failed() && m_do_print_yaml) {
                // Optional YAML diagnostic info (Since TAP v13)
                m_tap_stream << "  ---\n";
                for (int i = 0; i < result.total_part_count(); ++i)
                {
                    const auto &test_part_result = result.GetTestPartResult(i);
                    if (test_part_result.failed()) {
                        std::string summary = test_part_result.summary();
                        std::string indented_summary = std::regex_replace(
                                summary, std::regex("\n"), "\n    ");
                        m_tap_stream
                            << "  message: " << std::quoted(indented_summary) << "\n"
                            << "  severity: fail\n"
                            << "  at:\n"
                            << "    file: " << test_part_result.file_name() << "\n"
                            << "    line: " << test_part_result.line_number() << "\n";
                    }
                }
                m_tap_stream << "  ...\n";
            }

            // Flush now in case the TAP report is being followed for progress updates
            m_tap_stream << std::flush;
        }
    private:
        std::shared_ptr<std::ostream> m_tap_stream_ptr;
        std::ostream& m_tap_stream;
        bool m_do_print_yaml;
};

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    // Gtest removes known args from argv and decrements argc accordingly.
    // Let's handle the leftovers.
    std::shared_ptr<std::ostream> tap_out_stream = nullptr;
    bool do_tap = false;
    bool do_print_yaml = false;

    for (auto arg = argv; arg != argv + argc; ++arg) {
        std::string arg_str(*arg);
        if (arg_str == "--tap-out-path") {
            // --tap-out-path <PATH_TO_TAP_OUTPUT>
            if (arg + 1 == (argv + argc)) {
                std::cerr << "Error: Expect either \"-\" or a path to the tap output destination after " << *arg << ".";
                exit(1);
            }
            arg += 1;
            std::string tap_out_path = *arg;
            do_tap = true;
            if (tap_out_path != "-") {
                tap_out_stream.reset(new std::ofstream(tap_out_path));
            }
        } else if (arg_str == "--include-yaml") {
            // --include-yaml is a flag indicating whether to include TAP13
            // YAML diagnostic info on failed tests.
            do_print_yaml = true;
        }
    }

    testing::TestEventListeners &listeners = testing::UnitTest::GetInstance()->listeners();

    if (do_tap) {
        listeners.Append(new TapListener(tap_out_stream, do_print_yaml));
    }

    auto rc = RUN_ALL_TESTS();
    // Let the TAP states indicate test success/failure
    return do_tap ? 0 : rc;
}
