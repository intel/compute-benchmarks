#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import subprocess
import os
import re
import sys
import argparse

HEADER_PATTERN = "TestCase,Mean,Median,StdDev,Min,Max,Type"
AVAILABLE_COLUMNS = ["Mean", "Median", "StdDev", "Min", "Max"]

def ensure_directory_exists(directory):
    """
    Ensure the output directory exists. If it exists, inform the user and ask if a new directory with a timestamp should be created.
    """
    if os.path.exists(directory):
        print(f"Warning: The directory '{directory}' already exists. Plots may overwrite existing files.")
        response = input("Do you want to create a new directory with a timestamp instead? [y/N]: ").strip().lower()
        if response == 'y':
            timestamp = pd.Timestamp.now().strftime('%Y%m%d_%H%M%S')
            directory = f"{directory}_{timestamp}"
            print(f"Creating new directory: {directory}")
            os.makedirs(directory, exist_ok=True)
            return directory
        else:
            print(f"Using existing directory: {directory}")
            return directory
    else:
        os.makedirs(directory)
        print(f"Created directory: {directory}")
        return directory

def get_unique_filename(base_name: str) -> str:
    """Generate a unique filename with the current timestamp."""
    timestamp = pd.Timestamp.now().strftime('%Y%m%d_%H%M%S')
    return f"{base_name}_{timestamp}"

def split_test_case(test_case: str) -> tuple:
    """Split test case into test name and parameters."""
    match = re.match(r"(\w+)\((.*)\)", test_case)
    if match:
        return match.group(1), match.group(2)
    return test_case, ""

def split_params(compare_params: str) -> dict:
    """Split parameters provided in --compare_params into a dictionary."""
    params_dict = {}
    if compare_params:
        for param in compare_params.split(' '):
            if '=' in param:
                key, value = param.split('=', 1)
                params_dict[key.strip()] = value.strip()
            else:
                print(f"Warning: Invalid parameter format '{param}'. Expected 'key=value'.")
    return params_dict

def get_column_unit(test_data: pd.DataFrame, column: str):
    if column == 'StdDev':
        return '[%]'
    if 'Label [unit]' in test_data.columns and not test_data['Label [unit]'].isna().all():
        return f"{test_data['Label [unit]'].iloc[0]}"
    return ''

class TestGroup:
    def __init__(self, test_name: str, group_params, group_data: pd.DataFrame):
        self.test_name = test_name
        self.group_params = group_params
        self.group_data = group_data

    @staticmethod
    def get_groups(group_by_type, params_compare, test_data: pd.DataFrame):
        """Divides test entries into groups based on test name and parameters."""
        test_data[['TestName', 'TestParams']] = test_data['TestCase'].apply(lambda x: pd.Series(split_test_case(x)))
        test_data = test_data.drop(columns=['TestCase'])

        if params_compare:
            param_names = [param.split('=')[0] for param in params_compare if '=' in param]
            if len(set(param_names)) != 1:
                print("Error: All parameters must have the same name to compare them.")
                sys.exit(1)

        test_groups = []
        for _, row in test_data.iterrows():
            compare_params_dict = split_params(row['TestParams'])
            group_params = TestGroup._get_group_params(group_by_type, params_compare, compare_params_dict)
            if group_params is not None:
                existing_group = next((g for g in test_groups if g.test_name == row['TestName'] and g.group_params == group_params), None)
                row_df = pd.DataFrame([row])
                if existing_group:
                    existing_group.group_data = pd.concat([existing_group.group_data, row_df], ignore_index=True)
                else:
                    test_groups.append(TestGroup(row['TestName'], group_params, row_df))
        print(f"Found {len(test_groups)} test groups based on the provided parameters.")
        return test_groups

    @staticmethod
    def _get_group_params(group_by_type, params_compare, compare_params_dict):
        """
        Determine the parameters to group by based on the command line arguments.
        If only group_by_type is set, return an empty list - all tests with the same name will be grouped together.
        If params_compare is set, return the parameters that match the compare_params_dict.
        If both are set, return the parameters that match the compare_params_dict.
        If neither is set, return all parameters as a list of strings.
        """
        if group_by_type and not params_compare:
            return []
        elif group_by_type and params_compare:
            for param in params_compare:
                param_name, param_value = param.split('=')
                if param_name in compare_params_dict and param_value == compare_params_dict[param_name]:
                    return f"{param_name}={compare_params_dict[param_name]}"
        elif not group_by_type and params_compare:
            for param in params_compare:
                param_name, param_value = param.split('=')
                if param_name in compare_params_dict and param_value == compare_params_dict[param_name]:
                    return [f"{key}={value}" for key, value in compare_params_dict.items()]
        else:
            return [f"{key}={value}" for key, value in compare_params_dict.items()]

    def get_unique_params(self) -> set:
        return set(self.group_data['TestParams'])

