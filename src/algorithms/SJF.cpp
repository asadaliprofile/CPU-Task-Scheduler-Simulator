
#include "algorithms/SJF.h"

#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

using namespace std;

ScheduleResult scheduleSJF(
    const vector<Process>& inputProcesses
) {
    ScheduleResult result;

    if (inputProcesses.empty())
        return result;

    // Preserve the original process input.
    vector<Process> processes = inputProcesses;

    const size_t n = processes.size();

    unordered_set<int> ids;

    // Validate the workload.
    for (const auto& process : processes) {
        if (process.arrivalTime < 0) {
            throw invalid_argument(
                "Arrival time cannot be negative."
            );
        }

        if (process.burstTime <= 0) {
            throw invalid_argument(
                "Burst time must be positive."
            );
        }

        if (!ids.insert(process.id).second) {
            throw invalid_argument(
                "Process IDs must be unique."
            );
        }
    }

    vector<bool> completed(n, false);

    long long currentTime = 0;
    long long busyTime = 0;

    long long totalWaiting = 0;
    long long totalTurnaround = 0;
    long long totalResponse = 0;

    size_t finishedCount = 0;

    while (finishedCount < n) {
        int selected = -1;

        // Find the shortest job that has arrived.
        for (size_t i = 0; i < n; ++i) {

            if (completed[i])
                continue;

            if (processes[i].arrivalTime > currentTime)
                continue;

            if (selected == -1) {
                selected = static_cast<int>(i);
                continue;
            }

            const Process& candidate = processes[i];
            const Process& best = processes[selected];

            // Prefer shorter bursts.
            if (candidate.burstTime < best.burstTime) {
                selected = static_cast<int>(i);
            }
            // Tie: prefer earlier arrival.
            else if (
                candidate.burstTime == best.burstTime &&
                candidate.arrivalTime < best.arrivalTime
            ) {
                selected = static_cast<int>(i);
            }

            // If burst and arrival are both equal,
            // preserve original input order.
        }

        // No process is available yet.
        if (selected == -1) {
            int nextArrival = numeric_limits<int>::max();

            for (size_t i = 0; i < n; ++i) {
                if (!completed[i] &&
                    processes[i].arrivalTime < nextArrival) {
                    nextArrival = processes[i].arrivalTime;
                }
            }

            result.timeline.push_back({
                "IDLE",
                static_cast<int>(currentTime),
                nextArrival
            });

            currentTime = nextArrival;
            continue;
        }

        Process& process = processes[selected];

        long long finish =
            currentTime + process.burstTime;

        if (finish > numeric_limits<int>::max()) {
            throw overflow_error(
                "Simulation time exceeds supported range."
            );
        }

        process.startTime = static_cast<int>(currentTime);
        process.completionTime = static_cast<int>(finish);

        process.turnaroundTime =
            process.completionTime - process.arrivalTime;

        process.waitingTime =
            process.turnaroundTime - process.burstTime;

        process.responseTime =
            process.startTime - process.arrivalTime;

        result.timeline.push_back({
            process.name,
            process.startTime,
            process.completionTime
        });

        totalWaiting += process.waitingTime;
        totalTurnaround += process.turnaroundTime;
        totalResponse += process.responseTime;

        busyTime += process.burstTime;
        currentTime = finish;

        completed[selected] = true;
        ++finishedCount;

        // Keep results in actual execution order.
        result.processes.push_back(process);
    }

    const double count = static_cast<double>(n);

    result.averageWaitingTime =
        static_cast<double>(totalWaiting) / count;

    result.averageTurnaroundTime =
        static_cast<double>(totalTurnaround) / count;

    result.averageResponseTime =
        static_cast<double>(totalResponse) / count;

    result.totalTime = static_cast<int>(currentTime);

    if (currentTime > 0) {
        result.cpuUtilization =
            100.0 * static_cast<double>(busyTime) /
            static_cast<double>(currentTime);

        result.throughput = count /
            static_cast<double>(currentTime);
    }

    // Count process-to-process transitions.
    // Do not count initial dispatch or idle periods.
    if (n > 1) {
        result.contextSwitches =
            static_cast<int>(n - 1);
    }

    return result;
}
