/*
 * virtual_machine.c
 *
 * The virtual machine (VM) is the core component that executes quantum operations.
 * It maintains the quantum state and processes instructions one at a time.
 */
#include "quantum_interpreter.h"



/* Initialize a fresh virtual machine with no state */
void init_vm(VM* vm) {
    vm->chunk = NULL;
    vm->state = NULL;
    vm->ip = 0;
    
    // initialize performance metrics
    init_performance_metrics(&vm->metrics);
}

/*  Reset the quantum state to |0...0⟩ with the specified number of qubits.*/
void reset_state(VM* vm, int num_qubits, Algorithm algorithm) {

    if (vm->state != NULL) {
        QuantumState* qs = (QuantumState*)vm->state;
        if (qs->algorithm == STATE_VECTOR) {
            free_state_vector(qs->state);
        } else if (qs->algorithm == TENSOR_NETWORK) {
            free_tensor_network_state(qs);
        } else if (qs->algorithm == STABILIZER) {
            free_stabilizer_state(qs);
        }
        free(qs);
        vm->state = NULL;
    }
    
    QuantumState* new_state = malloc(sizeof(QuantumState));
    if (!new_state) {
        fprintf(stderr, "Failed to allocate memory for quantum state\n");
        exit(EXIT_FAILURE);
    }
    
    new_state->algorithm = algorithm;
    new_state->num_qubits = num_qubits;
    
    // sets to specific algorithm
    if (algorithm == STATE_VECTOR) {
        new_state->state = init_state_vector(num_qubits);
    } else if (algorithm == TENSOR_NETWORK) {
        fprintf(stderr, "Note: Using tensor network simulation\n");
        new_state = init_tensor_network_state(num_qubits);
    } else if (algorithm == STABILIZER) {
        fprintf(stderr, "Note: Using stabilizer simulation\n");
        new_state = init_stabilizer_state(num_qubits);
    } else {
        fprintf(stderr, "Unknown algorithm type, defaulting to state vector\n");
        new_state->state = init_state_vector(num_qubits);
    }
    
    vm->state = (void*)new_state;
    vm->ip = 0;  // reset instruction pointer
    
    init_performance_metrics(&vm->metrics);
}

 /* Clean up all resources used by the VM. */
void free_vm(VM* vm) {
    if (vm->state != NULL) {
        QuantumState* qs = (QuantumState*)vm->state;
        if (qs->algorithm == STATE_VECTOR) {
            free_state_vector(qs->state);
        } else if (qs->algorithm == TENSOR_NETWORK) {
            free_tensor_network_state(qs);
        } else if (qs->algorithm == STABILIZER) {
            free_stabilizer_state(qs);
        }
        free(qs);
        vm->state = NULL;
    }
    
    if (vm->chunk != NULL) {
        free_chunk(vm->chunk);
        vm->chunk = NULL;
    }
}

 /* Execute a single quantum instruction */
