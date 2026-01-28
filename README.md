# OS Threading Algorithms

Three multithreaded programs demonstrating concurrent programming concepts in C using POSIX threads.

## Programs

### 1. Sudoku Validator
Validates 9x9 Sudoku solutions using 27 parallel threads (9 rows + 9 columns + 9 subgrids).

### 2. Round Robin CPU Scheduler
Simulates Round Robin scheduling algorithm with I/O operations and process management.

### 3. Boolean Circuit Evaluator
Evaluates Boolean logic circuits in parallel with memoization and thread synchronization.

## Features

- **POSIX Threads**: All programs use pthread library
- **Thread Synchronization**: Mutexes and condition variables
- **Concurrent Execution**: Parallel validation and computation
- **Real-world Simulations**: CPU scheduling and circuit evaluation

## Project Structure

```
os-threading-algorithms/
├── sudoku-validator/
│   └── sudoku_validator.c
├── cpu-scheduler/
│   ├── round_robin.c
│   ├── scheduler_utils.h
│   └── processinfo.txt
├── circuit-evaluator/
│   ├── circuit_evaluator.c
│   └── circuit.txt
└── README.md
```

## Compilation & Usage

### Sudoku Validator
```bash
cd sudoku-validator
gcc sudoku_validator.c -pthread -o sudoku
./sudoku

# Or provide input
echo "6 2 4 5 3 9 1 8 7 5 1 9 ..." | ./sudoku
```

### CPU Scheduler
```bash
cd cpu-scheduler
gcc round_robin.c -o scheduler
./scheduler
```

Format of `processinfo.txt`:
```
<num_processes> <time_quantum>
<process_id> <arrival_time> <cpu_burst1> <io_burst1> <cpu_burst2> ... -1
```

### Circuit Evaluator
```bash
cd circuit-evaluator
gcc circuit_evaluator.c -pthread -o circuit
./circuit circuit.txt
```

Format of `circuit.txt`:
```
<num_inputs>
<num_outputs>
<output_id1> <output_id2> ...
<num_gates>
<gate_id> <GATE_TYPE> <input1> [input2]
...
<input_values>
```

## Technical Details

### Sudoku Validator
- **Threads**: 27 (validates rows, columns, and 3x3 subgrids concurrently)
- **Synchronization**: Thread join for result aggregation
- **Input**: 81 integers or default valid grid

### CPU Scheduler
- **Algorithm**: Round Robin with time quantum
- **Data Structures**: Queue (ready), Min-Heap (waiting for I/O)
- **Metrics**: Turnaround time, waiting time, completion time

### Circuit Evaluator
- **Techniques**: Memoization, parallel evaluation
- **Gates**: NOT, AND, OR
- **Synchronization**: Mutex and condition variables to prevent duplicate computation

## Requirements

- GCC compiler
- POSIX-compliant OS (Linux/Unix)
- pthread library

## Sample Outputs

**Sudoku Validator:**
```
Sudoku grid:
6 2 4 5 3 9 1 8 7
...
VALID Sudoku solution.
```

**CPU Scheduler:**
```
0 : Starting
0 : Process P1 joins ready queue upon arrival
0 : Process P1 is scheduled to run for time 4
...
Average Turnaround Time = 45.3
Average Waiting Time = 12.7
```

**Circuit Evaluator:**
```
Thread 1 starting evaluation for output x8
Thread 2 starting evaluation for output x9
...
Output x8 = 0
Output x9 = 1
```

## Learning Objectives

- Thread creation and management
- Mutex locks and condition variables
- Producer-consumer patterns
- Concurrent data structure access
- Synchronization mechanisms
- Race condition prevention

## License

Educational use - OS/Systems Programming course project
