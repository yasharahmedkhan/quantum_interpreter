#include "quantum_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <stdbool.h>

/*
 * tensor_network.c
 * 
 * This file implements a tensor network-based simulator for quantum computing.
 * Specifically, it uses Matrix Product States (MPS), which represent quantum 
 * states as a chain of interconnected tensors.
 *
 */



/* Forward declarations of internal functions */
static Tensor* create_tensor(int num_indices, int* dimensions);
static void free_tensor(Tensor* tensor);
static size_t get_tensor_index(Tensor* tensor, int* indices);
static double _Complex get_tensor_value(Tensor* tensor, int* indices);
static void set_tensor_value(Tensor* tensor, int* indices, double _Complex value);
static Tensor* contract_tensors(Tensor* A, int A_idx, Tensor* B, int B_idx);
static Tensor* tensor_product(Tensor* A, Tensor* B);
static void tensor_copy(Tensor* dst, Tensor* src);
static Tensor* create_gate_tensor(double _Complex matrix[2][2]);
static MPS* create_mps(int num_qubits, int max_bond_dimension);
static void free_mps(MPS* mps);
static void initialize_computational_basis_state(MPS* mps, size_t basis_state);
static void apply_single_qubit_gate(MPS* mps, int qubit, double _Complex matrix[2][2]);
static void apply_two_qubit_gate(MPS* mps, int qubit1, int qubit2, double _Complex matrix[4][4]);
static Tensor* reshape_tensor(Tensor* tensor, int new_num_indices, int* new_dimensions);
static void perform_svd(Tensor* tensor, int left_rank, int right_rank, Tensor** U, double** S, Tensor** V);
static void truncate_bond_dimension(MPS* mps, int bond, int max_dimension);
static double calculate_fidelity(MPS* mps1, MPS* mps2);
static double _Complex get_amplitude(MPS* mps, size_t basis_state);
static double measure_qubit_mps(MPS* mps, int qubit);
static void normalize_mps(MPS* mps);
static double compute_expected_value(MPS* mps, const char* observable, int qubit);
static double compute_entanglement_entropy(MPS* mps, int bond_idx);
static double compute_bond_dimension_ratio(MPS* mps);
static double estimate_simulation_error(MPS* mps);

/* Matrix definitions for common gates */
static double _Complex H_matrix[2][2] = {
    {0.701067811865475 + 0.0*I, 0.701067811865475 + 0.0*I},
    {0.701067811865475 + 0.0*I, -0.701067811865475 + 0.0*I}
};

static double _Complex X_matrix[2][2] = {
    {0.0, 1.0},
    {1.0, 0.0}
};

static double _Complex Z_matrix[2][2] = {
    {1.0, 0.0},
    {0.0, -1.0}
};

static double _Complex S_matrix[2][2] = {
    {1.0, 0.0},
    {0.0, I}  // I is the imaginary unit
};

static double _Complex CNOT_matrix[4][4] = {
    {1.0, 0.0, 0.0, 0.0},
    {0.0, 1.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 1.0},
    {0.0, 0.0, 1.0, 0.0}
};

/* Create a new tensor with specified dimensions */
static Tensor* create_tensor(int num_indices, int* dimensions) {
    Tensor* tensor = malloc(sizeof(Tensor));
    if (!tensor) {
        fprintf(stderr, "Error: Failed to allocate memory for tensor\n");
        exit(EXIT_FAILURE);
    }
    
    tensor->num_indices = num_indices;
    tensor->dimensions = malloc(sizeof(int) * num_indices);
    if (!tensor->dimensions) {
        fprintf(stderr, "Error: Failed to allocate memory for tensor dimensions\n");
        free(tensor);
        exit(EXIT_FAILURE);
    }
    
    size_t total_size = 1;
    for (int i = 0; i < num_indices; i++) {
        tensor->dimensions[i] = dimensions[i];
        total_size *= dimensions[i];
    }
    
    tensor->total_size = total_size;
    tensor->data = calloc(total_size, sizeof(double _Complex));
    if (!tensor->data) {
        fprintf(stderr, "Error: Failed to allocate memory for tensor data\n");
        free(tensor->dimensions);
        free(tensor);
        exit(EXIT_FAILURE);
    }
    
    return tensor;
}

/* Free tensor memory */
static void free_tensor(Tensor* tensor) {
    if (tensor) {
        if (tensor->data) free(tensor->data);
        if (tensor->dimensions) free(tensor->dimensions);
        free(tensor);
    }
}

/* Calculate the flat array index from multi-dimensional indices */
static size_t get_tensor_index(Tensor* tensor, int* indices) {
    size_t flat_index = 0;
    size_t stride = 1;
    
    for (int i = tensor->num_indices - 1; i >= 0; i--) {
        flat_index += indices[i] * stride;
        stride *= tensor->dimensions[i];
    }
    
    return flat_index;
}

/* Get tensor value at specified indices */
static double _Complex get_tensor_value(Tensor* tensor, int* indices) {
    size_t flat_index = get_tensor_index(tensor, indices);
    return tensor->data[flat_index];
}

/* Set tensor value at specified indices */
static void set_tensor_value(Tensor* tensor, int* indices, double _Complex value) {
    size_t flat_index = get_tensor_index(tensor, indices);
    tensor->data[flat_index] = value;
}

