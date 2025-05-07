#ifndef quantum_interpreter_h
#define quantum_interpreter_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>
#include <time.h>

/* enumeration for simulation algorithms */
typedef enum {
    STATE_VECTOR,
    TENSOR_NETWORK,
    STABILIZER
} Algorithm;

/* Performance metrics structure */
typedef struct {
    clock_t start_time;
    clock_t end_time;
    double time_elapsed;
    size_t memory_used;
    int gate_count;
} PerformanceMetrics;

/* Tokens for our quantum language */
typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_COMMA,
    TOKEN_NEWLINE, TOKEN_EOF,
    
    // Keywords
    TOKEN_QUBIT, TOKEN_MEASURE,
    
    // Gates
    TOKEN_H, TOKEN_X, TOKEN_Z, TOKEN_CNOT, TOKEN_S,
    
    // Literals
    TOKEN_NUMBER,
    
    // Error
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    double value;  // Numbers
} Token;

/* Scanner struct */
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

/* Instruction types for VM */
typedef enum {
    OP_H,      // Hadamard gate
    OP_X,      // X gate
    OP_Z,      // Z gate
    OP_S,      // Phase (S) gate
    OP_CNOT,   // CNOT gate
    OP_MEASURE // Measurement
} OpCode;

/* Instruction struct */
typedef struct {
    OpCode code;
    int operands[2];  //for operations that need multiple qubits 
    int line;
} Instruction;

/* State vector struct */
typedef struct {
    double _Complex* amplitudes;
    int num_qubits;
    size_t size;  // 2^n
} StateVector;

/* Chunk of bytecode */
typedef struct {
    Instruction* code;
    int count;
    int capacity;
} Chunk;

/* quantum state structure */
typedef struct {
    Algorithm algorithm;
    void *state;     // Pointer to algorithm-specific data
    int num_qubits;
} QuantumState;

/* Structure for a quantum gate */
typedef struct {
    char name[16];   // Gate name 
    int num_targets; // Number of qubits this gate acts on
    int targets[3];  // Qubit indices (adjust size as needed)
} Gate;

/* Virtual Machine struct */
typedef struct {
    Chunk* chunk;
    StateVector* state;
    int ip;  // Instruction pointer
    PerformanceMetrics metrics;
} VM;

/* Interpretation result type */
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

/* struct for tensor bonds */
typedef struct {
    int dimension;          // Dimension of the bond
    double* singular_values; // Singular values for the bond
} Bond;

/* Tensor struct */
typedef struct {
    double _Complex* data;    
    int* dimensions;          
    int num_indices;         
    size_t total_size;
} Tensor;

/* MPS representation */
typedef struct MPS {
    Tensor* tensors;          // Array of tensors (one per qubit)
    Bond* bonds;              // Array of bonds between tensors
    int num_qubits;           // Number of qubits in the MPS
    int max_bond_dimension;   // Maximum allowed bond dimension
    double truncation_error;  // Error from last truncation
} MPS;

/* Scanner functions */
void init_scanner(Scanner* scanner, const char* source);
Token scan_token(Scanner* scanner);

/* Chunk management */
void init_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, Instruction instruction);
void free_chunk(Chunk* chunk);

/* State vector operations  */
StateVector* init_state_vector(int num_qubits);
void free_state_vector(StateVector* state);
void apply_h_gate(StateVector* state, int qubit);
void apply_x_gate(StateVector* state, int qubit);
void apply_z_gate(StateVector* state, int qubit);
void apply_s_gate(StateVector* state, int qubit);
void apply_cnot_gate(StateVector* state, int control, int target);
double measure_qubit(StateVector* state, int qubit);

/* Tensor network operations  */
QuantumState* init_tensor_network_state(int num_qubits);
void apply_gate_tensor_network_sim(QuantumState* qs, const Gate* gate);
void free_tensor_network_state(QuantumState* qs);
void print_tensor_network_state(void* state);
size_t get_tensor_network_memory_usage(QuantumState* qs);
int get_tensor_network_bond_dimension(QuantumState* qs);

/* Stabilizer operations */
QuantumState* init_stabilizer_state(int num_qubits);
void apply_gate_stabilizer_sim(QuantumState* qs, const Gate* gate);
void free_stabilizer_state(QuantumState* qs);
void print_stabilizer_state(void* state);

/* Virtual machine operations */
void init_vm(VM* vm);
void free_vm(VM* vm);
void reset_state(VM* vm, int num_qubits, Algorithm algorithm);
InterpretResult interpret(VM* vm, const char* source, int num_qubits, Algorithm algorithm);
void print_state(QuantumState* state);

/* Circuit diagram  */
void print_circuit(Chunk* chunk, int num_qubits);

/* Enhanced visualization */
void save_circuit_diagram_svg(Chunk* chunk, int num_qubits, const char* filename);
void save_state_visualization(StateVector* state, const char* filename);
void save_algorithm_comparison(PerformanceMetrics* state_vector_metrics,
                              PerformanceMetrics* tensor_network_metrics,
                              PerformanceMetrics* stabilizer_metrics,
                              const char* filename);
void print_fancy_circuit(Chunk* chunk, int num_qubits);
void save_bloch_sphere_html(StateVector* state, int qubit, const char* filename);

/* Performance metrics */
void init_performance_metrics(PerformanceMetrics* metrics);
void start_timing(PerformanceMetrics* metrics);
void end_timing(PerformanceMetrics* metrics);
void update_memory_usage(PerformanceMetrics* metrics, size_t memory_used);
void increment_gate_count(PerformanceMetrics* metrics);
void print_performance_metrics(PerformanceMetrics* metrics);
void compare_performance_metrics(PerformanceMetrics* state_vector_metrics,
                             PerformanceMetrics* tensor_network_metrics,
                             PerformanceMetrics* stabilizer_metrics);


#endif