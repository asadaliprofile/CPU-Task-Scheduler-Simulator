
# CPU Task Scheduler Simulator

A modular **C++17 CPU scheduling simulator** implementing nine scheduling
configurations, workload management, performance analysis, randomized
correctness testing, and CSV persistence.

The project demonstrates operating-system scheduling concepts alongside
modern C++ software engineering practices, including modular architecture,
automated testing, input validation, and reproducible benchmarking.

## Features

- Seven scheduling families with nine configurations
- Non-preemptive and preemptive scheduling
- Configurable Round Robin time quantum
- Priority scheduling with optional aging
- Three-level Multi-Level Feedback Queue (MLFQ)
- Interactive command-line interface
- Process creation, removal, and workload management
- CSV workload import and export with validation
- Execution timelines and Gantt-style output
- Per-process waiting, turnaround, and response times
- CPU utilization, throughput, and context-switch statistics
- Side-by-side algorithm comparisons
- Randomized scheduling consistency validation
- Automated regression tests using CTest
- Reproducible Release-mode performance benchmarks

## Scheduling Algorithms

| Algorithm | Type | Description |
|-----------|------|-------------|
| FCFS | Non-preemptive | Executes processes in arrival order |
| SJF | Non-preemptive | Selects the shortest available CPU burst |
| SRTF | Preemptive | Favors the process with the shortest remaining time |
| Round Robin | Preemptive | Rotates processes using a configurable time quantum |
| Priority | Non-preemptive | Selects the highest-priority ready process |
| Priority | Preemptive | Allows higher-priority processes to interrupt execution |
| Priority Aging | Non-preemptive | Adjusts effective priorities to reduce starvation |
| Priority Aging | Preemptive | Combines priority adjustment with preemption |
| MLFQ | Preemptive | Uses three feedback queues with configurable quanta and priority boosts |

For priority-based scheduling, a **lower numerical value indicates
higher priority**.

MLFQ supports configurable queue quanta and a periodic priority-boost
interval.

## Interactive Command-Line Interface

Launch the simulator to access the main menu:

```text
========================================
       CPU TASK SCHEDULER SIMULATOR
========================================

1. View processes
2. Add process
3. Remove process
4. Run a scheduling algorithm
5. Compare all algorithms
6. Configure scheduling parameters
7. Load sample workload
8. Save workload to CSV
9. Load workload from CSV
0. Exit
```

The CLI supports custom workloads, scheduling parameter configuration,
individual algorithm execution, and comparisons across all nine
configurations.

### Example Workload

| Process | Arrival | Burst | Priority |
|---------|---------|-------|----------|
| P1 | 0 | 8 | 2 |
| P2 | 2 | 3 | 1 |
| P3 | 3 | 1 | 3 |

These processes can be simulated using different scheduling policies
to compare their execution order and performance metrics.

## Performance Metrics

The simulator reports the following process-level measurements:

**Turnaround time**

```text
Turnaround Time = Completion Time - Arrival Time
```

**Waiting time**

```text
Waiting Time = Turnaround Time - Burst Time
```

**Response time**

```text
Response Time = First Execution Time - Arrival Time
```

It also reports aggregate measurements:

- Average waiting time
- Average turnaround time
- Average response time
- CPU utilization
- Throughput
- Context-switch count
- Total simulated execution time

CPU utilization is calculated as:

```text
CPU Utilization (%) = 100 * Busy Time / Total Time
```

Throughput is calculated as:

```text
Throughput = Completed Processes / Total Time
```

The simulator uses a single-core CPU model.

## CSV Workload Persistence

Workloads can be exported to and imported from CSV files.

Example:

```csv
id,name,arrivalTime,burstTime,priority
1,P1,0,8,2
2,P2,2,3,1
3,P3,3,1,3
```

The application validates process identifiers, names, arrival times,
burst times, priorities, duplicate entries, and CSV structure.

**Safe loading:** A workload is replaced only after the entire input
file has been parsed and validated successfully.

**Safer saving:** The application writes a temporary file before
replacing the destination, using a backup during replacement.
This improves recovery from ordinary I/O failures but does not
guarantee crash-atomic persistence.

CSV files used through the interactive menu are stored in the
`data/` directory.

## Project Architecture

```text
CPU-Task-Scheduler-Simulator/
|
|-- include/
|   |-- Process.h
|   |-- SchedulingTypes.h
|   |-- algorithms/
|   |-- cli/
|   |-- io/
|   |-- output/
|   `-- validation/
|
|-- src/
|   |-- main.cpp
|   |-- algorithms/
|   |-- cli/
|   |-- io/
|   |-- output/
|   `-- validation/
|
|-- tests/
|   |-- test_fcfs.cpp
|   |-- test_sjf.cpp
|   |-- test_srtf.cpp
|   |-- test_round_robin.cpp
|   |-- test_priority.cpp
|   |-- test_priority_aging.cpp
|   |-- test_mlfq.cpp
|   |-- test_cli_input.cpp
|   |-- test_workload_io.cpp
|   |-- test_consistency.cpp
|   `-- test_scheduler_boundaries.cpp
|
|-- benchmarks/
|   `-- benchmark.cpp
|
|-- data/
|   |-- sample_workload.csv
|   `-- benchmark_results.csv
|
|-- CMakeLists.txt
|-- .gitignore
`-- README.md
```