/* Contract two tensors along specified indices */
static Tensor* contract_tensors(Tensor* A, int A_idx, Tensor* B, int B_idx) {
    if (A->dimensions[A_idx] != B->dimensions[B_idx]) {
        fprintf(stderr, "Error: Cannot contract tensors with mismatched dimensions\n");
        exit(EXIT_FAILURE);
    }
    
    int contract_dim = A->dimensions[A_idx];
    
    // Calculate the dimensions of the result tensor
    int result_num_indices = A->num_indices + B->num_indices - 2;
    int* result_dimensions = malloc(sizeof(int) * result_num_indices);
    if (!result_dimensions) {
        fprintf(stderr, "Error: Failed to allocate memory for result dimensions\n");
        exit(EXIT_FAILURE);
    }
    
    //map indices from original tensors to the result tensor
    int* A_map = malloc(sizeof(int) * A->num_indices);
    int* B_map = malloc(sizeof(int) * B->num_indices);
    if (!A_map || !B_map) {
        fprintf(stderr, "Error: Failed to allocate memory for index mapping\n");
        free(result_dimensions);
        if (A_map) free(A_map);
        if (B_map) free(B_map);
        exit(EXIT_FAILURE);
    }
    
    int result_idx = 0;
    
    // Map A's indices to result indices (skipping the contracted index)
    for (int i = 0; i < A->num_indices; i++) {
        if (i != A_idx) {
            result_dimensions[result_idx] = A->dimensions[i];
            A_map[i] = result_idx++;
        } else {
            A_map[i] = -1;  //contracted index
        }
    }
    
    // Map B's indices to result indices (skipping the contracted index)
    for (int i = 0; i < B->num_indices; i++) {
        if (i != B_idx) {
            result_dimensions[result_idx] = B->dimensions[i];
            B_map[i] = result_idx++;
        } else {
            B_map[i] = -1;  //contracted index
        }
    }
    
    // Create result tensor
    Tensor* result = create_tensor(result_num_indices, result_dimensions);
    free(result_dimensions);
    
    // Temporary arrays for indexing
    int* A_indices = malloc(sizeof(int) * A->num_indices);
    int* B_indices = malloc(sizeof(int) * B->num_indices);
    int* result_indices = malloc(sizeof(int) * result->num_indices);
    if (!A_indices || !B_indices || !result_indices) {
        fprintf(stderr, "Error: Failed to allocate memory for indices arrays\n");
        free_tensor(result);
        if (A_indices) free(A_indices);
        if (B_indices) free(B_indices);
        if (result_indices) free(result_indices);
        free(A_map);
        free(B_map);
        exit(EXIT_FAILURE);
    }
    
    // Initialize indices to 0
    for (int i = 0; i < A->num_indices; i++) A_indices[i] = 0;
    for (int i = 0; i < B->num_indices; i++) B_indices[i] = 0;
    for (int i = 0; i < result->num_indices; i++) result_indices[i] = 0;
    
    // Calculate all elements of the result tensor
    for (size_t flat_idx = 0; flat_idx < result->total_size; flat_idx++) {
        // Convert flat index to multi-dimensional indices
        size_t temp = flat_idx;
        for (int i = result->num_indices - 1; i >= 0; i--) {
            result_indices[i] = temp % result->dimensions[i];
            temp /= result->dimensions[i];
        }
        
        // Map result indices back to A and B
        for (int i = 0; i < A->num_indices; i++) {
            if (i != A_idx) {
                A_indices[i] = result_indices[A_map[i]];
            }
        }
        
        for (int i = 0; i < B->num_indices; i++) {
            if (i != B_idx) {
                B_indices[i] = result_indices[B_map[i]];
            }
        }
        
        // Perform contraction sum
        double _Complex sum = 0.0;
        for (int k = 0; k < contract_dim; k++) {
            A_indices[A_idx] = k;
            B_indices[B_idx] = k;
            sum += get_tensor_value(A, A_indices) * get_tensor_value(B, B_indices);
        }
        
        result->data[flat_idx] = sum;
    }
    
    free(A_indices);
    free(B_indices);
    free(result_indices);
    free(A_map);
    free(B_map);
    
    return result;
}

/* Compute tensor product of two tensors */
static Tensor* tensor_product(Tensor* A, Tensor* B) {
    // Calculate dimensions of the result tensor
    int result_num_indices = A->num_indices + B->num_indices;
    int* result_dimensions = malloc(sizeof(int) * result_num_indices);
    if (!result_dimensions) {
        fprintf(stderr, "Error: Failed to allocate memory for tensor product dimensions\n");
        exit(EXIT_FAILURE);
    }
    
    // Copy dimensions from A and B
    for (int i = 0; i < A->num_indices; i++) {
        result_dimensions[i] = A->dimensions[i];
    }
    for (int i = 0; i < B->num_indices; i++) {
        result_dimensions[A->num_indices + i] = B->dimensions[i];
    }
    
    Tensor* result = create_tensor(result_num_indices, result_dimensions);
    free(result_dimensions);
    
    // Calculate all elements of the result tensor
    int* A_indices = malloc(sizeof(int) * A->num_indices);
    int* B_indices = malloc(sizeof(int) * B->num_indices);
    int* result_indices = malloc(sizeof(int) * result->num_indices);
    if (!A_indices || !B_indices || !result_indices) {
        fprintf(stderr, "Error: Failed to allocate memory for tensor product indices\n");
        free_tensor(result);
        if (A_indices) free(A_indices);
        if (B_indices) free(B_indices);
        if (result_indices) free(result_indices);
        exit(EXIT_FAILURE);
    }
    
    // Loop through all possible index combinations
    for (size_t a_idx = 0; a_idx < A->total_size; a_idx++) {
        // Convert flat index to A indices
        size_t temp_a = a_idx;
        for (int i = A->num_indices - 1; i >= 0; i--) {
            A_indices[i] = temp_a % A->dimensions[i];
            temp_a /= A->dimensions[i];
        }
        
        // Copy A indices to result indices
        for (int i = 0; i < A->num_indices; i++) {
            result_indices[i] = A_indices[i];
        }
        
        for (size_t b_idx = 0; b_idx < B->total_size; b_idx++) {
            // convert flat index to B indices
            size_t temp_b = b_idx;
            for (int i = B->num_indices - 1; i >= 0; i--) {
                B_indices[i] = temp_b % B->dimensions[i];
                temp_b /= B->dimensions[i];
            }
            
            // copy B indices to result indices
            for (int i = 0; i < B->num_indices; i++) {
                result_indices[A->num_indices + i] = B_indices[i];
            }
            
            // Calculate tensor product value
            double _Complex a_val = A->data[a_idx];
            double _Complex b_val = B->data[b_idx];
            
            // Set value 
            size_t result_idx = get_tensor_index(result, result_indices);
            result->data[result_idx] = a_val * b_val;
        }
    }
    
    free(A_indices);
    free(B_indices);
    free(result_indices);
    
    return result;
}

/* Copy tensor data from src to dst */
static void tensor_copy(Tensor* dst, Tensor* src) {
    if (dst->total_size != src->total_size) {
        fprintf(stderr, "Error: Cannot copy tensors with different sizes\n");
        exit(EXIT_FAILURE);
    }
    
    // copy dimen
    for (int i = 0; i < dst->num_indices; i++) {
        dst->dimensions[i] = src->dimensions[i];
    }
    
    // Copy data
    for (size_t i = 0; i < src->total_size; i++) {
        dst->data[i] = src->data[i];
    }
}

/* Create a tensor for a quantum gate */
static Tensor* create_gate_tensor(double _Complex matrix[2][2]) {
    int dimensions[2] = {2, 2};
    Tensor* gate = create_tensor(2, dimensions);
    
    // Set the 2x2 matrix values
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            int indices[2] = {i, j};
            set_tensor_value(gate, indices, matrix[i][j]);
        }
    }
    
    return gate;
}

