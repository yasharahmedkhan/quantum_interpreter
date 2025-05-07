#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <errno.h>

// enum for simulation algorithms
typedef enum {
    STATE_VECTOR,
    TENSOR_NETWORK,
    STABILIZER
} Algorithm;

// Structure for a quantum state
typedef struct {
    Algorithm algorithm;
    void *state;
    int num_qubits;
} QuantumState;

// struct for a quantum gate
typedef struct {
    char name[16];
    int num_targets;
    int targets[3];
} Gate;

// a quantum program is a list of gates 
typedef struct {
    Gate *gates;
    size_t num_gates;
} QuantumProgram;

typedef struct {
    double _Complex *amplitudes;
} StateVector;

/* Parse a quantum program from a text file */
QuantumProgram* parse_program(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening program file");
        exit(EXIT_FAILURE);
    }

    QuantumProgram *program = malloc(sizeof(QuantumProgram));
    if (!program) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    program->num_gates = 0;
    program->gates = NULL;

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue; //skips empty lines

        program->gates = realloc(program->gates, (program->num_gates + 1) * sizeof(Gate));
        if (!program->gates) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
        Gate *gate = &program->gates[program->num_gates];
        memset(gate, 0, sizeof(Gate));

        int num_fields = sscanf(line, "%15s %d %d", gate->name, &gate->targets[0], &gate->targets[1]);
        if (num_fields < 2) {
            fprintf(stderr, "Invalid line in program: %s", line);
            continue;
        }
        gate->num_targets = num_fields - 1;
        program->num_gates++;
    }
    fclose(file);
    return program;
}

/* Initialize a state vector simulator */
QuantumState* init_state_vector(int num_qubits) {
    QuantumState *qs = malloc(sizeof(QuantumState));
    if (!qs) { perror("malloc failed"); exit(EXIT_FAILURE); }
    qs->algorithm = STATE_VECTOR;
    qs->num_qubits = num_qubits;
    StateVector *sv = malloc(sizeof(StateVector));
    if (!sv) { perror("malloc failed"); exit(EXIT_FAILURE); }
    size_t dim = 1 << num_qubits;
    sv->amplitudes = malloc(sizeof(double _Complex) * dim);
    if (!sv->amplitudes) { perror("malloc failed"); exit(EXIT_FAILURE); }
    for (size_t i = 0; i < dim; i++) {
        sv->amplitudes[i] = 0.0 + 0.0 * I;
    }
    sv->amplitudes[0] = 1.0 + 0.0 * I;
    qs->state = sv;
    return qs;
}

/* Apply a single-qubit gate to the state vector */
void apply_single_qubit_gate(StateVector *sv, int qubit, double _Complex m[2][2], int num_qubits) {
    size_t dim = 1 << num_qubits;
    double _Complex *new_amp = malloc(sizeof(double _Complex) * dim);
    if (!new_amp) { perror("malloc failed"); exit(EXIT_FAILURE); }

    size_t block_size = 1 << (qubit + 1);
    size_t half_block = 1 << qubit;
    for (size_t base = 0; base < dim; base += block_size) {
        for (size_t j = 0; j < half_block; j++) {
            size_t idx0 = base + j;
            size_t idx1 = idx0 + half_block;
            new_amp[idx0] = m[0][0] * sv->amplitudes[idx0] + m[0][1] * sv->amplitudes[idx1];
            new_amp[idx1] = m[1][0] * sv->amplitudes[idx0] + m[1][1] * sv->amplitudes[idx1];
        }
    }
    free(sv->amplitudes);
    sv->amplitudes = new_amp;
}

/* Apply a CNOT gate to the state vector */
void apply_cnot(StateVector *sv, int control, int target, int num_qubits) {
    size_t dim = 1 << num_qubits;
    double _Complex *new_amp = calloc(dim, sizeof(double _Complex));
    if (!new_amp) { perror("calloc failed"); exit(EXIT_FAILURE); }
    for (size_t i = 0; i < dim; i++) {
        if (((i >> control) & 1) == 1) {
            size_t j = i ^ (1 << target); // Flip the target bit
            new_amp[j] += sv->amplitudes[i];
        } else {
            new_amp[i] += sv->amplitudes[i];
        }
    }
    free(sv->amplitudes);
    sv->amplitudes = new_amp;
}

/* Apply a gate to the state vector simulator */
void apply_gate_state_vector(QuantumState *qs, const Gate *gate) {
    StateVector *sv = (StateVector*) qs->state;
    int n = qs->num_qubits;
    if (strcmp(gate->name, "H") == 0) {
        int qubit = gate->targets[0];
        double inv_sqrt2 = 1.0 / sqrt(2.0);
        double _Complex m[2][2] = {
            {inv_sqrt2, inv_sqrt2},
            {inv_sqrt2, -inv_sqrt2}
        };
        apply_single_qubit_gate(sv, qubit, m, n);
    }
    else if (strcmp(gate->name, "X") == 0) {
        int qubit = gate->targets[0];
        double _Complex m[2][2] = {
            {0.0, 1.0},
            {1.0, 0.0}
        };
        apply_single_qubit_gate(sv, qubit, m, n);
    }
    else if (strcmp(gate->name, "Z") == 0) {
        int qubit = gate->targets[0];
        double _Complex m[2][2] = {
            {1.0, 0.0},
            {0.0, -1.0}
        };
        apply_single_qubit_gate(sv, qubit, m, n);
    }
    else if (strcmp(gate->name, "CNOT") == 0) {
        if (gate->num_targets < 2) {
            fprintf(stderr, "Error: CNOT requires two qubits.\n");
            return;
        }
        int control = gate->targets[0];
        int target  = gate->targets[1];
        apply_cnot(sv, control, target, n);
    }
    else {
        fprintf(stderr, "Unknown gate: %s\n", gate->name);
    }
}

