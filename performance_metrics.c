/*
 * performance_metrics.c - Enhanced implementation 
 * 
 * This module provides comprehensive performance tracking for quantum 
 * simulation algorithms
 */

#include "quantum_interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <complex.h>


/* Initialize performance metrics */
void init_performance_metrics(PerformanceMetrics* metrics) {
    metrics->start_time = 0;
    metrics->end_time = 0;
    metrics->time_elapsed = 0.0;
    metrics->memory_used = 0;
    metrics->gate_count = 0;
}

/* starts timing */
void start_timing(PerformanceMetrics* metrics) {
    metrics->start_time = clock();
}

/* Ends timing and calculates*/
void end_timing(PerformanceMetrics* metrics) {
    metrics->end_time = clock();
    metrics->time_elapsed = ((double)(metrics->end_time - metrics->start_time)) / CLOCKS_PER_SEC;
}

/* updates memory usage */
void update_memory_usage(PerformanceMetrics* metrics, size_t memory_used) {
    metrics->memory_used = memory_used;
}

/* Increments gate count */
void increment_gate_count(PerformanceMetrics* metrics) {
    metrics->gate_count++;
}

/* Prints basic performance metrics */
void print_performance_metrics(PerformanceMetrics* metrics) {
    printf("\nPerformance Metrics:\n");
    printf("Time elapsed: %.6f seconds\n", metrics->time_elapsed);
    printf("Memory used: %zu bytes (%.2f KB, %.2f MB)\n", 
           metrics->memory_used,
           metrics->memory_used / 1024.0,
           metrics->memory_used / (1024.0 * 1024.0));
    printf("Gate count: %d\n", metrics->gate_count);
}

/*
* enhanced performance metrics structure  
*/
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
            double vector_sparsity;  // Percentage of non-zero amplitudes
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
    
    // Identification
    Algorithm algorithm_type;
    char algorithm_name[32];
    char circuit_name[64];
    int num_qubits;
} EnhancedPerformanceMetrics;

/* Initialize enhanced performance metrics */
void init_enhanced_metrics(EnhancedPerformanceMetrics* metrics, Algorithm algo, int num_qubits) {
    memset(metrics, 0, sizeof(EnhancedPerformanceMetrics));
    
    //sets alg type
    metrics->algorithm_type = algo;
    metrics->num_qubits = num_qubits;
    
    // Sets alg name
    switch (algo) {
        case STATE_VECTOR:
            strcpy(metrics->algorithm_name, "State Vector");
            break;
        case TENSOR_NETWORK:
            strcpy(metrics->algorithm_name, "Tensor Network");
            break;
        case STABILIZER:
            strcpy(metrics->algorithm_name, "Stabilizer");
            break;
        default:
            strcpy(metrics->algorithm_name, "Unknown");
            break;
    }
    
    // Default circuit name
    strcpy(metrics->circuit_name, "Unnamed Circuit");
}

/* Starts timing */
void start_enhanced_timing(EnhancedPerformanceMetrics* metrics) {
    gettimeofday(&metrics->start_wall_time, NULL);
    metrics->start_cpu_time = clock();
}

/* End timings and calculates */
void end_enhanced_timing(EnhancedPerformanceMetrics* metrics) {
    gettimeofday(&metrics->end_wall_time, NULL);
    metrics->end_cpu_time = clock();
    
    metrics->wall_time_elapsed = 
        (metrics->end_wall_time.tv_sec - metrics->start_wall_time.tv_sec) +
        (metrics->end_wall_time.tv_usec - metrics->start_wall_time.tv_usec) / 1000000.0;
    
    // Calculate CPU time in seconds
    metrics->cpu_time_elapsed = 
        ((double)(metrics->end_cpu_time - metrics->start_cpu_time)) / CLOCKS_PER_SEC;
}

/* Update memory usage information */
void update_enhanced_memory_usage(EnhancedPerformanceMetrics* metrics, size_t memory_used) {
    metrics->current_memory_used = memory_used;
    
    if (memory_used > metrics->peak_memory_used) {
        metrics->peak_memory_used = memory_used;
    }
}