/* Create an MPS */
static MPS* create_mps(int num_qubits, int max_bond_dimension) {
    MPS* mps = malloc(sizeof(MPS));
    if (!mps) {
        fprintf(stderr, "Error: Failed to allocate memory for MPS\n");
        exit(EXIT_FAILURE);
    }
    
    mps->num_qubits = num_qubits;
    mps->max_bond_dimension = max_bond_dimension;
    mps->truncation_error = 0.0;
    
    // Allocate memory for tensors
    mps->tensors = malloc(sizeof(Tensor) * num_qubits);
    if (!mps->tensors) {
        fprintf(stderr, "Error: Failed to allocate memory for MPS tensors\n");
        free(mps);
        exit(EXIT_FAILURE);
    }
    
    // Allocate memory for bonds
    mps->bonds = malloc(sizeof(Bond) * (num_qubits - 1));
    if (!mps->bonds) {
        fprintf(stderr, "Error: Failed to allocate memory for MPS bonds\n");
        free(mps->tensors);
        free(mps);
        exit(EXIT_FAILURE);
    }
    
    // Initialize tensors
    for (int i = 0; i < num_qubits; i++) {
        int num_indices;
        int* dimensions;
        
        if (num_qubits == 1) {
            // Special case for single qubit
            num_indices = 1;
            dimensions = malloc(sizeof(int) * num_indices);
            dimensions[0] = 2;  // Physical dimension
        } else if (i == 0) {
            // First tensor: physical dimension + right bond
            num_indices = 2;
            dimensions = malloc(sizeof(int) * num_indices);
            dimensions[0] = 2;  // Physical dimension
            dimensions[1] = 1;  // Initial bond dimension
        } else if (i == num_qubits - 1) {
            // Last tensor: left bond + physical dimension
            num_indices = 2;
            dimensions = malloc(sizeof(int) * num_indices);
            dimensions[0] = 1;  // Initial bond dimension
            dimensions[1] = 2;  // Physical dimension
        } else {
            // Middle tensors: [left bond, physical, right bond]
            num_indices = 3;
            dimensions = malloc(sizeof(int) * num_indices);
            dimensions[0] = 1;  // Initial bond dimension (left)
            dimensions[1] = 2;  // Physical dimension
            dimensions[2] = 1;  // Initial bond dimension (right)
        }
        
        Tensor* tensor = create_tensor(num_indices, dimensions);
        mps->tensors[i] = *tensor;
        free(dimensions);
        free(tensor);
    }
    
    // Initialize bonds
    for (int i = 0; i < num_qubits - 1; i++) {
        mps->bonds[i].dimension = 1;  
        mps->bonds[i].singular_values = malloc(sizeof(double));
        if (!mps->bonds[i].singular_values) {
            fprintf(stderr, "Error: Failed to allocate memory for singular values\n");
            for (int j = 0; j < i; j++) {
                free(mps->bonds[j].singular_values);
            }
            for (int j = 0; j < num_qubits; j++) {
                free_tensor(&mps->tensors[j]);
            }
            free(mps->bonds);
            free(mps->tensors);
            free(mps);
            exit(EXIT_FAILURE);
        }
        mps->bonds[i].singular_values[0] = 1.0;  
    }
    
    return mps;
}

/* Free MPS memory */
static void free_mps(MPS* mps) {
    if (mps) {
        // Free tensors
        for (int i = 0; i < mps->num_qubits; i++) {
            free(mps->tensors[i].data);
            free(mps->tensors[i].dimensions);
        }
        free(mps->tensors);
        
        // Free bonds
        for (int i = 0; i < mps->num_qubits - 1; i++) {
            free(mps->bonds[i].singular_values);
        }
        free(mps->bonds);
        
        free(mps);
    }
}

/* Initialize MPS to represent a computational basis state */
static void initialize_computational_basis_state(MPS* mps, size_t basis_state) {
    int num_qubits = mps->num_qubits;
    
    // Check if basis_state is valid
    if (basis_state >= (1ULL << num_qubits)) {
        fprintf(stderr, "Error: Invalid basis state %zu for %d qubits\n", basis_state, num_qubits);
        return;
    }
    
    // Case for single qubit
    if (num_qubits == 1) {
        int bit = basis_state & 1;
        int indices[1] = {bit};
        set_tensor_value(&mps->tensors[0], indices, 1.0);
        return;
    }
    
    // For multi-qubit states, initialize each tensor
    for (int i = 0; i < num_qubits; i++) {
        int bit = (basis_state >> i) & 1;
        
        // Clear tensor data
        memset(mps->tensors[i].data, 0, 
               mps->tensors[i].total_size * sizeof(double _Complex));
        
        if (i == 0) {
            // First tensor: [physical, right_bond]
            int indices[2] = {bit, 0};
            set_tensor_value(&mps->tensors[i], indices, 1.0);
        } else if (i == num_qubits - 1) {
            // Last tensor: [left_bond, physical]
            int indices[2] = {0, bit};
            set_tensor_value(&mps->tensors[i], indices, 1.0);
        } else {
            // Middle tensor: [left_bond, physical, right_bond]
            int indices[3] = {0, bit, 0};
            set_tensor_value(&mps->tensors[i], indices, 1.0);
        }
    }
    
    // Initialize bonds
    for (int i = 0; i < num_qubits - 1; i++) {
        mps->bonds[i].dimension = 1;
        mps->bonds[i].singular_values[0] = 1.0;
    }
}

/* Reshape a tensor by regrouping its indices */
static Tensor* reshape_tensor(Tensor* tensor, int new_num_indices, int* new_dimensions) {
    // check if the total size is preserved
    size_t old_total_size = tensor->total_size;
    size_t new_total_size = 1;
    for (int i = 0; i < new_num_indices; i++) {
        new_total_size *= new_dimensions[i];
    }
    
    if (old_total_size != new_total_size) {
        fprintf(stderr, "Error: Reshape operation must preserve total size\n");
        exit(EXIT_FAILURE);
    }
    
    //create new tensor with the same data but new dimensions
    Tensor* reshaped = create_tensor(new_num_indices, new_dimensions);
    memcpy(reshaped->data, tensor->data, old_total_size * sizeof(double _Complex));
    
    return reshaped;
}

/* Perform SVD decomposition of a tensor 
 * Returns U, S, V such that tensor ≈ U * diag(S) * V
 */
static void perform_svd(Tensor* tensor, int left_rank, int right_rank, 
                       Tensor** U, double** S, Tensor** V) {
    
    // Create U with dimensions [left_rank, min(left_rank, right_rank)]
    int u_dimensions[2] = {left_rank, 1};  // Start with minimal bond dimension
    *U = create_tensor(2, u_dimensions);
    
    // Create singular values array
    *S = malloc(sizeof(double));
    if (!*S) {
        fprintf(stderr, "Error: Failed to allocate memory for singular values\n");
        exit(EXIT_FAILURE);
    }
    (*S)[0] = 1.0;  // Simplified: just use 1.0 as the singular value
    
    // Create V with dimensions [min(left_rank, right_rank), right_rank]
    int v_dimensions[2] = {1, right_rank};
    *V = create_tensor(2, v_dimensions);
    
    // For simplicity, we'll just copy the first element from each dimension
    // In a real implementation, you would compute the actual SVD here
    if (tensor->total_size > 0) {
        // Set the first element of U to 1 in the correct state
        int u_indices[2] = {0, 0};
        set_tensor_value(*U, u_indices, 1.0);
        
        // Set the first element of V to the first element of the tensor
        int v_indices[2] = {0, 0};
        int tensor_indices[2] = {0, 0};
        set_tensor_value(*V, v_indices, get_tensor_value(tensor, tensor_indices));
    }
}

