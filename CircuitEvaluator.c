#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_NODES 200

typedef enum {
    GATE_INPUT = 0,
    GATE_NOT,
    GATE_AND,
    GATE_OR
} gate_type_t;

typedef struct {
    int id;
    gate_type_t type;
    int input1_id;
    int input2_id;
} circuit_node_t;

// Global variables
circuit_node_t circuit[MAX_NODES];
int m_inputs = 0;
int n_outputs = 0;
int num_gates = 0;
int output_ids[MAX_NODES];
int input_values[MAX_NODES];

// Memoization arrays
int global_values[MAX_NODES]; 
int global_status[MAX_NODES];  // 0=not started, 1=in progress, 2=completed

// Thread synchronization
pthread_mutex_t mutex;
pthread_cond_t cond[MAX_NODES];

typedef struct {
    int thread_id;
    int target_id;
} thread_args_t;

// Read circuit configuration from file
void read_circuit_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening circuit file");
        exit(EXIT_FAILURE);
    }
    
    // Read number of inputs and outputs
    if (fscanf(file, "%d", &m_inputs) != 1) exit(EXIT_FAILURE);
    if (fscanf(file, "%d", &n_outputs) != 1) exit(EXIT_FAILURE);
    
    // Read output IDs
    for (int i = 0; i < n_outputs; i++) {
        if (fscanf(file, "%d", &output_ids[i]) != 1) exit(EXIT_FAILURE);
    }
    
    // Read number of gates
    if (fscanf(file, "%d", &num_gates) != 1) exit(EXIT_FAILURE);
    
    // Initialize input nodes
    for (int i = 0; i < m_inputs; i++) {
        circuit[i].id = i;
        circuit[i].type = GATE_INPUT;
        circuit[i].input1_id = -1;
        circuit[i].input2_id = -1;
    }
    
    // Read gate definitions
    for (int i = 0; i < num_gates; i++) {
        int id;
        char op_str[5];
        int input1, input2;
        
        if (fscanf(file, "%d %s", &id, op_str) != 2) exit(EXIT_FAILURE);
        
        circuit[id].id = id;
        
        if (strcmp(op_str, "OR") == 0) {
            circuit[id].type = GATE_OR;
            if (fscanf(file, "%d %d", &input1, &input2) != 2) exit(EXIT_FAILURE);
            circuit[id].input1_id = input1;
            circuit[id].input2_id = input2;
        } else if (strcmp(op_str, "AND") == 0) {
            circuit[id].type = GATE_AND;
            if (fscanf(file, "%d %d", &input1, &input2) != 2) exit(EXIT_FAILURE);
            circuit[id].input1_id = input1;
            circuit[id].input2_id = input2;
        } else if (strcmp(op_str, "NOT") == 0) {
            circuit[id].type = GATE_NOT;
            if (fscanf(file, "%d", &input1) != 1) exit(EXIT_FAILURE);
            circuit[id].input1_id = input1;
            circuit[id].input2_id = -1;
        } else {
            fprintf(stderr, "Unknown gate type: %s\n", op_str);
            exit(EXIT_FAILURE);
        }
    }
    
    // Read input values (test vector)
    for (int i = 0; i < m_inputs; i++) {
        if (fscanf(file, "%d", &input_values[i]) != 1) {
            fprintf(stderr, "Error: Could not read all %d input values\n", m_inputs);
            exit(EXIT_FAILURE);
        }
    }
    
    fclose(file);
    fprintf(stderr, "Circuit loaded: %d inputs, %d gates, %d outputs.\n", 
            m_inputs, num_gates, n_outputs);
}

