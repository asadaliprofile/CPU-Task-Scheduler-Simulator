
#ifndef SCHEDULING_TYPES_H
#define SCHEDULING_TYPES_H

#include "Process.h"

#include <string>
#include <vector>

// One continuous interval of CPU activity.
// "IDLE" represents time when the CPU is not executing.
struct ExecutionSlice {
    std::string processName;
    int start;
    int end;
};

// Common result format for every scheduling algorithm.
struct ScheduleResult {
    std::vector<Process> processes;
    std::vector<ExecutionSlice> timeline;

    double averageWaitingTime = 0.0;
    double averageTurnaroundTime = 0.0;
    double averageResponseTime = 0.0;

    double cpuUtilization = 0.0;
    double throughput = 0.0;

    int totalTime = 0;
    int contextSwitches = 0;
};

#endif
