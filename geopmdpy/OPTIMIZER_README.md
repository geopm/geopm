# Bayesian Optimizer for GEOPM Control Parameters

The Bayesian optimizer is a tool that uses Bayesian optimization to find
optimal control parameter settings for applications. It integrates with the
ControlGrid functionality to define the parameter space and uses subprocess
execution to evaluate different configurations.

## Features

- **Bayesian Optimization**: Uses scikit-optimize for intelligent parameter space exploration
- **ControlGrid Integration**: Leverages existing grid functionality for parameter space definition
- **Flexible Application Evaluation**: Supports any application that can be launched via subprocess
- **Regex-based Metric Extraction**: Uses Python-style regular expressions to extract performance metrics
- **Configurable Timeouts**: Customizable application execution timeouts
- **Comprehensive Logging**: Tracks optimization progress and evaluation history
- **Debug Output**: Optional application stdout logging for troubleshooting

## Requirements

- Python 3.7+
- GEOPM Python package (geopmdpy) with the optional `optimize` dependencies
  (`scikit-optimize` and `pyyaml`): `python3 -m pip install 'geopmdpy[optimize]'`

## Usage

### Basic Command Line Usage

```bash
geopmopt \
    --cpu-frequency package \
    --metric-regex "Performance: ([0-9.]+)" \
    --trials 50 \
    --output-file geopmwrite.conf \
    -- ./run.sh
```

### Command Line Arguments

#### Control Parameters
All control parameters from ControlGrid are supported:
- `--cpu-frequency DOMAIN`: CPU frequency control
- `--cpu-uncore-frequency DOMAIN`: CPU uncore frequency control
- `--cpu-power DOMAIN`: CPU power limit control
- `--gpu-frequency DOMAIN`: GPU frequency control
- `--gpu-uncore-frequency DOMAIN`: GPU uncore frequency control
- `--gpu-power DOMAIN`: GPU power control

#### Optimization Options
- `--trials N`: Number of optimization iterations (default: 50)
- `--n-initial-points N`: Number of random initial evaluations (default: 10)
- `--metric-regex PATTERN`: Python-style regex to extract metric from stdout (required)
- `--minimize`: Minimize the metric instead of maximizing
- `--random-seed SEED`: Random seed for reproducibility (default: 42)
- `--application-timeout SECONDS`: Timeout in seconds for application execution (default: 300)
- `--output-file FILE`: Output file for best configuration (default: stdout)
- `--verbosity LEVEL`: Verbosity level 0-3 (default: 1)
- `--print-stdout`: Print application stdout to info level log (default: False)
- `--efficiency`: **Optimize for efficiency** by dividing the metric by the average power consumed (see below)

#### Application Command
- `COMMAND...`: Command and arguments to launch the application (after `--`)

### Examples

#### Optimize CPU Frequency for a Benchmark
```bash
geopmopt \
    --cpu-frequency package \
    --metric-regex "GFLOPS: ([0-9.]+)" \
    --trials 30 \
    -- ./stream_benchmark
```

#### Optimize Multiple Parameters with Custom Timeout and Debug Output
```bash
geopmopt \
    --cpu-frequency package \
    --cpu-power board \
    --metric-regex "Performance: ([0-9.]+)" \
    --trials 100 \
    --application-timeout 600 \
    --verbosity 2 \
    --print-stdout \
    --output-file geopmwrite.config \
    -- python3 my_ml_training.py
```

#### Minimize Energy Consumption with Full Debug Output
```bash
geopmopt \
    --cpu-frequency package \
    --metric-regex "Energy: ([0-9.]+) Joules" \
    --minimize \
    --trials 40 \
    --verbosity 3 \
    --print-stdout \
    -- ./energy_intensive_app
```

#### Quick Test Run with Minimal Output
```bash
geopmopt \
    --cpu-frequency package \
    --metric-regex "Score: ([0-9.]+)" \
    --trials 10 \
    --verbosity 0 \
    -- ./quick_benchmark
```

## Programming Interface

### ApplicationEvaluator Class

```python
from geopmdpy.optimizer import ApplicationEvaluator

evaluator = ApplicationEvaluator(
    launch_command=["./my_app", "arg1", "arg2"],
    metric_regex="Performance: ([0-9.]+)",
    maximize=True,      # Set to False for minimization
    timeout=300,        # Application timeout in seconds
    print_stdout=False  # Set to True to log application output
)

# Evaluate a specific configuration
control_grid = ControlGrid(['--cpu-frequency', 'package'])
coordinate = [2]  # Use appropriate coordinate for your grid
metric = evaluator.evaluate(control_grid, coordinate)
```

### BayesianOptimizer Class