// Evaluate a gate with memoization and thread safety
int evaluate_gate_memoized(int node_id) {
    int result;
    
    // Input nodes return their values directly
    if (node_id < m_inputs) {
        return input_values[node_id];
    }
    
    pthread_mutex_lock(&mutex);
    
    // Already computed
    if (global_status[node_id] == 2) {
        result = global_values[node_id];
        pthread_mutex_unlock(&mutex);
        return result;
    }
    
    // Another thread is computing this - wait
    if (global_status[node_id] == 1) {
        pthread_cond_wait(&cond[node_id], &mutex);
        result = global_values[node_id];
        pthread_mutex_unlock(&mutex);
        return result;
    }
    
    // Mark as in progress
    global_status[node_id] = 1;
    pthread_mutex_unlock(&mutex);
    
    // Compute the gate output
    circuit_node_t *node = &circuit[node_id];
    int input1_val, input2_val;
    
    switch (node->type) {
        case GATE_NOT:
            input1_val = evaluate_gate_memoized(node->input1_id);
            result = !input1_val;
            break;
        case GATE_AND:
            input1_val = evaluate_gate_memoized(node->input1_id);
            input2_val = evaluate_gate_memoized(node->input2_id);
            result = input1_val & input2_val;
            break;
        case GATE_OR:
            input1_val = evaluate_gate_memoized(node->input1_id);
            input2_val = evaluate_gate_memoized(node->input2_id);
            result = input1_val | input2_val;
            break;
        default:
            result = 0;
            break;
    }
    
    // Store result and mark as completed
    pthread_mutex_lock(&mutex);
    global_values[node_id] = result;
    global_status[node_id] = 2;
    pthread_cond_broadcast(&cond[node_id]);
    pthread_mutex_unlock(&mutex);
    
    return result;
}

// Thread routine to evaluate an output
void* thread_routine(void *args) {
    thread_args_t *t_args = (thread_args_t *)args;
    int target_id = t_args->target_id;
    
    printf("Thread %d starting evaluation for output x%d\n", 
           t_args->thread_id, target_id);
    
    int final_result = evaluate_gate_memoized(target_id);
    
    printf("Thread %d finished. Final result for x%d is %d\n", 
           t_args->thread_id, target_id, final_result);
    
    free(args);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <circuit_data_file>\n", argv[0]);
        return 1;
    }
    
    read_circuit_file(argv[1]);
    
    int total_nodes = m_inputs + num_gates;
    
    // Print input configuration
    fprintf(stderr, "Input Configuration: %d Boolean values (x0 through x%d):\n", 
            m_inputs, m_inputs - 1);
    fprintf(stderr, "--- Input Values ---\n");
    for (int i = 0; i < m_inputs; i++) {
        fprintf(stderr, "Input x%d = %d\n", i, input_values[i]);
    }
    fprintf(stderr, "--------------------\n");
    
    usleep(100000);
    fflush(stdout);
    
    // Initialize synchronization primitives
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < total_nodes; i++) {
        global_values[i] = -1;
        global_status[i] = 0;
        pthread_cond_init(&cond[i], NULL);
    }
    
    pthread_t threads[n_outputs];
    
    printf("\nStarting parallel evaluation with %d threads...\n", n_outputs);
    
    // Create threads for each output
    for (int i = 0; i < n_outputs; i++) {
        thread_args_t *args = malloc(sizeof(thread_args_t));
        if (args == NULL) {
            perror("Failed to allocate thread arguments");
            return 1;
        }
        args->thread_id = i + 1;
        args->target_id = output_ids[i];
        
        if (pthread_create(&threads[i], NULL, &thread_routine, args) != 0) {
            perror("Error creating thread");
            free(args);
            return 1;
        }
    }
    
    // Wait for all threads
    for (int i = 0; i < n_outputs; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Print final results
    printf("\n--- Final output values: ---\n");
    for (int i = 0; i < n_outputs; i++) {
        int id = output_ids[i];
        printf("Output x%d = %d\n", id, global_values[id]);
    }
    
    // Cleanup
    pthread_mutex_destroy(&mutex);
    for (int i = 0; i < total_nodes; i++) {
        pthread_cond_destroy(&cond[i]);
    }
    
    return 0;
}
