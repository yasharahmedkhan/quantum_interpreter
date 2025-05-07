
#include "quantum_interpreter.h"

/* 
 * circuit_diagram.c
 *
 * The circuit diagram component visualizes quantum circuits using ASCII art.
 */

/* Helper function to calculate the width needed for each column in the circuit */
static int get_gate_width(OpCode code) {
    switch (code) {
        case OP_H:
        case OP_X:
        case OP_Z:
            return 3;  //[H], [X], [Z]
        case OP_CNOT:
            return 3;  // [•] or [⊕]
        case OP_MEASURE:
            return 3;  //[M]
        default:
            return 1;
    }
}

/* Function to draw a gate symbol */
static void draw_gate_symbol(OpCode code, int is_control, int is_target) {
    if (is_control) {
        printf("[•]");
    } else if (is_target) {
        printf("[⊕]");
    } else {
        switch (code) {
            case OP_H:
                printf("[H]");
                break;
            case OP_X:
                printf("[X]");
                break;
            case OP_Z:
                printf("[Z]");
                break;
            case OP_MEASURE:
                printf("[M]");
                break;
            default:
                printf("───");
        }
    }
}

/* function to draw wire segments between gates */
static void draw_wire(int length) {
    for (int i = 0; i < length; i++) {
        printf("─");
    }
}

void print_circuit(Chunk* chunk, int num_qubits) {
    if (chunk == NULL || chunk->count == 0) {
        printf("Empty circuit\n");
        return;
    }

    printf("\nQuantum Circuit:\n");

    // Calculate the total width needed for the circuit diagram 
    int total_width = 0;
    for (int i = 0; i < chunk->count; i++) {
        total_width += get_gate_width(chunk->code[i].code) + 1;  
    }

    //draw the circuit diagram for each qubit 
    for (int qubit = 0; qubit < num_qubits; qubit++) {
        printf("q%d: ", qubit);

        // Initialize the circuit with wires 
        for (int i = 0; i < chunk->count; i++) {
            Instruction instr = chunk->code[i];
            
            // determine if the qubit is involved in the current gate 
            int is_control = (instr.code == OP_CNOT && instr.operands[0] == qubit);
            int is_target = (instr.code == OP_CNOT && instr.operands[1] == qubit);
            int is_single = (instr.code != OP_CNOT && instr.operands[0] == qubit);

            // Draw the appropriate gate symbol or wire segment 
            if (is_control || is_target || is_single) {
                draw_gate_symbol(instr.code, is_control, is_target);
            } else {
                draw_wire(get_gate_width(instr.code));
            }

            //add connecting wire if this isn't the last gate 
            if (i < chunk->count - 1) {
                printf("─");
            }
        }
        printf("\n");
    }

    if (chunk->count > 1) {
        printf("\nLegend:\n");
        printf("[H] : Hadamard gate\n");
        printf("[X] : Pauli-X gate\n");
        printf("[Z] : Pauli-Z gate\n");
        printf("[•] : CNOT control\n");
        printf("[⊕] : CNOT target\n");
        printf("[M] : Measurement\n");
    }
    printf("\n");
}