### Module Responsibilities

| Module | Responsibility |
|--------|----------------|
| Process | Defines process input and scheduling metrics |
| SchedulingTypes | Common scheduling results and execution slices |
| algorithms | Implements the scheduling policies |
| cli | Interactive console and input handling |
| io | CSV persistence and workload validation |
| output | Process tables, timelines, and comparisons |
| validation | Checks scheduling result invariants |
| tests | Unit, regression, and randomized testing |
| benchmarks | Measures actual scheduling computation time |

The algorithms share common input and output types, keeping the
simulation logic separate from command-line interaction and formatting.

## Requirements

- C++17-compatible compiler
- CMake
- C++ standard library with filesystem support
- A compatible build tool, such as MinGW Make or Ninja

The project has been developed using Windows, VS Code, CMake,
and MinGW-w64.

## Building the Project

From the project root, configure an optimized Release build.

### Windows PowerShell (MinGW)

```powershell
cmake -S . -B build-release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
```

If CMake is not on `PATH`, invoke `cmake.exe` using its full path.

If CMake cannot locate the compiler or build tool, configure the
appropriate MinGW C++ compiler and `mingw32-make` executable.

### Run the Simulator

From the project root:

```powershell
.\build-release\cpu_scheduler.exe
```

Use the menu to manage processes, run algorithms, and compare results.

## Automated Testing

The project includes **11 CTest suites**, covering:

- FCFS
- SJF
- SRTF
- Round Robin
- Priority Scheduling
- Priority Aging
- MLFQ
- CLI input validation
- CSV workload persistence
- Randomized scheduling consistency
- Scheduler boundary conditions

Run all tests:

```powershell
ctest --test-dir build-release --output-on-failure
```

### Randomized Consistency Testing

The consistency suite uses a fixed random seed to generate
reproducible workloads.

It evaluates 200 random workloads across all nine scheduling
configurations, resulting in 1,800 randomized scheduling
validations.

The validator checks important invariants, including:

- Every process receives exactly its requested CPU burst time
- Processes do not execute before arriving
- Execution intervals do not overlap
- Timeline intervals are contiguous
- Reported response, waiting, and turnaround times are consistent
- Aggregate utilization and throughput match the timeline
- Context-switch accounting follows the simulator's convention

The project also includes targeted tests for MLFQ boundary
conditions, priority aging, and other scheduling edge cases.

These tests establish consistency for tested workloads; they are
not a formal proof of optimality or correctness for every input.

## Performance Benchmarking

A standalone benchmark evaluates all nine scheduling configurations
using workloads containing:

- 100 processes
- 250 processes
- 500 processes
- 1,000 processes

Each configuration is measured five times after a warm-up run.

The benchmark reports:

- Average computation time
- Median computation time
- Minimum computation time
- Maximum computation time

### Run the Benchmark

```powershell
.\build-release\scheduler_benchmark.exe
```

Results are exported to:

```text
data/benchmark_results.csv
```

**Important:** Benchmark time measures how long the C++ implementation
takes to calculate a schedule. It is not the simulated CPU execution
time of the workload.

Measurements depend on the processor, compiler, optimization settings,
workload, and execution environment.

The CSV contains the results measured on the machine running the
benchmark. They should not be interpreted as universal algorithm
performance rankings.

## Engineering Highlights

This project demonstrates:

- C++17 programming with STL containers and modern language features
- Process scheduling and operating-system concepts
- Preemptive and non-preemptive scheduling logic
- Modular software design with reusable interfaces
- Robust input parsing and defensive validation
- CSV parsing, escaping, and safer persistence
- Automated unit and regression testing
- Reproducible randomized testing
- Performance measurement using `std::chrono`
- Build management with CMake
- Practical debugging of timing and scheduling edge cases

## Scope and Limitations

This application is an **educational discrete-time CPU scheduling
simulator**, not an operating-system kernel scheduler.

It does not schedule real operating-system threads or processes.

The simulator models a single CPU and does not include real
context-switch overhead, I/O blocking, multiprocessor scheduling,
or hardware-level effects.

Large workloads may also be constrained by integer time limits
and the computational characteristics of individual implementations.

## Future Improvements

Possible extensions include:

- Workload visualization
- Interactive graphical Gantt charts
- Additional scheduling policies
- Larger-scale workload optimization
- Explicit simulated context-switch costs
- CPU and I/O burst modeling
- Multi-core scheduling simulation
- Cross-platform continuous integration


## 🤝 Connect With Me

I'm always interested in learning new technologies, discussing software engineering, and connecting with fellow developers.

Feel free to explore my projects or reach out!

- **GitHub:** [asadaliprofile](https://github.com/asadaliprofile)
- **LinkedIn:** [asadaliprofile](https://www.linkedin.com/in/asadaliprofile/)

If you found this project interesting, feel free to ⭐ the repository!

---