/* Track gate operations */
void track_gate_operation(EnhancedPerformanceMetrics* metrics, OpCode gate, int num_targets) {
    metrics->gate_count++;
    
    switch (gate) {
        case OP_H:
            metrics->h_gate_count++;
            break;
        case OP_X:
            metrics->x_gate_count++;
            break;
        case OP_Z:
            metrics->z_gate_count++;
            break;
        case OP_S:
            metrics->s_gate_count++;
            break;
        case OP_CNOT:
            metrics->cnot_gate_count++;
            break;
        case OP_MEASURE:
            metrics->measure_count++;
            break;
    }
    
    // update algorithm-specific metrics
    if (metrics->algorithm_type == STABILIZER) {
        // track Clifford vs non-Clifford operations for stabilizer simulation
        if (gate == OP_H || gate == OP_S || gate == OP_CNOT) {
            metrics->algorithm_specific.stabilizer.clifford_count++;
        } else {
            metrics->algorithm_specific.stabilizer.non_clifford_count++;
        }
    }
}

/* Update circuit name */
void set_circuit_name(EnhancedPerformanceMetrics* metrics, const char* name) {
    strncpy(metrics->circuit_name, name, sizeof(metrics->circuit_name) - 1);
    metrics->circuit_name[sizeof(metrics->circuit_name) - 1] = '\0';
}

/* Update state vector metrics */
void update_state_vector_metrics(EnhancedPerformanceMetrics* metrics, 
                               StateVector* state) {
    if (metrics->algorithm_type != STATE_VECTOR) return;
    
    metrics->algorithm_specific.state_vector.state_vector_size = state->size;
    
    // Calculate percentage of non-zero amplitudes
    double threshold = 1e-10;
    size_t non_zero_count = 0;
    
    for (size_t i = 0; i < state->size; i++) {
        double real = creal(state->amplitudes[i]);
        double imag = cimag(state->amplitudes[i]);
        
        if (fabs(real) > threshold || fabs(imag) > threshold) {
            non_zero_count++;
        }
    }
    
    metrics->algorithm_specific.state_vector.vector_sparsity = 
        100.0 * ((double)non_zero_count / state->size);
}

/* Update tensor network  metrics */
void update_tensor_network_metrics(EnhancedPerformanceMetrics* metrics, 
                                 void* tn_state) {
    if (metrics->algorithm_type != TENSOR_NETWORK) return;
    
    // not a full implementation
    // For now, we'll just use placeholder values
    
    // Set some reasonable default values
    metrics->algorithm_specific.tensor_network.max_bond_dimension = 4;
    metrics->algorithm_specific.tensor_network.avg_bond_dimension = 2.5;
    metrics->algorithm_specific.tensor_network.max_entanglement_entropy = 
        log2(metrics->algorithm_specific.tensor_network.max_bond_dimension);
        
    printf("Note: Using placeholder tensor network metrics. Implement detailed analysis for production use.\n");
}

/* Update stabilizer metrics */
void update_stabilizer_metrics(EnhancedPerformanceMetrics* metrics, 
                             void* stabilizer_state) {
    if (metrics->algorithm_type != STABILIZER) return;
    
    metrics->algorithm_specific.stabilizer.tableau_size = metrics->num_qubits * metrics->num_qubits;
    
    printf("Note: Using placeholder stabilizer metrics. Implement detailed analysis for production use.\n");
}

