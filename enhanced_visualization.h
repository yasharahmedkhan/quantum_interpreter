#ifndef enhanced_visualization_h
#define enhanced_visualization_h

#include "quantum_interpreter.h"

void save_circuit_diagram_svg(Chunk* chunk, int num_qubits, const char* filename);

void save_state_visualization(StateVector* state, const char* filename);

void save_algorithm_comparison(PerformanceMetrics* state_vector_metrics,
                              PerformanceMetrics* tensor_network_metrics,
                              PerformanceMetrics* stabilizer_metrics,
                              const char* filename);

void print_fancy_circuit(Chunk* chunk, int num_qubits);

void save_bloch_sphere_html(StateVector* state, int qubit, const char* filename);

#endif