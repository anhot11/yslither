CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c11 -I../../app/src/game -I.
LDFLAGS ?= -lm

all: benchmark optimizer

benchmark: benchmark.c simulator.c simulator.h ../../app/src/game/sbot_weights.h
	$(CC) $(CFLAGS) benchmark.c simulator.c -o benchmark $(LDFLAGS)

optimizer: optimizer.c simulator.c simulator.h ../../app/src/game/sbot_weights.h
	$(CC) $(CFLAGS) optimizer.c simulator.c -o optimizer $(LDFLAGS)

run_benchmark: benchmark
	./benchmark

run_optimizer: optimizer
	./optimizer

clean:
	rm -f benchmark optimizer *.o *.jsonl

.PHONY: all clean run_benchmark run_optimizer
