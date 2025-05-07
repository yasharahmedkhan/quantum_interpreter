/* main.c - Main entry point for quantum interpreter */
#include "quantum_interpreter.h"
#include "performance_metrics.h"
#include <ctype.h>

/* Constants for the interpreter */
#define MAX_LINE 1024        
#define MAX_QUBITS 16        
#define COMMAND_SIZE 32     

/* Function to print the welcome message and basic information */
static void print_welcome() {
    printf("Quantum Computing Interpreter v2.0\n");
    printf("Supports three simulation algorithms: State Vector, Tensor Network, and Stabilizer\n");
    printf("Type 'help' for usage information or 'exit' to quit\n\n");
}

/* Function to print detailed help information about available commands */
static void print_help() {
    printf("\nQuantum Computing Interpreter Commands:\n");
    printf("----------------------------------------\n");
    printf("init <n> <algo>  - Initialize quantum register with n qubits using algorithm <algo>\n");
    printf("                   where <algo> is one of: state, tensor, stabilizer\n");
    printf("H <q>            - Apply Hadamard gate to qubit q\n");
    printf("X <q>            - Apply Pauli-X gate to qubit q\n");
    printf("Z <q>            - Apply Pauli-Z gate to qubit q\n");
    printf("S <q>            - Apply Phase (S) gate to qubit q\n");
    printf("CNOT <c> <t>     - Apply CNOT gate with control qubit c and target qubit t\n");
    printf("measure <q>      - Measure qubit q\n");
    printf("show             - Display current quantum state\n");
    printf("circuit          - Display quantum circuit diagram\n");
    printf("fancy-circuit    - Display fancy circuit diagram with UTF-8 characters\n");
    printf("save-circuit <f> - Save circuit diagram as SVG file\n");
    printf("save-state <f>   - Save state visualization as SVG file\n");
    printf("bloch <q> <f>    - Save Bloch sphere visualization for qubit q as HTML file\n");
    printf("performance      - Show performance metrics for current algorithm\n");
    printf("compare <f>      - Run current circuit on all algorithms and compare performance\n");
    printf("                   Saves comparison chart to file f\n");
    printf("clear            - Reset quantum state\n");
    printf("help             - Display this help message\n");
    printf("exit             - Exit the interpreter\n\n");
    printf("Example usage:\n");
    printf("  init 2 state     - Initialize 2 qubits using state vector simulation\n");
    printf("  H 0              - Apply Hadamard to qubit 0\n");
    printf("  CNOT 0 1         - Apply CNOT with qubit 0 as control and qubit 1 as target\n");
    printf("  show             - Show the quantum state\n");
    printf("  measure 0        - Measure qubit 0\n");
    printf("  save-circuit c.svg - Save circuit diagram to c.svg\n");
    printf("compare_algorithms <file> <name> - Run the circuit in <file> on all three algorithms\n");
    printf("                   and generate comparative performance analysis and visualizations\n");
    printf("  compare comp.svg - Compare all algorithms and save chart to comp.svg\n\n");
}

/* Read a line of input from the user */
static char* read_line() {
    char* line = malloc(MAX_LINE);
    if (!fgets(line, MAX_LINE, stdin)) {
        free(line);
        return NULL;
    }
    line[strcspn(line, "\n")] = 0;
    return line;
}

/* Parse algorithm type from string */
static Algorithm parse_algorithm(const char* str) {
    if (strcmp(str, "state") == 0 || strcmp(str, "statevector") == 0) {
        return STATE_VECTOR;
    } else if (strcmp(str, "tensor") == 0 || strcmp(str, "tensornetwork") == 0) {
        return TENSOR_NETWORK;
    } else if (strcmp(str, "stabilizer") == 0) {
        return STABILIZER;
    } else {
        fprintf(stderr, "Unknown algorithm: %s. Using state vector by default.\n", str);
        return STATE_VECTOR;
    }
}

/* Get algorithm name from enum */
static const char* algorithm_name(Algorithm algo) {
    switch (algo) {
        case STATE_VECTOR: return "State Vector";
        case TENSOR_NETWORK: return "Tensor Network";
        case STABILIZER: return "Stabilizer";
        default: return "Unknown";
    }
}

