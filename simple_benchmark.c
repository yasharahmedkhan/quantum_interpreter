#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <complex.h>
#include <math.h>

typedef struct {
    double _Complex* amplitudes;
    int num_qubits;
    size_t size;
} StateVector;

/* Initialize state vector to |0...0> */
StateVector* init_state_vector(int num_qubits) {
    StateVector* state = malloc(sizeof(StateVector));
    if (!state) {
        fprintf(stderr, "Failed to allocate StateVector\n");
        exit(EXIT_FAILURE);
    }
    
    state->num_qubits = num_qubits;
    state->size = 1ULL << num_qubits;
    
    state->amplitudes = calloc(state->size, sizeof(double _Complex));
    if (!state->amplitudes) {
        fprintf(stderr, "Failed to allocate amplitudes array\n");
        free(state);
        exit(EXIT_FAILURE);
    }
    
    state->amplitudes[0] = 1.0 + 0.0 * I;
    
    return state;
}

/* Apply H gate to specified qubit */
void apply_h_gate(StateVector* state, int qubit) {
    const double inv_sqrt2 = 1.0 / sqrt(2.0);
    const size_t target_bit = 1ULL << qubit;
    
    double _Complex* new_amplitudes = malloc(sizeof(double _Complex) * state->size);
    if (!new_amplitudes) {
        fprintf(stderr, "Failed to allocate memory for Hadamard transformation\n");
        exit(EXIT_FAILURE);
    }
    
    for (size_t i = 0; i < state->size; i++) {
        if ((i & target_bit) == 0) {
            size_t paired_index = i | target_bit;
            new_amplitudes[i] = inv_sqrt2 * (state->amplitudes[i] + state->amplitudes[paired_index]);
            new_amplitudes[paired_index] = inv_sqrt2 * (state->amplitudes[i] - state->amplitudes[paired_index]);
        }
    }
    
    free(state->amplitudes);
    state->amplitudes = new_amplitudes;
}

/* Apply H gates to all qubits */
void apply_all_h_circuit(StateVector* state) {
    for (int i = 0; i < state->num_qubits; i++) {
        apply_h_gate(state, i);
    }
}

/* Free state vector resources */
void free_state_vector(StateVector* state) {
    if (state) {
        free(state->amplitudes);
        free(state);
    }
}

typedef struct {
    int num_qubits;
    int* tableau;
    int tableau_size;
} StabilizerState;

/* initialize stabilizer state */
StabilizerState* init_stabilizer_state(int num_qubits) {
    StabilizerState* state = malloc(sizeof(StabilizerState));
    if (!state) {
        fprintf(stderr, "Failed to allocate StabilizerState\n");
        exit(EXIT_FAILURE);
    }
    
    state->num_qubits = num_qubits;
    state->tableau_size = 2 * num_qubits * (2 * num_qubits + 1);
    
    state->tableau = calloc(state->tableau_size, sizeof(int));
    if (!state->tableau) {
        fprintf(stderr, "Failed to allocate tableau array\n");
        free(state);
        exit(EXIT_FAILURE);
    }
    
    return state;
}

/* Apply H gates to all qubits in stabilizer formalism */
void apply_all_h_stabilizer(StabilizerState* state) {
    for (int i = 0; i < state->num_qubits; i++) {
        for (int j = 0; j < 2 * state->num_qubits; j++) {
            state->tableau[i * (2 * state->num_qubits + 1) + j] ^= 
                state->tableau[i * (2 * state->num_qubits + 1) + j + 1];
        }
    }
}

/* Free stabilizer state resources */
void free_stabilizer_state(StabilizerState* state) {
    if (state) {
        free(state->tableau);
        free(state);
    }
}

typedef struct {
    int num_qubits;
    int bond_dimension;
    double _Complex* tensors;
    size_t tensor_size;
} TensorNetworkState;

/* Initialize tensor network state */
TensorNetworkState* init_tensor_network_state(int num_qubits) {
    TensorNetworkState* state = malloc(sizeof(TensorNetworkState));
    if (!state) {
        fprintf(stderr, "Failed to allocate TensorNetworkState\n");
        exit(EXIT_FAILURE);
    }
    
    state->num_qubits = num_qubits;
    state->bond_dimension = 4;
    
    state->tensor_size = num_qubits * state->bond_dimension * state->bond_dimension * 2;
    
    state->tensors = calloc(state->tensor_size, sizeof(double _Complex));
    if (!state->tensors) {
        fprintf(stderr, "Failed to allocate tensors array\n");
        free(state);
        exit(EXIT_FAILURE);
    }
    
    return state;
}