/* Print detailed performance metrics */
void print_enhanced_metrics(EnhancedPerformanceMetrics* metrics) {
    printf("\n===== Performance Metrics for %s (%s) =====\n", 
           metrics->circuit_name, metrics->algorithm_name);
    printf("Number of qubits: %d\n", metrics->num_qubits);
    
    printf("\n----- Timing -----\n");
    printf("Wall clock time: %.6f seconds\n", metrics->wall_time_elapsed);
    printf("CPU time: %.6f seconds\n", metrics->cpu_time_elapsed);
    
    printf("\n----- Memory Usage -----\n");
    printf("Current memory used: %zu bytes (%.2f KB, %.2f MB)\n", 
           metrics->current_memory_used,
           metrics->current_memory_used / 1024.0,
           metrics->current_memory_used / (1024.0 * 1024.0));
    printf("Peak memory used: %zu bytes (%.2f KB, %.2f MB)\n", 
           metrics->peak_memory_used,
           metrics->peak_memory_used / 1024.0,
           metrics->peak_memory_used / (1024.0 * 1024.0));
    
    printf("\n----- Operations -----\n");
    printf("Total gates: %d\n", metrics->gate_count);
    printf("Gate breakdown:\n");
    printf("  H gates: %d\n", metrics->h_gate_count);
    printf("  X gates: %d\n", metrics->x_gate_count);
    printf("  Z gates: %d\n", metrics->z_gate_count);
    printf("  S gates: %d\n", metrics->s_gate_count);
    printf("  CNOT gates: %d\n", metrics->cnot_gate_count);
    printf("  Measurements: %d\n", metrics->measure_count);
    
    printf("\n----- Algorithm-Specific Metrics -----\n");
    switch (metrics->algorithm_type) {
        case STATE_VECTOR:
            printf("State vector size: %zu (2^%d)\n", 
                   metrics->algorithm_specific.state_vector.state_vector_size,
                   metrics->num_qubits);
            printf("Vector sparsity: %.2f%% non-zero amplitudes\n", 
                   metrics->algorithm_specific.state_vector.vector_sparsity);
            break;
            
        case TENSOR_NETWORK:
            printf("Maximum bond dimension: %d\n", 
                   metrics->algorithm_specific.tensor_network.max_bond_dimension);
            printf("Average bond dimension: %.2f\n", 
                   metrics->algorithm_specific.tensor_network.avg_bond_dimension);
            printf("Maximum entanglement entropy: %.2f\n", 
                   metrics->algorithm_specific.tensor_network.max_entanglement_entropy);
            break;
            
        case STABILIZER:
            printf("Tableau size: %d\n", 
                   metrics->algorithm_specific.stabilizer.tableau_size);
            printf("Clifford operations: %d\n", 
                   metrics->algorithm_specific.stabilizer.clifford_count);
            printf("Non-Clifford operations: %d\n", 
                   metrics->algorithm_specific.stabilizer.non_clifford_count);
            break;
            
        default:
            printf("No algorithm-specific metrics available.\n");
            break;
    }
    
    printf("\n");
}

/* Compare multiple algorithm performance metrics */
void compare_enhanced_metrics(EnhancedPerformanceMetrics metrics[], int count) {
    if (count <= 0) return;
    
    printf("\n===== Comparative Performance Analysis =====\n");
    printf("Circuit: %s\n", metrics[0].circuit_name);
    printf("Number of qubits: %d\n", metrics[0].num_qubits);
    
    printf("\n----- Timing Comparison (seconds) -----\n");
    printf("%-20s %-15s %-15s\n", "Algorithm", "Wall Time", "CPU Time");
    printf("%-20s %-15s %-15s\n", "----------", "---------", "--------");
    
    for (int i = 0; i < count; i++) {
        printf("%-20s %-15.6f %-15.6f\n", 
               metrics[i].algorithm_name,
               metrics[i].wall_time_elapsed,
               metrics[i].cpu_time_elapsed);
    }
    
    printf("\n----- Memory Usage Comparison (MB) -----\n");
    printf("%-20s %-15s\n", "Algorithm", "Peak Memory");
    printf("%-20s %-15s\n", "----------", "-----------");
    
    for (int i = 0; i < count; i++) {
        printf("%-20s %-15.2f\n", 
               metrics[i].algorithm_name,
               metrics[i].peak_memory_used / (1024.0 * 1024.0));
    }
    
    printf("\n----- Operation Count -----\n");
    printf("%-20s %-10s\n", "Algorithm", "Gates");
    printf("%-20s %-10s\n", "----------", "-----");
    
    for (int i = 0; i < count; i++) {
        printf("%-20s %-10d\n", 
               metrics[i].algorithm_name,
               metrics[i].gate_count);
    }
    
    //calculate efficiency ratios
    if (count > 1) {
        printf("\n----- Relative Efficiency (compared to State Vector) -----\n");
        
        // Find state vector metrics
        int sv_idx = -1;
        for (int i = 0; i < count; i++) {
            if (metrics[i].algorithm_type == STATE_VECTOR) {
                sv_idx = i;
                break;
            }
        }
        
        if (sv_idx >= 0) {
            printf("%-20s %-15s %-15s\n", "Algorithm", "Time Ratio", "Memory Ratio");
            printf("%-20s %-15s %-15s\n", "----------", "----------", "------------");
            
            double sv_time = metrics[sv_idx].wall_time_elapsed;
            double sv_memory = metrics[sv_idx].peak_memory_used;
            
            for (int i = 0; i < count; i++) {
                if (i != sv_idx) {
                    double time_ratio = sv_time / metrics[i].wall_time_elapsed;
                    double memory_ratio = sv_memory / metrics[i].peak_memory_used;
                    
                    printf("%-20s %-15.2fx %-15.2fx\n", 
                           metrics[i].algorithm_name,
                           time_ratio,
                           memory_ratio);
                }
            }
        }
    }
    
    printf("\n");
}

