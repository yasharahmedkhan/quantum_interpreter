/* enhanced_visualization.c */
#include "enhanced_visualization.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

/* Calculate probabilities from state vector */
static void calculate_probabilities(StateVector* state, double* probabilities) {
    for (size_t i = 0; i < state->size; i++) {
        double _Complex amp = state->amplitudes[i];
        probabilities[i] = creal(amp * conj(amp));
    }
}

/* Enhanced circuit_diagram.c visualization function */
void save_circuit_diagram_svg(Chunk* chunk, int num_qubits, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing\n", filename);
        return;
    }
    
    //calculate dimensions
    int width = chunk->count * 100 + 150;
    int height = num_qubits * 60 + 100;
    

    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(file, "<svg width=\"%d\" height=\"%d\" xmlns=\"http://www.w3.org/2000/svg\">\n", width, height);
    
    fprintf(file, "<defs>\n");
    fprintf(file, "  <linearGradient id=\"wireGradient\" x1=\"0%%\" y1=\"0%%\" x2=\"100%%\" y2=\"0%%\">\n");
    fprintf(file, "    <stop offset=\"0%%\" style=\"stop-color:#3498db;stop-opacity:0.2\" />\n");
    fprintf(file, "    <stop offset=\"100%%\" style=\"stop-color:#3498db;stop-opacity:0.8\" />\n");
    fprintf(file, "  </linearGradient>\n");
    

    fprintf(file, "  <filter id=\"dropShadow\" x=\"-20%%\" y=\"-20%%\" width=\"140%%\" height=\"140%%\">\n");
    fprintf(file, "    <feGaussianBlur in=\"SourceAlpha\" stdDeviation=\"2\" />\n");
    fprintf(file, "    <feOffset dx=\"1\" dy=\"1\" result=\"offsetblur\" />\n");
    fprintf(file, "    <feComponentTransfer>\n");
    fprintf(file, "      <feFuncA type=\"linear\" slope=\"0.2\" />\n");
    fprintf(file, "    </feComponentTransfer>\n");
    fprintf(file, "    <feMerge>\n");
    fprintf(file, "      <feMergeNode />\n");
    fprintf(file, "      <feMergeNode in=\"SourceGraphic\" />\n");
    fprintf(file, "    </feMerge>\n");
    fprintf(file, "  </filter>\n");
    fprintf(file, "</defs>\n");

    fprintf(file, "<style>\n");
    fprintf(file, "  .wire { stroke: url(#wireGradient); stroke-width: 2; }\n");
    fprintf(file, "  .gate-h { fill: #3498db; stroke: #2980b9; stroke-width: 2; }\n");
    fprintf(file, "  .gate-x { fill: #e74c3c; stroke: #c0392b; stroke-width: 2; }\n");
    fprintf(file, "  .gate-z { fill: #2ecc71; stroke: #27ae60; stroke-width: 2; }\n");
    fprintf(file, "  .gate-s { fill: #9b59b6; stroke: #8e44ad; stroke-width: 2; }\n");
    fprintf(file, "  .control { fill: #2c3e50; stroke: #2c3e50; stroke-width: 0; }\n");
    fprintf(file, "  .target { fill: #fff; stroke: #2c3e50; stroke-width: 2; }\n");
    fprintf(file, "  .measure { fill: #34495e; stroke: #2c3e50; stroke-width: 2; }\n");
    fprintf(file, "  .label { font-family: 'Arial', sans-serif; font-size: 16px; text-anchor: middle; }\n");
    fprintf(file, "  .gate-label { font-family: 'Arial', sans-serif; font-size: 16px; text-anchor: middle; fill: white; font-weight: bold; }\n");
    fprintf(file, "  .qubit-label { font-family: 'Arial', sans-serif; font-size: 16px; text-anchor: end; }\n");
    fprintf(file, "  .title { font-family: 'Arial', sans-serif; font-size: 20px; font-weight: bold; text-anchor: middle; }\n");
    fprintf(file, "</style>\n");
    
    fprintf(file, "<text x=\"%d\" y=\"30\" class=\"title\">Quantum Circuit Diagram</text>\n", width / 2);
    
    for (int q = 0; q < num_qubits; q++) {
        int y_pos = q * 60 + 80;
        
        fprintf(file, "<text x=\"40\" y=\"%d\" class=\"qubit-label\">q<tspan dy=\"-5\" font-size=\"12\">%d</tspan></text>\n", 
                y_pos + 5, q);

        fprintf(file, "<line x1=\"50\" y1=\"%d\" x2=\"%d\" y2=\"%d\" class=\"wire\" />\n", 
                y_pos, width - 50, y_pos);
    }
    
    for (int i = 0; i < chunk->count; i++) {
        Instruction instr = chunk->code[i];
        int x_pos = i * 100 + 100;
        
        switch (instr.code) {
            case OP_H: {
                int y_pos = instr.operands[0] * 60 + 80;
                fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"40\" height=\"40\" rx=\"5\" ry=\"5\" class=\"gate-h\" filter=\"url(#dropShadow)\" />\n", 
                        x_pos - 20, y_pos - 20);
                fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"gate-label\">H</text>\n", 
                        x_pos, y_pos + 6);
                break;
            }
            case OP_X: {
                int y_pos = instr.operands[0] * 60 + 80;
                fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"40\" height=\"40\" rx=\"5\" ry=\"5\" class=\"gate-x\" filter=\"url(#dropShadow)\" />\n", 
                        x_pos - 20, y_pos - 20);
                fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"gate-label\">X</text>\n", 
                        x_pos, y_pos + 6);
                break;
            }
            case OP_Z: {
                int y_pos = instr.operands[0] * 60 + 80;
                fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"40\" height=\"40\" rx=\"5\" ry=\"5\" class=\"gate-z\" filter=\"url(#dropShadow)\" />\n", 
                        x_pos - 20, y_pos - 20);
                fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"gate-label\">Z</text>\n", 
                        x_pos, y_pos + 6);
                break;
            }
            case OP_S: {
                int y_pos = instr.operands[0] * 60 + 80;
                fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"40\" height=\"40\" rx=\"5\" ry=\"5\" class=\"gate-s\" filter=\"url(#dropShadow)\" />\n", 
                        x_pos - 20, y_pos - 20);
                fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"gate-label\">S</text>\n", 
                        x_pos, y_pos + 6);
                break;
            }
            case OP_CNOT: {
                int control_y = instr.operands[0] * 60 + 80;
                int target_y = instr.operands[1] * 60 + 80;
                
                fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#2c3e50\" stroke-width=\"2\" />\n", 
                        x_pos, control_y, x_pos, target_y);
                
                fprintf(file, "<circle cx=\"%d\" cy=\"%d\" r=\"8\" class=\"control\" />\n", 
                        x_pos, control_y);
                
                fprintf(file, "<circle cx=\"%d\" cy=\"%d\" r=\"16\" class=\"target\" filter=\"url(#dropShadow)\" />\n", 
                        x_pos, target_y);
                fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#2c3e50\" stroke-width=\"2\" />\n", 
                        x_pos - 10, target_y, x_pos + 10, target_y);
                fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#2c3e50\" stroke-width=\"2\" />\n", 
                        x_pos, target_y - 10, x_pos, target_y + 10);
                break;
            }
            case OP_MEASURE: {
                int y_pos = instr.operands[0] * 60 + 80;
                
                fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"40\" height=\"40\" rx=\"5\" ry=\"5\" class=\"measure\" filter=\"url(#dropShadow)\" />\n", 
                        x_pos - 20, y_pos - 20);
                
              
                fprintf(file, "<path d=\"M %d %d L %d %d L %d %d\" stroke=\"white\" stroke-width=\"2\" fill=\"none\" />\n",
                        x_pos - 10, y_pos + 5, x_pos, y_pos - 5, x_pos + 10, y_pos + 5);
                fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"white\" stroke-width=\"2\" />\n",
                        x_pos, y_pos - 5, x_pos, y_pos + 10);
                break;
            }
        }
    }
    
  
    int legend_y = num_qubits * 60 + 130;
    fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"title\">Legend</text>\n", width / 2, legend_y - 20);
    
  
    const char* gates[] = {"H", "X", "Z", "S", "•", "⊕", "M"};
    const char* descriptions[] = {"Hadamard", "Pauli-X", "Pauli-Z", "Phase", "Control", "Target", "Measure"};
    const char* classes[] = {"gate-h", "gate-x", "gate-z", "gate-s", "control", "target", "measure"};
    
    for (int i = 0; i < 7; i++) {
        int legend_x = 80 + i * 100;
        
        if (i < 4) { 
            fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"30\" height=\"30\" rx=\"5\" ry=\"5\" class=\"%s\" />\n",
                   legend_x - 15, legend_y - 15, classes[i]);
            fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"gate-label\">%s</text>\n",
                   legend_x, legend_y + 2, gates[i]);
        } else if (i == 4) { 
            fprintf(file, "<circle cx=\"%d\" cy=\"%d\" r=\"8\" class=\"%s\" />\n",
                   legend_x, legend_y, classes[i]);
        } else if (i == 5) {
            fprintf(file, "<circle cx=\"%d\" cy=\"%d\" r=\"12\" class=\"%s\" />\n",
                   legend_x, legend_y, classes[i]);
            fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#2c3e50\" stroke-width=\"2\" />\n",
                   legend_x - 8, legend_y, legend_x + 8, legend_y);
            fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#2c3e50\" stroke-width=\"2\" />\n",
                   legend_x, legend_y - 8, legend_x, legend_y + 8);
        } else { 
            fprintf(file, "<rect x=\"%d\" y=\"%d\" width=\"30\" height=\"30\" rx=\"5\" ry=\"5\" class=\"%s\" />\n",
                   legend_x - 15, legend_y - 15, classes[i]);
            fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"gate-label\">%s</text>\n",
                   legend_x, legend_y + 2, gates[i]);
        }
        
        fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"label\">%s</text>\n",
               legend_x, legend_y + 25, descriptions[i]);
    }
    
    fprintf(file, "</svg>\n");
    fclose(file);
    printf("Enhanced circuit diagram saved to %s\n", filename);
}

