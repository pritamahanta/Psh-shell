#!/bin/bash

set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PSH="$ROOT_DIR/psh"
COMMANDS="$ROOT_DIR/bench/jobcontrol_commands.txt"

TIMINGS="$ROOT_DIR/bench/jobcontrol_timings.txt"
SETUP_TIMES="$ROOT_DIR/bench/jobcontrol_setup.txt"
TEARDOWN_TIMES="$ROOT_DIR/bench/jobcontrol_teardown.txt"
TOTAL_TIMES="$ROOT_DIR/bench/jobcontrol_total.txt"

if [[ ! -x "$PSH" ]]; then
    echo "Error: $PSH not found."
    echo "Build the benchmark version with: make bench"
    exit 1
fi

if [[ ! -f "$COMMANDS" ]]; then
    echo "Error: $COMMANDS not found."
    exit 1
fi

echo "Psh job-control benchmark"
echo "========================="
echo "Commands: 10000"
echo

"$PSH" < "$COMMANDS" > /dev/null 2> "$TIMINGS"

awk '/PSH_JC_SETUP_NS/ {print $2}' "$TIMINGS" > "$SETUP_TIMES"
awk '/PSH_JC_TEARDOWN_NS/ {print $2}' "$TIMINGS" > "$TEARDOWN_TIMES"

paste "$SETUP_TIMES" "$TEARDOWN_TIMES" |
awk '{print $1 + $2}' > "$TOTAL_TIMES"

sort -n "$SETUP_TIMES" > "${SETUP_TIMES}.sorted"
sort -n "$TEARDOWN_TIMES" > "${TEARDOWN_TIMES}.sorted"
sort -n "$TOTAL_TIMES" > "${TOTAL_TIMES}.sorted"

echo "Setup:"
awk '
{
    a[NR] = $1
}
END {
    if (NR == 0) {
        print "  No samples found."
        exit 1
    }

    if (NR % 2 == 1)
        median = a[(NR + 1) / 2]
    else
        median = (a[NR / 2] + a[NR / 2 + 1]) / 2

    idx = int(NR * 0.95)
    if (idx < 1)
        idx = 1

    printf "  Samples: %d\n", NR
    printf "  Median : %.2f us\n", median / 1000
    printf "  P95    : %.2f us\n", a[idx] / 1000
}' "${SETUP_TIMES}.sorted"

echo
echo "Teardown:"
awk '
{
    a[NR] = $1
}
END {
    if (NR == 0) {
        print "  No samples found."
        exit 1
    }

    if (NR % 2 == 1)
        median = a[(NR + 1) / 2]
    else
        median = (a[NR / 2] + a[NR / 2 + 1]) / 2

    idx = int(NR * 0.95)
    if (idx < 1)
        idx = 1

    printf "  Samples: %d\n", NR
    printf "  Median : %.2f us\n", median / 1000
    printf "  P95    : %.2f us\n", a[idx] / 1000
}' "${TEARDOWN_TIMES}.sorted"

echo
echo "Total job-control bookkeeping:"
awk '
{
    a[NR] = $1
}
END {
    if (NR == 0) {
        print "  No samples found."
        exit 1
    }

    if (NR % 2 == 1)
        median = a[(NR + 1) / 2]
    else
        median = (a[NR / 2] + a[NR / 2 + 1]) / 2

    idx = int(NR * 0.95)
    if (idx < 1)
        idx = 1

    printf "  Samples: %d\n", NR
    printf "  Median : %.2f us\n", median / 1000
    printf "  P95    : %.2f us\n", a[idx] / 1000
}' "${TOTAL_TIMES}.sorted"

echo
echo "Raw results saved under:"
echo "  $ROOT_DIR/bench/"
