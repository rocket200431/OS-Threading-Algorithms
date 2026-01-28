#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler_utils.h"

typedef struct {
    int completion_time;
    int turnaround_time;
    int waiting_time;
    int finished;
} Metrics;

void round_robin_scheduler(Process processes[], int n, int quantum) {
    Queue ready_queue;
    MinHeap waiting_queue;
    initQueue(&ready_queue);
    heapInit(&waiting_queue, 1);
    
    int current_time = 0;
    int completed = 0;
    int visited[Max_Process] = {0};
    
    Metrics stats[Max_Process] = {0};
    int total_cpu[Max_Process] = {0};
    
    // Calculate total CPU time for each process
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < processes[i].brust_count; j += 2) {
            total_cpu[i] += processes[i].brusts[j];
        }
    }
    
    printf("0 : Starting\n");
    
    while (completed < n) {
        // Add newly arrived processes to ready queue
        for (int i = 0; i < n; i++) {
            if (!visited[i] && processes[i].arival <= current_time) {
                enqueue(&ready_queue, processes[i]);
                visited[i] = 1;
                printf("%d : Process %s joins ready queue upon arrival\n", 
                       current_time, processes[i].id);
            }
        }
        
        // Move processes from waiting queue (I/O complete) to ready queue
        while (!heapIsEmpty(&waiting_queue) && 
               heapPeek(&waiting_queue).sceduled_time <= current_time) {
            Process p = heapExtractMin(&waiting_queue);
            enqueue(&ready_queue, p);
            printf("%d : Process %s joins ready queue after IO completion\n", 
                   current_time, p.id);
        }
        
        // If ready queue is empty, advance time
        if (isEmptyQueue(&ready_queue)) {
            if (!heapIsEmpty(&waiting_queue)) {
                current_time = heapPeek(&waiting_queue).sceduled_time;
                continue;
            }
            
            int next_arrival = 1e9;
            for (int i = 0; i < n; i++) {
                if (!visited[i] && processes[i].arival < next_arrival)
                    next_arrival = processes[i].arival;
            }
            if (next_arrival != 1e9) {
                current_time = next_arrival;
                continue;
            }
            break;
        }
        
        // Schedule process from ready queue
        Process current = dequeue(&ready_queue);
        int idx = atoi(current.id + 1) - 1;
        
        int remaining = current.brusts[current.curr_brust_idx];
        int exec_time = (remaining > quantum) ? quantum : remaining;
        
        printf("%d : Process %s is scheduled to run for time %d\n", 
               current_time, current.id, exec_time);
        
        current.brusts[current.curr_brust_idx] -= exec_time;
        current_time += exec_time;
        
        if (current.brusts[current.curr_brust_idx] == 0) {
            current.curr_brust_idx++;
            
            if (current.curr_brust_idx >= current.brust_count) {
                // Process finished
                stats[idx].completion_time = current_time;
                stats[idx].turnaround_time = current_time - processes[idx].arival;
                stats[idx].waiting_time = stats[idx].turnaround_time - total_cpu[idx];
                stats[idx].finished = 1;
                completed++;
                printf("%d : Process %s exits. Turnaround time = %d, Wait time = %d\n",
                       current_time, current.id, stats[idx].turnaround_time, 
                       stats[idx].waiting_time);
            } else {
                // Start I/O operation
                int io_time = current.brusts[current.curr_brust_idx];
                current.curr_brust_idx++;
                current.sceduled_time = current_time + io_time;
                heapInsert(&waiting_queue, current);
            }
        } else {
            // Process timeout - add new arrivals and I/O completions first
            for (int i = 0; i < n; i++) {
                if (!visited[i] && processes[i].arival <= current_time) {
                    enqueue(&ready_queue, processes[i]);
                    visited[i] = 1;
                    printf("%d : Process %s joins ready queue upon arrival\n", 
                           current_time, processes[i].id);
                }
            }
            
            while (!heapIsEmpty(&waiting_queue) && 
                   heapPeek(&waiting_queue).sceduled_time <= current_time) {
                Process p = heapExtractMin(&waiting_queue);
                enqueue(&ready_queue, p);
                printf("%d : Process %s joins ready queue after IO completion\n", 
                       current_time, p.id);
            }
            
            enqueue(&ready_queue, current);
            printf("%d : Process %s joins ready queue after timeout\n", 
                   current_time, current.id);
        }
    }
    
    printf("%d : CPU goes idle\n", current_time);
    
    // Print statistics
    double avg_tat = 0, avg_wt = 0;
    printf("\nProcess Completion Turnaround Waiting\n");
    for (int i = 0; i < n; i++) {
        printf("%s %d %d %d\n", processes[i].id,
               stats[i].completion_time,
               stats[i].turnaround_time,
               stats[i].waiting_time);
        avg_tat += stats[i].turnaround_time;
        avg_wt += stats[i].waiting_time;
    }
    avg_tat /= n;
    avg_wt /= n;
    printf("Average Turnaround Time = %.1f\n", avg_tat);
    printf("Average Waiting Time = %.1f\n", avg_wt);
}

int main() {
    FILE* file = fopen("processinfo.txt", "r");
    if (!file) {
        printf("Error: could not open processinfo.txt\n");
        return 1;
    }
    
    int n, quantum;
    fscanf(file, "%d%d", &n, &quantum);
    fgetc(file);
    
    Process processes[Max_Process];
    char line[256];
    
    for (int i = 0; i < n; i++) {
        if (fgets(line, sizeof(line), file) == NULL) break;
        
        char* token = strtok(line, " \t\n");
        sprintf(processes[i].id, "P%s", token);
        
        token = strtok(NULL, " \t\n");
        processes[i].arival = atoi(token);
        processes[i].curr_brust_idx = 0;
        processes[i].brust_count = 0;
        
        while ((token = strtok(NULL, " \t\n")) != NULL) {
            int val = atoi(token);
            if (val == -1) break;
            processes[i].brusts[processes[i].brust_count++] = val;
        }
    }
    fclose(file);
    
    round_robin_scheduler(processes, n, quantum);
    return 0;
}