/* Enhanced state visualization function */
void save_state_visualization(StateVector* state, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing\n", filename);
        return;
    }
    
    //calculate probabilities
    double* probabilities = malloc(sizeof(double) * state->size);
    for (size_t i = 0; i < state->size; i++) {
        double _Complex amp = state->amplitudes[i];
        probabilities[i] = creal(amp * conj(amp));
    }
    
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(file, "<svg width=\"900\" height=\"600\" xmlns=\"http://www.w3.org/2000/svg\">\n");
    
    fprintf(file, "<defs>\n");
    
    fprintf(file, "  <linearGradient id=\"barGradient\" x1=\"0%%\" y1=\"0%%\" x2=\"0%%\" y2=\"100%%\">\n");
    fprintf(file, "    <stop offset=\"0%%\" style=\"stop-color:#3498db;stop-opacity:1\" />\n");
    fprintf(file, "    <stop offset=\"100%%\" style=\"stop-color:#2980b9;stop-opacity:1\" />\n");
    fprintf(file, "  </linearGradient>\n");
    
    fprintf(file, "  <filter id=\"dropShadow\" x=\"-20%%\" y=\"-20%%\" width=\"140%%\" height=\"140%%\">\n");
    fprintf(file, "    <feGaussianBlur in=\"SourceAlpha\" stdDeviation=\"3\" />\n");
    fprintf(file, "    <feOffset dx=\"2\" dy=\"2\" result=\"offsetblur\" />\n");
    fprintf(file, "    <feComponentTransfer>\n");
    fprintf(file, "      <feFuncA type=\"linear\" slope=\"0.3\" />\n");
    fprintf(file, "    </feComponentTransfer>\n");
    fprintf(file, "    <feMerge>\n");
    fprintf(file, "      <feMergeNode />\n");
    fprintf(file, "      <feMergeNode in=\"SourceGraphic\" />\n");
    fprintf(file, "    </feMerge>\n");
    fprintf(file, "  </filter>\n");
    
    fprintf(file, "</defs>\n");
    
    fprintf(file, "<style>\n");
    fprintf(file, "  .bar { fill: url(#barGradient); filter: url(#dropShadow); }\n");
    fprintf(file, "  .bar:hover { fill: #2c3e50; }\n");
    fprintf(file, "  .axis { stroke: #7f8c8d; stroke-width: 2; }\n");
    fprintf(file, "  .grid { stroke: #ecf0f1; stroke-width: 1; stroke-dasharray: 2,2; }\n");
    fprintf(file, "  .label { font-family: 'Arial', sans-serif; font-size: 14px; text-anchor: middle; }\n");
    fprintf(file, "  .title { font-family: 'Arial', sans-serif; font-size: 24px; font-weight: bold; text-anchor: middle; }\n");
    fprintf(file, "  .subtitle { font-family: 'Arial', sans-serif; font-size: 16px; text-anchor: middle; fill: #7f8c8d; }\n");
    fprintf(file, "  .prob-label { font-family: 'Arial', sans-serif; font-size: 12px; text-anchor: middle; fill: white; font-weight: bold; }\n");
    fprintf(file, "  .axis-label { font-family: 'Arial', sans-serif; font-size: 16px; text-anchor: middle; }\n");
    fprintf(file, "</style>\n");
    
    //background
    fprintf(file, "<rect x=\"0\" y=\"0\" width=\"900\" height=\"600\" fill=\"#f9f9f9\" />\n");
    
    // title and subtitle
    fprintf(file, "<text x=\"450\" y=\"40\" class=\"title\">Quantum State Probabilities</text>\n");
    int num_qubits = log2(state->size);
    fprintf(file, "<text x=\"450\" y=\"70\" class=\"subtitle\">%d-Qubit System (%zu basis states)</text>\n", 
            num_qubits, state->size);
    
    // Define chart area
    int chart_x = 100;
    int chart_y = 100;
    int chart_width = 700;
    int chart_height = 400;
    
    // Draw grid lines
    for (int i = 0; i <= 10; i++) {
        int y = chart_y + chart_height - (i * chart_height / 10);
        fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" class=\"grid\" />\n",
                chart_x, y, chart_x + chart_width, y);
        fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"label\">%.1f</text>\n",
                chart_x - 10, y + 5, i * 0.1);
    }
    
    // Axes
    fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" class=\"axis\" />\n", 
            chart_x, chart_y + chart_height, chart_x + chart_width, chart_y + chart_height); // x-axis
    fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" class=\"axis\" />\n", 
            chart_x, chart_y, chart_x, chart_y + chart_height); // y-axis
    
    // axis labels
    fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"axis-label\">Basis States</text>\n",
            chart_x + chart_width/2, chart_y + chart_height + 50);
    fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"axis-label\" transform=\"rotate(-90,%d,%d)\">Probability</text>\n",
            chart_x - 60, chart_y + chart_height/2, chart_x - 60, chart_y + chart_height/2);
    
    // draw bars
    double bar_width = (double)chart_width / state->size;
    for (size_t i = 0; i < state->size; i++) {
        double bar_height = probabilities[i] * chart_height;
        double x = chart_x + i * bar_width;
        double y = chart_y + chart_height - bar_height;
        
        if (bar_height > 1.0) {
            fprintf(file, "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" class=\"bar\">\n", 
                    x, y, bar_width * 0.8, bar_height);
            fprintf(file, "  <title>|%zu⟩: %.4f</title>\n", i, probabilities[i]);
            fprintf(file, "</rect>\n");
            
            if (state->size <= 32) { 
                // generate binary representation
                char binary[33] = {0};
                for (int j = num_qubits - 1; j >= 0; j--) {
                    binary[num_qubits - 1 - j] = ((i >> j) & 1) ? '1' : '0';
                }
                binary[num_qubits] = '\0';
                
                fprintf(file, "<text x=\"%.2f\" y=\"%d\" class=\"label\" transform=\"rotate(-90,%.2f,%d)\">|%s⟩</text>\n", 
                        x + bar_width * 0.4, chart_y + chart_height + 20, 
                        x + bar_width * 0.4, chart_y + chart_height + 20, binary);
            
                if (bar_height > 30) {
                    fprintf(file, "<text x=\"%.2f\" y=\"%.2f\" class=\"prob-label\">%.3f</text>\n", 
                            x + bar_width * 0.4, y + bar_height / 2 + 5, probabilities[i]);
                }
            }
        }
    }
    
    // Add probability threshold line
    fprintf(file, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"#e74c3c\" stroke-width=\"2\" stroke-dasharray=\"5,5\" />\n", 
            chart_x, chart_y + chart_height - (chart_height / 2), chart_x + chart_width, chart_y + chart_height - (chart_height / 2));
    fprintf(file, "<text x=\"%d\" y=\"%d\" fill=\"#e74c3c\" class=\"label\">0.5</text>\n", 
            chart_x + chart_width + 20, chart_y + chart_height - (chart_height / 2) + 5);
    
    // display phase information for significant amplitudes
    fprintf(file, "<text x=\"450\" y=\"530\" class=\"title\">Phase Information</text>\n");
    
    int phase_y = 550;
    int count = 0;
    for (size_t i = 0; i < state->size; i++) {
        if (probabilities[i] > 0.01) { 
            double _Complex amp = state->amplitudes[i];
            double magnitude = sqrt(probabilities[i]);
            double phase = atan2(cimag(amp), creal(amp)) * 180.0 / M_PI;
            
            // generate binary representation
            char binary[33] = {0};
            for (int j = num_qubits - 1; j >= 0; j--) {
                binary[num_qubits - 1 - j] = ((i >> j) & 1) ? '1' : '0';
            }
            binary[num_qubits] = '\0';
            
            int phase_x = 200 + (count % 3) * 250;
            if (count % 3 == 0 && count > 0) phase_y += 25;
            
            fprintf(file, "<text x=\"%d\" y=\"%d\" class=\"label\">|%s⟩: %.3f∠%.1f°</text>\n", 
                    phase_x, phase_y, binary, magnitude, phase);
            
            count++;
        }
    }
    
    fprintf(file, "</svg>\n");
    fclose(file);
    free(probabilities);
    printf("Enhanced state visualization saved to %s\n", filename);
}
/* Save algorithm comparison visualization */
void save_algorithm_comparison(PerformanceMetrics* state_vector_metrics,
                              PerformanceMetrics* tensor_network_metrics,
                              PerformanceMetrics* stabilizer_metrics,
                              const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing\n", filename);
        return;
    }
    
    // Find maximum values for scaling
    double max_time = state_vector_metrics->time_elapsed;
    if (tensor_network_metrics->time_elapsed > max_time) max_time = tensor_network_metrics->time_elapsed;
    if (stabilizer_metrics->time_elapsed > max_time) max_time = stabilizer_metrics->time_elapsed;
    
    size_t max_memory = state_vector_metrics->memory_used;
    if (tensor_network_metrics->memory_used > max_memory) max_memory = tensor_network_metrics->memory_used;
    if (stabilizer_metrics->memory_used > max_memory) max_memory = stabilizer_metrics->memory_used;
    
    // SVG header
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    fprintf(file, "<svg width=\"800\" height=\"600\" xmlns=\"http://www.w3.org/2000/svg\">\n");
    
    // Style definitions
    fprintf(file, "<style>\n");
    fprintf(file, "  .bar1 { fill: #4285f4; }\n");
    fprintf(file, "  .bar2 { fill: #ea4335; }\n");
    fprintf(file, "  .bar3 { fill: #fbbc05; }\n");
    fprintf(file, "  .axis { stroke: #000; stroke-width: 1; }\n");
    fprintf(file, "  .label { font-family: Arial; font-size: 14px; text-anchor: middle; }\n");
    fprintf(file, "  .title { font-family: Arial; font-size: 18px; text-anchor: middle; font-weight: bold; }\n");
    fprintf(file, "  .legend { font-family: Arial; font-size: 14px; }\n");
    fprintf(file, "</style>\n");
    
    // title
    fprintf(file, "<text x=\"400\" y=\"30\" class=\"title\">Algorithm Performance Comparison</text>\n");
    
    // time comparison chart
    fprintf(file, "<text x=\"400\" y=\"70\" class=\"title\">Execution Time (seconds)</text>\n");
    
    // axes for time chart
    fprintf(file, "<line x1=\"100\" y1=\"250\" x2=\"700\" y2=\"250\" class=\"axis\" />\n");
    fprintf(file, "<line x1=\"100\" y1=\"100\" x2=\"100\" y2=\"250\" class=\"axis\" />\n");
    
    double time_scale = 150.0 / max_time;
    
    // State Vector
    double sv_time_height = state_vector_metrics->time_elapsed * time_scale;
    fprintf(file, "<rect x=\"150\" y=\"%f\" width=\"100\" height=\"%f\" class=\"bar1\">\n", 
            250 - sv_time_height, sv_time_height);
    fprintf(file, "  <title>State Vector: %.6f sec</title>\n", state_vector_metrics->time_elapsed);
    fprintf(file, "</rect>\n");
    fprintf(file, "<text x=\"200\" y=\"270\" class=\"label\">State Vector</text>\n");
    
    // Tensor Network
    double tn_time_height = tensor_network_metrics->time_elapsed * time_scale;
    fprintf(file, "<rect x=\"350\" y=\"%f\" width=\"100\" height=\"%f\" class=\"bar2\">\n", 
            250 - tn_time_height, tn_time_height);
    fprintf(file, "  <title>Tensor Network: %.6f sec</title>\n", tensor_network_metrics->time_elapsed);
    fprintf(file, "</rect>\n");
    fprintf(file, "<text x=\"400\" y=\"270\" class=\"label\">Tensor Network</text>\n");
    
    // Stabilizer
    double st_time_height = stabilizer_metrics->time_elapsed * time_scale;
    fprintf(file, "<rect x=\"550\" y=\"%f\" width=\"100\" height=\"%f\" class=\"bar3\">\n", 
            250 - st_time_height, st_time_height);
    fprintf(file, "  <title>Stabilizer: %.6f sec</title>\n", stabilizer_metrics->time_elapsed);
    fprintf(file, "</rect>\n");
    fprintf(file, "<text x=\"600\" y=\"270\" class=\"label\">Stabilizer</text>\n");
    
    fprintf(file, "<text x=\"400\" y=\"320\" class=\"title\">Memory Usage (bytes)</text>\n");
    

    fprintf(file, "<line x1=\"100\" y1=\"500\" x2=\"700\" y2=\"500\" class=\"axis\" />\n");
    fprintf(file, "<line x1=\"100\" y1=\"350\" x2=\"100\" y2=\"500\" class=\"axis\" />\n");
    
    double memory_scale = 150.0 / max_memory;
    
    // State Vector
    double sv_memory_height = state_vector_metrics->memory_used * memory_scale;
    fprintf(file, "<rect x=\"150\" y=\"%f\" width=\"100\" height=\"%f\" class=\"bar1\">\n", 
            500 - sv_memory_height, sv_memory_height);
    fprintf(file, "  <title>State Vector: %zu bytes</title>\n", state_vector_metrics->memory_used);
    fprintf(file, "</rect>\n");
    fprintf(file, "<text x=\"200\" y=\"520\" class=\"label\">State Vector</text>\n");
    
    // Tensor Network
    double tn_memory_height = tensor_network_metrics->memory_used * memory_scale;
    fprintf(file, "<rect x=\"350\" y=\"%f\" width=\"100\" height=\"%f\" class=\"bar2\">\n", 
            500 - tn_memory_height, tn_memory_height);
    fprintf(file, "  <title>Tensor Network: %zu bytes</title>\n", tensor_network_metrics->memory_used);
    fprintf(file, "</rect>\n");
    fprintf(file, "<text x=\"400\" y=\"520\" class=\"label\">Tensor Network</text>\n");
    
    // Stabilizer
    double st_memory_height = stabilizer_metrics->memory_used * memory_scale;
    fprintf(file, "<rect x=\"550\" y=\"%f\" width=\"100\" height=\"%f\" class=\"bar3\">\n", 
            500 - st_memory_height, st_memory_height);
    fprintf(file, "  <title>Stabilizer: %zu bytes</title>\n", stabilizer_metrics->memory_used);
    fprintf(file, "</rect>\n");
    fprintf(file, "<text x=\"600\" y=\"520\" class=\"label\">Stabilizer</text>\n");
    
    fprintf(file, "</svg>\n");
    fclose(file);
    printf("Algorithm comparison visualization saved to %s\n", filename);
}