/* Original implementation of compare_performance_metrics 
void compare_performance_metrics(PerformanceMetrics* state_vector_metrics,
                             PerformanceMetrics* tensor_network_metrics,
                             PerformanceMetrics* stabilizer_metrics) {
    printf("\n=== Performance Comparison ===\n");
    printf("                  State Vector    Tensor Network    Stabilizer\n");
    printf("Time (sec)       %12.6f    %12.6f    %12.6f\n", 
           state_vector_metrics->time_elapsed,
           tensor_network_metrics->time_elapsed,
           stabilizer_metrics->time_elapsed);
    printf("Memory (bytes)   %12zu    %12zu    %12zu\n", 
           state_vector_metrics->memory_used,
           tensor_network_metrics->memory_used,
           stabilizer_metrics->memory_used);
    printf("Gate count       %12d    %12d    %12d\n", 
           state_vector_metrics->gate_count,
           tensor_network_metrics->gate_count,
           stabilizer_metrics->gate_count);
    
    // Calculate efficiency ratios
    printf("\nRelative Efficiency (compared to State Vector):\n");
    printf("Time efficiency:  Tensor Network: %.2fx,  Stabilizer: %.2fx\n",
           state_vector_metrics->time_elapsed / tensor_network_metrics->time_elapsed,
           state_vector_metrics->time_elapsed / stabilizer_metrics->time_elapsed);
    printf("Memory efficiency: Tensor Network: %.2fx,  Stabilizer: %.2fx\n",
           (double)state_vector_metrics->memory_used / tensor_network_metrics->memory_used,
           (double)state_vector_metrics->memory_used / stabilizer_metrics->memory_used);
}

/* Save metrics to CSV file for external analysis */
void save_metrics_to_csv(EnhancedPerformanceMetrics metrics[], int count, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing\n", filename);
        return;
    }
    
    // Write CSV header
    fprintf(file, "Algorithm,Qubits,Circuit,WallTime,CPUTime,PeakMemory,GateCount,HGates,XGates,ZGates,SGates,CNOTGates,Measurements");
    
    // Add algorithm-specific headers
    fprintf(file, ",StateVectorSize,Sparsity,MaxBondDim,AvgBondDim,MaxEntropy,TableauSize,CliffordCount,NonCliffordCount\n");
    
    // Write data rows
    for (int i = 0; i < count; i++) {
        EnhancedPerformanceMetrics* m = &metrics[i];
        
        fprintf(file, "%s,%d,%s,%.6f,%.6f,%zu,%d,%d,%d,%d,%d,%d,%d",
                m->algorithm_name,
                m->num_qubits,
                m->circuit_name,
                m->wall_time_elapsed,
                m->cpu_time_elapsed,
                m->peak_memory_used,
                m->gate_count,
                m->h_gate_count,
                m->x_gate_count,
                m->z_gate_count,
                m->s_gate_count,
                m->cnot_gate_count,
                m->measure_count);
        
        // Add algorithm-specific data (write 0 for metrics that don't apply to this algorithm)
        if (m->algorithm_type == STATE_VECTOR) {
            fprintf(file, ",%zu,%.2f,0,0.0,0.0,0,0,0\n",
                    m->algorithm_specific.state_vector.state_vector_size,
                    m->algorithm_specific.state_vector.vector_sparsity);
        } else if (m->algorithm_type == TENSOR_NETWORK) {
            fprintf(file, ",0,0.0,%d,%.2f,%.2f,0,0,0\n",
                    m->algorithm_specific.tensor_network.max_bond_dimension,
                    m->algorithm_specific.tensor_network.avg_bond_dimension,
                    m->algorithm_specific.tensor_network.max_entanglement_entropy);
        } else if (m->algorithm_type == STABILIZER) {
            fprintf(file, ",0,0.0,0,0.0,0.0,%d,%d,%d\n",
                    m->algorithm_specific.stabilizer.tableau_size,
                    m->algorithm_specific.stabilizer.clifford_count,
                    m->algorithm_specific.stabilizer.non_clifford_count);
        } else {
            fprintf(file, ",0,0.0,0,0.0,0.0,0,0,0\n");
        }
    }
    
    fclose(file);
    printf("Performance metrics saved to %s\n", filename);
}

