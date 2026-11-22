
#include "algorithms/SRTF.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

using namespace std;

ScheduleResult scheduleSRTF(
    const vector<Process>& inputProcesses
) {
    ScheduleResult result;

    if (inputProcesses.empty())
        return result;

    // Copy input so the original workload is unchanged.
    vector<Process> processes = inputProcesses;

    const size_t n = processes.size();

    unordered_set<int> ids;

    for (auto& process : processes) {
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

        // Clear any results from an earlier simulation.
        process.startTime = -1;
        process.completionTime = -1;
        process.turnaroundTime = 0;
        process.waitingTime = 0;
        process.responseTime = 0;
    }

    // Track how much CPU time each process still needs.
    vector<int> remainingTime(n);

    for (size_t i = 0; i < n; ++i) {
        remainingTime[i] = processes[i].burstTime;
    }

    long long currentTime = 0;
    long long busyTime = 0;

    size_t finishedCount = 0;

    // Index of the most recently executed process.
    int lastRunning = -1;

    // Add a CPU execution slice to the timeline.
    // Merge adjacent slices of the same process.
    auto addSlice = [&](const string& name,
                        int start,
                        int end) {
        if (start == end)
            return;

        if (!result.timeline.empty() &&
            result.timeline.back().processName == name &&
            result.timeline.back().end == start) {

            result.timeline.back().end = end;
        }
        else {
            result.timeline.push_back({
                name, start, end
            });
        }
    };

    while (finishedCount < n) {
        int selected = -1;

        // Select the arrived process with the
        // shortest remaining execution time.
        for (size_t i = 0; i < n; ++i) {
            if (remainingTime[i] == 0 ||
                processes[i].arrivalTime > currentTime) {
                continue;
            }

            if (selected == -1) {
                selected = static_cast<int>(i);
                continue;
            }

            const int best = selected;

            if (remainingTime[i] < remainingTime[best]) {
                selected = static_cast<int>(i);
            }
            else if (remainingTime[i] == remainingTime[best]) {

                // Continue the currently running process
                // when remaining times are equal.
                if (static_cast<int>(i) == lastRunning) {
                    selected = static_cast<int>(i);
                }
                else if (best != lastRunning) {
                    // Otherwise prefer earlier arrival.
                    // Input order resolves complete ties.
                    if (processes[i].arrivalTime <
                        processes[best].arrivalTime) {
                        selected = static_cast<int>(i);
                    }
                }
            }
        }

        // If no process is ready, advance to
        // the next arrival without wasting iterations.
        if (selected == -1) {
            int nextArrival = numeric_limits<int>::max();

            for (size_t i = 0; i < n; ++i) {
                if (remainingTime[i] > 0 &&
                    processes[i].arrivalTime < nextArrival) {

                    nextArrival = processes[i].arrivalTime;
                }
            }

            if (nextArrival <= currentTime) {
                throw logic_error(
                    "Invalid scheduler state."
                );
            }

            addSlice(
                "IDLE",
                static_cast<int>(currentTime),
                nextArrival
            );

            currentTime = nextArrival;
            continue;
        }

        Process& process = processes[selected];

        // Record the first time this process gets CPU time.
        if (process.startTime == -1) {
            process.startTime =
                static_cast<int>(currentTime);
        }

        // Determine when another unfinished process
        // will arrive and force a scheduling decision.
        long long nextArrival =
            numeric_limits<long long>::max();

        for (size_t i = 0; i < n; ++i) {
            if (remainingTime[i] > 0 &&
                processes[i].arrivalTime > currentTime) {

                nextArrival = min(
                    nextArrival,
                    static_cast<long long>(
                        processes[i].arrivalTime
                    )
                );
            }
        }

        // Run until completion or the next arrival.
        long long duration = remainingTime[selected];

        if (nextArrival !=
            numeric_limits<long long>::max()) {

            duration = min(
                duration,
                nextArrival - currentTime
            );
        }

        const long long finish = currentTime + duration;

        if (finish > numeric_limits<int>::max()) {
            throw overflow_error(
                "Simulation time exceeds supported range."
            );
        }

        // Count actual switches between different processes.
        // Initial dispatch and idle intervals are excluded.
        if (lastRunning != -1 &&
            lastRunning != selected) {

            ++result.contextSwitches;
        }

        addSlice(
            process.name,
            static_cast<int>(currentTime),
            static_cast<int>(finish)
        );

        remainingTime[selected] -=
            static_cast<int>(duration);

        busyTime += duration;
        currentTime = finish;
        lastRunning = selected;

        // A process may execute in several slices,
        // but completion is recorded only once.
        if (remainingTime[selected] == 0) {
            process.completionTime =
                static_cast<int>(currentTime);

            process.turnaroundTime =
                process.completionTime -
                process.arrivalTime;

            process.waitingTime =
                process.turnaroundTime -
                process.burstTime;

            process.responseTime =
                process.startTime -
                process.arrivalTime;

            ++finishedCount;
        }
    }

    long long totalWaiting = 0;
    long long totalTurnaround = 0;
    long long totalResponse = 0;

    for (const auto& process : processes) {
        totalWaiting += process.waitingTime;
        totalTurnaround += process.turnaroundTime;
        totalResponse += process.responseTime;
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

        result.throughput =
            count / static_cast<double>(currentTime);
    }

    // Return processes in original input order.
    result.processes = std::move(processes);

    return result;
}