static void simple_compare_algorithms(Chunk* chunk, int num_qubits, const char* comparison_file) {
    printf("\nGenerating theoretical comparison for %d qubits and %d gates...\n", 
           num_qubits, chunk->count);
    
    int h_gates = 0, x_gates = 0, z_gates = 0, s_gates = 0, cnot_gates = 0, measure_gates = 0;
    
    for (int i = 0; i < chunk->count; i++) {
        switch (chunk->code[i].code) {
            case OP_H: h_gates++; break;
            case OP_X: x_gates++; break;
            case OP_Z: z_gates++; break;
            case OP_S: s_gates++; break;
            case OP_CNOT: cnot_gates++; break;
            case OP_MEASURE: measure_gates++; break;
        }
    }
    
    // Calculate theoretical metrics
    PerformanceMetrics state_vector_metrics;
    PerformanceMetrics tensor_network_metrics;
    PerformanceMetrics stabilizer_metrics;
    
    init_performance_metrics(&state_vector_metrics);
    init_performance_metrics(&tensor_network_metrics);
    init_performance_metrics(&stabilizer_metrics);
    
 
    state_vector_metrics.time_elapsed = 0.000001 * (1 << num_qubits) * chunk->count;
    
   
    int bond_dimension = 4; 
    tensor_network_metrics.time_elapsed = 0.000001 * num_qubits * 
                                         bond_dimension * bond_dimension * bond_dimension * 
                                         chunk->count;
    

    stabilizer_metrics.time_elapsed = 0.000001 * num_qubits * num_qubits * chunk->count;
    

    state_vector_metrics.memory_used = sizeof(double _Complex) * (1 << num_qubits);

    tensor_network_metrics.memory_used = num_qubits * bond_dimension * bond_dimension * 
                                        sizeof(double _Complex);

    stabilizer_metrics.memory_used = 2 * num_qubits * num_qubits * sizeof(int);
    
    state_vector_metrics.gate_count = chunk->count;
    tensor_network_metrics.gate_count = chunk->count;
    stabilizer_metrics.gate_count = chunk->count;
    
    // print performance comparison
    printf("\n=== Theoretical Performance Comparison ===\n");
    printf("                  State Vector    Tensor Network    Stabilizer\n");
    printf("Time (est.)      %12.6f    %12.6f    %12.6f\n", 
           state_vector_metrics.time_elapsed,
           tensor_network_metrics.time_elapsed,
           stabilizer_metrics.time_elapsed);
    printf("Memory (bytes)   %12zu    %12zu    %12zu\n", 
           state_vector_metrics.memory_used,
           tensor_network_metrics.memory_used,
           stabilizer_metrics.memory_used);
    printf("Gate count       %12d    %12d    %12d\n", 
           state_vector_metrics.gate_count,
           tensor_network_metrics.gate_count,
           stabilizer_metrics.gate_count);
    
    printf("\nRelative Efficiency (compared to State Vector):\n");
    printf("Time efficiency:  Tensor Network: %.2fx,  Stabilizer: %.2fx\n",
           state_vector_metrics.time_elapsed / tensor_network_metrics.time_elapsed,
           state_vector_metrics.time_elapsed / stabilizer_metrics.time_elapsed);
    printf("Memory efficiency: Tensor Network: %.2fx,  Stabilizer: %.2fx\n",
           (double)state_vector_metrics.memory_used / tensor_network_metrics.memory_used,
           (double)state_vector_metrics.memory_used / stabilizer_metrics.memory_used);
    
    // save comparison
    if (comparison_file != NULL) {
        save_algorithm_comparison(&state_vector_metrics, 
                                 &tensor_network_metrics, 
                                 &stabilizer_metrics, 
                                 comparison_file);
        printf("Algorithm comparison visualization saved to %s\n", comparison_file);
    }
}