/* Free state vector simulator resources */
void free_state_vector(QuantumState *qs) {
    StateVector *sv = (StateVector*) qs->state;
    free(sv->amplitudes);
    free(sv);
    free(qs);
}

/* Initialize a tensor network simulator */
QuantumState* init_tensor_network(int num_qubits) {
    fprintf(stderr, "Tensor network simulation not implemented.\n");
    exit(EXIT_FAILURE);
    return NULL;
}

/* Apply a gate to the tensor network simulator */
void apply_gate_tensor_network(QuantumState *qs, const Gate *gate) {
    fprintf(stderr, "Tensor network simulation not implemented.\n");
    exit(EXIT_FAILURE);
}

/* Free tensor network simulator resources */
void free_tensor_network(QuantumState *qs) {
    fprintf(stderr, "Tensor network simulation not implemented.\n");
    exit(EXIT_FAILURE);
}

/* Initialize a stabilizer simulator */
QuantumState* init_stabilizer(int num_qubits) {
    fprintf(stderr, "Stabilizer simulation not implemented.\n");
    exit(EXIT_FAILURE);
    return NULL;
}

/* Apply a gate to the stabilizer simulator */
void apply_gate_stabilizer(QuantumState *qs, const Gate *gate) {
    fprintf(stderr, "Stabilizer simulation not implemented.\n");
    exit(EXIT_FAILURE);
}

/* Free stabilizer simulator resources */
void free_stabilizer(QuantumState *qs) {
    fprintf(stderr, "Stabilizer simulation not implemented.\n");
    exit(EXIT_FAILURE);
}

/* Print program usage information */
void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s -a [state|tensor|stabilizer] -q num_qubits -f program_file\n", progname);
}

int main(int argc, char **argv) {
    Algorithm algo = STATE_VECTOR;
    int num_qubits = 1;
    char *filename = NULL;

    // Process command-line args
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 && i+1 < argc) {
            if (strcmp(argv[i+1], "state") == 0)
                algo = STATE_VECTOR;
            else if (strcmp(argv[i+1], "tensor") == 0)
                algo = TENSOR_NETWORK;
            else if (strcmp(argv[i+1], "stabilizer") == 0)
                algo = STABILIZER;
            else {
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            i++;
        }
        else if (strcmp(argv[i], "-q") == 0 && i+1 < argc) {
            num_qubits = atoi(argv[i+1]);
            i++;
        }
        else if (strcmp(argv[i], "-f") == 0 && i+1 < argc) {
            filename = argv[i+1];
            i++;
        }
        else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (filename == NULL) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    //parse the quantum program from a file
    QuantumProgram *program = parse_program(filename);
    if (!program) {
        fprintf(stderr, "Failed to parse quantum program.\n");
        return EXIT_FAILURE;
    }

    // Initialize the selected simulation backend
    QuantumState *qs = NULL;
    switch (algo) {
        case STATE_VECTOR:
            qs = init_state_vector(num_qubits);
            break;
        case TENSOR_NETWORK:
            qs = init_tensor_network(num_qubits);
            break;
        case STABILIZER:
            qs = init_stabilizer(num_qubits);
            break;
        default:
            fprintf(stderr, "Unknown simulation algorithm.\n");
            exit(EXIT_FAILURE);
    }
    qs->algorithm = algo;
    qs->num_qubits = num_qubits;
    
    // Process each gate in the program
    for (size_t i = 0; i < program->num_gates; i++) {
        Gate *gate = &program->gates[i];
        switch (algo) {
            case STATE_VECTOR:
                apply_gate_state_vector(qs, gate);
                break;
            case TENSOR_NETWORK:
                apply_gate_tensor_network(qs, gate);
                break;
            case STABILIZER:
                apply_gate_stabilizer(qs, gate);
                break;
        }
    }
    // output final state
    if (algo == STATE_VECTOR) {
        StateVector *sv = (StateVector*) qs->state;
        size_t dim = 1 << num_qubits;
        printf("Final state vector:\n");
        for (size_t i = 0; i < dim; i++) {
            printf("State |%zu>: %g + %gi\n", i, creal(sv->amplitudes[i]), cimag(sv->amplitudes[i]));
        }
        free_state_vector(qs);
    }
    else if (algo == TENSOR_NETWORK) {
        free_tensor_network(qs);
    }
    else if (algo == STABILIZER) {
        free_stabilizer(qs);
    }
    
    //cleanup
    free(program->gates);
    free(program);

    return EXIT_SUCCESS;
}