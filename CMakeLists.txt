cmake_minimum_required(VERSION 3.16)

project(CPUTaskSchedulerSimulator
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reusable scheduling library

add_library(scheduler_core
    src/algorithms/FCFS.cpp
    src/algorithms/SJF.cpp
    src/algorithms/SRTF.cpp
    src/algorithms/RoundRobin.cpp
    src/algorithms/Priority.cpp
    src/algorithms/PriorityAging.cpp
    src/algorithms/MLFQ.cpp
    src/output/OutputFormatter.cpp
    src/cli/InputUtils.cpp
    src/cli/ConsoleApp.cpp
    src/io/WorkloadIO.cpp
    src/validation/ScheduleValidator.cpp
)


target_include_directories(scheduler_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Main application
add_executable(cpu_scheduler
    src/main.cpp
)

# Performance benchmarking executable
add_executable(scheduler_benchmark
    benchmarks/benchmark.cpp
)

target_link_libraries(scheduler_benchmark
    PRIVATE scheduler_core
)

target_link_libraries(cpu_scheduler
    PRIVATE scheduler_core
)

# Automated tests
enable_testing()

# FCFS tests
add_executable(scheduler_tests
    tests/test_fcfs.cpp
)

target_link_libraries(scheduler_tests
    PRIVATE scheduler_core
)

add_test(
    NAME FCFSUnitTests
    COMMAND scheduler_tests
)

# SJF tests
add_executable(sjf_tests
    tests/test_sjf.cpp
)

target_link_libraries(sjf_tests
    PRIVATE scheduler_core
)

add_test(
    NAME SJFUnitTests
    COMMAND sjf_tests
)

# SRTF tests
add_executable(srtf_tests
    tests/test_srtf.cpp
)

target_link_libraries(srtf_tests
    PRIVATE scheduler_core
)

add_test(
    NAME SRTFUnitTests
    COMMAND srtf_tests
)


# Round Robin tests
add_executable(round_robin_tests
    tests/test_round_robin.cpp
)

target_link_libraries(round_robin_tests
    PRIVATE scheduler_core
)

add_test(
    NAME RoundRobinUnitTests
    COMMAND round_robin_tests
)


# Priority scheduling tests
add_executable(priority_tests
    tests/test_priority.cpp
)

target_link_libraries(priority_tests
    PRIVATE scheduler_core
)

add_test(
    NAME PriorityUnitTests
    COMMAND priority_tests
)


# Priority aging tests
add_executable(priority_aging_tests
    tests/test_priority_aging.cpp
)

target_link_libraries(priority_aging_tests
    PRIVATE scheduler_core
)

add_test(
    NAME PriorityAgingUnitTests
    COMMAND priority_aging_tests
)

# MLFQ tests
add_executable(mlfq_tests
    tests/test_mlfq.cpp
)

target_link_libraries(mlfq_tests
    PRIVATE scheduler_core
)

add_test(
    NAME MLFQUnitTests
    COMMAND mlfq_tests
)


# Interactive input validation tests
add_executable(cli_input_tests
    tests/test_cli_input.cpp
)

target_link_libraries(cli_input_tests
    PRIVATE scheduler_core
)

add_test(
    NAME CLIInputTests
    COMMAND cli_input_tests
)


# CSV workload persistence tests
add_executable(workload_io_tests
    tests/test_workload_io.cpp
)

target_link_libraries(workload_io_tests
    PRIVATE scheduler_core
)

add_test(
    NAME WorkloadIOTests
    COMMAND workload_io_tests
)


# Randomized scheduling consistency tests
add_executable(consistency_tests
    tests/test_consistency.cpp
)

target_link_libraries(consistency_tests
    PRIVATE scheduler_core
)

add_test(
    NAME SchedulingConsistencyTests
    COMMAND consistency_tests
)


# Scheduler boundary and reliability tests
add_executable(scheduler_boundary_tests
    tests/test_scheduler_boundaries.cpp
)

target_link_libraries(scheduler_boundary_tests
    PRIVATE scheduler_core
)

add_test(
    NAME SchedulerBoundaryTests
    COMMAND scheduler_boundary_tests
)

# Compiler warnings
if(MSVC)
    foreach(target
        scheduler_core
        cpu_scheduler
        scheduler_tests
        sjf_tests
        srtf_tests
        round_robin_tests
        priority_tests
        priority_aging_tests
        mlfq_tests
        cli_input_tests
        workload_io_tests
        scheduler_benchmark
    )
        target_compile_options(${target} PRIVATE /W4)
    endforeach()
else()
    foreach(target
        scheduler_core
        cpu_scheduler
        scheduler_tests
        sjf_tests
        srtf_tests
        round_robin_tests
        priority_tests
        priority_aging_tests
        mlfq_tests
        cli_input_tests
        workload_io_tests
        scheduler_benchmark
    )
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endforeach()
endif()
