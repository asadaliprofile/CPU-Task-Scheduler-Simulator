
#include "algorithms/FCFS.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>

using namespace std;

ScheduleResult scheduleFCFS(
    const vector<Process>& inputProcesses
) {
    ScheduleResult result;

    if (inputProcesses.empty())
        return result;

    // Work on a copy to preserve the original workload.
    result.processes = inputProcesses;

    unordered_set<int> ids;

    for (const auto& process : result.processes) {
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

    // FCFS: earliest arrival first.
    // Preserve input order for equal arrival times.
    stable_sort(
        result.processes.begin(),
        result.processes.end(),
        [](const Process& a, const Process& b) {
            return a.arrivalTime < b.arrivalTime;
        }
    );

    long long currentTime = 0;
    long long busyTime = 0;

    long long totalWaiting = 0;
    long long totalTurnaround = 0;
    long long totalResponse = 0;

    for (auto& process : result.processes) {

        // If no process has arrived, the CPU is idle.
        if (currentTime < process.arrivalTime) {
            result.timeline.push_back({
                "IDLE",
                static_cast<int>(currentTime),
                process.arrivalTime
            });

            currentTime = process.arrivalTime;
        }

        long long finish =
            currentTime + process.burstTime;

        if (finish > numeric_limits<int>::max()) {
            throw overflow_error(
                "Simulation time exceeds supported range."
            );
        }

        process.startTime =
            static_cast<int>(currentTime);

        process.completionTime =
            static_cast<int>(finish);

        process.turnaroundTime =
            process.completionTime -
            process.arrivalTime;

        process.waitingTime =
            process.turnaroundTime -
            process.burstTime;

        process.responseTime =
            process.startTime -
            process.arrivalTime;

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
    }

    const double count =
        static_cast<double>(result.processes.size());

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

    // Count changes from one process to another.
    // Initial dispatch and idle periods are excluded.
    result.contextSwitches =
        static_cast<int>(result.processes.size()) - 1;

    return result;
}