/* Print fancy circuit diagram with enhanced UTF-8 characters */
void print_fancy_circuit(Chunk* chunk, int num_qubits) {
    printf("\n╔═══════════════ Quantum Circuit ═══════════════╗\n");
    
    // calculate the maximum width needed for the circuit
    int max_width = chunk->count * 4 + 5;
    
    // draw header row with gate numbers
    printf("   ");
    for (int i = 0; i < chunk->count; i++) {
        printf("  %d ", i);
    }
    printf("\n");
    
    for (int q = 0; q < num_qubits; q++) {
        printf("q%d: ", q);
        
        // Print the gates for this qubit
        for (int i = 0; i < chunk->count; i++) {
            Instruction instr = chunk->code[i];
            
            //check if this qubit is involved in the current instruction
            bool involved = false;
            bool is_control = false;
            bool is_target = false;
            
            if (instr.code == OP_CNOT) {
                if (instr.operands[0] == q) {
                    involved = true;
                    is_control = true;
                } else if (instr.operands[1] == q) {
                    involved = true;
                    is_target = true;
                }
            } else if (instr.operands[0] == q) {
                involved = true;
            }

            if (involved) {
                if (is_control) {
                    printf("┃•┃ ");
                } else if (is_target) {
                    printf("┃⊕┃ ");
                } else {
                    switch (instr.code) {
                        case OP_H:
                            printf("┃H┃ ");
                            break;
                        case OP_X:
                            printf("┃X┃ ");
                            break;
                        case OP_Z:
                            printf("┃Z┃ ");
                            break;
                        case OP_S:
                            printf("┃S┃ ");
                            break;
                        case OP_MEASURE:
                            printf("┃M┃ ");
                            break;
                        default:
                            printf("┃?┃ ");
                    }
                }
            } else {
                bool needs_vertical_line = false;
                
                if (i > 0 && chunk->code[i-1].code == OP_CNOT) {
                    int control = chunk->code[i-1].operands[0];
                    int target = chunk->code[i-1].operands[1];
                    if ((q > control && q < target) || (q < control && q > target)) {
                        needs_vertical_line = true;
                    }
                }
                
                if (needs_vertical_line) {
                    printf(" ┃  ");
                } else {
                    printf("━━━ ");
                }
            }
        }
        printf("\n");
    }
    
    printf("╚");
    for (int i = 0; i < max_width - 2; i++) {
        printf("═");
    }
    printf("╝\n");
    
    // Print legend
    printf("\nGate Legend:\n");
    printf("┃H┃ : Hadamard    ┃X┃ : Pauli-X    ┃Z┃ : Pauli-Z\n");
    printf("┃S┃ : Phase       ┃•┃ : Control    ┃⊕┃ : Target\n");
    printf("┃M┃ : Measurement\n\n");
}

