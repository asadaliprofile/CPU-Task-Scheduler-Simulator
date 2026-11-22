
#ifndef CONSOLE_APP_H
#define CONSOLE_APP_H

#include "Process.h"
#include "SchedulingTypes.h"

#include <array>
#include <vector>

struct SchedulerConfig {
    int roundRobinQuantum = 2;
    int agingInterval = 2;
    std::array<int, 3> mlfqQuanta = {2, 4, 8};
    int boostInterval = 20;
};

// Run the interactive console application.
void runConsoleApp();

#endif
