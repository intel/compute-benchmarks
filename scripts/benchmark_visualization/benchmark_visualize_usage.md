# Benchmark Visualization Tool Usage Guide

## Overview

The `benchmark_visualize.py` script is a tool designed for executing benchmark tests and visualizing their results. It can either run test commands and capture their output or process existing CSV files containing benchmark results to generate visualizations.

## Features

- **Command Execution**: Run benchmark commands and automatically capture CSV output
- **CSV Processing**: Load and visualize existing CSV files with benchmark results
- **Plotting**: Support for both linear plots (single column) and scatter plots (two columns)
- **Grouping Options**: Group results by test case type or individual test cases

## Prerequisites

### Required Python Packages
```bash
pip install numpy pandas matplotlib
```

### Expected CSV Format
The tool expects CSV data with the following header:
```
TestCase,Mean,Median,StdDev,Min,Max,Type,Label [unit]
```
The last column can be omitted since it is not always generated. In this case, the script automatically adds the column name.

## Command Line Usage

### Basic Syntax
```bash
python benchmark_visualize.py [OPTIONS] [COLUMNS]
```

### Arguments

#### Required (one of the following):
- `--command`: Command to execute the test
- `--csv_file`: Path to an existing CSV file with test results

#### Optional Positional Arguments:
- `columns`: Column names to use for plotting (default: `Mean`)
  - Specify 1 column for linear plots
  - Specify 2 columns for scatter plots

#### Optional Flags:
- `--group_by_type`: Group plots by test case type, ignoring parameters
- `--output_dir`: Directory to save plots (default: `plots`)
- `--plot_line`: Add x=y reference line to scatter plots
- `--show_output`: Display command output in the console
- `--compare`: Parameters to filter and compare. Specify parameters with different values in format `"param=value1,param=value2"`

## Usage Examples

There are no restrictions on the commands (e.g., which types of gtest arguments will be used) passed to the "--command" parameter.

It's recommended to use the `--gtest_repeat` flag, especially when creating linear plots, since the plot shows the number of times a given test case has been executed on the X-axis.

### 1. Execute Command and Generate Plots

#### Basic command execution with default columns (Min vs Max):
```bash
python benchmark_visualize.py --command "./benchmark --gtest_filter='*EventTime*'"
```

#### Execute command with specific columns:
```bash
python benchmark_visualize.py --command "./benchmark --gtest_filter='*EventTime*'" Mean Median
```

#### Execute with custom output directory and show console output:
```bash
python benchmark_visualize.py --command "./my_benchmark" --output_dir results --show_output
```

#### Complete example with all options:
```bash
python benchmark_visualize.py \
  --command "./benchmark --gtest_repeat=10 --gtest_filter='*EventTime*'" \
  Mean Median \
  --group_by_type \
  --output_dir memory_analysis \
  --plot_line \
  --show_output
```

### 2. Process Existing CSV Files

#### Plot from existing CSV with default settings:
```bash
python benchmark_visualize.py --csv_file results.csv
```

#### Plot specific columns from CSV:
```bash
python benchmark_visualize.py --csv_file benchmark_results.csv Mean StdDev
```

#### Group by test type with custom output directory:
```bash
python benchmark_visualize.py --csv_file results.csv --group_by_type --output_dir analysis_plots
```

### 3. Parameter Comparison Examples

#### Compare specific parameter values:
```bash
python benchmark_visualize.py --csv_file results.csv --compare 'signal=device,signal=none'
```

#### Compare parameters with grouped visualization:
```bash
python benchmark_visualize.py --csv_file results.csv --group_by_type --compare 'signal=device,signal=none'
```

#### Single column comparison across parameter values:
```bash
python benchmark_visualize.py --csv_file results.csv Mean --compare 'signal=device,signal=none'
```

#### Single column linear plot:
```bash
python benchmark_visualize.py --csv_file results.csv Mean
```

#### Scatter plot with x=y reference line:
```bash
python benchmark_visualize.py --csv_file results.csv Min Max --plot_line
```

## Available Columns

The tool supports the following standard benchmark columns:
- `Mean`: Average execution time/value (default column)
- `Median`: Median execution time/value
- `StdDev`: Standard deviation (displayed as percentage)
- `Min`: Minimum execution time/value
- `Max`: Maximum execution time/value

## Parameter Comparison with --compare

The `--compare` parameter allows you to filter and compare specific parameter values across test cases.

### Usage Format:
```bash
--compare 'param=value1,param=value2'
```

### Key Features:
- **Parameter Filtering**: Only test cases matching the specified parameter values will be included
- **Multiple Values**: Compare different values of the same parameter


## Output Files

### When executing commands:
- **CSV file**: `output_YYYYMMDD_HHMMSS.csv` - Parsed benchmark results
- **Full output**: `full_output_YYYYMMDD_HHMMSS.txt` - Complete command output
- **Error log**: `errors_YYYYMMDD_HHMMSS.log` - Error messages and warnings

### Plot files:
- **Location**: Specified output directory (default: `plots/`)
- **Format**: PNG images
- **Naming**: Based on test case names and parameters (special characters replaced with underscores)
- **Directory Management**: If output directory exists, user is prompted to create timestamped directory or overwrite

## Plot Types

### Linear Plots (Single Column)
- **X-axis**: Execution number (1, 2, 3, ...)
- **Y-axis**: Selected metric value
- **With --compare**: Different parameter values shown as different colored lines with labels

### Scatter Plots (Two Columns)
- **X-axis**: First specified column
- **Y-axis**: Second specified column
- **Optional**: x=y reference line with `--plot_line` for comparison analysis
- **With --compare**: Different parameter values shown as different colored points with labels

## Grouping Behavior

### Default Grouping (Individual Test Cases):
- Each unique test case (including all parameters) gets its own plot
- Example: `MyTest(param1=1,param2=A)` and `MyTest(param1=1,param2=B)` generate separate plots

### Type Grouping (--group_by_type):
- Test cases with the same base name are grouped together
- Different parameter combinations shown as different colored points/lines on the same plot
- Example: All `MyTest` variants appear on one plot with different colors for each parameter set

### Parameter Comparison Grouping (--compare):
- **With --group_by_type**: Creates one plot per test name, comparing specified parameter values
- **Without --group_by_type**: Creates plots for each unique combination of non-compared parameters
- Only test cases matching the specified parameter values are included

## Support and Contact

If you encounter any issues while using this tool or have suggestions for improvements, feel free to reach out to alicja.lukaszewicz@intel.com.