# Main objects
OBJS = main.o chunk.o scanner.o virtual_machine.o state_vector.o tensor_network.o \
       stabilizer.o circuit_diagram.o enhanced_visualization.o performance_metrics.o

# Compiler options
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I. -lm

# Targets
all: quantum_interpreter 

quantum_interpreter: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

benchmark: $(BENCHMARK_OBJS)
	$(CC) -o $@ $(BENCHMARK_OBJS) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

benchmark.o benchmark_circuits.o benchmark_visualization.o benchmark_main.o: benchmark.h quantum_interpreter.h enhanced_visualization.h performance_metrics.h

# Clean
clean:
	rm -f *.o quantum_interpreter benchmark