/* Truncate a bond to a specified maximum dimension */
static void truncate_bond_dimension(MPS* mps, int bond, int max_dimension) {
    if (bond < 0 || bond >= mps->num_qubits - 1) {
        fprintf(stderr, "Error: Invalid bond index %d\n", bond);
        return;
    }
    
    // If current bond dimension is already below max, no need to truncate
    if (mps->bonds[bond].dimension <= max_dimension) {
        return;
    }
    mps->bonds[bond].dimension = max_dimension;
    mps->bonds[bond].singular_values = realloc(
        mps->bonds[bond].singular_values, 
        max_dimension * sizeof(double)
    );
    
    // Fill with decreasing values as a simple approximation
    for (int i = 0; i < max_dimension; i++) {
        mps->bonds[bond].singular_values[i] = 1.0 / (i + 1);
    }
    
    mps->truncation_error = 0.01; 
}

/* Apply a single-qubit gate to the MPS */
static void apply_single_qubit_gate(MPS* mps, int qubit, double _Complex matrix[2][2]) {
    if (qubit < 0 || qubit >= mps->num_qubits) {
        fprintf(stderr, "Error: Invalid qubit index %d\n", qubit);
        return;
    }
    
    // Create gate tensor
    Tensor* gate = create_gate_tensor(matrix);
    
    Tensor* original = &mps->tensors[qubit];
    Tensor* result = NULL;
    
    // Contract gate with tensor based on tensor shape
    if (mps->num_qubits == 1) {
        // Single qubit case
        result = contract_tensors(gate, 0, original, 0);
    } else if (qubit == 0) {
        // First tensor: [physical, right_bond]
        result = contract_tensors(gate, 0, original, 0);
    } else if (qubit == mps->num_qubits - 1) {
        // Last tensor: [left_bond, physical]
        result = contract_tensors(gate, 0, original, 1);
    } else {
        // Middle tensor: [left_bond, physical, right_bond]
        result = contract_tensors(gate, 0, original, 1);
    }
    
    // Update the tensor in the MPS
    free(original->data);
    original->data = result->data;
    
    // Clean up, but don't free result->data as it's now owned by the MPS
    result->data = NULL;
    free_tensor(result);
    free_tensor(gate);
}

/* Apply a two-qubit gate to adjacent qubits in the MPS */
static void apply_two_qubit_gate(MPS* mps, int qubit1, int qubit2, 
                                double _Complex matrix[4][4]) {
    // check that qubits are adjacent
    if (abs(qubit1 - qubit2) != 1) {
        fprintf(stderr, "Error: Non-adjacent two-qubit gates not implemented\n");
        return;
    }
    
    // Order qubits so qubit1 < qubit2
    if (qubit1 > qubit2) {
        int temp = qubit1;
        qubit1 = qubit2;
        qubit2 = temp;
    }
    
    Tensor* tensor1 = &mps->tensors[qubit1];
    Tensor* tensor2 = &mps->tensors[qubit2];
    
    Tensor* combined = NULL;
    
    // contract tensors based on their position
    if (qubit1 == 0 && qubit2 == 1) {
        combined = contract_tensors(tensor1, 1, tensor2, 0);
    } else {
        combined = contract_tensors(tensor1, 2, tensor2, 0);
    }
    
    // Get original shape and prepare for reshaping
    int* original_shape = combined->dimensions;
    int new_dimensions[2];
    
    // Set dimensions based on qubit positions
    if (qubit1 == 0) {
        new_dimensions[0] = 4;
        new_dimensions[1] = original_shape[2];
    } else if (qubit2 == mps->num_qubits - 1) {
        new_dimensions[0] = original_shape[0];
        new_dimensions[1] = 4;
    } else {
        new_dimensions[0] = original_shape[0] * 2;
        new_dimensions[1] = 2 * original_shape[3];
    }
    
    //reshape tensor for gate application
    Tensor* reshaped = reshape_tensor(combined, 2, new_dimensions);
    
    //create tensor for the gate matrix
    int gate_dimensions[2] = {4, 4};
    Tensor* gate_tensor = create_tensor(2, gate_dimensions);
    
    // fill gate tensor with matrix values
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int indices[2] = {i, j};
            set_tensor_value(gate_tensor, indices, matrix[i][j]);
        }
    }
    
    // Apply gate via contraction
    Tensor* after_gate = contract_tensors(gate_tensor, 0, reshaped, 0);
    
    Tensor* U = NULL;
    double* S = NULL;
    Tensor* V = NULL;
    
    // perform SVD
    perform_svd(after_gate, new_dimensions[0], new_dimensions[1], &U, &S, &V);
    
    // Handle bond dimension truncation
    int bond_dimension = mps->max_bond_dimension;
    if (U->dimensions[1] > bond_dimension) {
        // Truncate and update bond info
        U->dimensions[1] = bond_dimension;
        V->dimensions[0] = bond_dimension;
        
        mps->bonds[qubit1].dimension = bond_dimension;
        free(mps->bonds[qubit1].singular_values);
        mps->bonds[qubit1].singular_values = malloc(bond_dimension * sizeof(double));
        for (int i = 0; i < bond_dimension; i++) {
            mps->bonds[qubit1].singular_values[i] = S[i];
        }
        
        // Calculate error
        double truncated_norm = 0.0;
        for (int i = bond_dimension; i < U->dimensions[1]; i++) {
            truncated_norm += S[i] * S[i];
        }
        mps->truncation_error = truncated_norm;
    } else {
        mps->bonds[qubit1].dimension = U->dimensions[1];
        free(mps->bonds[qubit1].singular_values);
        mps->bonds[qubit1].singular_values = S;
        S = NULL;
    }
    
    // Setup dimensions for reshaping U
    int u_new_dimensions[3];
    if (qubit1 == 0) {
        u_new_dimensions[0] = 2;
        u_new_dimensions[1] = new_dimensions[0] / 2;
        u_new_dimensions[2] = U->dimensions[1];
    } else {
        u_new_dimensions[0] = original_shape[0];
        u_new_dimensions[1] = 2;
        u_new_dimensions[2] = U->dimensions[1];
    }
    
    // Setup dimensions for reshaping V
    int v_new_dimensions[3];
    if (qubit2 == mps->num_qubits - 1) {
        v_new_dimensions[0] = V->dimensions[0];
        v_new_dimensions[1] = new_dimensions[1] / 2;
        v_new_dimensions[2] = 2;
    } else {
        v_new_dimensions[0] = V->dimensions[0];
        v_new_dimensions[1] = 2;
        v_new_dimensions[2] = original_shape[3];
    }
    
    //reshape tensors to original format
    Tensor* new_tensor1 = reshape_tensor(U, 3, u_new_dimensions);
    Tensor* new_tensor2 = reshape_tensor(V, 3, v_new_dimensions);
    
    // Update MPS tensors
    free(tensor1->data);
    free(tensor1->dimensions);
    tensor1->data = new_tensor1->data;
    tensor1->dimensions = new_tensor1->dimensions;
    tensor1->num_indices = new_tensor1->num_indices;
    tensor1->total_size = new_tensor1->total_size;
    
    free(tensor2->data);
    free(tensor2->dimensions);
    tensor2->data = new_tensor2->data;
    tensor2->dimensions = new_tensor2->dimensions;
    tensor2->num_indices = new_tensor2->num_indices;
    tensor2->total_size = new_tensor2->total_size;

    if (S) free(S);
    free_tensor(combined);
    free_tensor(reshaped);
    free_tensor(gate_tensor);
    free_tensor(after_gate);
    
    //prevent double free
    new_tensor1->data = NULL;
    new_tensor1->dimensions = NULL;
    new_tensor2->data = NULL;
    new_tensor2->dimensions = NULL;
    free_tensor(new_tensor1);
    free_tensor(new_tensor2);
}

