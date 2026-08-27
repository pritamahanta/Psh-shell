#!/bin/bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PSH="$ROOT_DIR/psh"
COMMANDS="$ROOT_DIR/bench/exec_commands.txt"

TIMINGS="$ROOT_DIR/bench/exec_timings.txt"
SORTED="$ROOT_DIR/bench/exec_sorted.txt"

if [[ ! -x "$PSH" ]]; then
    echo "Error: $PSH not found."
    echo "Build the project first with: make"
    exit 1
fi

if [[ ! -f "$COMMANDS" ]]; then
    echo "Error: $COMMANDS not found."
    exit 1
fi

echo "Running Psh execution benchmark..."

"$PSH" < "$COMMANDS" > /dev/null 2> "$TIMINGS"

awk '/PSH_EXEC_NS/ {print $2}' "$TIMINGS" > "$ROOT_DIR/bench/exec_times.txt"

sort -n "$ROOT_DIR/bench/exec_times.txt" > "$SORTED"

echo
echo "Samples:"
wc -l < "$SORTED"

awk '
{
    a[NR] = $1
}
END {
    if (NR == 0) {
        print "No benchmark samples found."
        exit 1
    }

    if (NR % 2)
        median = a[(NR + 1) / 2]
    else
        median = (a[NR / 2] + a[NR / 2 + 1]) / 2

    p95_index = int(NR * 0.95)
    if (p95_index < 1)
        p95_index = 1

    sum = 0
    for (i = 1; i <= NR; i++)
        sum += a[i]

    average = sum / NR

    printf "Median : %.2f us\n", median / 1000
    printf "Average: %.2f us\n", average / 1000
    printf "P95    : %.2f us\n", a[p95_index] / 1000
}' "$SORTED"

