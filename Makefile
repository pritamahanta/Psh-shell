CC = gcc

CFLAGS = -Wall -Wextra -g
BENCH_CFLAGS = $(CFLAGS) -DPSH_BENCH

SRC = src/main.c src/runner.c src/builtins.c src/helpers.c src/parser.c src/history.c src/jobs.c src/signals.c src/prompt.c src/execute.c src/input.c

OBJ = $(SRC:.c=.o)

TARGET = psh

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build benchmark-enabled version
bench: CFLAGS := $(BENCH_CFLAGS)
bench: clean $(TARGET)

# Run execution benchmark
bench-exec: bench
	./bench/benchmark_exec.sh

# Run pipeline benchmark
bench-pipeline: $(TARGET)
	./bench/benchmark_pipeline.sh

# Run job-control benchmark
bench-jobcontrol: bench
	./bench/benchmark_jobcontrol.sh

clean:
	rm -f $(OBJ) $(TARGET)