/* Generate Python script for visualization */
/* doesn't work */
void generate_visualization_script(EnhancedPerformanceMetrics metrics[], int count, const char* csv_filename, const char* script_filename) {
    FILE* file = fopen(script_filename, "w");
    if (!file) {
        fprintf(stderr, "Error opening file %s for writing\n", script_filename);
        return;
    }
    
    // Write Python script with matplotlib
    fprintf(file, "#!/usr/bin/env python3\n");
    fprintf(file, "# Auto-generated visualization script for quantum interpreter performance metrics\n\n");
    
    fprintf(file, "import pandas as pd\n");
    fprintf(file, "import matplotlib.pyplot as plt\n");
    fprintf(file, "import numpy as np\n");
    fprintf(file, "import os\n\n");
    
    fprintf(file, "# Read the CSV data\n");
    fprintf(file, "data = pd.read_csv('%s')\n\n", csv_filename);
    
    fprintf(file, "# Create output directory\n");
    fprintf(file, "os.makedirs('performance_plots', exist_ok=True)\n\n");
    
    fprintf(file, "# Plot 1: Time comparison\n");
    fprintf(file, "plt.figure(figsize=(10, 6))\n");
    fprintf(file, "algorithms = data['Algorithm'].unique()\n");
    fprintf(file, "time_data = data[['Algorithm', 'WallTime', 'CPUTime']]\n");
    fprintf(file, "time_data = time_data.melt(id_vars=['Algorithm'], var_name='TimeType', value_name='Seconds')\n");
    fprintf(file, "plt.bar(np.arange(len(time_data)), time_data['Seconds'], color=['#3498db', '#2980b9'] * len(algorithms))\n");
    fprintf(file, "plt.xticks(np.arange(len(time_data)), [f\"{row['Algorithm']}\\n{row['TimeType']}\" for _, row in time_data.iterrows()], rotation=45)\n");
    fprintf(file, "plt.ylabel('Time (seconds)')\n");
    fprintf(file, "plt.title('Execution Time Comparison')\n");
    fprintf(file, "plt.tight_layout()\n");
    fprintf(file, "plt.grid(axis='y', linestyle='--', alpha=0.7)\n");
    fprintf(file, "plt.savefig('performance_plots/time_comparison.png', dpi=300)\n");
    fprintf(file, "plt.close()\n\n");
    
    fprintf(file, "# Plot 2: Memory comparison\n");
    fprintf(file, "plt.figure(figsize=(10, 6))\n");
    fprintf(file, "memory_data = data[['Algorithm', 'PeakMemory']]\n");
    fprintf(file, "memory_data['Memory (MB)'] = memory_data['PeakMemory'] / (1024 * 1024)\n");
    fprintf(file, "plt.bar(memory_data['Algorithm'], memory_data['Memory (MB)'], color='#27ae60')\n");
    fprintf(file, "plt.ylabel('Memory (MB)')\n");
    fprintf(file, "plt.title('Peak Memory Usage Comparison')\n");
    fprintf(file, "plt.xticks(rotation=45)\n");
    fprintf(file, "plt.tight_layout()\n");
    fprintf(file, "plt.grid(axis='y', linestyle='--', alpha=0.7)\n");
    fprintf(file, "plt.savefig('performance_plots/memory_comparison.png', dpi=300)\n");
    fprintf(file, "plt.close()\n\n");
    

    fprintf(file, "# Plot 3: Gate count comparison\n");
    fprintf(file, "plt.figure(figsize=(12, 6))\n");
    fprintf(file, "gate_cols = ['HGates', 'XGates', 'ZGates', 'SGates', 'CNOTGates', 'Measurements']\n");
    fprintf(file, "gate_data = data[['Algorithm'] + gate_cols]\n");
    fprintf(file, "gate_data = gate_data.melt(id_vars=['Algorithm'], var_name='GateType', value_name='Count')\n");
    fprintf(file, "gate_data['GateType'] = gate_data['GateType'].str.replace('Gates', '')\n");
    fprintf(file, "colors = ['#3498db', '#e74c3c', '#2ecc71', '#f39c12', '#9b59b6', '#34495e']\n");
    fprintf(file, "ax = plt.subplot(111)\n");
    fprintf(file, "for i, alg in enumerate(data['Algorithm']):\n");
    fprintf(file, "    alg_data = gate_data[gate_data['Algorithm'] == alg]\n");
    fprintf(file, "    bottom = 0\n");
    fprintf(file, "    for j, gate in enumerate(alg_data['GateType']):\n");
    fprintf(file, "        count = alg_data.iloc[j]['Count']\n");
    fprintf(file, "        ax.bar(i, count, bottom=bottom, color=colors[j], label=gate if i == 0 else \"\")\n");
    fprintf(file, "        if count > 0:\n");
    fprintf(file, "            ax.text(i, bottom + count/2, str(int(count)), ha='center', va='center', color='white')\n");
    fprintf(file, "        bottom += count\n");
    fprintf(file, "ax.set_xticks(range(len(data['Algorithm'])))\n");
    fprintf(file, "ax.set_xticklabels(data['Algorithm'], rotation=45)\n");
    fprintf(file, "ax.set_ylabel('Gate Count')\n");
    fprintf(file, "ax.set_title('Gate Usage by Algorithm')\n");
    fprintf(file, "ax.legend(loc='upper right')\n");
    fprintf(file, "plt.tight_layout()\n");
    fprintf(file, "plt.savefig('performance_plots/gate_counts.png', dpi=300)\n");
    fprintf(file, "plt.close()\n\n");
    
    fprintf(file, "# Plot 4: Efficiency ratios (compared to State Vector)\n");
    fprintf(file, "plt.figure(figsize=(10, 6))\n");
    fprintf(file, "# Get state vector data as baseline\n");
    fprintf(file, "sv_data = data[data['Algorithm'] == 'State Vector']\n");
    fprintf(file, "if not sv_data.empty:\n");
    fprintf(file, "    sv_time = sv_data['WallTime'].values[0]\n");
    fprintf(file, "    sv_memory = sv_data['PeakMemory'].values[0]\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Calculate ratios for other algorithms\n");
    fprintf(file, "    other_algs = data[data['Algorithm'] != 'State Vector']\n");
    fprintf(file, "    if not other_algs.empty:\n");
    fprintf(file, "        other_algs['TimeRatio'] = sv_time / other_algs['WallTime']\n");
    fprintf(file, "        other_algs['MemoryRatio'] = sv_memory / other_algs['PeakMemory']\n");
    fprintf(file, "        \n");
    fprintf(file, "        # Plot\n");
    fprintf(file, "        x = np.arange(len(other_algs))\n");
    fprintf(file, "        width = 0.35\n");
    fprintf(file, "        \n");
    fprintf(file, "        plt.bar(x - width/2, other_algs['TimeRatio'], width, label='Time Efficiency', color='#e74c3c')\n");
    fprintf(file, "        plt.bar(x + width/2, other_algs['MemoryRatio'], width, label='Memory Efficiency', color='#3498db')\n");
    fprintf(file, "        \n");
    fprintf(file, "        plt.axhline(y=1, linestyle='--', color='gray', alpha=0.7)\n");
    fprintf(file, "        plt.ylabel('Efficiency Ratio (higher is better)')\n");
    fprintf(file, "        plt.title('Efficiency Compared to State Vector Algorithm')\n");
    fprintf(file, "        plt.xticks(x, other_algs['Algorithm'])\n");
    fprintf(file, "        plt.legend()\n");
    fprintf(file, "        \n");
    fprintf(file, "        # Add ratio labels\n");
    fprintf(file, "        for i, v in enumerate(other_algs['TimeRatio']):\n");
    fprintf(file, "            plt.text(i - width/2, v + 0.1, f'{v:.2f}x', ha='center')\n");
    fprintf(file, "            \n");
    fprintf(file, "        for i, v in enumerate(other_algs['MemoryRatio']):\n");
    fprintf(file, "            plt.text(i + width/2, v + 0.1, f'{v:.2f}x', ha='center')\n");
    fprintf(file, "        \n");
    fprintf(file, "        plt.tight_layout()\n");
    fprintf(file, "        plt.grid(axis='y', linestyle='--', alpha=0.7)\n");
    fprintf(file, "        plt.savefig('performance_plots/efficiency_ratios.png', dpi=300)\n");
    fprintf(file, "else:\n");
    fprintf(file, "    print('No State Vector data for efficiency comparison')\n");
    fprintf(file, "plt.close()\n\n");

    // Algorithm-specific plots
    fprintf(file, "# Plot 5: Algorithm-specific metrics\n");
    fprintf(file, "plt.figure(figsize=(12, 8))\n");
    fprintf(file, "fig, axes = plt.subplots(3, 1, figsize=(12, 12))\n");
    fprintf(file, "\n");
    fprintf(file, "# State Vector metrics\n");
    fprintf(file, "sv_data = data[data['Algorithm'] == 'State Vector']\n");
    fprintf(file, "if not sv_data.empty:\n");
    fprintf(file, "    axes[0].bar(['Sparsity (%%%)'], sv_data['Sparsity'], color='#3498db')\n");
    fprintf(file, "    axes[0].set_title('State Vector Metrics')\n");
    fprintf(file, "    axes[0].grid(axis='y', linestyle='--', alpha=0.7)\n");
    fprintf(file, "    axes[0].text(0, sv_data['Sparsity'].values[0]/2, f\"{sv_data['Sparsity'].values[0]:.2f}%%\", \n");
    fprintf(file, "               ha='center', va='center', color='white')\n");
    fprintf(file, "    axes[0].set_ylim(0, 100)\n");
    fprintf(file, "\n");
    fprintf(file, "# Tensor Network metrics\n");
    fprintf(file, "tn_data = data[data['Algorithm'] == 'Tensor Network']\n");
    fprintf(file, "if not tn_data.empty:\n");
    fprintf(file, "    tn_metrics = ['MaxBondDim', 'AvgBondDim', 'MaxEntropy']\n");
    fprintf(file, "    axes[1].bar(tn_metrics, tn_data[tn_metrics].values[0], color='#2ecc71')\n");
    fprintf(file, "    axes[1].set_title('Tensor Network Metrics')\n");
    fprintf(file, "    axes[1].grid(axis='y', linestyle='--', alpha=0.7)\n");
    fprintf(file, "    for i, metric in enumerate(tn_metrics):\n");
    fprintf(file, "        axes[1].text(i, tn_data[metric].values[0]/2, f\"{tn_data[metric].values[0]:.2f}\", \n");
    fprintf(file, "                   ha='center', va='center', color='white')\n");
    fprintf(file, "\n");
    fprintf(file, "# Stabilizer metrics\n");
    fprintf(file, "stab_data = data[data['Algorithm'] == 'Stabilizer']\n");
    fprintf(file, "if not stab_data.empty:\n");
    fprintf(file, "    stab_metrics = ['TableauSize', 'CliffordCount', 'NonCliffordCount']\n");
    fprintf(file, "    stab_display = ['Tableau Size', 'Clifford Gates', 'Non-Clifford Gates']\n");
    fprintf(file, "    axes[2].bar(stab_display, stab_data[stab_metrics].values[0], color='#e74c3c')\n");
    fprintf(file, "    axes[2].set_title('Stabilizer Metrics')\n");
    fprintf(file, "    axes[2].grid(axis='y', linestyle='--', alpha=0.7)\n");
    fprintf(file, "    for i, metric in enumerate(stab_metrics):\n");
    fprintf(file, "        val = stab_data[metric].values[0]\n");
    fprintf(file, "        axes[2].text(i, val/2, str(int(val)), ha='center', va='center', color='white')\n");
    fprintf(file, "\n");
    fprintf(file, "plt.tight_layout()\n");
    fprintf(file, "plt.savefig('performance_plots/algorithm_specific.png', dpi=300)\n");
    fprintf(file, "plt.close()\n");
    fprintf(file, "\n");
    fprintf(file, "# Plot 6: Scaling analysis (if multiple qubit sizes available)\n");
    fprintf(file, "qubits = data['Qubits'].unique()\n");
    fprintf(file, "if len(qubits) > 1:\n");
    fprintf(file, "    plt.figure(figsize=(12, 10))\n");
    fprintf(file, "    fig, axes = plt.subplots(2, 1, figsize=(12, 10))\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Time scaling\n");
    fprintf(file, "    pivot_time = pd.pivot_table(data, values='WallTime', index='Qubits', columns='Algorithm')\n");
    fprintf(file, "    pivot_time.plot(ax=axes[0], marker='o', loglog=True)\n");
    fprintf(file, "    axes[0].set_title('Time Scaling with Qubit Count')\n");
    fprintf(file, "    axes[0].set_xlabel('Number of Qubits (log scale)')\n");
    fprintf(file, "    axes[0].set_ylabel('Time in seconds (log scale)')\n");
    fprintf(file, "    axes[0].grid(True, which=\"both\", ls=\"--\")\n");
    fprintf(file, "    \n");
    fprintf(file, "    # Memory scaling\n");
    fprintf(file, "    pivot_mem = pd.pivot_table(data, values='PeakMemory', index='Qubits', columns='Algorithm')\n");
    fprintf(file, "    pivot_mem = pivot_mem / (1024 * 1024)  # Convert to MB\n");
    fprintf(file, "    pivot_mem.plot(ax=axes[1], marker='o', loglog=True)\n");
    fprintf(file, "    axes[1].set_title('Memory Scaling with Qubit Count')\n");
    fprintf(file, "    axes[1].set_xlabel('Number of Qubits (log scale)')\n");
    fprintf(file, "    axes[1].set_ylabel('Memory in MB (log scale)')\n");
    fprintf(file, "    axes[1].grid(True, which=\"both\", ls=\"--\")\n");
    fprintf(file, "    \n");
    fprintf(file, "    plt.tight_layout()\n");
    fprintf(file, "    plt.savefig('performance_plots/scaling_analysis.png', dpi=300)\n");
    fprintf(file, "    plt.close()\n");
    fprintf(file, "\n");
    fprintf(file, "print(\"Generated visualization plots in 'performance_plots/' directory\")\n");
    
    fclose(file);
    printf("Visualization script generated: %s\n", script_filename);
    printf("Run 'python3 %s' to generate the plots\n", script_filename);
}

