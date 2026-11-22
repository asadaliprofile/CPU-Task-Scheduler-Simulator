#ifndef PROCESS_H
#define PROCESS_H

#include <string>

struct Process {
    int id;
    std::string name;

    int arrivalTime;
    int burstTime;
    int priority;

    // Simulation results
    int startTime = -1;
    int completionTime = -1;
    int turnaroundTime = 0;
    int waitingTime = 0;
    int responseTime = 0;
};

#endif
