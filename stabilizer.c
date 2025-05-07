/* stabilizer.c - Complete Implementation */

#include "quantum_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * Stabilizer Simulator Implementation
 * 
 * This file implements a stabilizer formalism-based simulator for quantum computing.
 * Based on the Gottesman-Knill theorem, this simulator efficiently tracks the
 * stabilizer group of a quantum state, which allows for polynomial-time simulation
 * of Clifford circuits (H, S, CNOT gates) on classical computers.
 */

/* Pauli operators */
typedef enum {
    PAULI_I = 0,  // Identity
    PAULI_X = 1,  // Pauli-X
    PAULI_Z = 2,  // Pauli-Z
    PAULI_Y = 3   // Pauli-Y (equivalent to iXZ)
} PauliOp;

/* A Pauli string is a tensor product of Pauli operators */
typedef struct {
    PauliOp* ops;     // Array of Pauli operators (one per qubit)
    bool phase;       // Global phase factor: false for +1, true for -1
} PauliString;

/* Tableau representation for stabilizer states */
typedef struct {
    PauliString* stabilizers;   // n stabilizer generators
    PauliString* destabilizers; // n destabilizer generators
    int num_qubits;             // Number of qubits
} Tableau;

/* Forward declarations for all static functions */
static bool pauli_commute(PauliOp a, PauliOp b);
static PauliOp pauli_multiply(PauliOp a, PauliOp b, bool* phase);
static bool pauli_string_commute(PauliString* a, PauliString* b, int n);
static void multiply_pauli_strings_inplace(PauliString* a, PauliString* b, int n);
static void copy_pauli_string(PauliString* dst, PauliString* src, int n);
static Tableau* init_tableau(int n);
static void free_tableau(Tableau* tableau);
static void print_tableau(Tableau* tableau);
static void apply_h_tableau(Tableau* tableau, int qubit);
static void apply_s_tableau(Tableau* tableau, int qubit);
static void apply_cnot_tableau(Tableau* tableau, int control, int target);
static void apply_x_tableau(Tableau* tableau, int qubit);
static void apply_z_tableau(Tableau* tableau, int qubit);
static void row_multiply(Tableau* tableau, int r1, int r2, bool destabilizer);
static void row_swap(Tableau* tableau, int r1, int r2, bool destabilizer);
static int find_anticommuting_row(Tableau* tableau, int qubit, bool in_stabilizers);
static double measure_qubit_tableau(Tableau* tableau, int qubit);
static bool is_qubit_determined(Tableau* tableau, int qubit, bool* value);
static double compute_probability(Tableau* tableau, int qubit);
static bool is_valid_stabilizer_state(Tableau* tableau);
static double get_measurement_probability(Tableau* tableau, int qubit, int outcome);
static bool is_entangled_state(Tableau* tableau);
static void get_qubit_state_string(Tableau* tableau, char* buffer, size_t buffer_size);

/* Helper function: Check if two Pauli operators commute */
static bool pauli_commute(PauliOp a, PauliOp b) {
    // Pauli operators commute if they are the same or one is identity
    if (a == PAULI_I || b == PAULI_I) return true;
    
    // Pauli operators commute if they are the same
    if (a == b) return true;
    
    // Different non-identity Pauli operators anticommute
    return false;
}

/* Helper function: Multiply two Pauli operators */
static PauliOp pauli_multiply(PauliOp a, PauliOp b, bool* phase) {
    if (a == PAULI_I) return b;
    if (b == PAULI_I) return a;
    
    // Handle multiplication for non-identity operators
    if (a == b) {
        return PAULI_I; // X*X = Y*Y = Z*Z = I
    }
    
    // Handle X*Y, Y*Z, Z*X
    if ((a == PAULI_X && b == PAULI_Y) ||
        (a == PAULI_Y && b == PAULI_Z) ||
        (a == PAULI_Z && b == PAULI_X)) {
        *phase = !*phase; // Multiply by i (flip phase for +i)
        // X*Y = i*Z, Y*Z = i*X, Z*X = i*Y
        return (PauliOp)(3 - a - b);
    }
    
    // Handle Y*X, Z*Y, X*Z
    if ((a == PAULI_Y && b == PAULI_X) ||
        (a == PAULI_Z && b == PAULI_Y) ||
        (a == PAULI_X && b == PAULI_Z)) {
        *phase = !*phase; // Multiply by -i (flip phase for -i)
        *phase = !*phase; // Flip phase again for minus sign
        // Y*X = -i*Z, Z*Y = -i*X, X*Z = -i*Y
        return (PauliOp)(3 - a - b);
    }
    
    // This should not happen with valid Pauli operators
    fprintf(stderr, "Error: Invalid Pauli operator multiplication\n");
    return PAULI_I;
}

