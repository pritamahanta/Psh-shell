#!/bin/bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PSH="$ROOT_DIR/psh"
COMMAND_FILE="$ROOT_DIR/bench/pipeline_commands.txt"
RESULT_DIR="$ROOT_DIR/bench/results"
RESULT_FILE="$RESULT_DIR/pipeline_results.txt"

RUNS=10
SIZE_GIB=1

mkdir -p "$RESULT_DIR"

if [[ ! -x "$PSH" ]]; then
    echo "Error: $PSH not found."
    echo "Build the project first with: make"
    exit 1
fi

echo "Psh pipeline benchmark"
echo "======================"
echo "Data size : ${SIZE_GIB} GiB"
echo "Runs/test : $RUNS"
echo

: > "$RESULT_FILE"

for CATS in 1 2 4 8
do
    # Build pipeline:
    # dd -> cat -> cat -> ... -> /dev/null
    PIPELINE="dd if=/dev/zero bs=1M count=1024 status=none"

    for ((i=1; i<=CATS; i++))
    do
        PIPELINE+=" | cat"
    done

    PIPELINE+=" > /dev/null"

    printf "%s\nexit\n" "$PIPELINE" > "$COMMAND_FILE"

    TIMES=()

    echo "Stages: $CATS cat(s)"

    for ((RUN=1; RUN<=RUNS; RUN++))
    do
        TIME=$(
            { /usr/bin/time -f "%e" \
                "$PSH" < "$COMMAND_FILE" > /dev/null; } \
                2>&1
        )

        # Keep only the elapsed-time value.
        TIME=$(echo "$TIME" | tail -n 1)

        TIMES+=("$TIME")

        THROUGHPUT=$(awk -v t="$TIME" 'BEGIN {
            printf "%.3f", 1 / t
        }')

        printf "  Run %2d: %.3f s | %.3f GiB/s\n" \
            "$RUN" "$TIME" "$THROUGHPUT"
    done

    MEDIAN=$(
        printf '%s\n' "${TIMES[@]}" |
        sort -n |
        awk '
        {
            a[NR] = $1
        }
        END {
            if (NR % 2 == 1)
                print a[(NR + 1) / 2]
            else
                print (a[NR / 2] + a[NR / 2 + 1]) / 2
        }'
    )

    MEDIAN_THROUGHPUT=$(awk -v t="$MEDIAN" 'BEGIN {
        printf "%.3f", 1 / t
    }')

    printf "  Median: %.3f s | %.3f GiB/s\n" \
        "$MEDIAN" "$MEDIAN_THROUGHPUT"

    printf "%d stages: %.3f s median, %.3f GiB/s median\n" \
        "$CATS" "$MEDIAN" "$MEDIAN_THROUGHPUT" \
        >> "$RESULT_FILE"

    echo
done

echo "Results saved to:"
echo "$RESULT_FILE"