static void execute_instruction(VM* vm, Instruction* instr) {
    QuantumState* qs = (QuantumState*)vm->state;
    
    switch (instr->code) {
        case OP_H:
            if (instr->operands[0] >= qs->num_qubits) {
                fprintf(stderr, "Error: Qubit index out of range\n");
                return;
            }
            if (qs->algorithm == STATE_VECTOR) {
                apply_h_gate(qs->state, instr->operands[0]);
            } else {
                Gate gate = {.name = "H", .num_targets = 1, .targets = {instr->operands[0]}};
                if (qs->algorithm == TENSOR_NETWORK) {
                    apply_gate_tensor_network_sim(qs, &gate);
                } else if (qs->algorithm == STABILIZER) {
                    apply_gate_stabilizer_sim(qs, &gate);
                }
            }
            break;

        case OP_X:
            if (instr->operands[0] >= qs->num_qubits) {
                fprintf(stderr, "Error: Qubit index out of range\n");
                return;
            }
            if (qs->algorithm == STATE_VECTOR) {
                apply_x_gate(qs->state, instr->operands[0]);
            } else {
                Gate gate = {.name = "X", .num_targets = 1, .targets = {instr->operands[0]}};
                if (qs->algorithm == TENSOR_NETWORK) {
                    apply_gate_tensor_network_sim(qs, &gate);
                } else if (qs->algorithm == STABILIZER) {
                    apply_gate_stabilizer_sim(qs, &gate);
                }
            }
            break;

        case OP_Z:
            if (instr->operands[0] >= qs->num_qubits) {
                fprintf(stderr, "Error: Qubit index out of range\n");
                return;
            }
            if (qs->algorithm == STATE_VECTOR) {
                apply_z_gate(qs->state, instr->operands[0]);
            } else {
                Gate gate = {.name = "Z", .num_targets = 1, .targets = {instr->operands[0]}};
                if (qs->algorithm == TENSOR_NETWORK) {
                    apply_gate_tensor_network_sim(qs, &gate);
                } else if (qs->algorithm == STABILIZER) {
                    apply_gate_stabilizer_sim(qs, &gate);
                }
            }
            break;
            
        case OP_S:
            if (instr->operands[0] >= qs->num_qubits) {
                fprintf(stderr, "Error: Qubit index out of range\n");
                return;
            }
            if (qs->algorithm == STATE_VECTOR) {
                apply_s_gate(qs->state, instr->operands[0]);
            } else {
                Gate gate = {.name = "S", .num_targets = 1, .targets = {instr->operands[0]}};
                if (qs->algorithm == TENSOR_NETWORK) {
                    apply_gate_tensor_network_sim(qs, &gate);
                } else if (qs->algorithm == STABILIZER) {
                    apply_gate_stabilizer_sim(qs, &gate);
                }
            }
            break;

        case OP_CNOT:
            if (instr->operands[0] >= qs->num_qubits ||
                instr->operands[1] >= qs->num_qubits) {
                fprintf(stderr, "Error: Qubit index out of range\n");
                return;
            }
            if (instr->operands[0] == instr->operands[1]) {
                fprintf(stderr, "Error: CNOT control and target must be different qubits\n");
                return;
            }
            if (qs->algorithm == STATE_VECTOR) {
                apply_cnot_gate(qs->state, instr->operands[0], instr->operands[1]);
            } else {
                Gate gate = {.name = "CNOT", .num_targets = 2, 
                             .targets = {instr->operands[0], instr->operands[1]}};
                if (qs->algorithm == TENSOR_NETWORK) {
                    apply_gate_tensor_network_sim(qs, &gate);
                } else if (qs->algorithm == STABILIZER) {
                    apply_gate_stabilizer_sim(qs, &gate);
                }
            }
            break;

        case OP_MEASURE:
            if (instr->operands[0] >= qs->num_qubits) {
                fprintf(stderr, "Error: Qubit index out of range\n");
                return;
            }
            if (qs->algorithm == STATE_VECTOR) {
                double result = measure_qubit(qs->state, instr->operands[0]);
                printf("Measured qubit %d: %f\n", instr->operands[0], result);
            } else {
                fprintf(stderr, "Warning: Measurement not fully implemented for non-state vector algorithms\n");
            }
            break;
    }
    
    //update performance metrics
    increment_gate_count(&vm->metrics);
}


/*
* Main execution loop of the virtual machine.
* Processes instructions one at a time until the end of the program.
* 
* The VM uses a simple fetch-decode-execute cycle:
* 1. Fetch: Get the next instruction from the chunk
* 2. Decode: Identify the operation and its operands
* 3. Execute: Perform the quantum operation
*/
static InterpretResult run(VM* vm) {
    QuantumState* qs = (QuantumState*)vm->state;
    //track memory usage for performance metrics
    if (qs->algorithm == STATE_VECTOR) {
        update_memory_usage(&vm->metrics, 
                          sizeof(QuantumState) + 
                          sizeof(StateVector) + 
                          (1ULL << qs->num_qubits) * sizeof(double _Complex));
    } else if (qs->algorithm == TENSOR_NETWORK) {
        // approximation
        int bond_dimension = 4; 
        update_memory_usage(&vm->metrics, 
                          sizeof(QuantumState) + 
                          qs->num_qubits * bond_dimension * bond_dimension * sizeof(double _Complex));
    } else if (qs->algorithm == STABILIZER) {
        update_memory_usage(&vm->metrics, 
                          sizeof(QuantumState) + 
                          2 * qs->num_qubits * qs->num_qubits * sizeof(int));
    }
    
    // Start timing for execution
    start_timing(&vm->metrics);
    
    while (vm->ip < vm->chunk->count) {
        Instruction* instr = &vm->chunk->code[vm->ip];
        
        execute_instruction(vm, instr);
        
        vm->ip++;
        
        if (ferror(stderr)) {
            end_timing(&vm->metrics);
            return INTERPRET_RUNTIME_ERROR;
        }
    }
    
    end_timing(&vm->metrics);
    
    return INTERPRET_OK;
}