int main(int argc, char** argv) {
    srand(time(NULL));
    
    VM vm;
    init_vm(&vm);
    bool has_state = false;
    Chunk chunk;
    init_chunk(&chunk);
    vm.chunk = &chunk;
    
    // checks if in file mode
    if (argc > 1) {
        if (strcmp(argv[1], "-f") == 0 && argc > 3) {
            const char* filename = argv[2];
            int num_qubits = atoi(argv[3]);
            
            Algorithm algorithm = STATE_VECTOR;
            if (argc > 4) {
                algorithm = parse_algorithm(argv[4]);
            }
            
            if (num_qubits <= 0 || num_qubits > MAX_QUBITS) {
                fprintf(stderr, "Invalid number of qubits. Must be between 1 and %d.\n", MAX_QUBITS);
                return EXIT_FAILURE;
            }
            
            printf("Running quantum program from file %s with %d qubits using %s algorithm\n", 
                   filename, num_qubits, algorithm_name(algorithm));
            
            FILE* file = fopen(filename, "r");
            if (!file) {
                perror("Error opening file");
                return EXIT_FAILURE;
            }
            
            // Read entire file into a string
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char* source = malloc(file_size + 1);
            if (!source) {
                perror("Failed to allocate memory for source");
                fclose(file);
                return EXIT_FAILURE;
            }
            
            size_t bytes_read = fread(source, 1, file_size, file);
            source[bytes_read] = '\0';
            fclose(file);
            
            // Interpret the program
            InterpretResult result = interpret(&vm, source, num_qubits, algorithm);
            free(source);
            
            if (result == INTERPRET_OK) {
                print_fancy_circuit(&chunk, num_qubits);
                print_state((QuantumState*)vm.state);
                print_performance_metrics(&vm.metrics);
                
                // Save visualizations
                char svg_filename[256];
                sprintf(svg_filename, "%s.circuit.svg", filename);
                save_circuit_diagram_svg(&chunk, num_qubits, svg_filename);
                
                sprintf(svg_filename, "%s.state.svg", filename);
                if (algorithm == STATE_VECTOR) {
                    save_state_visualization(vm.state, svg_filename);
                }
                
                return EXIT_SUCCESS;
            } else {
                fprintf(stderr, "Error executing quantum program\n");
                return EXIT_FAILURE;
            }
        }
        print_help();
        return EXIT_FAILURE;
    }
    
    // Interactive mode
    print_welcome();
    
    for (;;) {
        printf("> ");
        char* line = read_line();
        if (line == NULL) break;
        
        // Parse command
        char command[COMMAND_SIZE];
        char arg_str[MAX_LINE];
        int args[2];  // For commands that take up to 2 arguments
        int num_args = sscanf(line, "%31s %d %d", command, &args[0], &args[1]);
        
        if (num_args < 1) {
            free(line);
            continue;
        }
        
        // Process commands
        if (strcmp(command, "exit") == 0) {
            free(line);
            break;
        }
        else if (strcmp(command, "help") == 0) {
            print_help();
        }
        else if (strcmp(command, "init") == 0) {
            Algorithm algo = STATE_VECTOR;
            
            if (sscanf(line, "%31s %d %s", command, &args[0], arg_str) >= 3) {
                algo = parse_algorithm(arg_str);
            }
            
            if (num_args < 2) {
                printf("Usage: init <number_of_qubits> [algorithm]\n");
                printf("Where algorithm is one of: state, tensor, stabilizer\n");
            } else if (args[0] <= 0 || args[0] > MAX_QUBITS) {
                printf("Number of qubits must be between 1 and %d\n", MAX_QUBITS);
            } else {
                reset_state(&vm, args[0], algo);
                chunk.count = 0;  // Clear circuit history on initialization
                has_state = true;
                printf("Initialized quantum register with %d qubits using %s algorithm\n", 
                       args[0], algorithm_name(algo));
            }
        }
        else if (!has_state) {
            printf("Please initialize the quantum state first using 'init <n> [algorithm]'\n");
        }
        else if (strcmp(command, "H") == 0) {
            if (num_args != 2) {
                printf("Usage: H <qubit>\n");
            } else if (args[0] >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else {
                Instruction instr = {.code = OP_H, .operands = {args[0]}};
                write_chunk(&chunk, instr);
                
                // Apply the gate based on the algorithm
                Algorithm algo = ((QuantumState*)vm.state)->algorithm;
                
                start_timing(&vm.metrics);
                
                if (algo == STATE_VECTOR) {
                    apply_h_gate(((QuantumState*)vm.state)->state, args[0]);
                } else {
                    Gate gate = {.name = "H", .num_targets = 1, .targets = {args[0]}};
                    if (algo == TENSOR_NETWORK) {
                        apply_gate_tensor_network_sim((QuantumState*)vm.state, &gate);
                    } else if (algo == STABILIZER) {
                        apply_gate_stabilizer_sim((QuantumState*)vm.state, &gate);
                    }
                }
                
                end_timing(&vm.metrics);
                increment_gate_count(&vm.metrics);
                
                printf("Applied Hadamard gate to qubit %d\n", args[0]);
            }
        }
        else if (strcmp(command, "X") == 0) {
            if (num_args != 2) {
                printf("Usage: X <qubit>\n");
            } else if (args[0] >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else {
                Instruction instr = {.code = OP_X, .operands = {args[0]}};
                write_chunk(&chunk, instr);
                
                // Apply the gate based on the algorithm
                Algorithm algo = ((QuantumState*)vm.state)->algorithm;
                
                start_timing(&vm.metrics);
                
                if (algo == STATE_VECTOR) {
                    apply_x_gate(((QuantumState*)vm.state)->state, args[0]);
                } else {
                    Gate gate = {.name = "X", .num_targets = 1, .targets = {args[0]}};
                    if (algo == TENSOR_NETWORK) {
                        apply_gate_tensor_network_sim((QuantumState*)vm.state, &gate);
                    } else if (algo == STABILIZER) {
                        apply_gate_stabilizer_sim((QuantumState*)vm.state, &gate);
                    }
                }
                
                end_timing(&vm.metrics);
                increment_gate_count(&vm.metrics);
                
                printf("Applied X gate to qubit %d\n", args[0]);
            }
        }
        else if (strcmp(command, "Z") == 0) {
            if (num_args != 2) {
                printf("Usage: Z <qubit>\n");
            } else if (args[0] >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else {
                Instruction instr = {.code = OP_Z, .operands = {args[0]}};
                write_chunk(&chunk, instr);
                
                // Apply the gate based on the algorithm
                Algorithm algo = ((QuantumState*)vm.state)->algorithm;
                
                start_timing(&vm.metrics);
                
                if (algo == STATE_VECTOR) {
                    apply_z_gate(((QuantumState*)vm.state)->state, args[0]);
                } else {
                    Gate gate = {.name = "Z", .num_targets = 1, .targets = {args[0]}};
                    if (algo == TENSOR_NETWORK) {
                        apply_gate_tensor_network_sim((QuantumState*)vm.state, &gate);
                    } else if (algo == STABILIZER) {
                        apply_gate_stabilizer_sim((QuantumState*)vm.state, &gate);
                    }
                }
                
                end_timing(&vm.metrics);
                increment_gate_count(&vm.metrics);
                
                printf("Applied Z gate to qubit %d\n", args[0]);
            }
        }
        else if (strcmp(command, "S") == 0) {
            if (num_args != 2) {
                printf("Usage: S <qubit>\n");
            } else if (args[0] >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else {
                Instruction instr = {.code = OP_S, .operands = {args[0]}};
                write_chunk(&chunk, instr);
                
            
                Algorithm algo = ((QuantumState*)vm.state)->algorithm;
                
                start_timing(&vm.metrics);
                
                if (algo == STATE_VECTOR) {
                    apply_s_gate(((QuantumState*)vm.state)->state, args[0]);
                } else {
                    Gate gate = {.name = "S", .num_targets = 1, .targets = {args[0]}};
                    if (algo == TENSOR_NETWORK) {
                        apply_gate_tensor_network_sim((QuantumState*)vm.state, &gate);
                    } else if (algo == STABILIZER) {
                        apply_gate_stabilizer_sim((QuantumState*)vm.state, &gate);
                    }
                }
                
                end_timing(&vm.metrics);
                increment_gate_count(&vm.metrics);
                
                printf("Applied S gate to qubit %d\n", args[0]);
            }
        }
        else if (strcmp(command, "CNOT") == 0) {
            if (num_args != 3) {
                printf("Usage: CNOT <control> <target>\n");
            } else if (args[0] >= ((QuantumState*)vm.state)->num_qubits || 
                       args[1] >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else if (args[0] == args[1]) {
                printf("Error: Control and target qubits must be different\n");
            } else {
                Instruction instr = {.code = OP_CNOT, .operands = {args[0], args[1]}};
                write_chunk(&chunk, instr);
                
                Algorithm algo = ((QuantumState*)vm.state)->algorithm;
                
                start_timing(&vm.metrics);
                
                if (algo == STATE_VECTOR) {
                    apply_cnot_gate(((QuantumState*)vm.state)->state, args[0], args[1]);
                } else {
                    Gate gate = {.name = "CNOT", .num_targets = 2, .targets = {args[0], args[1]}};
                    if (algo == TENSOR_NETWORK) {
                        apply_gate_tensor_network_sim((QuantumState*)vm.state, &gate);
                    } else if (algo == STABILIZER) {
                        apply_gate_stabilizer_sim((QuantumState*)vm.state, &gate);
                    }
                }
                
                end_timing(&vm.metrics);
                increment_gate_count(&vm.metrics);
                
                printf("Applied CNOT gate with control=%d, target=%d\n", args[0], args[1]);
            }
        }
        else if (strcmp(command, "measure") == 0) {
            if (num_args != 2) {
                printf("Usage: measure <qubit>\n");
            } else if (args[0] >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else {
                Instruction instr = {.code = OP_MEASURE, .operands = {args[0]}};
                write_chunk(&chunk, instr);
                
                // Apply measurement based on the algorithm
                Algorithm algo = ((QuantumState*)vm.state)->algorithm;
                double result = 0.0;
                
                start_timing(&vm.metrics);
                
                if (algo == STATE_VECTOR) {
                    result = measure_qubit(((QuantumState*)vm.state)->state, args[0]);
                } else {
                        fprintf(stderr, "Warning: Measurement not fully implemented in non-state vector simulators\n");
                    }
                
                
                end_timing(&vm.metrics);
                increment_gate_count(&vm.metrics);
                
                printf("Measured qubit %d: %g\n", args[0], result);
            }
        }
        else if (strcmp(command, "show") == 0) {
            print_state((QuantumState*)vm.state);
        }
        else if (strcmp(command, "circuit") == 0) {
            print_circuit(&chunk, ((QuantumState*)vm.state)->num_qubits);
        }
        else if (strcmp(command, "fancy-circuit") == 0) {
            print_fancy_circuit(&chunk, ((QuantumState*)vm.state)->num_qubits);
        }
        else if (strcmp(command, "save-circuit") == 0) {
            char filename[MAX_LINE];
            if (sscanf(line, "%31s %s", command, filename) != 2) {
                printf("Usage: save-circuit <filename>\n");
            } else {
                save_circuit_diagram_svg(&chunk, ((QuantumState*)vm.state)->num_qubits, filename);
            }
        }
        else if (strcmp(command, "save-state") == 0) {
            char filename[MAX_LINE];
            if (sscanf(line, "%31s %s", command, filename) != 2) {
                printf("Usage: save-state <filename>\n");
            } else {
                if (((QuantumState*)vm.state)->algorithm == STATE_VECTOR) {
                    save_state_visualization(((QuantumState*)vm.state)->state, filename);
                } else {
                    printf("State visualization is currently only supported for the state vector algorithm\n");
                }
            }
        }
        else if (strcmp(command, "bloch") == 0) {
            int qubit;
            char filename[MAX_LINE];
            if (sscanf(line, "%31s %d %s", command, &qubit, filename) != 3) {
                printf("Usage: bloch <qubit> <filename>\n");
            } else if (qubit >= ((QuantumState*)vm.state)->num_qubits) {
                printf("Error: Qubit index out of range\n");
            } else {
                if (((QuantumState*)vm.state)->algorithm == STATE_VECTOR) {
                    save_bloch_sphere_html(((QuantumState*)vm.state)->state, qubit, filename);
                } else {
                    printf("Bloch sphere visualization is currently only supported for the state vector algorithm\n");
                }
            }
        }
        else if (strcmp(command, "performance") == 0) {
            print_performance_metrics(&vm.metrics);
        }
        else if (strcmp(command, "compare") == 0) {
            char filename[MAX_LINE];
            if (sscanf(line, "%31s %s", command, filename) != 2) {
                printf("Usage: compare <filename>\n");
            } else {
                simple_compare_algorithms(&chunk, ((QuantumState*)vm.state)->num_qubits, filename);
            }
        }

        else if (strcmp(command, "compare_algorithms") == 0) {
            char circuit_file[MAX_LINE];
            char circuit_name[MAX_LINE];
            
            if (sscanf(line, "%31s %s %s", command, circuit_file, circuit_name) != 3) {
                printf("Usage: compare_algorithms <circuit_file> <circuit_name>\n");
                free(line);
                continue;
            }
            
            FILE* file = fopen(circuit_file, "r");
            if (!file) {
                perror("Error opening circuit file");
                free(line);
                continue;
            }
            
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            
            char* source = malloc(file_size + 1);
            if (!source) {
                perror("Failed to allocate memory for source");
                fclose(file);
                free(line);
                continue;
            }
            
            size_t bytes_read = fread(source, 1, file_size, file);
            source[bytes_read] = '\0';
            fclose(file);
            
            int current_qubits = 0;
            if (vm.state != NULL) {
                current_qubits = ((QuantumState*)vm.state)->num_qubits;
            }
            
            if (current_qubits <= 0) {
                printf("Please initialize the quantum register first using 'init <n> [algorithm]'\n");
                free(source);
                free(line);
                continue;
            }
            
            run_algorithm_comparison(&vm, source, current_qubits, circuit_name);
            
            free(source);
        }

        else if (strcmp(command, "clear") == 0) {
            Algorithm algo = ((QuantumState*)vm.state)->algorithm;
            int num_qubits = ((QuantumState*)vm.state)->num_qubits;
            
            reset_state(&vm, num_qubits, algo);
            chunk.count = 0;  // Clear the circuit history
            
            printf("Quantum state reset to |0...0⟩\n");
        }
        else {
            printf("Unknown command. Type 'help' for usage information.\n");
        }
        
        free(line);
    }
    
    // Cleanup
    free_chunk(&chunk);
    free_vm(&vm);
    
    return EXIT_SUCCESS;
}