```python
from geopmdpy.optimizer import BayesianOptimizer
from geopmdpy.grid import ControlGrid

# Create control grid
grid = ControlGrid(['--cpu-frequency', 'package'])

# Create evaluator
evaluator = ApplicationEvaluator(
    launch_command=["./benchmark"],
    metric_regex="Score: ([0-9.]+)",
    maximize=True,
    timeout=600,
    print_stdout=True
)

# Run optimization
optimizer = BayesianOptimizer(grid, evaluator)
result = optimizer.optimize(n_calls=50)

print(f"Best metric: {result['best_metric']}")
print(f"Best config: {result['best_config']}")
```

## Metric Extraction

The optimizer uses Python regular expressions to extract performance metrics from application output:

```bash
--metric-regex "Performance: ([0-9.]+)"
```

The regex should capture the numeric metric value in a group. Examples:
- `"GFLOPS: ([0-9.]+)"` - Extracts floating point after "GFLOPS: "
- `"Time: ([0-9]+) seconds"` - Extracts integer time value
- `"Score: ([0-9]*\.?[0-9]+)"` - Extracts decimal numbers with optional decimal point

## Configuration Application

The optimizer applies configurations using the GEOPM PIO interface:

1. **Push Controls**: Each parameter is pushed to the system
2. **Adjust Values**: Parameter values are set based on optimization coordinates
3. **Write Batch**: All changes are applied atomically
4. **Launch Application**: The application runs with the new configuration
5. **Extract Metric**: Performance metric is parsed from application output

## Error Handling

The optimizer handles various failure modes:

- **Timeout**: Applications that exceed the specified timeout are terminated
- **Configuration Errors**: Invalid control parameters are caught early
- **Regex Failures**: Missing or invalid metric patterns are reported
- **Application Failures**: Non-zero exit codes and stderr output are captured

## Best Practices

1. **Start Small**: Begin with `--trials 20` to test your setup
2. **Validate Regex**: Test your metric regex on sample application output
3. **Choose Appropriate Domains**: Match control domains to distinct application behavior
4. **Monitor Progress**: Watch the console output to verify convergence
5. **Use Seeds**: Set `--random-seed` for reproducible experiments
6. **Baseline First**: Run without optimization to establish baseline metrics
7. **Set Appropriate Timeouts**: Use `--application-timeout` for long-running applications
8. **Debug Systematically**: Use `--print-stdout` and higher verbosity levels when troubleshooting

## Troubleshooting

### Common Issues

1. **Import Error**: Install the optimize extra: `python3 -m pip install 'geopmdpy[optimize]'`
2. **No Metric Found**: Check that your regex matches the application output
3. **Configuration Failures**: Verify GEOPM service is running and accessible
4. **Slow Convergence**: Increase `--trials` or check parameter ranges
5. **Timeout Issues**: Adjust `--application-timeout` for your application's runtime

### Debug Mode

Set the `GEOPM_DEBUG` environment variable to see detailed error information:

```bash
GEOPM_DEBUG=1 geopmopt ...
```

### Verbosity Levels

Control logging output with `--verbosity`:
- `0`: ERROR - Only critical errors
- `1`: WARNING - Errors and warnings (default)
- `2`: INFO - Progress information and results
- `3`: DEBUG - Detailed debugging information

### Application Output Debugging

Use `--print-stdout` to see application output in the logs:

```bash
geopmopt \
    --cpu-frequency package \
    --metric-regex "Performance: ([0-9.]+)" \
    --verbosity 2 \
    --print-stdout \
    -- ./my_app
```

This is particularly useful for:
- Verifying your regex pattern matches the actual output
- Debugging application execution issues
- Understanding why metric extraction is failing

## Integration with Existing Tools

The optimizer integrates seamlessly with existing GEOPM tools:

- **ControlGrid**: Reuses parameter space definitions
- **geopmsession**: Similar launch mechanism for applications
- **geopmwrite**: Uses same control interface for parameter application
- **GEOPM Service**: Leverages existing PIO infrastructure

This allows the optimizer to work with any application and parameter combination supported by the GEOPM ecosystem.

## Efficiency Optimization (`--efficiency`)

When the `--efficiency` flag is specified, the optimizer will **divide the extracted metric by the average power consumed** during the application's execution. This is useful for finding configurations that maximize performance per watt (or minimize energy per operation), rather than just raw performance.

- **How it works:**  
  The optimizer measures the system's energy usage before and after each application run, calculates the average power, and divides the metric by this value.
- **Use case:**  
  Useful for energy-constrained environments, green computing, or when you want to maximize efficiency rather than absolute performance.

**Example:**
```bash
geopmopt \
    --cpu-frequency package \
    --metric-regex "GFLOPS: ([0-9.]+)" \
    --trials 30 \
    --efficiency \
    -- ./stream_benchmark
```