/* Run a comparison of all three algorithms on the same circuit */
void run_algorithm_comparison(VM* vm, const char* source, int num_qubits, const char* circuit_name) {
    printf("\nRunning algorithm comparison for %s (%d qubits)...\n", 
           circuit_name, num_qubits);
    

    const int num_algorithms = 3;
    EnhancedPerformanceMetrics metrics[num_algorithms];
    
    init_enhanced_metrics(&metrics[0], STATE_VECTOR, num_qubits);
    init_enhanced_metrics(&metrics[1], TENSOR_NETWORK, num_qubits);
    init_enhanced_metrics(&metrics[2], STABILIZER, num_qubits);
    

    for (int i = 0; i < num_algorithms; i++) {
        set_circuit_name(&metrics[i], circuit_name);
    }
    
    for (int i = 0; i < num_algorithms; i++) {
        Algorithm algo = metrics[i].algorithm_type;
        
        printf("\nRunning %s algorithm...\n", metrics[i].algorithm_name);
        
        reset_state(vm, num_qubits, algo);
        
        start_enhanced_timing(&metrics[i]);
        
        InterpretResult result = interpret(vm, source, num_qubits, algo);
        if (result != INTERPRET_OK) {
            fprintf(stderr, "Error running %s algorithm\n", metrics[i].algorithm_name);
            continue;
        }
        
        end_enhanced_timing(&metrics[i]);

        switch (algo) {
            case STATE_VECTOR: {
                StateVector* state = (StateVector*)((QuantumState*)vm->state)->state;
                update_state_vector_metrics(&metrics[i], state);
                
                size_t memory_used = sizeof(QuantumState) + 
                                  sizeof(StateVector) + 
                                  state->size * sizeof(double _Complex);
                update_enhanced_memory_usage(&metrics[i], memory_used);
                break;
            }
            case TENSOR_NETWORK: {
                void* tn_state = ((QuantumState*)vm->state)->state;
                update_tensor_network_metrics(&metrics[i], tn_state);
                
                size_t memory_used = sizeof(QuantumState) + 
                                  sizeof(void*) + 
                                  metrics[i].algorithm_specific.tensor_network.max_bond_dimension * 
                                  metrics[i].algorithm_specific.tensor_network.avg_bond_dimension * 
                                  num_qubits * sizeof(double _Complex);
                update_enhanced_memory_usage(&metrics[i], memory_used);
                break;
            }
            case STABILIZER: {
                void* stab_state = ((QuantumState*)vm->state)->state;
                update_stabilizer_metrics(&metrics[i], stab_state);
                
                size_t memory_used = sizeof(QuantumState) + 
                                  sizeof(void*) + 
                                  metrics[i].algorithm_specific.stabilizer.tableau_size * sizeof(int);
                update_enhanced_memory_usage(&metrics[i], memory_used);
                break;
            }
        }
        
        metrics[i].gate_count = vm->metrics.gate_count;
        
        print_enhanced_metrics(&metrics[i]);
    }
    
    compare_enhanced_metrics(metrics, num_algorithms);
    
    char csv_filename[256];
    char script_filename[256];
    snprintf(csv_filename, sizeof(csv_filename), "%s_metrics.csv", circuit_name);
    snprintf(script_filename, sizeof(script_filename), "%s_visualize.py", circuit_name);
    
    save_metrics_to_csv(metrics, num_algorithms, csv_filename);
    generate_visualization_script(metrics, num_algorithms, csv_filename, script_filename);
    
    printf("\nTo generate visualization plots, run:\n");
    printf("python3 %s\n\n", script_filename);
}