class BenchmarkVisualizer:
    FIGURE_SIZE = (10, 8)
    PADDING_SIZE = 0.1
    MARKER_ALPHA = 0.6
    GRID_ALPHA = 0.5
    TITLE_FONT_SIZE = 16
    SUBTITLE_FONT_SIZE = 12

    def __init__(self, command=None, csv_file=None, columns=['Mean'], group_by_type=False, output_dir='plots', plot_line=False, show_output=False, compare_params=None):
        self.command = command
        self.csv_file = csv_file
        self.columns = columns
        self.group_by_type = group_by_type
        self.output_dir = output_dir
        self.plot_line = plot_line
        self.show_output = show_output
        self.compare_params = compare_params if isinstance(compare_params, list) else [compare_params] if compare_params else []
        self.test_data: pd.DataFrame = None

    def run(self):
        csv_output_filename = self._prepare_csv()
        self._load_and_validate_data(csv_output_filename)
        self._plot_results()

    def _prepare_csv(self):
        if self.command and self.csv_file:
            print("Warning: Both --command and --csv_file are provided. The script will use the CSV file and ignore the command.")
        if self.csv_file:
            if not os.path.isfile(self.csv_file):
                print(f"Error: The specified CSV file '{self.csv_file}' does not exist.")
                sys.exit(1)
            return self.csv_file
        elif self.command:
            csv_output_filename = get_unique_filename('output') + '.csv'
            full_output_filename = get_unique_filename('full_output') + '.txt'
            log_filename = get_unique_filename('errors') + '.log'
            self.command = self._modify_command()
            csv_output = self._execute_command(full_output_filename, log_filename)
            self._save_csv_output(csv_output, csv_output_filename, log_filename)
            print(f"Command executed successfully. Output saved to {csv_output_filename}")
            print(f"Full output saved to {full_output_filename}")
            return csv_output_filename
        else:
            print("Error: Either --command or --csv_file must be specified.")
            sys.exit(1)

    def _modify_command(self):
        if '--gtest_repeat' not in self.command:
            self.command += ' --gtest_repeat=1'
            print("Warning: '--gtest_repeat' not found in command. Defaulting to 1 run.")
        if '--csv' not in self.command:
            self.command += ' --csv'
            print("Warning: '--csv' not found in command. Adding it to save output to CSV.")
        return self.command

    def _execute_command(self, full_output_filename, log_filename):
        try:
            process = subprocess.Popen(self.command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            full_output, csv_output = [], []
            print("Executing command: ", self.command)
            while True:
                line = process.stdout.readline()
                if line:
                    if self.show_output:
                        print(line.strip())
                    full_output.append(line.strip())
                    if HEADER_PATTERN in line or len(line.strip().split(',')) >= len(HEADER_PATTERN.split(',')):
                        csv_output.append(line.strip())
                if process.poll() is not None:
                    break
            stderr = process.stderr.read()
            if stderr:
                print("Error:", stderr.strip())
                full_output.append(stderr.strip())
                with open(log_filename, 'a') as log_file:
                    log_file.write(stderr.strip() + '\n')
            with open(full_output_filename, 'w') as file:
                file.write('\n'.join(full_output))
            return '\n'.join(csv_output)
        except Exception as e:
            print(f"An error occurred while executing the command: {e}")
            sys.exit(1)

    def _save_csv_output(self, output, csv_output_filename, log_filename):
        data_lines, error_lines = [], []
        lines = output.split('\n')
        header_found = False
        for line in lines:
            if not header_found and HEADER_PATTERN in line:
                header_found = True
                if ',Label [unit]' not in line:
                    line += ',Label [unit]'
                data_lines.append(line)
            elif header_found:
                columns = line.split(',')
                if len(columns) == len(data_lines[0].split(',')):
                    data_lines.append(line)
                else:
                    error_lines.append(line)
        if not header_found or not data_lines:
            print("Error: Header or valid data not found in the output. Please check the command output.")
            sys.exit(1)
        with open(csv_output_filename, 'w') as file:
            file.write('\n'.join(data_lines))
        if error_lines:
            with open(log_filename, 'a') as log_file:
                log_file.write('\n'.join(error_lines) + '\n')

    def _load_and_validate_data(self, csv_output_filename: str):
        """Load and check if the CSV file is valid and if user-specified columns are available."""
        try:
            self.test_data = pd.read_csv(csv_output_filename)
            if len(self.columns) > 2:
                print("Error: You can specify either one or two columns for plotting.")
                sys.exit(1)
            for col in self.columns:
                if col not in AVAILABLE_COLUMNS:
                    print(f"Warning: Column '{col}' is not a valid column.")
                    print(f"Available columns: {AVAILABLE_COLUMNS}")
                    sys.exit(1)
        except FileNotFoundError:
            print(f"The file {csv_output_filename} was not found. Please check the file path.")
            sys.exit(1)
        except pd.errors.EmptyDataError:
            print(f"The file {csv_output_filename} is empty. Please check the file content.")
            sys.exit(1)
        except Exception as e:
            print(f"An error occurred while reading the CSV file: {e}")
            sys.exit(1)

    def _plot_results(self):
        if self.group_by_type and len(self.columns) == 1:
            print("Warning: --group_by_type is set, but only one column is specified. Grouping by type will be ignored.")
            self.group_by_type = False
            
        self.output_dir = ensure_directory_exists(self.output_dir)
        test_groups = TestGroup.get_groups(
            group_by_type=self.group_by_type,
            params_compare=[param.strip() for param in ",".join(self.compare_params).split(",")] if self.compare_params else [],
            test_data=self.test_data
        )
        axis_limits = self._get_axis_limits(test_groups)
        if self.compare_params and self.group_by_type:
            self._plot_by_test_name(test_groups, axis_limits)
        elif self.compare_params and not self.group_by_type:
            self._plot_by_params(test_groups, axis_limits)
        else:
            self._plot_individual(test_groups, axis_limits)

    def _plot_by_test_name(self, test_groups: list[TestGroup], axis_limits: dict):
        """Create a single plot for each test name with all its groups."""
        test_name_groups = {}
        for group in test_groups:
            test_name_groups.setdefault(group.test_name, []).append(group)
        for test_name, groups in test_name_groups.items():
            colors = plt.get_cmap('viridis', len(groups))
            plt.figure(figsize=self.FIGURE_SIZE)
            axes = plt.gca()
            for i, group in enumerate(groups):
                self._create_plot(group, axis_limits[group.test_name], axes, color=colors(i))
            file_name = f"{test_name}_{'_'.join(self.compare_params)}".replace(' ', '_').replace(',', '_')
            plt.savefig(os.path.join(self.output_dir, f"{file_name}.png"))
            plt.close()

    def _plot_by_params(self, test_groups, axis_limits):
        """Create a single plot for each unique combination of parameters."""
        test_params_groups = {}
        compare_param_names = [param.split('=')[0] for param in self.compare_params]
        for group in test_groups:
            params = tuple(param for param in group.group_params if param.split('=')[0] not in compare_param_names)
            test_params_groups.setdefault(params, []).append(group)
        for params, groups in test_params_groups.items():
            colors = plt.get_cmap('viridis', len(groups))
            plt.figure(figsize=self.FIGURE_SIZE)
            axes = plt.gca()
            plt.title(f"{', '.join(params)}", fontsize=self.SUBTITLE_FONT_SIZE)
            for i, group in enumerate(groups):
                self._create_plot(group, axis_limits[group.test_name], axes, color=colors(i))
            file_name = f"{groups[0].test_name}_{'_'.join(params)}".replace(' ', '_').replace(',', '_')
            plt.savefig(os.path.join(self.output_dir, f"{file_name}.png"))
            plt.close()

    def _plot_individual(self, test_groups, axis_limits):
        """Create a single plot for each test group."""
        for group in test_groups:
            plt.figure(figsize=self.FIGURE_SIZE)
            axes = plt.gca()
            plt.title(f"{', '.join(group.group_params)}", fontsize=self.SUBTITLE_FONT_SIZE)
            self._create_plot(group, axis_limits[group.test_name], axes)
            if self.group_by_type:
                file_name = f"{group.test_name}"
            else:
                file_name = (f"{group.test_name}_{'_'.join(group.group_params)}").replace(' ', '_').replace(',', '_')
            plt.savefig(os.path.join(self.output_dir, f"{file_name}.png"))
            plt.close()

    def _create_plot(self, test_group: TestGroup, axis_limits: dict, axes, color='blue'):
        plt.suptitle(f"{test_group.test_name}", fontsize=self.TITLE_FONT_SIZE, fontweight='bold')
        if len(self.columns) == 1:
            self._plot_linear(axes, test_group.group_data, axis_limits['y_min'], axis_limits['y_max'], color)
        elif len(self.columns) == 2:
            self._plot_scatter(axes, test_group, axis_limits, color)
        else:
            raise ValueError("Invalid number of columns for plotting.")

    def _plot_linear(self, axes, group_data: pd.DataFrame, y_axis_min, y_axis_max, color='blue'):
        label = self._get_param_label(group_data) if self.compare_params else None
        x = np.arange(1, len(group_data) + 1)
        axes.plot(
            x,
            group_data[self.columns[0]],
            marker='o',
            color=color,
            alpha=self.MARKER_ALPHA,
            label=label
        )
        if label:
            axes.legend()
        unit = get_column_unit(group_data, self.columns[0])
        axes.set_xlabel('Execution')
        axes.set_ylabel(f"{self.columns[0]} {unit}" if unit else self.columns[0])
        axes.set_ylim(
            y_axis_min - self.PADDING_SIZE * abs(y_axis_min),
            y_axis_max + self.PADDING_SIZE * abs(y_axis_max)
        )
        axes.grid(alpha=self.GRID_ALPHA)

    def _plot_scatter(self, axes, test_group: TestGroup, axis_limits: dict, color='blue'):
        if self.plot_line:
            min_limit = min(axis_limits['x_min'], axis_limits['y_min'])
            max_limit = max(axis_limits['x_max'], axis_limits['y_max'])
            pad_min = min_limit - self.PADDING_SIZE * abs(min_limit)
            pad_max = max_limit + self.PADDING_SIZE * abs(max_limit)
            axes.plot([pad_min, pad_max], [pad_min, pad_max], color='red', linestyle='--', label='x=y')
        
        if self.group_by_type and not self.compare_params:
            unique_params = test_group.group_data['TestParams'].unique()
            cmap = plt.get_cmap('tab20', len(unique_params))
            for idx, param in enumerate(unique_params):
                group_data = test_group.group_data[test_group.group_data['TestParams'] == param]
                if not group_data.empty:
                    axes.scatter(
                        group_data[self.columns[0]],
                        group_data[self.columns[1]],
                        color=cmap(idx),
                        alpha=self.MARKER_ALPHA
                    )
        else:
            label = self._get_param_label(test_group.group_data) if self.compare_params else None
            axes.scatter(
                test_group.group_data[self.columns[0]],
                test_group.group_data[self.columns[1]],
                label=label,
                alpha=self.MARKER_ALPHA,
                color=color
            )
            if label:
                axes.legend()

        unit_x = get_column_unit(test_group.group_data, self.columns[0])
        unit_y = get_column_unit(test_group.group_data, self.columns[1])
        axes.set_xlabel(f"{self.columns[0]} {unit_x}" if unit_x else self.columns[0])
        axes.set_ylabel(f"{self.columns[1]} {unit_y}" if unit_y else self.columns[1])
        axes.set_xlim(
            axis_limits['x_min'] - self.PADDING_SIZE * abs(axis_limits['x_min']),
            axis_limits['x_max'] + self.PADDING_SIZE * abs(axis_limits['x_max'])
        )
        axes.set_ylim(
            axis_limits['y_min'] - self.PADDING_SIZE * abs(axis_limits['y_min']),
            axis_limits['y_max'] + self.PADDING_SIZE * abs(axis_limits['y_max'])
        )
        axes.grid(alpha=self.GRID_ALPHA)

    def _get_param_label(self, group_data: pd.DataFrame) -> str:
        """Return the compare_param and its value for this group."""
        if not self.compare_params or group_data.empty:
            return ""
        param_name = self.compare_params[0].split('=')[0]
        params_dict = split_params(group_data.iloc[0]['TestParams'])
        if param_name in params_dict:
            return f"{param_name}={params_dict[param_name]}"
        return ""

    def _get_axis_limits(self, test_groups: list[TestGroup]) -> dict:
        """
        Calculate axis limits for each test name so that all plots for the same test
        share consistent axis scales.
        """
        tests_axis_limits = {}
        test_name_groups = {}
        for group in test_groups:
            test_name_groups.setdefault(group.test_name, []).append(group.group_data)
        for test_name, group_data_list in test_name_groups.items():
            x_axis_min, x_axis_max = float('inf'), float('-inf')
            y_axis_min, y_axis_max = float('inf'), float('-inf')
            for group_data in group_data_list:
                if len(self.columns) == 1:
                    y_min, y_max = group_data[self.columns[0]].min(), group_data[self.columns[0]].max()
                    y_axis_min = min(y_axis_min, y_min)
                    y_axis_max = max(y_axis_max, y_max)
                elif len(self.columns) == 2:
                    x_min, x_max = group_data[self.columns[0]].min(), group_data[self.columns[0]].max()
                    y_min, y_max = group_data[self.columns[1]].min(), group_data[self.columns[1]].max()
                    x_axis_min = min(x_axis_min, x_min)
                    x_axis_max = max(x_axis_max, x_max)
                    y_axis_min = min(y_axis_min, y_min)
                    y_axis_max = max(y_axis_max, y_max)
            tests_axis_limits[test_name] = {
                'x_min': x_axis_min, 'x_max': x_axis_max,
                'y_min': y_axis_min, 'y_max': y_axis_max
            }
        return tests_axis_limits

def main():
    parser = argparse.ArgumentParser(
        description=(
            "Benchmark Results Comparison Tool\n\n"
            "This tool is designed for executing a test command and visualizing results from output or a CSV file.\n"
            "This function parses command-line arguments to either:\n"
            "1. Run a specified test command, capture its output, and save results to a CSV file.\n"
            "2. Load an existing CSV file for plotting.\n"
            "It then generates plots based on the specified columns, with options to group by test case type,\n"
            "customize output directory, and add an x=y reference line to scatter plots.\n\n"
            "Usage examples:\n"
            "---------------\n"
            "# Run a test command and plot 'Mean' vs 'Median' columns:\n"
            "$ ./benchmark_visualize --command \"./api_overhead_benchmark_l0 --gtest_filter='*EventCreation*'\" Mean Median\n"
            "# Run a test command and group by test case type/name:\n"
            "$ ./benchmark_visualize --command \"./api_overhead_benchmark_l0 --gtest_filter='*EventCreation*'\" --group_by_type\n"
            "# Run a test command, save plots to a custom directory, and add x=y line to the scatter plot:\n"
            "$ ./benchmark_visualize --command \"./api_overhead_benchmark_l0 --gtest_filter='*EventCreation*'\" Min Max --output_dir myplots --plot_line\n"
            "# Run a test command and filter by parameters:\n"
            "$ ./benchmark_visualize --command \"./api_overhead_benchmark_l0 --gtest_filter='*EventCreation*'\" --group_by_type --compare 'signal=device,signal=none'\n"
            "# Run a test command, comparing specific parameters across 15 executions:\n"
            "$ ./benchmark_visualize --command \"./api_overhead_benchmark_l0 --gtest_filter='*EventCreation*' --gtest_repeat=15\" --compare 'signal=device,signal=none'\n"
            "# Plot from an existing CSV file:\n"
            "$ ./benchmark_visualize --csv_file output.csv Mean Median\n"
        ),
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument('--command', type=str, help='Command to execute the test (default: 1 run if "--gtest_repeat" not set in --command section). Required unless --csv_file is specified')
    parser.add_argument('--csv_file', type=str, help='Path to an existing CSV file for plotting.')
    parser.add_argument('columns', nargs='*', default=['Mean'], help='Columns to use for plotting (default: Mean). Specify one or two.')
    parser.add_argument('--group_by_type', action='store_true', help='Group plots by TestCase type, ignoring parameters.')
    parser.add_argument('--output_dir', type=str, default='plots', help='Directory to save plots (default: plots).')
    parser.add_argument('--plot_line', action='store_true', help='Add x=y line to scatter plots.')
    parser.add_argument('--show_output', action='store_true', help='Show command execution output in the console.')
    parser.add_argument('--compare', type=str, default=None, help='Parameters to filter by. Specify one parameters with different values in format "param=value1,param=value2".\n')

    args = parser.parse_args()
    visualizer = BenchmarkVisualizer(
        command=args.command,
        csv_file=args.csv_file,
        columns=args.columns,
        group_by_type=args.group_by_type,
        output_dir=args.output_dir,
        plot_line=args.plot_line,
        show_output=args.show_output,
        compare_params=args.compare
    )
    visualizer.run()

if __name__ == '__main__':
    main()
