#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define N 9
#define TOTAL_THREADS 27

// Thread validation types
#define TYPE_ROW     0
#define TYPE_COLUMN  1
#define TYPE_SUBGRID 2

// Global Sudoku grid and validation results
static int grid[N][N];
static int results[TOTAL_THREADS]; // 0-8: rows, 9-17: columns, 18-26: subgrids

// Thread parameter structure
typedef struct {
    int start_row;
    int start_col;
    int type;
    int index;
} thread_params;

// Validate a single row
static int validate_row(int row) {
    int seen[10] = {0};
    for (int col = 0; col < N; col++) {
        int value = grid[row][col];
        if (value < 1 || value > 9 || seen[value]) 
            return 0;
        seen[value] = 1;
    }
    return 1;
}

// Validate a single column
static int validate_col(int col) {
    int seen[10] = {0};
    for (int row = 0; row < N; row++) {
        int value = grid[row][col];
        if (value < 1 || value > 9 || seen[value]) 
            return 0;
        seen[value] = 1;
    }
    return 1;
}

// Validate a 3x3 subgrid
static int validate_subgrid(int start_row, int start_col) {
    int seen[10] = {0};
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int value = grid[start_row + r][start_col + c];
            if (value < 1 || value > 9 || seen[value]) 
                return 0;
            seen[value] = 1;
        }
    }
    return 1;
}

// Thread worker function
static void* worker(void* arg) {
    thread_params* params = (thread_params*)arg;
    int valid = 0;
    
    switch (params->type) {
        case TYPE_ROW:
            valid = validate_row(params->start_row);
            break;
        case TYPE_COLUMN:
            valid = validate_col(params->start_col);
            break;
        case TYPE_SUBGRID:
            valid = validate_subgrid(params->start_row, params->start_col);
            break;
    }
    
    results[params->index] = valid;
    free(params);
    return NULL;
}

// Load default valid Sudoku grid
static void load_default_grid(void) {
    int default_grid[N][N] = {
        {6,2,4,5,3,9,1,8,7},
        {5,1,9,7,2,8,6,3,4},
        {8,3,7,6,1,4,2,9,5},
        {1,4,3,8,6,5,7,2,9},
        {9,5,8,2,4,7,3,6,1},
        {7,6,2,3,9,1,4,5,8},
        {3,7,1,9,5,6,8,4,2},
        {4,9,6,1,8,2,5,7,3},
        {2,8,5,4,7,3,9,1,6}
    };
    memcpy(grid, default_grid, sizeof(grid));
}

// Try to load grid from stdin
static int try_load_from_stdin(void) {
    int values[N * N];
    int count = 0;
    
    if (getchar() == '\n') {
        return 0;
    }
    ungetc('\n', stdin);
    
    while (count < N * N) {
        int x;
        if (scanf("%d", &x) != 1) break;
        values[count++] = x;
    }
    
    if (count == N * N) {
        int k = 0;
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                grid[r][c] = values[k++];
            }
        }
        return 1;
    }
    return 0;
}

// Print the Sudoku grid
static void print_grid(void) {
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            printf("%d%c", grid[r][c], c == N - 1 ? '\n' : ' ');
        }
    }
}

int main(void) {
    printf("Enter 81 numbers for Sudoku grid, or press Enter for default grid:\n");
    
    // Load grid
    if (!try_load_from_stdin()) {
        load_default_grid();
        fprintf(stderr, "Using default grid.\n");
    }
    
    printf("\nSudoku grid:\n");
    print_grid();
    printf("\n");
    
    pthread_t threads[TOTAL_THREADS];
    int thread_count = 0;
    
    // Initialize results
    memset(results, 0, sizeof(results));
    
    // Create row validation threads (0-8)
    for (int r = 0; r < N; r++) {
        thread_params* p = malloc(sizeof(thread_params));
        p->start_row = r;
        p->start_col = 0;
        p->type = TYPE_ROW;
        p->index = thread_count;
        pthread_create(&threads[thread_count++], NULL, worker, p);
    }
    
    // Create column validation threads (9-17)
    for (int c = 0; c < N; c++) {
        thread_params* p = malloc(sizeof(thread_params));
        p->start_row = 0;
        p->start_col = c;
        p->type = TYPE_COLUMN;
        p->index = thread_count;
        pthread_create(&threads[thread_count++], NULL, worker, p);
    }
    
    // Create subgrid validation threads (18-26)
    for (int gr = 0; gr < 3; gr++) {
        for (int gc = 0; gc < 3; gc++) {
            thread_params* p = malloc(sizeof(thread_params));
            p->start_row = gr * 3;
            p->start_col = gc * 3;
            p->type = TYPE_SUBGRID;
            p->index = thread_count;
            pthread_create(&threads[thread_count++], NULL, worker, p);
        }
    }
    
    // Wait for all threads
    for (int i = 0; i < TOTAL_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Check results
    int all_valid = 1;
    for (int i = 0; i < TOTAL_THREADS; i++) {
        if (results[i] != 1) {
            all_valid = 0;
            break;
        }
    }
    
    // Print results
    if (!all_valid) {
        printf("INVALID Sudoku solution.\nDetails:\n");
        for (int i = 0; i < 9; i++) {
            if (!results[i]) printf("  Row %d invalid\n", i);
        }
        for (int i = 9; i < 18; i++) {
            if (!results[i]) printf("  Column %d invalid\n", i - 9);
        }
        for (int i = 18; i < 27; i++) {
            if (!results[i]) {
                int gr = (i - 18) / 3;
                int gc = (i - 18) % 3;
                printf("  Subgrid (%d,%d) invalid\n", gr, gc);
            }
        }
    } else {
        printf("VALID Sudoku solution.\n");
    }
    
    return all_valid ? 0 : 2;
}
