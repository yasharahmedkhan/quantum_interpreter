/*
 * state_vector.c 
 *
 * The state vector simulator represents a quantum state as a complex vector
 * of dimension 2^n, where n is the number of qubits. Each amplitude in the
 * vector represents the probability amplitude of a basis state.
 */


#include "quantum_interpreter.h"

/* 
* Initialize a quantum state with n qubits in the |0...0⟩ state.
* The size of the state vector is 2^n, where n is the number of qubits.
*/
StateVector* init_state_vector(int num_qubits) {
    StateVector* state = malloc(sizeof(StateVector));
    if (!state) {
        fprintf(stderr, "Failed to allocate state vector structure\n");
        exit(EXIT_FAILURE);
    }

    state->num_qubits = num_qubits;
    state->size = 1ULL << num_qubits;  // 2^n

    // Allocate the complex amplitudes array
    state->amplitudes = malloc(sizeof(double _Complex) * state->size);
    if (!state->amplitudes) {
        fprintf(stderr, "Failed to allocate amplitudes array\n");
        free(state);
        exit(EXIT_FAILURE);
    }

    state->amplitudes[0] = 1.0 + 0.0 * I;
    for (size_t i = 1; i < state->size; i++) {
        state->amplitudes[i] = 0.0 + 0.0 * I;
    }

    return state;
}

/* Clean up the state vector's allocated memory. */
void free_state_vector(StateVector* state) {
    if (state) {
        free(state->amplitudes);
        free(state);
    }
}


/*
* Apply the Hadamard gate to the specified qubit.
*/
void apply_h_gate(StateVector* state, int qubit) {
    const double inv_sqrt2 = 1.0 / sqrt(2.0);
    const size_t target_step = 1ULL << qubit;
    
    // Temp array to store transformed state
    double _Complex* new_amplitudes = malloc(sizeof(double _Complex) * state->size);
    if (!new_amplitudes) {
        fprintf(stderr, "Failed to allocate memory for Hadamard transformation\n");
        exit(EXIT_FAILURE);
    }

    // Apply transformation
    for (size_t i = 0; i < state->size; i++) {
        if ((i & target_step) == 0) {
            size_t paired_index = i | target_step;  // State with qubit = 1
            new_amplitudes[i] = inv_sqrt2 * (state->amplitudes[i] + state->amplitudes[paired_index]);
            new_amplitudes[paired_index] = inv_sqrt2 * (state->amplitudes[i] - state->amplitudes[paired_index]);
        }
    }

    //update the state vector
    free(state->amplitudes);
    state->amplitudes = new_amplitudes;
}

/*
* Apply the Pauli-X (NOT) gate to the specified qubit.
*/
void apply_x_gate(StateVector* state, int qubit) {
    const size_t target_step = 1ULL << qubit;

    // for each pair of basis states that differ only in the target qubit
    for (size_t i = 0; i < state->size; i += (target_step * 2)) {
        for (size_t j = 0; j < target_step; j++) {
            size_t index0 = i + j;              // State with qubit = 0
            size_t index1 = index0 + target_step;  // State with qubit = 1
            
            // swap the amplitudes
            double _Complex temp = state->amplitudes[index0];
            state->amplitudes[index0] = state->amplitudes[index1];
            state->amplitudes[index1] = temp;
        }
    }
}

/*
* Apply the Pauli-Z gate to the specified qubit.
*/
void apply_z_gate(StateVector* state, int qubit) {
    const size_t target_step = 1ULL << qubit;

    // For each basis state where the target qubit is 1
    for (size_t i = 0; i < state->size; i++) {
        if (i & target_step) {  // If the qubit is 1 in this basis state
            state->amplitudes[i] = -state->amplitudes[i];
        }
    }
}


/*
* Apply the CNOT (Controlled-NOT) gate.
*/
void apply_cnot_gate(StateVector* state, int control, int target) {
    const size_t control_step = 1ULL << control;
    const size_t target_step = 1ULL << target;

    // Temp array to store transformed state
    double _Complex* new_amplitudes = malloc(sizeof(double _Complex) * state->size);
    if (!new_amplitudes) {
        fprintf(stderr, "Failed to allocate memory for CNOT transformation\n");
        exit(EXIT_FAILURE);
    }

    // copy initial state
    memcpy(new_amplitudes, state->amplitudes, sizeof(double _Complex) * state->size);

    // Apply transformation
    for (size_t i = 0; i < state->size; i++) {
        if (i & control_step) {  // if control qubit is 1
            // Calculate the paired index by flipping the target qubit
            size_t paired_index = i ^ target_step;
            
            // swap amplitudes if control is 1
            new_amplitudes[i] = state->amplitudes[paired_index];
            new_amplitudes[paired_index] = state->amplitudes[i];
        }
    }

    // update the state vector
    free(state->amplitudes);
    state->amplitudes = new_amplitudes;
}

/*
* Apply the Phase (S) gate to the specified qubit.
*/
void apply_s_gate(StateVector* state, int qubit) {
    const size_t target_step = 1ULL << qubit;

    for (size_t i = 0; i < state->size; i++) {
        if (i & target_step) {  
            state->amplitudes[i] *= (0.0 + 1.0 * I);
        }
    }
}

/*
* Measure the specified qubit and collapse the state accordingly.
* Returns the measurement result (0 or 1) based on quantum probability.
*/
double measure_qubit(StateVector* state, int qubit) {
    const size_t target_step = 1ULL << qubit;
    double prob_one = 0.0;  // Probability of measuring |1⟩

    // calculate probability of measuring |1⟩
    for (size_t i = 0; i < state->size; i++) {
        if (i & target_step) {  
            double _Complex amp = state->amplitudes[i];
            prob_one += creal(amp * conj(amp)); 
        }
    }

    // determine measurement outcome based on probability
    double random_val = (double)rand() / RAND_MAX;
    int outcome = (random_val < prob_one) ? 1 : 0;

    //normalize the remaining amplitudes
    double norm_factor = 0.0;
    for (size_t i = 0; i < state->size; i++) {
        if (((i & target_step) != 0) == outcome) {
            double _Complex amp = state->amplitudes[i];
            norm_factor += creal(amp * conj(amp));
        } else {
            state->amplitudes[i] = 0.0;
        }
    }

    // Normalize the state vector
    norm_factor = 1.0 / sqrt(norm_factor);
    for (size_t i = 0; i < state->size; i++) {
        state->amplitudes[i] *= norm_factor;
    }

    return outcome;
}