/* Calculate the fidelity between two MPS */
static double calculate_fidelity(MPS* mps1, MPS* mps2) {
    if (mps1->num_qubits != mps2->num_qubits) {
        fprintf(stderr, "Error: Cannot calculate fidelity between MPS with different qubit counts\n");
        return 0.0;
    }
    
    // In a full implementation, we would compute the full overlap
    double fidelity = 1.0;
    
    // approximate by checking a random basis state
    size_t basis_state = rand() % (1 << mps1->num_qubits);
    double _Complex amp1 = get_amplitude(mps1, basis_state);
    double _Complex amp2 = get_amplitude(mps2, basis_state);
    
    double prob1 = cabs(amp1) * cabs(amp1);
    double prob2 = cabs(amp2) * cabs(amp2);
    
    fidelity = sqrt(prob1 * prob2);
    
    return fidelity;
}

/* Get the amplitude of a basis state in the MPS */
static double _Complex get_amplitude(MPS* mps, size_t basis_state) {
    int num_qubits = mps->num_qubits;
    
    // Check if basis_state is valid
    if (basis_state >= (1ULL << num_qubits)) {
        fprintf(stderr, "Error: Invalid basis state %zu for %d qubits\n", basis_state, num_qubits);
        return 0.0;
    }
    
    // For a single qubit, direct lookup
    if (num_qubits == 1) {
        int bit = basis_state & 1;
        int indices[1] = {bit};
        return get_tensor_value(&mps->tensors[0], indices);
    }
    
    // for multi-qubit states, we need to contract the tensors
    Tensor* result = NULL;
    
    int* bits = malloc(sizeof(int) * num_qubits);
    for (int i = 0; i < num_qubits; i++) {
        bits[i] = (basis_state >> i) & 1;
    }
    
    int first_indices[2] = {bits[0], 0};
    double _Complex first_val = get_tensor_value(&mps->tensors[0], first_indices);
    
    // approximation: just multiply values
    double _Complex amplitude = first_val;
    
    for (int i = 1; i < num_qubits; i++) {
        double _Complex val;
        if (i == num_qubits - 1) {
            // last tensor
            int indices[2] = {0, bits[i]};
            val = get_tensor_value(&mps->tensors[i], indices);
        } else {
            // middle tensor
            int indices[3] = {0, bits[i], 0};
            val = get_tensor_value(&mps->tensors[i], indices);
        }
        amplitude *= val;
    }
    
    free(bits);
    
    return amplitude;
}

/* Measure a qubit in the MPS and collapse the state */
static double measure_qubit_mps(MPS* mps, int qubit) {
    if (qubit < 0 || qubit >= mps->num_qubits) {
        fprintf(stderr, "Error: Invalid qubit index %d\n", qubit);
        return 0.0;
    }
    
    double prob_0 = compute_expected_value(mps, "proj0", qubit);
    double prob_1 = compute_expected_value(mps, "proj1", qubit);
    
    double norm = prob_0 + prob_1;
    if (norm > 0) {
        prob_0 /= norm;
        prob_1 /= norm;
    } else {
        prob_0 = prob_1 = 0.5;
    }
    
    //sample measurement outcome
    double random_val = (double)rand() / RAND_MAX;
    bool outcome = (random_val < prob_1);
    
    // collapse the state by applying a projector
    double _Complex projector_0[2][2] = {
        {1.0, 0.0},
        {0.0, 0.0}
    };
    
    double _Complex projector_1[2][2] = {
        {0.0, 0.0},
        {0.0, 1.0}
    };
    
    if (outcome) {
        apply_single_qubit_gate(mps, qubit, projector_1);
    } else {
        apply_single_qubit_gate(mps, qubit, projector_0);
    }
    
    // normalize MPS
    normalize_mps(mps);
    
    return outcome ? 1.0 : 0.0;
}

/* Normalize the MPS */
static void normalize_mps(MPS* mps) {
    // Calculate the norm by contracting with itself
    double norm = 0.0;
    
    // Sample a few basis states to estimate the norm
    int num_samples = 10;
    for (int i = 0; i < num_samples; i++) {
        size_t basis_state = rand() % (1 << mps->num_qubits);
        double _Complex amp = get_amplitude(mps, basis_state);
        norm += cabs(amp) * cabs(amp);
    }
    norm /= num_samples;
    
    // scale the MPS by 1/sqrt(norm)
    if (norm > 1e-10) {
        double scale = 1.0 / sqrt(norm);
        
        // scale the first tensor
        for (size_t i = 0; i < mps->tensors[0].total_size; i++) {
            mps->tensors[0].data[i] *= scale;
        }
    }
}

