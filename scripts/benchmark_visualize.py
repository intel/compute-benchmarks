#
# Copyright (C) 2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import subprocess
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import argparse
import re
import os
import sys

HEADER_PATTERN = "TestCase,Mean,Median,StdDev,Min,Max,Type"


def modify_command(command):
    if "--gtest_repeat" not in command:
        command += " --gtest_repeat=1"
        print("Warning: '--gtest_repeat' not found in command. Defaulting to 1 run.")
    if "--csv" not in command:
        command += " --csv"
        print("Warning: '--csv' not found in command. Adding it to save output to CSV.")
    return command


def execute_command(command, full_output_filename, log_filename, show_output):
    try:
        process = subprocess.Popen(
            command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        full_output = []
        csv_output = []

        print("Executing command: ", command)
        while True:
            line = process.stdout.readline()
            if line:
                if show_output:
                    print(line.strip())
                full_output.append(line.strip())
                if "TestCase,Mean,Median,StdDev,Min,Max,Type" in line:
                    csv_output.append(line.strip())
                elif len(line.strip().split(",")) >= len(HEADER_PATTERN.split(",")):
                    csv_output.append(line.strip())
            if process.poll() is not None:
                break

        stderr = process.stderr.read()
        if stderr:
            print("Error:", stderr.strip())
            full_output.append(stderr.strip())
            with open(log_filename, "a") as log_file:
                log_file.write(stderr.strip() + "\n")

        with open(full_output_filename, "w") as file:
            file.write("\n".join(full_output))

        return "\n".join(csv_output)

    except Exception as e:
        print(f"An error occurred while executing the command: {e}")
        sys.exit(1)


def save_csv_output(output, csv_output_filename, log_filename):
    data_lines = []
    error_lines = []

    lines = output.split("\n")
    header_found = False

    for line in lines:
        if not header_found:
            if HEADER_PATTERN in line:
                header_found = True
                if not ",Label [unit]" in line:
                    line += ",Label [unit]"
                data_lines.append(line)
        elif header_found:
            columns = line.split(",")
            if len(columns) == len(data_lines[0].split(",")):
                data_lines.append(line)
            else:
                error_lines.append(line)

    if not header_found:
        print("Error: Header not found in the output. Please check the command output.")
        sys.exit(1)
    if not data_lines:
        print(
            "Error: No valid data found in the output. Please check the command output."
        )
        sys.exit(1)

    with open(csv_output_filename, "w") as file:
        file.write("\n".join(data_lines))

    if error_lines:
        with open(log_filename, "a") as log_file:
            log_file.write("\n".join(error_lines) + "\n")


def split_testcase(testcase):
    match = re.match(r"(\w+)\((.*)\)", testcase)
    if match:
        test_name = match.group(1)
        test_params = match.group(2)
    else:
        test_name = testcase
        test_params = ""
    return test_name, test_params


def get_unique_filename(base_name):
    timestamp = pd.Timestamp.now().strftime("%Y%m%d_%H%M%S")
    return f"{base_name}_{timestamp}"


def get_unit(col, df):
    if col == "StdDev":
        return "[%]"
    if "Label [unit]" in df.columns:
        return str(df["Label [unit]"].iloc[-1])
    return ""


def plot_data(df, columns, group_by_type, output_dir, plot_x_equals_y):
    try:
        df[["TestName", "TestParams"]] = df["TestCase"].apply(
            lambda x: pd.Series(split_testcase(x))
        )
        group_col = "TestName" if group_by_type else "TestCase"
        grouped = df.groupby(group_col)

        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        for col in columns:
            if col == "StdDev":
                df[col] = df[col].str.replace("%", "", regex=False)

        global_x_min, global_x_max = float("inf"), float("-inf")
        global_y_min, global_y_max = float("inf"), float("-inf")

        for name, group in grouped:
            if len(columns) == 1:
                y_min, y_max = group[columns[0]].min(), group[columns[0]].max()
                global_y_min = min(global_y_min, y_min)
                global_y_max = max(global_y_max, y_max)
            elif len(columns) == 2:
                x_min, x_max = group[columns[0]].min(), group[columns[0]].max()
                y_min, y_max = group[columns[1]].min(), group[columns[1]].max()
                global_x_min = min(global_x_min, x_min)
                global_x_max = max(global_x_max, x_max)
                global_y_min = min(global_y_min, y_min)
                global_y_max = max(global_y_max, y_max)

        for name, group in grouped:
            print(f"Creating plot for {name}")

            plt.figure(figsize=(10, 8))
            ax = plt.gca()
            plt.suptitle(f"{group['TestName'].iloc[0]}", fontsize=18, fontweight="bold")
            if len(columns) == 1:
                execution_numbers = [f"{i+1}" for i in range(len(group))]
                ax.plot(execution_numbers, group[columns[0]], marker="o", color="blue")
                plt.title(f"{group['TestParams'].iloc[0]}")
                unit = get_unit(columns[0], df)
                plt.xlabel("Execution")
                plt.ylabel(f"{columns[0]} {unit}" if unit else columns[0])
                ax.set_ylim(
                    global_y_min - 0.1 * abs(global_y_min),
                    global_y_max + 0.1 * abs(global_y_max),
                )
                ax.grid(True, alpha=0.5)
            elif len(columns) == 2:
                unit_x = get_unit(columns[0], df)
                unit_y = get_unit(columns[1], df)
                plt.xlabel(f"{columns[0]} {unit_x}" if unit_x else columns[0])
                plt.ylabel(f"{columns[1]} {unit_y}" if unit_y else columns[1])
                ax.set_xlim(
                    global_x_min - 0.1 * abs(global_x_min),
                    global_x_max + 0.1 * abs(global_x_max),
                )
                ax.set_ylim(
                    global_y_min - 0.1 * abs(global_y_min),
                    global_y_max + 0.1 * abs(global_y_max),
                )
                ax.grid(True, alpha=0.5)

                if plot_x_equals_y:
                    min_limit = min(global_x_min, global_y_min) - 0.1 * abs(
                        min(global_x_min, global_y_min)
                    )
                    max_limit = max(global_x_max, global_y_max) + 0.1 * abs(
                        max(global_x_max, global_y_max)
                    )
                    ax.plot(
                        [min_limit, max_limit],
                        [min_limit, max_limit],
                        color="red",
                        linestyle="--",
                        label="x=y",
                    )
                    ax.legend()

                if group_by_type:
                    unique_params = group["TestParams"].unique()
                    colors = plt.cm.tab20(np.linspace(0, 1, len(unique_params)))
                    for i, param in enumerate(unique_params):
                        param_group = group[group["TestParams"] == param]
                        ax.scatter(
                            param_group[columns[0]],
                            param_group[columns[1]],
                            color=colors[i],
                            alpha=0.8,
                        )
                else:
                    plt.title(f"{group['TestParams'].iloc[0]}", fontsize=14)
                    ax.scatter(
                        group[columns[0]], group[columns[1]], color="blue", alpha=0.7
                    )
            else:
                raise ValueError("Invalid number of columns for plotting.")

            plt.savefig(os.path.join(output_dir, f"{name.replace('/', '_')}.png"))
            plt.close()

    except KeyError as e:
        print(f"Column error: {e}. Please ensure the columns exist in the CSV.")
        print(f"Available columns: {list(df.columns)}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred while plotting data: {e}")
        sys.exit(1)


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
            "Arguments are parsed from the command line:\n"
            "- --command: Command to execute the test (required unless --csv_file is specified).\n"
            "  Any additional gtest_options can be passed as part of this command string.\n"
            "- --csv_file: Path to an existing CSV file with test results.\n"
            "- columns: Names of columns to use for plotting (default: Min and Max). Can be either one or two columns.\n"
            "  If one column is specified, it will be plotted against its execution iteration - linear plot.\n"
            "  If two columns are specified, they will be plotted against each other - scatter plot.\n"
            "- --group_by_type: If set, group plots by test case type, ignoring parameters.\n"
            "- --output_dir: Directory to save plots (default: 'plots').\n"
            "- --plot_x_equals_y: If set, add an x=y line to scatter plots.\n\n"
            "Usage examples:\n"
            "---------------\n"
            "# Run a test command and plot 'Mean' vs 'Median' columns:\n"
            "$ ./benchmark_visualize --command \"./benchmark --gtest_filter='*MyTest*'\" Mean Median\n"
            "# Plot from an existing CSV file, grouping by test case type:\n"
            "$ ./benchmark_visualize --csv_file results.csv --group_by_type\n"
            "# Plot from CSV, save plots to a custom directory, and add x=y line:\n"
            "$ ./benchmark_visualize --csv_file results.csv Min Max --output_dir myplots --plot_x_equals_y\n"
        ),
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "--command",
        type=str,
        help='Command to execute the test (default: 1 run if "--gtest_repeat" not set in --command section). Required unless --csv_file is specified',
    )
    parser.add_argument(
        "--csv_file", type=str, help="Path to an existing CSV file for plotting."
    )
    parser.add_argument(
        "columns",
        nargs="*",
        default=["Min", "Max"],
        help="Positional argument: columns to use for plotting (default: Min and Max). Specify one or two column names separated by space.",
    )
    parser.add_argument(
        "--group_by_type",
        action="store_true",
        help="Group plots by TestCase type, ignoring parameters.",
    )
    parser.add_argument(
        "--output_dir",
        type=str,
        default="plots",
        help="Directory to save plots (default: plots).",
    )
    parser.add_argument(
        "--plot_x_equals_y", action="store_true", help="Add x=y line to scatter plots."
    )
    parser.add_argument(
        "--show_output", action="store_true", help="Show command output in the console."
    )

    args = parser.parse_args()

    if args.command and args.csv_file:
        print(
            "Warning: Both --command and --csv_file are provided. The script will use the CSV file and ignore the command."
        )
    if args.csv_file:
        csv_output_filename = args.csv_file
        if not os.path.isfile(csv_output_filename):
            print(
                f"Error: The specified CSV file '{csv_output_filename}' does not exist."
            )
            sys.exit(1)
    elif args.command:
        csv_output_filename = get_unique_filename("output") + ".csv"
        full_output_filename = get_unique_filename("full_output") + ".txt"
        log_filename = get_unique_filename("errors") + ".log"

        modified_command = modify_command(args.command)
        csv_output = execute_command(
            modified_command, full_output_filename, log_filename, args.show_output
        )
        save_csv_output(csv_output, csv_output_filename, log_filename)
        print(f"Command executed successfully. Output saved to {csv_output_filename}")
        print(f"Full output saved to {full_output_filename}")
    else:
        print("Error: Either --command or --csv_file must be specified.")
        sys.exit(1)

    try:
        df = pd.read_csv(csv_output_filename)

        available_columns = ["Mean", "Median", "StdDev", "Min", "Max"]
        if len(args.columns) > 2:
            print("Error: You can specify either one or two columns for plotting.")
            sys.exit(1)
        for col in args.columns:
            if col not in available_columns:
                print(f"Warning: Column '{col}' is not a valid column.")
                print(f"Available columns: {available_columns}")
                sys.exit(1)

        plot_data(
            df, args.columns, args.group_by_type, args.output_dir, args.plot_x_equals_y
        )
    except FileNotFoundError:
        print(
            f"The file {csv_output_filename} was not found. Please check the file path."
        )
        sys.exit(1)
    except pd.errors.EmptyDataError:
        print(
            f"The file {csv_output_filename} is empty. Please check the file content."
        )
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred while reading the CSV file: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