/* Enhanced Bloch sphere HTML visualization */
void save_bloch_sphere_html(StateVector* state, int qubit, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing\n", filename);
        return;
    }
    
    //extract the single-qubit state for the selected qubit
    double prob_0 = 0.0, prob_1 = 0.0;
    double _Complex amp_0 = 0.0, amp_1 = 0.0;
    size_t mask = 1ULL << qubit;
    
    for (size_t i = 0; i < state->size; i++) {
        if ((i & mask) == 0) {
            prob_0 += creal(state->amplitudes[i] * conj(state->amplitudes[i]));
            amp_0 += state->amplitudes[i];
        } else {
            prob_1 += creal(state->amplitudes[i] * conj(state->amplitudes[i]));
            amp_1 += state->amplitudes[i];
        }
    }
    
    //normalize amplitudes
    double norm = sqrt(prob_0 + prob_1);
    if (norm > 1e-10) {
        amp_0 /= norm;
        amp_1 /= norm;
    }
    
    // Calculate Bloch sphere coordinates
    double theta = 2.0 * acos(cabs(amp_0));
    double phi = 0.0;
    if (cabs(amp_1) > 1e-10) {
        phi = carg(amp_1 / amp_0);
    }
    
    double x = sin(theta) * cos(phi);
    double y = sin(theta) * sin(phi);
    double z = cos(theta);
    
    //create HTML file with Three.js for 3D visualization
    fprintf(file, "<!DOCTYPE html>\n");
    fprintf(file, "<html lang=\"en\">\n");
    fprintf(file, "<head>\n");
    fprintf(file, "    <meta charset=\"UTF-8\">\n");
    fprintf(file, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(file, "    <title>Enhanced Bloch Sphere - Qubit %d</title>\n", qubit);
    fprintf(file, "    <script src=\"https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js\"></script>\n");
    fprintf(file, "    <script src=\"https://cdnjs.cloudflare.com/ajax/libs/dat-gui/0.7.7/dat.gui.min.js\"></script>\n");
    fprintf(file, "    <style>\n");
    fprintf(file, "        body {\n");
    fprintf(file, "            margin: 0;\n");
    fprintf(file, "            overflow: hidden;\n");
    fprintf(file, "            font-family: 'Arial', sans-serif;\n");
    fprintf(file, "            background: linear-gradient(135deg, #f5f7fa 0%%, #c3cfe2 100%%);\n");
    fprintf(file, "        }\n");
    fprintf(file, "        #info {\n");
    fprintf(file, "            position: absolute;\n");
    fprintf(file, "            top: 0;\n");
    fprintf(file, "            width: 100%%;\n");
    fprintf(file, "            box-sizing: border-box;\n");
    fprintf(file, "            text-align: center;\n");
    fprintf(file, "            padding: 10px;\n");
    fprintf(file, "            color: #2c3e50;\n");
    fprintf(file, "            font-size: 20px;\n");
    fprintf(file, "            font-weight: bold;\n");
    fprintf(file, "            z-index: 100;\n");
    fprintf(file, "            text-shadow: 1px 1px 2px rgba(255, 255, 255, 0.8);\n");
    fprintf(file, "        }\n");
    fprintf(file, "        #stats {\n");
    fprintf(file, "            position: absolute;\n");
    fprintf(file, "            top: 60px;\n");
    fprintf(file, "            right: 20px;\n");
    fprintf(file, "            width: 300px;\n");
    fprintf(file, "            padding: 15px;\n");
    fprintf(file, "            border-radius: 10px;\n");
    fprintf(file, "            color: #fff;\n");
    fprintf(file, "            font-size: 14px;\n");
    fprintf(file, "            z-index: 100;\n");
    fprintf(file, "            background: rgba(44, 62, 80, 0.8);\n");
    fprintf(file, "            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);\n");
    fprintf(file, "        }\n");
    fprintf(file, "        #stats h3 {\n");
    fprintf(file, "            margin-top: 0;\n");
    fprintf(file, "            border-bottom: 1px solid rgba(255, 255, 255, 0.3);\n");
    fprintf(file, "            padding-bottom: 10px;\n");
    fprintf(file, "        }\n");
    fprintf(file, "        #stats table {\n");
    fprintf(file, "            width: 100%%;\n");
    fprintf(file, "            border-collapse: collapse;\n");
    fprintf(file, "        }\n");
    fprintf(file, "        #stats td {\n");
    fprintf(file, "            padding: 5px 0;\n");
    fprintf(file, "        }\n");
    fprintf(file, "        #stats td:first-child {\n");
    fprintf(file, "            font-weight: bold;\n");
    fprintf(file, "            color: #e74c3c;\n");
    fprintf(file, "        }\n");
    fprintf(file, "        .dg.ac {\n");
    fprintf(file, "            z-index: 101 !important;\n");
    fprintf(file, "        }\n");
    fprintf(file, "        .tooltip {\n");
    fprintf(file, "            position: absolute;\n");
    fprintf(file, "            background: rgba(0, 0, 0, 0.7);\n");
    fprintf(file, "            color: white;\n");
    fprintf(file, "            padding: 5px 10px;\n");
    fprintf(file, "            border-radius: 5px;\n");
    fprintf(file, "            font-size: 12px;\n");
    fprintf(file, "            pointer-events: none;\n");
    fprintf(file, "            opacity: 0;\n");
    fprintf(file, "            transition: opacity 0.3s;\n");
    fprintf(file, "        }\n");
    fprintf(file, "    </style>\n");
    fprintf(file, "</head>\n");
    fprintf(file, "<body>\n");
    fprintf(file, "    <div id=\"info\">Bloch Sphere Visualization for Qubit %d</div>\n", qubit);
    fprintf(file, "    <div id=\"stats\">\n");
    fprintf(file, "        <h3>Qubit Properties</h3>\n");
    fprintf(file, "        <table>\n");
    fprintf(file, "            <tr><td>Probability |0⟩:</td><td>%.4f</td></tr>\n", prob_0);
    fprintf(file, "            <tr><td>Probability |1⟩:</td><td>%.4f</td></tr>\n", prob_1);
    fprintf(file, "            <tr><td>θ (theta):</td><td>%.4f rad (%.2f°)</td></tr>\n", theta, theta * 180.0 / M_PI);
    fprintf(file, "            <tr><td>φ (phi):</td><td>%.4f rad (%.2f°)</td></tr>\n", phi, phi * 180.0 / M_PI);
    fprintf(file, "            <tr><td>Bloch vector:</td><td>(%.4f, %.4f, %.4f)</td></tr>\n", x, y, z);
    fprintf(file, "        </table>\n");
    fprintf(file, "        <h3>State Representation</h3>\n");
    fprintf(file, "        <div style=\"font-family:monospace; margin-top:10px;\">\n");
    fprintf(file, "            |ψ⟩ = (%.4f + %.4fi)|0⟩ + (%.4f + %.4fi)|1⟩\n", 
            creal(amp_0), cimag(amp_0), creal(amp_1), cimag(amp_1));
    fprintf(file, "        </div>\n");
    fprintf(file, "    </div>\n");
    fprintf(file, "    <div id=\"tooltip\" class=\"tooltip\"></div>\n");
    fprintf(file, "    <script>\n");
    fprintf(file, "        // Set up Three.js scene\n");
    fprintf(file, "        const scene = new THREE.Scene();\n");
    fprintf(file, "        const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);\n");
    fprintf(file, "        const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });\n");
    fprintf(file, "        renderer.setSize(window.innerWidth, window.innerHeight);\n");
    fprintf(file, "        renderer.setClearColor(0x000000, 0); // Transparent background\n");
    fprintf(file, "        document.body.appendChild(renderer.domElement);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Camera controls\n");
    fprintf(file, "        camera.position.z = 3;\n");
    fprintf(file, "        const controls = {\n");
    fprintf(file, "            rotateSpeed: 0.01,\n");
    fprintf(file, "            autoRotate: true,\n");
    fprintf(file, "            showGrid: true,\n");
    fprintf(file, "            showLabels: true,\n");
    fprintf(file, "            sphereOpacity: 0.3,\n");
    fprintf(file, "            stateColor: '#ff9900',\n");
    fprintf(file, "            resetCamera: function() {\n");
    fprintf(file, "                camera.position.set(0, 0, 3);\n");
    fprintf(file, "                camera.lookAt(0, 0, 0);\n");
    fprintf(file, "            }\n");
    fprintf(file, "        };\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Set up lighting\n");
    fprintf(file, "        const ambientLight = new THREE.AmbientLight(0x404040, 1.5);\n");
    fprintf(file, "        scene.add(ambientLight);\n");
    fprintf(file, "        \n");
    fprintf(file, "        const light1 = new THREE.DirectionalLight(0xffffff, 1);\n");
    fprintf(file, "        light1.position.set(1, 1, 1);\n");
    fprintf(file, "        scene.add(light1);\n");
    fprintf(file, "        \n");
    fprintf(file, "        const light2 = new THREE.DirectionalLight(0xffffff, 0.5);\n");
    fprintf(file, "        light2.position.set(-1, -1, -1);\n");
    fprintf(file, "        scene.add(light2);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Create a group for all objects\n");
    fprintf(file, "        const sphereGroup = new THREE.Group();\n");
    fprintf(file, "        scene.add(sphereGroup);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Create Bloch sphere with custom material\n");
    fprintf(file, "        const sphereGeometry = new THREE.SphereGeometry(1, 64, 64);\n");
    fprintf(file, "        const sphereMaterial = new THREE.MeshPhongMaterial({\n");
    fprintf(file, "            color: 0x3498db,\n");
    fprintf(file, "            transparent: true,\n");
    fprintf(file, "            opacity: controls.sphereOpacity,\n");
    fprintf(file, "            specular: 0x444444,\n");
    fprintf(file, "            shininess: 30,\n");
    fprintf(file, "            side: THREE.DoubleSide\n");
    fprintf(file, "        });\n");
    fprintf(file, "        const sphere = new THREE.Mesh(sphereGeometry, sphereMaterial);\n");
    fprintf(file, "        sphereGroup.add(sphere);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add coordinate grid\n");
    fprintf(file, "        const gridHelper = new THREE.Group();\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Equator circle (XY plane)\n");
    fprintf(file, "        const equatorGeometry = new THREE.RingGeometry(0.99, 1.01, 64);\n");
    fprintf(file, "        const equatorMaterial = new THREE.MeshBasicMaterial({ \n");
    fprintf(file, "            color: 0xcccccc, \n");
    fprintf(file, "            transparent: true, \n");
    fprintf(file, "            opacity: 0.3, \n");
    fprintf(file, "            side: THREE.DoubleSide \n");
    fprintf(file, "        });\n");
    fprintf(file, "        const equator = new THREE.Mesh(equatorGeometry, equatorMaterial);\n");
    fprintf(file, "        equator.rotation.x = Math.PI / 2;\n");
    fprintf(file, "        gridHelper.add(equator);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Prime meridian (XZ plane)\n");
    fprintf(file, "        const xzGeometry = new THREE.RingGeometry(0.99, 1.01, 64);\n");
    fprintf(file, "        const xzMaterial = new THREE.MeshBasicMaterial({ \n");
    fprintf(file, "            color: 0xcccccc, \n");
    fprintf(file, "            transparent: true, \n");
    fprintf(file, "            opacity: 0.3, \n");
    fprintf(file, "            side: THREE.DoubleSide \n");
    fprintf(file, "        });\n");
    fprintf(file, "        const xzPlane = new THREE.Mesh(xzGeometry, xzMaterial);\n");
    fprintf(file, "        gridHelper.add(xzPlane);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // YZ plane\n");
    fprintf(file, "        const yzGeometry = new THREE.RingGeometry(0.99, 1.01, 64);\n");
    fprintf(file, "        const yzMaterial = new THREE.MeshBasicMaterial({ \n");
    fprintf(file, "            color: 0xcccccc, \n");
    fprintf(file, "            transparent: true, \n");
    fprintf(file, "            opacity: 0.3, \n");
    fprintf(file, "            side: THREE.DoubleSide \n");
    fprintf(file, "        });\n");
    fprintf(file, "        const yzPlane = new THREE.Mesh(yzGeometry, yzMaterial);\n");
    fprintf(file, "        yzPlane.rotation.y = Math.PI / 2;\n");
    fprintf(file, "        gridHelper.add(yzPlane);\n");
    fprintf(file, "        \n");
    fprintf(file, "        sphereGroup.add(gridHelper);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add axis lines (enhanced with arrow helpers)\n");
    fprintf(file, "        const axisLength = 1.2;\n");
    fprintf(file, "        \n");
    fprintf(file, "        // X-axis (red)\n");
    fprintf(file, "        const xAxis = new THREE.ArrowHelper(\n");
    fprintf(file, "            new THREE.Vector3(1, 0, 0),\n");
    fprintf(file, "            new THREE.Vector3(0, 0, 0),\n");
    fprintf(file, "            axisLength,\n");
    fprintf(file, "            0xff0000,\n");
    fprintf(file, "            0.1,\n");
    fprintf(file, "            0.05\n");
    fprintf(file, "        );\n");
    fprintf(file, "        sphereGroup.add(xAxis);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Y-axis (green)\n");
    fprintf(file, "        const yAxis = new THREE.ArrowHelper(\n");
    fprintf(file, "            new THREE.Vector3(0, 1, 0),\n");
    fprintf(file, "            new THREE.Vector3(0, 0, 0),\n");
    fprintf(file, "            axisLength,\n");
    fprintf(file, "            0x00ff00,\n");
    fprintf(file, "            0.1,\n");
    fprintf(file, "            0.05\n");
    fprintf(file, "        );\n");
    fprintf(file, "        sphereGroup.add(yAxis);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Z-axis (blue)\n");
    fprintf(file, "        const zAxis = new THREE.ArrowHelper(\n");
    fprintf(file, "            new THREE.Vector3(0, 0, 1),\n");
    fprintf(file, "            new THREE.Vector3(0, 0, 0),\n");
    fprintf(file, "            axisLength,\n");
    fprintf(file, "            0x0000ff,\n");
    fprintf(file, "            0.1,\n");
    fprintf(file, "            0.05\n");
    fprintf(file, "        );\n");
    fprintf(file, "        sphereGroup.add(zAxis);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add axis labels with improved visibility\n");
    fprintf(file, "        const createTextSprite = (text, position, color) => {\n");
    fprintf(file, "            const canvas = document.createElement('canvas');\n");
    fprintf(file, "            const context = canvas.getContext('2d');\n");
    fprintf(file, "            canvas.width = 128;\n");
    fprintf(file, "            canvas.height = 64;\n");
    fprintf(file, "            \n");
    fprintf(file, "            // Background with rounded corners\n");
    fprintf(file, "            context.fillStyle = 'rgba(0, 0, 0, 0.5)';\n");
    fprintf(file, "            context.beginPath();\n");
    fprintf(file, "            context.roundRect(0, 0, 128, 64, 10);\n");
    fprintf(file, "            context.fill();\n");
    fprintf(file, "            \n");
    fprintf(file, "            // Text\n");
    fprintf(file, "            context.font = 'bold 36px Arial';\n");
    fprintf(file, "            context.fillStyle = color;\n");
    fprintf(file, "            context.textAlign = 'center';\n");
    fprintf(file, "            context.textBaseline = 'middle';\n");
    fprintf(file, "            context.fillText(text, 64, 32);\n");
    fprintf(file, "            \n");
    fprintf(file, "            const texture = new THREE.CanvasTexture(canvas);\n");
    fprintf(file, "            const material = new THREE.SpriteMaterial({ map: texture });\n");
    fprintf(file, "            const sprite = new THREE.Sprite(material);\n");
    fprintf(file, "            sprite.position.copy(position);\n");
    fprintf(file, "            sprite.scale.set(0.3, 0.15, 1);\n");
    fprintf(file, "            return sprite;\n");
    fprintf(file, "        };\n");
    fprintf(file, "        \n");
    fprintf(file, "        const labels = new THREE.Group();\n");
    fprintf(file, "        labels.add(createTextSprite('X', new THREE.Vector3(1.35, 0, 0), '#ff0000'));\n");
    fprintf(file, "        labels.add(createTextSprite('Y', new THREE.Vector3(0, 1.35, 0), '#00ff00'));\n");
    fprintf(file, "        labels.add(createTextSprite('Z', new THREE.Vector3(0, 0, 1.35), '#0000ff'));\n");
    fprintf(file, "        labels.add(createTextSprite('|0⟩', new THREE.Vector3(0, 0, 1.35), '#ffffff'));\n");
    fprintf(file, "        labels.add(createTextSprite('|1⟩', new THREE.Vector3(0, 0, -1.35), '#ffffff'));\n");
    fprintf(file, "        sphereGroup.add(labels);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add the state vector\n");
    fprintf(file, "        const stateVector = new THREE.Vector3(%.4f, %.4f, %.4f);\n", x, y, z);
    fprintf(file, "        \n");
    fprintf(file, "        // Create a line from origin to the state vector\n");
    fprintf(file, "        const stateMaterial = new THREE.LineBasicMaterial({ \n");
    fprintf(file, "            color: controls.stateColor, \n");
    fprintf(file, "            linewidth: 3,\n");
    fprintf(file, "            linecap: 'round',\n");
    fprintf(file, "            linejoin: 'round'\n");
    fprintf(file, "        });\n");
    fprintf(file, "        const stateGeometry = new THREE.BufferGeometry();\n");
    fprintf(file, "        stateGeometry.setAttribute('position', new THREE.Float32BufferAttribute(\n");
    fprintf(file, "            [0, 0, 0, stateVector.x, stateVector.y, stateVector.z], 3\n");
    fprintf(file, "        ));\n");
    fprintf(file, "        const stateLine = new THREE.Line(stateGeometry, stateMaterial);\n");
    fprintf(file, "        sphereGroup.add(stateLine);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add a point at the end of the state vector with glow effect\n");
    fprintf(file, "        const statePointGeometry = new THREE.SphereGeometry(0.05, 16, 16);\n");
    fprintf(file, "        const statePointMaterial = new THREE.MeshBasicMaterial({ color: controls.stateColor });\n");
    fprintf(file, "        const statePoint = new THREE.Mesh(statePointGeometry, statePointMaterial);\n");
    fprintf(file, "        statePoint.position.copy(stateVector);\n");
    fprintf(file, "        sphereGroup.add(statePoint);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add a glowing effect to the state point\n");
    fprintf(file, "        const glowGeometry = new THREE.SphereGeometry(0.07, 16, 16);\n");
    fprintf(file, "        const glowMaterial = new THREE.MeshBasicMaterial({\n");
    fprintf(file, "            color: controls.stateColor,\n");
    fprintf(file, "            transparent: true,\n");
    fprintf(file, "            opacity: 0.5\n");
    fprintf(file, "        });\n");
    fprintf(file, "        const glowMesh = new THREE.Mesh(glowGeometry, glowMaterial);\n");
    fprintf(file, "        glowMesh.position.copy(stateVector);\n");
    fprintf(file, "        sphereGroup.add(glowMesh);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add circle to show theta angle (elevation)\n");
    fprintf(file, "        const thetaGeometry = new THREE.TorusGeometry(1, 0.005, 16, 100, Math.PI - theta);\n");
    fprintf(file, "        const thetaMaterial = new THREE.MeshBasicMaterial({ color: 0xff5500, side: THREE.DoubleSide });\n");
    fprintf(file, "        const thetaCircle = new THREE.Mesh(thetaGeometry, thetaMaterial);\n");
    fprintf(file, "        thetaCircle.rotation.y = phi + Math.PI / 2;\n");
    fprintf(file, "        thetaCircle.rotation.z = Math.PI / 2;\n");
    fprintf(file, "        sphereGroup.add(thetaCircle);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add arc to show phi angle (azimuth)\n");
    fprintf(file, "        if (Math.abs(Math.sin(theta)) > 0.01) { // Only if not close to poles\n");
    fprintf(file, "            const phiGeometry = new THREE.TorusGeometry(Math.sin(theta), 0.005, 16, 100, phi);\n");
    fprintf(file, "            const phiMaterial = new THREE.MeshBasicMaterial({ color: 0x00ff00, side: THREE.DoubleSide });\n");
    fprintf(file, "            const phiCircle = new THREE.Mesh(phiGeometry, phiMaterial);\n");
    fprintf(file, "            phiCircle.rotation.x = Math.PI / 2;\n");
    fprintf(file, "            phiCircle.position.z = Math.cos(theta);\n");
    fprintf(file, "            sphereGroup.add(phiCircle);\n");
    fprintf(file, "        }\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add GUI controls\n");
    fprintf(file, "        const gui = new dat.GUI({ width: 300 });\n");
    fprintf(file, "        const viewFolder = gui.addFolder('View Options');\n");
    fprintf(file, "        viewFolder.add(controls, 'autoRotate').name('Auto Rotate');\n");
    fprintf(file, "        viewFolder.add(controls, 'rotateSpeed', 0.001, 0.05).name('Rotation Speed');\n");
    fprintf(file, "        viewFolder.add(controls, 'sphereOpacity', 0, 1).name('Sphere Opacity').onChange(value => {\n");
    fprintf(file, "            sphereMaterial.opacity = value;\n");
    fprintf(file, "        });\n");
    fprintf(file, "        viewFolder.addColor(controls, 'stateColor').name('State Color').onChange(value => {\n");
    fprintf(file, "            stateMaterial.color.set(value);\n");
    fprintf(file, "            statePointMaterial.color.set(value);\n");
    fprintf(file, "            glowMaterial.color.set(value);\n");
    fprintf(file, "        });\n");
    fprintf(file, "        viewFolder.add(controls, 'showGrid').name('Show Grid').onChange(value => {\n");
    fprintf(file, "            gridHelper.visible = value;\n");
    fprintf(file, "        });\n");
    fprintf(file, "        viewFolder.add(controls, 'showLabels').name('Show Labels').onChange(value => {\n");
    fprintf(file, "            labels.visible = value;\n");
    fprintf(file, "        });\n");
    fprintf(file, "        viewFolder.add(controls, 'resetCamera').name('Reset Camera');\n");
    fprintf(file, "        viewFolder.open();\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Handle window resize\n");
    fprintf(file, "        window.addEventListener('resize', () => {\n");
    fprintf(file, "            camera.aspect = window.innerWidth / window.innerHeight;\n");
    fprintf(file, "            camera.updateProjectionMatrix();\n");
    fprintf(file, "            renderer.setSize(window.innerWidth, window.innerHeight);\n");
    fprintf(file, "        });\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Mouse interaction\n");
    fprintf(file, "        let mouseDown = false;\n");
    fprintf(file, "        let mouseX = 0;\n");
    fprintf(file, "        let mouseY = 0;\n");
    fprintf(file, "        \n");
    fprintf(file, "        document.addEventListener('mousedown', (event) => {\n");
    fprintf(file, "            mouseDown = true;\n");
    fprintf(file, "            mouseX = event.clientX;\n");
    fprintf(file, "            mouseY = event.clientY;\n");
    fprintf(file, "        });\n");
    fprintf(file, "        \n");
    fprintf(file, "        document.addEventListener('mouseup', () => {\n");
    fprintf(file, "            mouseDown = false;\n");
    fprintf(file, "        });\n");
    fprintf(file, "        \n");
    fprintf(file, "        document.addEventListener('mousemove', (event) => {\n");
    fprintf(file, "            if (mouseDown) {\n");
    fprintf(file, "                const deltaX = event.clientX - mouseX;\n");
    fprintf(file, "                const deltaY = event.clientY - mouseY;\n");
    fprintf(file, "                mouseX = event.clientX;\n");
    fprintf(file, "                mouseY = event.clientY;\n");
    fprintf(file, "                \n");
    fprintf(file, "                sphereGroup.rotation.y += deltaX * 0.01;\n");
    fprintf(file, "                sphereGroup.rotation.x += deltaY * 0.01;\n");
    fprintf(file, "            }\n");
    fprintf(file, "        });\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Zoom with mouse wheel\n");
    fprintf(file, "        document.addEventListener('wheel', (event) => {\n");
    fprintf(file, "            event.preventDefault();\n");
    fprintf(file, "            camera.position.z += event.deltaY * 0.005;\n");
    fprintf(file, "            // Limit zooming\n");
    fprintf(file, "            camera.position.z = Math.max(1.5, Math.min(10, camera.position.z));\n");
    fprintf(file, "        }, { passive: false });\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Add tooltips\n");
    fprintf(file, "        const tooltip = document.getElementById('tooltip');\n");
    fprintf(file, "        const showTooltip = (text, x, y) => {\n");
    fprintf(file, "            tooltip.innerHTML = text;\n");
    fprintf(file, "            tooltip.style.left = x + 'px';\n");
    fprintf(file, "            tooltip.style.top = y + 'px';\n");
    fprintf(file, "            tooltip.style.opacity = 1;\n");
    fprintf(file, "        };\n");
    fprintf(file, "        \n");
    fprintf(file, "        const hideTooltip = () => {\n");
    fprintf(file, "            tooltip.style.opacity = 0;\n");
    fprintf(file, "        };\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Animation loop\n");
    fprintf(file, "        const animate = () => {\n");
    fprintf(file, "            requestAnimationFrame(animate);\n");
    fprintf(file, "            \n");
    fprintf(file, "            if (controls.autoRotate) {\n");
    fprintf(file, "                sphereGroup.rotation.y += controls.rotateSpeed;\n");
    fprintf(file, "            }\n");
    fprintf(file, "            \n");
    fprintf(file, "            // Pulsate the glow effect\n");
    fprintf(file, "            const time = Date.now() * 0.001; // Time in seconds\n");
    fprintf(file, "            const pulseFactor = 0.5 + 0.5 * Math.sin(time * 2);\n");
    fprintf(file, "            glowMaterial.opacity = 0.3 + 0.4 * pulseFactor;\n");
    fprintf(file, "            glowGeometry.scale(0.99, 0.99, 0.99);\n");
    fprintf(file, "            if (glowGeometry.parameters.radius < 0.06) {\n");
    fprintf(file, "                glowGeometry.scale(1.1, 1.1, 1.1);\n");
    fprintf(file, "            }\n");
    fprintf(file, "            \n");
    fprintf(file, "            renderer.render(scene, camera);\n");
    fprintf(file, "        };\n");
    fprintf(file, "        \n");
    fprintf(file, "        animate();\n");
    fprintf(file, "    </script>\n");
    fprintf(file, "    <script>\n");
    fprintf(file, "        // Add explanation text that appears when pressing H key\n");
    fprintf(file, "        const helpDiv = document.createElement('div');\n");
    fprintf(file, "        helpDiv.style.position = 'absolute';\n");
    fprintf(file, "        helpDiv.style.left = '50%%';\n");
    fprintf(file, "        helpDiv.style.top = '50%%';\n");
    fprintf(file, "        helpDiv.style.transform = 'translate(-50%%, -50%%)';\n");
    fprintf(file, "        helpDiv.style.backgroundColor = 'rgba(0, 0, 0, 0.8)';\n");
    fprintf(file, "        helpDiv.style.color = '#fff';\n");
    fprintf(file, "        helpDiv.style.padding = '20px';\n");
    fprintf(file, "        helpDiv.style.borderRadius = '10px';\n");
    fprintf(file, "        helpDiv.style.maxWidth = '600px';\n");
    fprintf(file, "        helpDiv.style.zIndex = '1000';\n");
    fprintf(file, "        helpDiv.style.display = 'none';\n");
    fprintf(file, "        helpDiv.style.boxShadow = '0 0 20px rgba(0, 0, 0, 0.5)';\n");
    fprintf(file, "        helpDiv.innerHTML = `\n");
    fprintf(file, "            <h2 style=\"margin-top:0;color:#3498db;\">Bloch Sphere Visualization Help</h2>\n");
    fprintf(file, "            <p>The Bloch sphere is a geometric representation of a qubit state.</p>\n");
    fprintf(file, "            <ul>\n");
    fprintf(file, "                <li><strong>Mouse Drag:</strong> Rotate the sphere</li>\n");
    fprintf(file, "                <li><strong>Mouse Wheel:</strong> Zoom in/out</li>\n");
    fprintf(file, "                <li><strong>H Key:</strong> Toggle this help screen</li>\n");
    fprintf(file, "                <li><strong>GUI Panel:</strong> Adjust visualization options</li>\n");
    fprintf(file, "            </ul>\n");
    fprintf(file, "            <p style=\"text-align:center;margin-top:20px;color:#95a5a6;\">(Press H to hide this help)</p>\n");
    fprintf(file, "        `;\n");
    fprintf(file, "        document.body.appendChild(helpDiv);\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Toggle help display with H key\n");
    fprintf(file, "        document.addEventListener('keydown', (event) => {\n");
    fprintf(file, "            if (event.key.toLowerCase() === 'h') {\n");
    fprintf(file, "                helpDiv.style.display = helpDiv.style.display === 'none' ? 'block' : 'none';\n");
    fprintf(file, "            }\n");
    fprintf(file, "        });\n");
    fprintf(file, "        \n");
    fprintf(file, "        // Show help for the first time after a brief delay\n");
    fprintf(file, "        setTimeout(() => {\n");
    fprintf(file, "            helpDiv.style.display = 'block';\n");
    fprintf(file, "            setTimeout(() => {\n");
    fprintf(file, "                helpDiv.style.display = 'none';\n");
    fprintf(file, "            }, 8000); // Hide after 8 seconds\n");
    fprintf(file, "        }, 1000);\n");
    fprintf(file, "    </script>\n");
    fprintf(file, "</body>\n");
    fprintf(file, "</html>\n");
    
    fclose(file);
    printf("Enhanced Bloch sphere visualization saved to %s\n", filename);
}