/* Apply H gates to all qubits in tensor network formalism */
void apply_all_h_tensor_network(TensorNetworkState* state) {
    for (int i = 0; i < state->num_qubits; i++) {
        size_t offset = i * state->bond_dimension * state->bond_dimension;
        for (int j = 0; j < state->bond_dimension; j++) {
            for (int k = 0; k < state->bond_dimension; k++) {
                state->tensors[offset + j * state->bond_dimension + k] *= 
                    (1.0 / sqrt(2.0)) * (1.0 + ((j == k) ? 1.0 : -1.0) * I);
            }
        }
    }
}

/* Free tensor network resources */
void free_tensor_network_state(TensorNetworkState* state) {
    if (state) {
        free(state->tensors);
        free(state);
    }
}

/* Benchmark state vector implementation */
void benchmark_state_vector(int min_qubits, int max_qubits) {
    printf("\n=== State Vector Algorithm Benchmark ===\n");
    printf("Qubits,Time(ms),Memory(KB)\n");
    
    for (int n = min_qubits; n <= max_qubits; n++) {
        if (n > 25) {
            printf("Skipping %d qubits for state vector (memory limitation)\n", n);
            continue;
        }
        
        clock_t start = clock();
        
        StateVector* state = init_state_vector(n);
        apply_all_h_circuit(state);
        
        clock_t end = clock();
        double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        
        size_t memory_bytes = sizeof(StateVector) + state->size * sizeof(double _Complex);
        double memory_kb = memory_bytes / 1024.0;
        
        printf("%d,%.2f,%.2f\n", n, time_ms, memory_kb);
        
        free_state_vector(state);
    }
}

/* Benchmark stabilizer implementation */
void benchmark_stabilizer(int min_qubits, int max_qubits) {
    printf("\n=== Stabilizer Algorithm Benchmark ===\n");
    printf("Qubits,Time(ms),Memory(KB)\n");
    
    for (int n = min_qubits; n <= max_qubits; n++) {
        clock_t start = clock();
        
        StabilizerState* state = init_stabilizer_state(n);
        apply_all_h_stabilizer(state);
        
        clock_t end = clock();
        double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        
        size_t memory_bytes = sizeof(StabilizerState) + state->tableau_size * sizeof(int);
        double memory_kb = memory_bytes / 1024.0;
        
        printf("%d,%.2f,%.2f\n", n, time_ms, memory_kb);
        
        free_stabilizer_state(state);
    }
}

/* Benchmark tensor network implementation */
void benchmark_tensor_network(int min_qubits, int max_qubits) {
    printf("\n=== Tensor Network Algorithm Benchmark ===\n");
    printf("Qubits,Time(ms),Memory(KB)\n");
    
    for (int n = min_qubits; n <= max_qubits; n++) {
        clock_t start = clock();
        
        TensorNetworkState* state = init_tensor_network_state(n);
        apply_all_h_tensor_network(state);
        
        clock_t end = clock();
        double time_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        
        size_t memory_bytes = sizeof(TensorNetworkState) + state->tensor_size * sizeof(double _Complex);
        double memory_kb = memory_bytes / 1024.0;
        
        printf("%d,%.2f,%.2f\n", n, time_ms, memory_kb);
        
        free_tensor_network_state(state);
    }
}

int main(int argc, char** argv) {
    int min_qubits = 1;
    int max_qubits = 20;
    
    if (argc > 1) {
        max_qubits = atoi(argv[1]);
    }
    
    printf("Benchmarking quantum algorithms from %d to %d qubits\n", min_qubits, max_qubits);
    
    benchmark_state_vector(min_qubits, max_qubits);
    benchmark_stabilizer(min_qubits, max_qubits);
    benchmark_tensor_network(min_qubits, max_qubits);
    
    printf("\nBenchmark complete. Use these CSV outputs to create plots.\n");
    
    return 0;
}