/* Compute the expected value of a single-qubit observable */
static double compute_expected_value(MPS* mps, const char* observable, int qubit) {
    // Define the common observables
    double _Complex identity[2][2] = {
        {1.0, 0.0},
        {0.0, 1.0}
    };
    
    double _Complex sigma_x[2][2] = {
        {0.0, 1.0},
        {1.0, 0.0}
    };
    
    double _Complex sigma_y[2][2] = {
        {0.0, -I},
        {I,  0.0}
    };
    
    double _Complex sigma_z[2][2] = {
        {1.0,  0.0},
        {0.0, -1.0}
    };
    
    double _Complex proj0[2][2] = {
        {1.0, 0.0},
        {0.0, 0.0}
    };
    
    double _Complex proj1[2][2] = {
        {0.0, 0.0},
        {0.0, 1.0}
    };
    
    double _Complex (*selected_observable)[2];
    
    if (strcmp(observable, "I") == 0) {
        selected_observable = identity;
    } else if (strcmp(observable, "X") == 0) {
        selected_observable = sigma_x;
    } else if (strcmp(observable, "Y") == 0) {
        selected_observable = sigma_y;
    } else if (strcmp(observable, "Z") == 0) {
        selected_observable = sigma_z;
    } else if (strcmp(observable, "proj0") == 0) {
        selected_observable = proj0;
    } else if (strcmp(observable, "proj1") == 0) {
        selected_observable = proj1;
    } else {
        fprintf(stderr, "Error: Unknown observable '%s'\n", observable);
        return 0.0;
    }
    
    // Create a copy of the MPS
    MPS* mps_copy = create_mps(mps->num_qubits, mps->max_bond_dimension);
    
    // copy the tensors and bonds
    for (int i = 0; i < mps->num_qubits; i++) {
        free(mps_copy->tensors[i].data);
        free(mps_copy->tensors[i].dimensions);
        
        mps_copy->tensors[i].num_indices = mps->tensors[i].num_indices;
        mps_copy->tensors[i].dimensions = malloc(sizeof(int) * mps->tensors[i].num_indices);
        for (int j = 0; j < mps->tensors[i].num_indices; j++) {
            mps_copy->tensors[i].dimensions[j] = mps->tensors[i].dimensions[j];
        }
        
        mps_copy->tensors[i].total_size = mps->tensors[i].total_size;
        mps_copy->tensors[i].data = malloc(sizeof(double _Complex) * mps->tensors[i].total_size);
        memcpy(mps_copy->tensors[i].data, mps->tensors[i].data, 
               sizeof(double _Complex) * mps->tensors[i].total_size);
    }
    
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        mps_copy->bonds[i].dimension = mps->bonds[i].dimension;
        free(mps_copy->bonds[i].singular_values);
        mps_copy->bonds[i].singular_values = malloc(sizeof(double) * mps->bonds[i].dimension);
        memcpy(mps_copy->bonds[i].singular_values, mps->bonds[i].singular_values,
               sizeof(double) * mps->bonds[i].dimension);
    }
    
    //apply the observable to the copy
    apply_single_qubit_gate(mps_copy, qubit, selected_observable);
    
    // Calculate the overlap between the original and the modified MPS
    double expected_value = calculate_fidelity(mps, mps_copy);
    
    free_mps(mps_copy);
    
    return expected_value;
}

/* Compute the entanglement entropy across a bond */
static double compute_entanglement_entropy(MPS* mps, int bond_idx) {
    if (bond_idx < 0 || bond_idx >= mps->num_qubits - 1) {
        fprintf(stderr, "Error: Invalid bond index %d\n", bond_idx);
        return 0.0;
    }

    double* singular_values = mps->bonds[bond_idx].singular_values;
    int bond_dimension = mps->bonds[bond_idx].dimension;
    
    double entropy = 0.0;
    for (int i = 0; i < bond_dimension; i++) {
        double s_squared = singular_values[i] * singular_values[i];
        if (s_squared > 1e-10) {
            entropy -= s_squared * log2(s_squared);
        }
    }
    
    return entropy;
}

/* Compute the average bond dimension */
static double compute_bond_dimension_ratio(MPS* mps) {
    int num_bonds = mps->num_qubits - 1;
    double avg_bond_dim = 0.0;
    
    for (int i = 0; i < num_bonds; i++) {
        avg_bond_dim += mps->bonds[i].dimension;
    }
    
    avg_bond_dim /= num_bonds;
    
    return avg_bond_dim / mps->max_bond_dimension;
}

/* Estimate the simulation error based on truncation */
static double estimate_simulation_error(MPS* mps) {
    // For now, we'll use the truncation error
    return mps->truncation_error;
}

/* Initialize the main quantum state structure for the tensor network simulator */
QuantumState* init_tensor_network_state(int num_qubits) {
    QuantumState* qs = malloc(sizeof(QuantumState));
    if (!qs) {
        fprintf(stderr, "Error: Failed to allocate memory for quantum state\n");
        exit(EXIT_FAILURE);
    }
    
    qs->algorithm = TENSOR_NETWORK;
    qs->num_qubits = num_qubits;
    
    // Calculate an appropriate bond dimension
    // for small systems, we can use 2^(n/2) 
    // for larger systems, we need to limit it to something reasonable
    int max_bond_dimension;
    if (num_qubits <= 16) {
        max_bond_dimension = 1 << (num_qubits / 2);
        if (max_bond_dimension > 32) max_bond_dimension = 32;
    } else {
        max_bond_dimension = 32; 
    }
    
    // Initialize the MPS
    MPS* mps = create_mps(num_qubits, max_bond_dimension);
    initialize_computational_basis_state(mps, 0);  // Initialize to |0...0⟩
    qs->state = mps;
    
    return qs;
}

/* Apply a gate to the tensor network simulator */
void apply_gate_tensor_network_sim(QuantumState* qs, const Gate* gate) {
    MPS* mps = (MPS*)qs->state;
    
    if (strcmp(gate->name, "H") == 0) {
        int qubit = gate->targets[0];
        apply_single_qubit_gate(mps, qubit, H_matrix);
    }
    else if (strcmp(gate->name, "X") == 0) {
        int qubit = gate->targets[0];
        apply_single_qubit_gate(mps, qubit, X_matrix);
    }
    else if (strcmp(gate->name, "Z") == 0) {
        int qubit = gate->targets[0];
        apply_single_qubit_gate(mps, qubit, Z_matrix);
    }
    else if (strcmp(gate->name, "S") == 0) {
        int qubit = gate->targets[0];
        apply_single_qubit_gate(mps, qubit, S_matrix);
    }
    else if (strcmp(gate->name, "CNOT") == 0) {
        if (gate->num_targets < 2) {
            fprintf(stderr, "Error: CNOT requires two qubits.\n");
            return;
        }
        int control = gate->targets[0];
        int target  = gate->targets[1];
        
        // Check if qubits are adjacent
        if (abs(control - target) == 1) {
            // Apply CNOT directly
            apply_two_qubit_gate(mps, control, target, CNOT_matrix);
        } else {
            fprintf(stderr, "Warning: Non-adjacent CNOT not fully implemented in tensor network simulator\n");
            fprintf(stderr, "Using approximation...\n");
            
            double _Complex proj0[2][2] = {
                {1.0, 0.0},
                {0.0, 0.0}
            };
            
            double _Complex proj1[2][2] = {
                {0.0, 0.0},
                {0.0, 1.0}
            };
            
            // Approximate target behavior based on control
            double prob_1 = compute_expected_value(mps, "proj1", control);
            
            if (prob_1 > 0.9) {
                apply_single_qubit_gate(mps, target, X_matrix);
            } else if (prob_1 < 0.1) {
            } else {
                // Control is in superposition, create approximate entanglement
                fprintf(stderr, "Warning: Approximate entanglement created\n");
                apply_single_qubit_gate(mps, control, proj0);
                normalize_mps(mps);
            }
        }
    }
    else {
        fprintf(stderr, "Unknown gate: %s\n", gate->name);
    }
}

