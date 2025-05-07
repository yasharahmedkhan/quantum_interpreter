#ifndef performance_metrics_h
#define performance_metrics_h

#include "quantum_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* Enhanced performance metrics struct */
typedef struct {
    struct timeval start_wall_time;
    struct timeval end_wall_time;
    double wall_time_elapsed;
    
    clock_t start_cpu_time;
    clock_t end_cpu_time;
    double cpu_time_elapsed;
    
    size_t peak_memory_used;
    size_t current_memory_used;
    
    int gate_count;
    int h_gate_count;
    int x_gate_count;
    int z_gate_count;
    int s_gate_count;
    int cnot_gate_count;
    int measure_count;
    
    // Algorithm specific
    union {
        struct {  
            size_t state_vector_size;
            double vector_sparsity;  
        } state_vector;
        
        struct {  
            int max_bond_dimension;
            double avg_bond_dimension;
            double max_entanglement_entropy;
        } tensor_network;
        
        struct { 
            int tableau_size;
            int clifford_count;
            int non_clifford_count;
        } stabilizer;
    } algorithm_specific;
    
    Algorithm algorithm_type;
    char algorithm_name[32];
    char circuit_name[64];
    int num_qubits;
} EnhancedPerformanceMetrics;

/* Functions */
void init_enhanced_metrics(EnhancedPerformanceMetrics* metrics, Algorithm algo, int num_qubits);
void start_enhanced_timing(EnhancedPerformanceMetrics* metrics);
void end_enhanced_timing(EnhancedPerformanceMetrics* metrics);
void update_enhanced_memory_usage(EnhancedPerformanceMetrics* metrics, size_t memory_used);
void track_gate_operation(EnhancedPerformanceMetrics* metrics, OpCode gate, int num_targets);
void set_circuit_name(EnhancedPerformanceMetrics* metrics, const char* name);
void update_state_vector_metrics(EnhancedPerformanceMetrics* metrics, StateVector* state);
void update_tensor_network_metrics(EnhancedPerformanceMetrics* metrics, void* tn_state);
void update_stabilizer_metrics(EnhancedPerformanceMetrics* metrics, void* stabilizer_state);
void print_enhanced_metrics(EnhancedPerformanceMetrics* metrics);
void compare_enhanced_metrics(EnhancedPerformanceMetrics metrics[], int count);
void save_metrics_to_csv(EnhancedPerformanceMetrics metrics[], int count, const char* filename);
void generate_visualization_script(EnhancedPerformanceMetrics metrics[], int count, const char* csv_filename, const char* script_filename);
void run_algorithm_comparison(VM* vm, const char* source, int num_qubits, const char* circuit_name);

#endif /* enhanced_performance_metrics_h */