/* Helper function: Check if two Pauli strings commute */
static bool pauli_string_commute(PauliString* a, PauliString* b, int n) {
    int non_commuting_sites = 0;
    
    for (int i = 0; i < n; i++) {
        if (a->ops[i] != PAULI_I && b->ops[i] != PAULI_I && a->ops[i] != b->ops[i]) {
            non_commuting_sites++;
        }
    }
    
    // Strings commute if they anticommute at an even number of sites
    return (non_commuting_sites % 2) == 0;
}

/* Helper function: Multiply second Pauli string into the first one */
static void multiply_pauli_strings_inplace(PauliString* a, PauliString* b, int n) {
    a->phase ^= b->phase; // XOR the phases
    
    for (int i = 0; i < n; i++) {
        bool phase_change = false;
        a->ops[i] = pauli_multiply(a->ops[i], b->ops[i], &phase_change);
        if (phase_change) {
            a->phase = !a->phase;
        }
    }
}

/* Helper function: Copy a Pauli string */
static void copy_pauli_string(PauliString* dst, PauliString* src, int n) {
    dst->phase = src->phase;
    for (int i = 0; i < n; i++) {
        dst->ops[i] = src->ops[i];
    }
}

/* Initialize a tableau for n qubits in the |0...0⟩ state */
static Tableau* init_tableau(int n) {
    Tableau* tableau = malloc(sizeof(Tableau));
    if (!tableau) {
        fprintf(stderr, "Failed to allocate memory for tableau\n");
        exit(EXIT_FAILURE);
    }
    
    tableau->num_qubits = n;
    tableau->stabilizers = malloc(sizeof(PauliString) * n);
    tableau->destabilizers = malloc(sizeof(PauliString) * n);
    
    if (!tableau->stabilizers || !tableau->destabilizers) {
        fprintf(stderr, "Failed to allocate memory for stabilizers/destabilizers\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize stabilizers and destabilizers
    for (int i = 0; i < n; i++) {
        tableau->stabilizers[i].ops = malloc(sizeof(PauliOp) * n);
        tableau->destabilizers[i].ops = malloc(sizeof(PauliOp) * n);
        
        if (!tableau->stabilizers[i].ops || !tableau->destabilizers[i].ops) {
            fprintf(stderr, "Failed to allocate memory for Pauli operators\n");
            exit(EXIT_FAILURE);
        }
        
        tableau->stabilizers[i].phase = false; // +1 phase
        tableau->destabilizers[i].phase = false; // +1 phase
        
        // Initialize all operators to Identity
        for (int j = 0; j < n; j++) {
            tableau->stabilizers[i].ops[j] = PAULI_I;
            tableau->destabilizers[i].ops[j] = PAULI_I;
        }
        
        tableau->stabilizers[i].ops[i] = PAULI_Z;
        tableau->destabilizers[i].ops[i] = PAULI_X;
    }
    
    return tableau;
}

/* Free tableau memory */
static void free_tableau(Tableau* tableau) {
    if (tableau) {
        for (int i = 0; i < tableau->num_qubits; i++) {
            free(tableau->stabilizers[i].ops);
            free(tableau->destabilizers[i].ops);
        }
        
        free(tableau->stabilizers);
        free(tableau->destabilizers);
        free(tableau);
    }
}

/* Print the full tableau in a readable format */
static void print_tableau(Tableau* tableau) {
    int n = tableau->num_qubits;
    
    printf("Stabilizer Tableau (%d qubits):\n", n);
    printf("Stabilizers:\n");
    
    for (int i = 0; i < n; i++) {
        printf("%c ", tableau->stabilizers[i].phase ? '-' : '+');
        
        for (int j = 0; j < n; j++) {
            switch (tableau->stabilizers[i].ops[j]) {
                case PAULI_I: printf("I "); break;
                case PAULI_X: printf("X "); break;
                case PAULI_Z: printf("Z "); break;
                case PAULI_Y: printf("Y "); break;
            }
        }
        printf("\n");
    }
    
    printf("\nDestabilizers:\n");
    for (int i = 0; i < n; i++) {
        printf("%c ", tableau->destabilizers[i].phase ? '-' : '+');
        
        for (int j = 0; j < n; j++) {
            switch (tableau->destabilizers[i].ops[j]) {
                case PAULI_I: printf("I "); break;
                case PAULI_X: printf("X "); break;
                case PAULI_Z: printf("Z "); break;
                case PAULI_Y: printf("Y "); break;
            }
        }
        printf("\n");
    }
}

/* Apply Hadamard gate to the tableau */
static void apply_h_tableau(Tableau* tableau, int qubit) {
    for (int i = 0; i < tableau->num_qubits; i++) {
        // Hadamard transforms: X → Z, Z → X, Y → -Y
        PauliOp stab_op = tableau->stabilizers[i].ops[qubit];
        PauliOp destab_op = tableau->destabilizers[i].ops[qubit];
        
        // Transform stabilizers
        if (stab_op == PAULI_X) {
            tableau->stabilizers[i].ops[qubit] = PAULI_Z;
        } else if (stab_op == PAULI_Z) {
            tableau->stabilizers[i].ops[qubit] = PAULI_X;
        } else if (stab_op == PAULI_Y) {
            tableau->stabilizers[i].ops[qubit] = PAULI_Y;
            tableau->stabilizers[i].phase = !tableau->stabilizers[i].phase; // Flip phase
        }
        
        // Transform destabilizers
        if (destab_op == PAULI_X) {
            tableau->destabilizers[i].ops[qubit] = PAULI_Z;
        } else if (destab_op == PAULI_Z) {
            tableau->destabilizers[i].ops[qubit] = PAULI_X;
        } else if (destab_op == PAULI_Y) {
            tableau->destabilizers[i].ops[qubit] = PAULI_Y;
            tableau->destabilizers[i].phase = !tableau->destabilizers[i].phase; // Flip phase
        }
    }
}

/* Apply Phase (S) gate to the tableau */
static void apply_s_tableau(Tableau* tableau, int qubit) {
    for (int i = 0; i < tableau->num_qubits; i++) {
        // Phase gate transforms: X → Y, Y → -X, Z → Z
        PauliOp stab_op = tableau->stabilizers[i].ops[qubit];
        PauliOp destab_op = tableau->destabilizers[i].ops[qubit];
        
        // Transform stabilizers
        if (stab_op == PAULI_X) {
            tableau->stabilizers[i].ops[qubit] = PAULI_Y;
        } else if (stab_op == PAULI_Y) {
            tableau->stabilizers[i].ops[qubit] = PAULI_X;
            tableau->stabilizers[i].phase = !tableau->stabilizers[i].phase; // Flip phase
        }
        // Z remains Z
        
        // Transform destabilizers
        if (destab_op == PAULI_X) {
            tableau->destabilizers[i].ops[qubit] = PAULI_Y;
        } else if (destab_op == PAULI_Y) {
            tableau->destabilizers[i].ops[qubit] = PAULI_X;
            tableau->destabilizers[i].phase = !tableau->destabilizers[i].phase; // Flip phase
        }
        // Z remains Z
    }
}

/* Apply CNOT gate to the tableau */
static void apply_cnot_tableau(Tableau* tableau, int control, int target) {
    // CNOT transformation rules:
    // X_c -> X_c          Z_c -> Z_c • Z_t
    // X_t -> X_c • X_t    Z_t -> Z_t

    for (int i = 0; i < tableau->num_qubits; i++) {
        // Apply transformations to stabilizers
        PauliOp c_stab = tableau->stabilizers[i].ops[control];
        PauliOp t_stab = tableau->stabilizers[i].ops[target];
        
        //handle X_t propagation
        if (t_stab == PAULI_X || t_stab == PAULI_Y) {
            // propagate X to control qubit
            if (c_stab == PAULI_I) {
                tableau->stabilizers[i].ops[control] = PAULI_X;
            } else if (c_stab == PAULI_Z) {
                tableau->stabilizers[i].ops[control] = PAULI_Y;
                tableau->stabilizers[i].phase = !tableau->stabilizers[i].phase;
            }
        }
        
        //handle Z_c propagation (Z_c -> Z_c • Z_t)
        if (c_stab == PAULI_Z || c_stab == PAULI_Y) {
            // propagate Z to target qubit
            if (t_stab == PAULI_I) {
                tableau->stabilizers[i].ops[target] = PAULI_Z;
            } else if (t_stab == PAULI_X) {
                tableau->stabilizers[i].ops[target] = PAULI_Y;
                tableau->stabilizers[i].phase = !tableau->stabilizers[i].phase;
            }
        }
        
        // Apply the same transformations to destabilizers
        PauliOp c_destab = tableau->destabilizers[i].ops[control];
        PauliOp t_destab = tableau->destabilizers[i].ops[target];
        
        if (t_destab == PAULI_X || t_destab == PAULI_Y) {
            if (c_destab == PAULI_I) {
                tableau->destabilizers[i].ops[control] = PAULI_X;
            } else if (c_destab == PAULI_Z) {
                tableau->destabilizers[i].ops[control] = PAULI_Y;
                tableau->destabilizers[i].phase = !tableau->destabilizers[i].phase;
            }
        }
        
        if (c_destab == PAULI_Z || c_destab == PAULI_Y) {
            if (t_destab == PAULI_I) {
                tableau->destabilizers[i].ops[target] = PAULI_Z;
            } else if (t_destab == PAULI_X) {
                tableau->destabilizers[i].ops[target] = PAULI_Y;
                tableau->destabilizers[i].phase = !tableau->destabilizers[i].phase;
            }
        }
    }
}

/* Apply Pauli-X gate to the tableau */
static void apply_x_tableau(Tableau* tableau, int qubit) {
    // Pauli-X flips the sign of all Z and Y operators on the qubit
    for (int i = 0; i < tableau->num_qubits; i++) {
        if (tableau->stabilizers[i].ops[qubit] == PAULI_Z || 
            tableau->stabilizers[i].ops[qubit] == PAULI_Y) {
            tableau->stabilizers[i].phase = !tableau->stabilizers[i].phase;
        }
        
        if (tableau->destabilizers[i].ops[qubit] == PAULI_Z || 
            tableau->destabilizers[i].ops[qubit] == PAULI_Y) {
            tableau->destabilizers[i].phase = !tableau->destabilizers[i].phase;
        }
    }
}

/* Apply Pauli-Z gate to the tableau */
static void apply_z_tableau(Tableau* tableau, int qubit) {
    // Pauli-Z flips the sign of all X and Y operators on the qubit
    for (int i = 0; i < tableau->num_qubits; i++) {
        if (tableau->stabilizers[i].ops[qubit] == PAULI_X || 
            tableau->stabilizers[i].ops[qubit] == PAULI_Y) {
            tableau->stabilizers[i].phase = !tableau->stabilizers[i].phase;
        }
        
        if (tableau->destabilizers[i].ops[qubit] == PAULI_X || 
            tableau->destabilizers[i].ops[qubit] == PAULI_Y) {
            tableau->destabilizers[i].phase = !tableau->destabilizers[i].phase;
        }
    }
}

/* Row operation: multiply */
static void row_multiply(Tableau* tableau, int r1, int r2, bool destabilizer) {
    PauliString* rows = destabilizer ? tableau->destabilizers : tableau->stabilizers;
    int n = tableau->num_qubits;
    
    multiply_pauli_strings_inplace(&rows[r1], &rows[r2], n);
}

/* Row operation: swap rows r1 and r2 */
static void row_swap(Tableau* tableau, int r1, int r2, bool destabilizer) {
    PauliString* rows = destabilizer ? tableau->destabilizers : tableau->stabilizers;
    int n = tableau->num_qubits;
    
    // temp storage for the swap
    PauliOp* temp_ops = malloc(sizeof(PauliOp) * n);
    if (!temp_ops) {
        fprintf(stderr, "Failed to allocate memory for row swap\n");
        exit(EXIT_FAILURE);
    }
    
    // Copy r1 to temp
    bool temp_phase = rows[r1].phase;
    for (int i = 0; i < n; i++) {
        temp_ops[i] = rows[r1].ops[i];
    }
    
    //copy r2 to r1
    rows[r1].phase = rows[r2].phase;
    for (int i = 0; i < n; i++) {
        rows[r1].ops[i] = rows[r2].ops[i];
    }
    
    // Copy temp to r2
    rows[r2].phase = temp_phase;
    for (int i = 0; i < n; i++) {
        rows[r2].ops[i] = temp_ops[i];
    }
    
    free(temp_ops);
}

/* Find a row that anticommutes with the Z operator on the specified qubit */
static int find_anticommuting_row(Tableau* tableau, int qubit, bool in_stabilizers) {
    PauliString* rows = in_stabilizers ? tableau->stabilizers : tableau->destabilizers;
    int n = tableau->num_qubits;
    
    for (int i = 0; i < n; i++) {
        // A row anticommutes with Z_qubit if it has an X or Y on that qubit
        if (rows[i].ops[qubit] == PAULI_X || rows[i].ops[qubit] == PAULI_Y) {
            return i;
        }
    }
    
    return -1; //none found
}

/* Measure a qubit in the Z basis */
static double measure_qubit_tableau(Tableau* tableau, int qubit) {
    int n = tableau->num_qubits;
    
    // First, check if the qubit is already in a Z eigenstate
    bool value;
    if (is_qubit_determined(tableau, qubit, &value)) {
        return value ? 1.0 : 0.0;
    }
    
    // Find a stabilizer that anticommutes with Z_qubit
    int anticommuting_idx = find_anticommuting_row(tableau, qubit, true);
    
    if (anticommuting_idx == -1) {
        fprintf(stderr, "Error: Measurement implementation inconsistency\n");
        return 0.0;
    }
    
    // Random outcome (50% probability)
    double rand_val = (double)rand() / RAND_MAX;
    bool outcome = rand_val < 0.5;
    
   // Replace the anti-commuting stabilizer with Z_qubit
    for (int j = 0; j < n; j++) {
        tableau->stabilizers[anticommuting_idx].ops[j] = PAULI_I;
    }
    tableau->stabilizers[anticommuting_idx].ops[qubit] = PAULI_Z;
    tableau->stabilizers[anticommuting_idx].phase = outcome;
    
    // Update other stabilizers to maintain commutation realations
    for (int i = 0; i < n; i++) {
        if (i != anticommuting_idx && 
            (tableau->stabilizers[i].ops[qubit] == PAULI_X || 
             tableau->stabilizers[i].ops[qubit] == PAULI_Y)) {
            row_multiply(tableau, i, anticommuting_idx, true);
        }
    }

    int dest_idx = find_anticommuting_row(tableau, qubit, false);
    if (dest_idx != -1) {
        //use this destabilizer as the new anticommuting pair for the new stabilizer
        for (int i = 0; i < n; i++) {
            if (i != dest_idx && 
                (tableau->destabilizers[i].ops[qubit] == PAULI_X || 
                 tableau->destabilizers[i].ops[qubit] == PAULI_Y)) {
                row_multiply(tableau, i, dest_idx, false);
            }
        }
    }
    
    return outcome ? 1.0 : 0.0;
}

/* function to check if a qubit is in a determined state */
static bool is_qubit_determined(Tableau* tableau, int qubit, bool* value) {
    // check if there exists a stabilizer with Z on this qubit and I elsewhere
    for (int i = 0; i < tableau->num_qubits; i++) {
        bool is_z_only = true;
        
        for (int j = 0; j < tableau->num_qubits; j++) {
            if (j == qubit) {
                if (tableau->stabilizers[i].ops[j] != PAULI_Z) {
                    is_z_only = false;
                    break;
                }
            } else if (tableau->stabilizers[i].ops[j] != PAULI_I) {
                is_z_only = false;
                break;
            }
        }
        
        if (is_z_only) {
            // Found a Z stabilizer for this qubit
            *value = tableau->stabilizers[i].phase;
            return true;
        }
    }
    
    // Check if there's any stabilizer with X or Y on this qubit
    for (int i = 0; i < tableau->num_qubits; i++) {
        if (tableau->stabilizers[i].ops[qubit] == PAULI_X ||
            tableau->stabilizers[i].ops[qubit] == PAULI_Y) {
            return false; 
        }
    }
    
    // If no stabilizer has X or Y on this qubit and we didn't find a Z stabilizer,
    // the qubit is in |0⟩ state by default
    *value = false;
    return true;
}

/* Function to compute probability of measuring |1⟩ for a qubit */
static double compute_probability(Tableau* tableau, int qubit) {
    bool value;
    if (is_qubit_determined(tableau, qubit, &value)) {
        return value ? 1.0 : 0.0;
    } else {
        return 0.5;
    }
}

/* Determine if the current state is a valid stabilizer state */
static bool is_valid_stabilizer_state(Tableau* tableau) {
    int n = tableau->num_qubits;
    
    // Check that stabilizers commute with each other
    for (int i = 0; i < n; i++) {
        for (int j = i+1; j < n; j++) {
            if (!pauli_string_commute(&tableau->stabilizers[i], 
                                     &tableau->stabilizers[j], n)) {
                return false;
            }
        }
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                // Destabilizer i should anticommute with stabilizer i
                if (pauli_string_commute(&tableau->destabilizers[i], 
                                        &tableau->stabilizers[j], n)) {
                    return false;
                }
            } else {
                // Destabilizer i should commute with stabilizer j when i != j
                if (!pauli_string_commute(&tableau->destabilizers[i], 
                                         &tableau->stabilizers[j], n)) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

/* Get measurement outcome probability for a specific qubit and outcome */
static double get_measurement_probability(Tableau* tableau, int qubit, int outcome) {
    // First check if the qubit is in a definite state
    bool value;
    if (is_qubit_determined(tableau, qubit, &value)) {
        // if determined, probability is either 0 or 1
        return (value == (outcome == 1)) ? 1.0 : 0.0;
    } else {
        // if superposition, probability is 0.5 for both outcomes
        return 0.5;
    }
}

/* Function to check if the tableau represents a entangled state */
static bool is_entangled_state(Tableau* tableau) {
    for (int i = 0; i < tableau->num_qubits; i++) {
        int non_identity_count = 0;
        for (int j = 0; j < tableau->num_qubits; j++) {
            if (tableau->stabilizers[i].ops[j] != PAULI_I) {
                non_identity_count++;
            }
        }
        if (non_identity_count > 1) {
            return true;
        }
    }
    return false;
}

/* Helper function to get a readable string representation of a qubit state */
static void get_qubit_state_string(Tableau* tableau, char* buffer, size_t buffer_size) {
    int n = tableau->num_qubits;
    
    if (n <= 3) {
        // Check if it's a basis state
        bool all_determined = true;
        bool values[3] = {false, false, false};
        
        for (int i = 0; i < n; i++) {
            if (!is_qubit_determined(tableau, i, &values[i])) {
                all_determined = false;
                break;
            }
        }
        
        if (all_determined) {
            // It's a computational basis state
            snprintf(buffer, buffer_size, "|");
            for (int i = 0; i < n; i++) {
                char bit = values[i] ? '1' : '0';
                snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%c", bit);
            }
            snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "⟩");
            return;
        }
        
        // check for common entangled states like Bell states
        if (n == 2 && is_entangled_state(tableau)) {
            // approximation
            snprintf(buffer, buffer_size, "Entangled state (possibly Bell state)");
            return;
        }
    }
    
    // Default general description
    if (is_entangled_state(tableau)) {
        snprintf(buffer, buffer_size, "Entangled stabilizer state");
    } else {
        snprintf(buffer, buffer_size, "Separable stabilizer state");
    }
}

    /* Advanced state analysis function */
    static void analyze_stabilizer_state(Tableau* tableau) {
        int n = tableau->num_qubits;
        char state_description[100];
        
        get_qubit_state_string(tableau, state_description, sizeof(state_description));
        
        printf("\nStabilizer State Analysis:\n");
        printf("State description: %s\n", state_description);
        printf("Entangled: %s\n", is_entangled_state(tableau) ? "Yes" : "No");
        
        // count number of determined qubits
        int determined_count = 0;
        for (int i = 0; i < n; i++) {
            bool value;
            if (is_qubit_determined(tableau, i, &value)) {
                determined_count++;
            }
        }
        
        printf("Determined qubits: %d/%d\n", determined_count, n);
        printf("Superposition dimension: ~%d\n", 1 << (n - determined_count));
    }

    /* Initialize the main quantum state structure for the stabilizer simulator */
    QuantumState* init_stabilizer_state(int num_qubits) {
        QuantumState* qs = malloc(sizeof(QuantumState));
        if (!qs) {
            fprintf(stderr, "Failed to allocate memory for quantum state\n");
            exit(EXIT_FAILURE);
        }
        
        qs->algorithm = STABILIZER;
        qs->num_qubits = num_qubits;
        Tableau* tableau = init_tableau(num_qubits);
        qs->state = tableau;
        
        return qs;
    }

    /* Apply a gate to the stabilizer simulator */
    void apply_gate_stabilizer_sim(QuantumState* qs, const Gate* gate) {
        Tableau* tableau = (Tableau*)qs->state;
        
        if (strcmp(gate->name, "H") == 0) {
            int qubit = gate->targets[0];
            apply_h_tableau(tableau, qubit);
        }
        else if (strcmp(gate->name, "X") == 0) {
            int qubit = gate->targets[0];
            apply_x_tableau(tableau, qubit);
        }
        else if (strcmp(gate->name, "Z") == 0) {
            int qubit = gate->targets[0];
            apply_z_tableau(tableau, qubit);
        }
        else if (strcmp(gate->name, "S") == 0) {
            int qubit = gate->targets[0];
            apply_s_tableau(tableau, qubit);
        }
        else if (strcmp(gate->name, "CNOT") == 0) {
            if (gate->num_targets < 2) {
                fprintf(stderr, "Error: CNOT requires two qubits.\n");
                return;
            }
            int control = gate->targets[0];
            int target  = gate->targets[1];
            apply_cnot_tableau(tableau, control, target);
        }
        else {
            fprintf(stderr, "Unknown or unsupported gate: %s\n", gate->name);
            fprintf(stderr, "Note: Stabilizer simulator only supports Clifford gates (H, S, CNOT, X, Z)\n");
        }
    }

    /* Free the stabilizer simulator resources */
    void free_stabilizer_state(QuantumState* qs) {
        if (qs) {
            Tableau* tableau = (Tableau*)qs->state;
            free_tableau(tableau);
            free(qs);
        }
    }

    /* Check if a stabilizer circuit can handle the given gate */
    bool is_clifford_gate(const char* gate_name) {
        return (strcmp(gate_name, "H") == 0 ||
                strcmp(gate_name, "X") == 0 ||
                strcmp(gate_name, "Z") == 0 ||
                strcmp(gate_name, "S") == 0 ||
                strcmp(gate_name, "CNOT") == 0);
    }

    /* Estimate resources needed for simulation */
    void estimate_stabilizer_resources(int num_qubits, int* memory_bytes, int* operations_per_gate) {
        *memory_bytes = 2 * num_qubits * num_qubits * 2 + 2 * num_qubits + 64;
        
        // Operations per gate is roughly O(n²)
        *operations_per_gate = num_qubits * num_qubits;
    }

    /* Core measurement function for the stabilizer simulator */
    double stabilizer_measure_qubit(QuantumState* qs, int qubit) {
        Tableau* tableau = (Tableau*)qs->state;
        
        if (qubit >= tableau->num_qubits) {
            fprintf(stderr, "Error: Qubit index out of range for measurement\n");
            return 0.0;
        }
        
        return measure_qubit_tableau(tableau, qubit);
    }

    /* Print the current state of the stabilizer tableau */
    void print_stabilizer_state(void* state) {
        Tableau* tableau = (Tableau*)state;
        printf("\nStabilizer State (Tableau representation):\n");
        printf("Number of qubits: %d\n", tableau->num_qubits);
        
        //print computational basis probabilities
        printf("\nQubit measurement probabilities:\n");
        for (int i = 0; i < tableau->num_qubits; i++) {
            double prob = compute_probability(tableau, i);
            printf("Qubit %d: P(|0⟩) = %.2f, P(|1⟩) = %.2f\n", 
                i, 1.0 - prob, prob);
        }
        
        // print the full tableau
        printf("\nStabilizer generators:\n");
        for (int i = 0; i < tableau->num_qubits; i++) {
            printf("%c ", tableau->stabilizers[i].phase ? '-' : '+');
            
            for (int j = 0; j < tableau->num_qubits; j++) {
                switch (tableau->stabilizers[i].ops[j]) {
                    case PAULI_I: printf("I "); break;
                    case PAULI_X: printf("X "); break;
                    case PAULI_Z: printf("Z "); break;
                    case PAULI_Y: printf("Y "); break;
                }
            }
            printf("\n");
        }
        
        printf("\nDestabilizer generators:\n");
        for (int i = 0; i < tableau->num_qubits; i++) {
            printf("%c ", tableau->destabilizers[i].phase ? '-' : '+');
            
            for (int j = 0; j < tableau->num_qubits; j++) {
                switch (tableau->destabilizers[i].ops[j]) {
                    case PAULI_I: printf("I "); break;
                    case PAULI_X: printf("X "); break;
                    case PAULI_Z: printf("Z "); break;
                    case PAULI_Y: printf("Y "); break;
                }
            }
            printf("\n");
        }
        
        // if the state is small enough, try to analyze it in more detail
        if (tableau->num_qubits <= 5) {
            analyze_stabilizer_state(tableau);
        }
    }

    /* Function that tests if a tableau represents a specific common state */
    static bool is_bell_state(Tableau* tableau) {
        // check if it's a 2-qubit entangled state
        if (tableau->num_qubits != 2) return false;
        
        // Check for Bell state |Φ+⟩ = (|00⟩ + |11⟩)/√2
        // In stabilizer formalism, this is stabilized by X⊗X and Z⊗Z
        bool has_xx = false;
        bool has_zz = false;
        
        for (int i = 0; i < 2; i++) {
            if (tableau->stabilizers[i].ops[0] == PAULI_X && 
                tableau->stabilizers[i].ops[1] == PAULI_X && 
                !tableau->stabilizers[i].phase) {
                has_xx = true;
            }
            else if (tableau->stabilizers[i].ops[0] == PAULI_Z && 
                    tableau->stabilizers[i].ops[1] == PAULI_Z && 
                    !tableau->stabilizers[i].phase) {
                has_zz = true;
            }
        }
        
        return has_xx && has_zz;
    }

    /* Function that tests if a tableau represents a GHZ state */
    static bool is_ghz_state(Tableau* tableau) {
        int n = tableau->num_qubits;
        if (n < 3) return false;
        
        bool has_all_x = false;
        int zz_pair_count = 0;
        
        for (int i = 0; i < n; i++) {
            // Check for X⊗X⊗...⊗X
            bool is_all_x = true;
            for (int j = 0; j < n; j++) {
                if (tableau->stabilizers[i].ops[j] != PAULI_X) {
                    is_all_x = false;
                    break;
                }
            }
            if (is_all_x && !tableau->stabilizers[i].phase) {
                has_all_x = true;
                continue;
            }
            
            // Check for Z⊗Z pairs
            int z_count = 0;
            for (int j = 0; j < n; j++) {
                if (tableau->stabilizers[i].ops[j] == PAULI_Z) {
                    z_count++;
                } else if (tableau->stabilizers[i].ops[j] != PAULI_I) {
                    z_count = 0;
                    break;
                }
            }
            if (z_count == 2 && !tableau->stabilizers[i].phase) {
                zz_pair_count++;
            }
        }
        
        return has_all_x && (zz_pair_count >= n-1);
    }

    /* Function that generates a human-readable state description */
    static void generate_state_description(Tableau* tableau, char* buffer, size_t buffer_size) {
        int n = tableau->num_qubits;
        
        // check for specific well-known states
        if (is_bell_state(tableau)) {
            snprintf(buffer, buffer_size, "Bell state (|00⟩ + |11⟩)/√2");
            return;
        }
        
        if (is_ghz_state(tableau)) {
            snprintf(buffer, buffer_size, "GHZ state (|%0*d⟩ + |%0*d⟩)/√2", 
                    n, 0, n, (1 << n) - 1);
            return;
        }
        
        // Check if it's a product state of single-qubit states
        bool all_determined = true;
        bool* values = malloc(sizeof(bool) * n);
        if (!values) {
            snprintf(buffer, buffer_size, "Unknown state");
            return;
        }
        
        for (int i = 0; i < n; i++) {
            if (!is_qubit_determined(tableau, i, &values[i])) {
                all_determined = false;
                break;
            }
        }
        
        if (all_determined) {
            snprintf(buffer, buffer_size, "|");
            for (int i = 0; i < n; i++) {
                char bit = values[i] ? '1' : '0';
                snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "%c", bit);
            }
            snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), "⟩");
            free(values);
            return;
        }
        
        free(values);
        
        if (is_entangled_state(tableau)) {
            snprintf(buffer, buffer_size, "%d-qubit entangled stabilizer state", n);
        } else {
            snprintf(buffer, buffer_size, "%d-qubit separable stabilizer state", n);
        }
    }

    /* Calculate the number of unique stabilizers */
    static int count_unique_stabilizers(Tableau* tableau) {
        int n = tableau->num_qubits;
        int count = 0;
        
        // Count non-identity stabilizers
        for (int i = 0; i < n; i++) {
            bool is_identity = true;
            for (int j = 0; j < n; j++) {
                if (tableau->stabilizers[i].ops[j] != PAULI_I) {
                    is_identity = false;
                    break;
                }
            }
            if (!is_identity) {
                count++;
            }
        }
        
        return count;
    }

    /* Extended analysis function for stabilizer states */
    void detailed_stabilizer_analysis(QuantumState* qs) {
        Tableau* tableau = (Tableau*)qs->state;
        int n = tableau->num_qubits;
        
        printf("\n=== Detailed Stabilizer State Analysis ===\n");
        
        // generate state description
        char description[200];
        generate_state_description(tableau, description, sizeof(description));
        printf("State description: %s\n", description);
        
        // Check state validity
        bool valid = is_valid_stabilizer_state(tableau);
        printf("Valid stabilizer state: %s\n", valid ? "Yes" : "No");
        
        // analyze entanglement
        bool entangled = is_entangled_state(tableau);
        printf("Contains entanglement: %s\n", entangled ? "Yes" : "No");
        
        // Calculate statistics
        int determined = 0;
        for (int i = 0; i < n; i++) {
            bool value;
            if (is_qubit_determined(tableau, i, &value)) {
                determined++;
            }
        }
        
        printf("Determined qubits: %d/%d\n", determined, n);
        printf("Superposition dimension: ~%d\n", 1 << (n - determined));
        printf("Number of unique stabilizers: %d\n", count_unique_stabilizers(tableau));
        
        //print measurement probabilities
        printf("\nMeasurement probabilities:\n");
        for (int i = 0; i < n; i++) {
            double prob_one = compute_probability(tableau, i);
            printf("Qubit %d: P(|0⟩) = %.2f, P(|1⟩) = %.2f\n", 
                i, 1.0 - prob_one, prob_one);
        }
        
        printf("\nNote: This analysis is an approximation; a full description\n"
            "      would require analyzing the complete stabilizer group.\n");
    }

    /* Check if two stabilizer states are equivalent */
    bool are_equivalent_states(Tableau* tab1, Tableau* tab2) {
        if (tab1->num_qubits != tab2->num_qubits) {
            return false;
        }
        
        int n = tab1->num_qubits;
        
        // clone tableaus for manipulation
        Tableau* clone1 = init_tableau(n);
        Tableau* clone2 = init_tableau(n);
        
        for (int i = 0; i < n; i++) {
            copy_pauli_string(&clone1->stabilizers[i], &tab1->stabilizers[i], n);
            copy_pauli_string(&clone1->destabilizers[i], &tab1->destabilizers[i], n);
            copy_pauli_string(&clone2->stabilizers[i], &tab2->stabilizers[i], n);
            copy_pauli_string(&clone2->destabilizers[i], &tab2->destabilizers[i], n);
        }
        
        bool equivalent = true;
        for (int i = 0; i < n; i++) {
            bool found_match = false;
            for (int j = 0; j < n; j++) {
                bool same_ops = true;
                for (int k = 0; k < n; k++) {
                    if (clone1->stabilizers[i].ops[k] != clone2->stabilizers[j].ops[k]) {
                        same_ops = false;
                        break;
                    }
                }
                
                if (same_ops && clone1->stabilizers[i].phase == clone2->stabilizers[j].phase) {
                    found_match = true;
                    break;
                }
            }
            
            if (!found_match) {
                equivalent = false;
                break;
            }
        }
        
        free_tableau(clone1);
        free_tableau(clone2);
        
        return equivalent;
    }

    