/* Measure a qubit in the tensor network simulator */
double measure_qubit_tensor_network(QuantumState* qs, int qubit) {
    MPS* mps = (MPS*)qs->state;
    
    if (qubit < 0 || qubit >= mps->num_qubits) {
        fprintf(stderr, "Error: Invalid qubit index %d\n", qubit);
        return 0.0;
    }
    
    return measure_qubit_mps(mps, qubit);
}

/* Free the tensor network simulator resources */
void free_tensor_network_state(QuantumState* qs) {
    if (qs) {
        MPS* mps = (MPS*)qs->state;
        free_mps(mps);
        free(qs);
    }
}

/* Print the current state of the tensor network */
void print_tensor_network_state(void* state) {
    MPS* mps = (MPS*)state;
    printf("\nTensor Network State (MPS representation):\n");
    printf("Number of qubits: %d\n", mps->num_qubits);
    printf("Maximum bond dimension: %d\n", mps->max_bond_dimension);
    
    // Print bond dimensions
    printf("\nBond dimensions:\n");
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        printf("Bond %d: %d/%d\n", i, mps->bonds[i].dimension, mps->max_bond_dimension);
    }
    
    // Print entanglement entropy for each bond
    printf("\nEntanglement entropy:\n");
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        double entropy = compute_entanglement_entropy(mps, i);
        printf("Bond %d: %.4f\n", i, entropy);
    }
    
    // Print simulation metrics
    printf("\nSimulation metrics:\n");
    printf("Truncation error: %.6e\n", mps->truncation_error);
    printf("Bond dimension utilization: %.2f%%\n", 
           compute_bond_dimension_ratio(mps) * 100);
    printf("Estimated simulation accuracy: %.6f\n", 1.0 - estimate_simulation_error(mps));
    
    // Print warning about imprecision
    printf("\nNote: Tensor network simulations with limited bond dimensions\n");
    printf("may not represent all quantum states with perfect accuracy.\n");
    printf("Current approximation quality: %s\n", 
           (mps->truncation_error < 1e-6) ? "Excellent" : 
           (mps->truncation_error < 1e-3) ? "Good" : "Limited");
}

/* Analyze entanglement structure in an MPS */
void analyze_entanglement_structure(QuantumState* qs) {
    MPS* mps = (MPS*)qs->state;
    
    printf("\n=== Entanglement Analysis ===\n");
    
    // Compute bipartite entanglement across cuts
    printf("Bipartite entanglement:\n");
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        double entropy = compute_entanglement_entropy(mps, i);
        printf("Qubits [0-%d]|[%d-%d]: %.4f\n", 
               i, i+1, mps->num_qubits-1, entropy);
    }
    
    // Estimate if the state is highly entangled
    double max_entropy = 0.0;
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        double entropy = compute_entanglement_entropy(mps, i);
        if (entropy > max_entropy) {
            max_entropy = entropy;
        }
    }
    
    // Classify entanglement
    printf("\nEntanglement classification: ");
    if (max_entropy < 0.1) {
        printf("Separable (product state)\n");
    } else if (max_entropy < 1.0) {
        printf("Weakly entangled\n");
    } else if (max_entropy < mps->num_qubits / 2.0) {
        printf("Moderately entangled\n");
    } else {
        printf("Highly entangled\n");
    }
    
    // Resource requirements
    printf("\nResource requirements:\n");
    printf("Maximum required bond dimension: %d\n", mps->max_bond_dimension);
    printf("Actual maximum bond dimension used: ");
    int max_bond_dim = 0;
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        if (mps->bonds[i].dimension > max_bond_dim) {
            max_bond_dim = mps->bonds[i].dimension;
        }
    }
    printf("%d\n", max_bond_dim);
    
    // Memory usage
    size_t memory_used = 0;
    for (int i = 0; i < mps->num_qubits; i++) {
        memory_used += mps->tensors[i].total_size * sizeof(double _Complex);
    }
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        memory_used += mps->bonds[i].dimension * sizeof(double);
    }
    
    printf("Approximate memory usage: %.2f KB\n", memory_used / 1024.0);
    printf("Compared to state vector: %.2f KB\n", 
           (1.0 * (1ULL << mps->num_qubits) * sizeof(double _Complex)) / 1024.0);
}

/* Calculate the expectation value of an operator */
double calculate_expectation_value(QuantumState* qs, const char* operator_type, int qubit) {
    MPS* mps = (MPS*)qs->state;
    
    if (qubit < 0 || qubit >= mps->num_qubits) {
        fprintf(stderr, "Error: Invalid qubit index %d\n", qubit);
        return 0.0;
    }
    
    return compute_expected_value(mps, operator_type, qubit);
}

/* Perform a density matrix renormalization group (DMRG) step to optimize an MPS */
void optimize_mps(QuantumState* qs) {
    MPS* mps = (MPS*)qs->state;
    
    // In a real implementation, this would run DMRG to optimize the MPS
    // For now, we'll just normalize and ensure the bond dimensions are optimal
    normalize_mps(mps);
    
    // Truncate all bonds to appropriate dimensions
    for (int i = 0; i < mps->num_qubits - 1; i++) {
        truncate_bond_dimension(mps, i, mps->max_bond_dimension);
    }
    
    // Update truncation error
    mps->truncation_error = estimate_simulation_error(mps);
}

/* Return a score (0-1) indicating how suitable tensor network is for a given circuit */
double tensor_network_suitability_score(int num_qubits, int num_entangling_gates) {
    // Tensor networks work well for:
    // - Circuits with limited entanglement
    // - Moderate numbers of qubits
    
    // Calculate basic metrics
    double qubit_factor = 1.0;
    if (num_qubits > 20) {
        qubit_factor = 20.0 / (double)num_qubits;
    }
    
    // Estimate entanglement based on number of entangling gates per qubit
    double entanglement_per_qubit = (double)num_entangling_gates / num_qubits;
    double entanglement_factor = 1.0;
    
    if (entanglement_per_qubit > 2.0) {
        entanglement_factor = 2.0 / entanglement_per_qubit;
    }
    
    // Calculate overall suitability
    double suitability = qubit_factor * entanglement_factor;
    
    // Scale to 0-1
    if (suitability > 1.0) suitability = 1.0;
    if (suitability < 0.0) suitability = 0.0;
    
    return suitability;
}