/* function to interpret a quantum program. */
InterpretResult interpret(VM* vm, const char* source, int num_qubits, Algorithm algorithm) {

    
    // initialize quantum state
    reset_state(vm, num_qubits, algorithm);
    
    // Initialize a chunk 
    Chunk chunk;
    init_chunk(&chunk);
    vm->chunk = &chunk;
    
    // Set up scanner for the source code
    Scanner scanner;
    init_scanner(&scanner, source);
    
    // Start timing for the interpretation
    start_timing(&vm->metrics);
    
    // parse tokens into instructions
    bool had_error = false;
    Token token;
    do {
        token = scan_token(&scanner);
        
        switch (token.type) {
            case TOKEN_H: {
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected qubit number after H\n");
                    had_error = true;
                    break;
                }
                Instruction instr = {
                    .code = OP_H,
                    .operands = {(int)token.value},
                    .line = token.line
                };
                write_chunk(&chunk, instr);
                break;
            }
            
            case TOKEN_X: {
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected qubit number after X\n");
                    had_error = true;
                    break;
                }
                Instruction instr = {
                    .code = OP_X,
                    .operands = {(int)token.value},
                    .line = token.line
                };
                write_chunk(&chunk, instr);
                break;
            }
            
            case TOKEN_Z: {
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected qubit number after Z\n");
                    had_error = true;
                    break;
                }
                Instruction instr = {
                    .code = OP_Z,
                    .operands = {(int)token.value},
                    .line = token.line
                };
                write_chunk(&chunk, instr);
                break;
            }
            
            case TOKEN_S: {
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected qubit number after S\n");
                    had_error = true;
                    break;
                }
                Instruction instr = {
                    .code = OP_S,
                    .operands = {(int)token.value},
                    .line = token.line
                };
                write_chunk(&chunk, instr);
                break;
            }
            
            case TOKEN_CNOT: {
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected control qubit number after CNOT\n");
                    had_error = true;
                    break;
                }
                int control = (int)token.value;
                
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected target qubit number after CNOT control\n");
                    had_error = true;
                    break;
                }
                Instruction instr = {
                    .code = OP_CNOT,
                    .operands = {control, (int)token.value},
                    .line = token.line
                };
                write_chunk(&chunk, instr);
                break;
            }
            
            case TOKEN_MEASURE: {
                token = scan_token(&scanner);
                if (token.type != TOKEN_NUMBER) {
                    fprintf(stderr, "Error: Expected qubit number after measure\n");
                    had_error = true;
                    break;
                }
                Instruction instr = {
                    .code = OP_MEASURE,
                    .operands = {(int)token.value},
                    .line = token.line
                };
                write_chunk(&chunk, instr);
                break;
            }
            
            case TOKEN_ERROR:
                fprintf(stderr, "Error: %.*s\n", token.length, token.start);
                had_error = true;
                break;
                
            case TOKEN_EOF:
                break;
                
            case TOKEN_NEWLINE:
                break;
                
            default:
                fprintf(stderr, "Error: Unexpected token\n");
                had_error = true;
                break;
        }
    } while (token.type != TOKEN_EOF && !had_error);
    
    if (had_error) {
        free_chunk(&chunk);
        end_timing(&vm->metrics);
        return INTERPRET_COMPILE_ERROR;
    }
    
    // Execute the compiled program
    InterpretResult result = run(vm);
    
    // end timing 
    end_timing(&vm->metrics);
    
    free_chunk(&chunk);
    
    return result;
}

/* Display the current quantum state based on the algorithm being used. */
void print_state(QuantumState* qs) {
    if (qs == NULL) {
        printf("Quantum state not initialized\n");
        return;
    }
    
    printf("\nQuantum State (%s algorithm):\n", 
           qs->algorithm == STATE_VECTOR ? "State Vector" : 
           qs->algorithm == TENSOR_NETWORK ? "Tensor Network" : 
           qs->algorithm == STABILIZER ? "Stabilizer" : "Unknown");
    
    if (qs->algorithm == STATE_VECTOR) {
        StateVector* state = (StateVector*)qs->state;
        
        //print state vector representation
        for (size_t i = 0; i < state->size; i++) {
            double real = creal(state->amplitudes[i]);
            double imag = cimag(state->amplitudes[i]);
            
            if (fabs(real) > 1e-10 || fabs(imag) > 1e-10) {
                printf("|%zu⟩: ", i);
                
                // Print real part if non-zero
                if (fabs(real) > 1e-10) {
                    printf("%.6f", real);
                }
                
                // Print imaginary part if non-zero
                if (fabs(imag) > 1e-10) {
                    if (imag > 0 && fabs(real) > 1e-10) printf("+");
                    printf("%.6fi", imag);
                }
                
                printf("\n");
            }
        }
    } else if (qs->algorithm == TENSOR_NETWORK) {
        printf("Tensor Network representation (not shown in full detail)\n");
        printf("Number of qubits: %d\n", qs->num_qubits);
        print_tensor_network_state(qs->state);
    } else if (qs->algorithm == STABILIZER) {
        printf("Stabilizer representation (not shown in full detail)\n");
        printf("Number of qubits: %d\n", qs->num_qubits);
        print_stabilizer_state(qs->state);
    }
    
    printf("\n");
}