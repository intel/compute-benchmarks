#!/bin/bash

#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

# Master script that runs every compute benchmark binary with the
# --listTestSuites option and aggregates the reported test suites and
# test-case counts into a Markdown summary.
#
# GoogleTest registers a single test case as several test suites (its base
# instantiation plus optional Extended/LIMITED variants), so the reported suites
# are merged by fixture - one row per test case. The summary contains a
# per-benchmark total table (with a grand total across all binaries) followed by
# a per-benchmark breakdown listing how many test cases each benchmark has in
# each suite.

set -euo pipefail

print_usage() {
    cat <<'EOF'
Usage: list_test_suites.sh --bin-dir <dir> [--output <file>] [--outliers <x>]

  --bin-dir <dir>   Directory containing the built benchmark binaries (required),
                    e.g. build/bin or buildWindows/bin/Debug.
  --output <file>   Write the Markdown summary to <file> (default: stdout).
  --outliers <x>    Highlight suites with more than <x> test cases in a dedicated
                    Outliers section (default: 50).
  --help            Show this message.
EOF
}

binDir=""
output=""
outlierThreshold=50

require_value() {
    # $1 = option name, $2 = number of remaining args (i.e. $#)
    if [[ "$2" -lt 2 ]]; then
        echo "Error: $1 requires a value" >&2
        exit 1
    fi
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bin-dir) require_value "$1" "$#"; binDir="$2"; shift 2 ;;
        --bin-dir=*) binDir="${1#*=}"; shift ;;
        --output) require_value "$1" "$#"; output="$2"; shift 2 ;;
        --output=*) output="${1#*=}"; shift ;;
        --outliers) require_value "$1" "$#"; outlierThreshold="$2"; shift 2 ;;
        --outliers=*) outlierThreshold="${1#*=}"; shift ;;
        --help|-h) print_usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; print_usage >&2; exit 1 ;;
    esac
done

if [[ -z "$binDir" ]]; then
    echo "Error: --bin-dir is required" >&2
    print_usage >&2
    exit 1
fi
if ! [[ "$outlierThreshold" =~ ^[0-9]+$ ]]; then
    echo "Error: --outliers must be a non-negative integer" >&2
    exit 1
fi
if [[ ! -d "$binDir" ]]; then
    echo "Error: bin dir '$binDir' does not exist" >&2
    exit 1
fi

# Discover benchmark binaries: regular, executable files whose name contains
# "benchmark", excluding build artifacts, kernel sources and the docs generator.
shopt -s nullglob
benchmarks=()
for file in "$binDir"/*; do
    name="$(basename "$file")"
    case "$name" in
        *.cl|*.spv|*.cu|*.cpp|*.h|*.pdb|*.ilk|*.lib|*.exp|*.bc|*.ll) continue ;;
        docs_generator*) continue ;;
        *benchmark*) ;;
        *) continue ;;
    esac
    [[ -f "$file" && -x "$file" ]] || continue
    benchmarks+=("$file")
done

if [[ ${#benchmarks[@]} -eq 0 ]]; then
    echo "Error: no benchmark binaries found in '$binDir'" >&2
    exit 1
fi

mapfile -t benchmarks < <(printf '%s\n' "${benchmarks[@]}" | sort)

generate() {
    echo "# Test case counts"
    echo
    echo "Number of test suites and test cases per benchmark binary, collected by"
    echo "running each binary with the --listTestSuites option. Here a suite is a"
    echo "single test case: the GoogleTest instantiations that share a fixture (the"
    echo "base plus any Extended/LIMITED variants) are merged into one row, so these"
    echo "suite counts are lower than the raw GoogleTest suite count printed by"
    echo "--listTestSuites. The per-benchmark breakdown lists how many test cases each"
    echo "benchmark has in each suite. Suites with more than ${outlierThreshold} test cases are listed"
    echo "as outliers."
    echo
    echo "| Benchmark | Test suites | Test cases |"
    echo "|-----------|-------------|------------|"

    local grandSuites=0
    local grandTests=0
    local details=()
    local outliers=()

    local exe name raw suiteName count suites tests detail fixture suite s
    for exe in "${benchmarks[@]}"; do
        name="$(basename "$exe")"
        name="${name%.exe}"

        if ! raw="$("$exe" --listTestSuites 2>/dev/null)"; then
            echo "Warning: '$name' failed to list test suites, skipping" >&2
            continue
        fi
        # Binaries built on Windows emit CRLF line endings; drop the CR so the
        # count field parses cleanly.
        raw="${raw//$'\r'/}"

        # A single test case is registered as several GoogleTest test suites - its
        # base instantiation plus optional Extended/LIMITED variants - all sharing
        # the same fixture. Merge them by fixture so each row is one test case.
        unset suiteCounts
        declare -A suiteCounts
        local suiteOrder=()
        tests=0
        while IFS=';' read -r suiteName count; do
            [[ "$count" =~ ^[0-9]+$ ]] || continue
            # Fixture is the component after the last '/'; drop a trailing "Test"
            # for readability, keeping the full name if nothing would remain.
            fixture="${suiteName##*/}"
            suite="${fixture%Test}"
            [[ -n "$suite" ]] || suite="$fixture"
            if [[ -z "${suiteCounts[$suite]+set}" ]]; then
                suiteOrder+=("$suite")
            fi
            suiteCounts[$suite]=$(( ${suiteCounts[$suite]:-0} + count ))
            tests=$(( tests + count ))
        done <<< "$raw"

        suites=${#suiteOrder[@]}
        detail="### ${name}"$'\n\n'
        detail+="| Test suite | Test cases |"$'\n'
        detail+="|------------|------------|"$'\n'
        for s in "${suiteOrder[@]}"; do
            detail+="| ${s} | ${suiteCounts[$s]} |"$'\n'
            if (( ${suiteCounts[$s]} > outlierThreshold )); then
                outliers+=("${suiteCounts[$s]}"$'\t'"${name}"$'\t'"${s}")
            fi
        done
        detail+="| **Total** | **${tests}** |"$'\n'

        grandSuites=$((grandSuites + suites))
        grandTests=$((grandTests + tests))
        echo "| ${name} | ${suites} | ${tests} |"
        details+=("$detail")
    done

    echo "| **Total** | **${grandSuites}** | **${grandTests}** |"
    echo
    echo "## Outliers (more than ${outlierThreshold} test cases)"
    echo
    if [[ ${#outliers[@]} -eq 0 ]]; then
        echo "None."
    else
        echo "| Benchmark | Test suite | Test cases |"
        echo "|-----------|------------|------------|"
        local oc ob os
        printf '%s\n' "${outliers[@]}" | sort -t"$(printf '\t')" -k1,1nr | while IFS="$(printf '\t')" read -r oc ob os; do
            echo "| ${ob} | ${os} | ${oc} |"
        done
    fi
    echo
    echo "## Per-benchmark breakdown"
    echo
    local d
    for d in "${details[@]}"; do
        echo "$d"
    done
}

if [[ -n "$output" ]]; then
    generate > "$output"
    echo "Wrote summary to $output" >&2
else
    generate
fi