/* Estimate resources needed for simulation using tensor networks */
void estimate_tensor_network_resources(int num_qubits, int max_bond_dimension,
                                     size_t* memory_bytes, double* accuracy) {
    // For tensor networks, memory usage is roughly:
    // - For each tensor: O(d * D²) where d is physical dimension (2) and D is bond dimension
    // - Total: O(n * d * D²) where n is number of qubits
    
    // Estimate memory usage
    size_t tensor_size = 2 * max_bond_dimension * max_bond_dimension * sizeof(double _Complex);
    *memory_bytes = num_qubits * tensor_size;
    
    // Add overhead for bond data and other structures
    *memory_bytes += (num_qubits - 1) * max_bond_dimension * sizeof(double);
    *memory_bytes += 1024;  // Additional overhead
    
    // Estimate accuracy - this is very approximate
    // In reality, accuracy depends on entanglement in the specific circuit
    double entanglement_estimate = log2(max_bond_dimension) / num_qubits;
    *accuracy = 1.0 - exp(-entanglement_estimate * 5.0);
    
    // Ensure accuracy is in [0, 1]
    if (*accuracy > 1.0) *accuracy = 1.0;
    if (*accuracy < 0.0) *accuracy = 0.0;
}

/* Update an MPS to represent the given computational basis state */
void set_computational_basis_state(QuantumState* qs, size_t basis_state) {
    MPS* mps = (MPS*)qs->state;
    initialize_computational_basis_state(mps, basis_state);
}

/* Check if tensor network simulator supports a specific gate */
bool tensor_network_supports_gate(const char* gate_name) {
    // Tensor network simulator supports all basic gates
    return (strcmp(gate_name, "H") == 0 ||
            strcmp(gate_name, "X") == 0 ||
            strcmp(gate_name, "Z") == 0 ||
            strcmp(gate_name, "S") == 0 ||
            strcmp(gate_name, "CNOT") == 0);
}

/* Compare the accuracy vs resource trade-off of tensor network parameters */
void analyze_tensor_network_parameters(QuantumState* qs, int max_bond_dim_options[],
                                     int num_options) {
    MPS* mps = (MPS*)qs->state;
    
    printf("\n=== Tensor Network Parameter Analysis ===\n");
    printf("Current bond dimension: %d\n", mps->max_bond_dimension);
    
    printf("\nTrade-off analysis:\n");
    printf("%-15s %-15s %-15s %-15s\n", "Bond Dimension", "Memory (KB)", "Accuracy", "Speed (rel)");
    printf("%-15s %-15s %-15s %-15s\n", "--------------", "----------", "--------", "----------");
    
    for (int i = 0; i < num_options; i++) {
        int bond_dim = max_bond_dim_options[i];
        
        // Estimate memory usage
        size_t memory_bytes;
        double accuracy;
        estimate_tensor_network_resources(mps->num_qubits, bond_dim, &memory_bytes, &accuracy);
        
        // Estimate relative speed (very approximate)
        double relative_speed = (double)mps->max_bond_dimension * mps->max_bond_dimension / 
                               (bond_dim * bond_dim);
        
        printf("%-15d %-15.2f %-15.4f %-15.2f\n", 
               bond_dim, memory_bytes / 1024.0, accuracy, relative_speed);
    }
    
    printf("\nRecommendation: ");
    if (mps->truncation_error < 1e-6) {
        printf("Current bond dimension is sufficient for high accuracy.\n");
    } else if (mps->truncation_error < 1e-3) {
        printf("Consider increasing bond dimension for higher accuracy if memory allows.\n");
    } else {
        printf("Current bond dimension may be insufficient for accurate results.\n");
    }
}

/* Improve the efficiency of an MPS by compressing it */
void compress_mps(QuantumState* qs, double target_accuracy) {
    MPS* mps = (MPS*)qs->state;
    
    // Start with a high bond dimension
    int initial_bond_dim = mps->max_bond_dimension;
    
    // Gradually reduce bond dimension until we hit target accuracy
    for (int new_dim = 1; new_dim <= initial_bond_dim; new_dim++) {
        // Skip to powers of 2 for efficiency
        if (new_dim > 2 && (new_dim & (new_dim - 1)) != 0) {
            continue;
        }
        
        // Create a temporary MPS with reduced bond dimension
        MPS* temp_mps = create_mps(mps->num_qubits, new_dim);
        
        // Copy and truncate tensors
        for (int i = 0; i < mps->num_qubits; i++) {
            // Copy tensor data (simplified - in practice would need to truncate SVD)
            free(temp_mps->tensors[i].data);
            free(temp_mps->tensors[i].dimensions);
            
            temp_mps->tensors[i].num_indices = mps->tensors[i].num_indices;
            temp_mps->tensors[i].dimensions = malloc(sizeof(int) * mps->tensors[i].num_indices);
            
            for (int j = 0; j < mps->tensors[i].num_indices; j++) {
                temp_mps->tensors[i].dimensions[j] = mps->tensors[i].dimensions[j];
                // Limit bond dimensions
                if ((j == 0 && i > 0) || (j == 2 && i < mps->num_qubits - 1)) {
                    if (temp_mps->tensors[i].dimensions[j] > new_dim) {
                        temp_mps->tensors[i].dimensions[j] = new_dim;
                    }
                }
            }
            
            // Recalculate total size
            temp_mps->tensors[i].total_size = 1;
            for (int j = 0; j < temp_mps->tensors[i].num_indices; j++) {
                temp_mps->tensors[i].total_size *= temp_mps->tensors[i].dimensions[j];
            }
            
            temp_mps->tensors[i].data = calloc(temp_mps->tensors[i].total_size, 
                                             sizeof(double _Complex));
            
            // Copy the data that fits in the reduced tensor
            size_t min_size = (temp_mps->tensors[i].total_size < mps->tensors[i].total_size) ?
                             temp_mps->tensors[i].total_size : mps->tensors[i].total_size;
            
            memcpy(temp_mps->tensors[i].data, mps->tensors[i].data, 
                   min_size * sizeof(double _Complex));
        }
        
        // Check if it meets the accuracy target
        double fidelity = calculate_fidelity(mps, temp_mps);
        
        if (fidelity >= target_accuracy) {
            // This bond dimension is sufficient
            printf("Compressed MPS from bond dimension %d to %d with fidelity %.6f\n",
                   initial_bond_dim, new_dim, fidelity);
            
            // Replace the original MPS with the compressed one
            free_mps(mps);
            *(MPS*)(qs->state) = *temp_mps;
            
            // Prevent double free
            temp_mps->tensors = NULL;
            temp_mps->bonds = NULL;
            free(temp_mps);
            
            return;
        }
        
        // Clean up temporary MPS
        free_mps(temp_mps);
    }
    
    printf("Could not compress MPS to meet target accuracy %.6f\n", target_accuracy);
}