
#include "algorithms/Priority.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

using namespace std;

namespace {

ScheduleResult runPriority(
    const vector<Process>& input,
    bool preemptive
) {
    ScheduleResult result;

    if (input.empty())
        return result;

    vector<Process> processes = input;
    const size_t n = processes.size();

    unordered_set<int> ids;

    // Validate input and clear previous results.
    for (auto& p : processes) {
        if (p.arrivalTime < 0) {
            throw invalid_argument(
                "Arrival time cannot be negative."
            );
        }

        if (p.burstTime <= 0) {
            throw invalid_argument(
                "Burst time must be positive."
            );
        }

        if (!ids.insert(p.id).second) {
            throw invalid_argument(
                "Process IDs must be unique."
            );
        }

        p.startTime = -1;
        p.completionTime = -1;
        p.turnaroundTime = 0;
        p.waitingTime = 0;
        p.responseTime = 0;
    }

    vector<int> remaining(n);

    for (size_t i = 0; i < n; ++i) {
        remaining[i] = processes[i].burstTime;
    }

    long long currentTime = 0;
    long long busyTime = 0;

    size_t finished = 0;
    int lastRunning = -1;

    // Combine consecutive slices of the same process.
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

    while (finished < n) {
        int selected = -1;

        // Find the highest-priority ready process.
        for (size_t i = 0; i < n; ++i) {
            if (remaining[i] == 0 ||
                processes[i].arrivalTime > currentTime) {
                continue;
            }

            if (selected == -1) {
                selected = static_cast<int>(i);
                continue;
            }

            const Process& candidate = processes[i];
            const Process& best = processes[selected];

            if (candidate.priority < best.priority) {
                selected = static_cast<int>(i);
            }
            else if (candidate.priority == best.priority) {

                // In preemptive mode, keep the current
                // process when priorities are equal.
                if (preemptive &&
                    static_cast<int>(i) == lastRunning) {

                    selected = static_cast<int>(i);
                }
                else if (!preemptive ||
                         selected != lastRunning) {

                    // Otherwise prefer earlier arrival.
                    // Input order breaks complete ties.
                    if (candidate.arrivalTime <
                        best.arrivalTime) {

                        selected = static_cast<int>(i);
                    }
                }
            }
        }

        // No process is ready: advance to next arrival.
        if (selected == -1) {
            long long nextArrival =
                numeric_limits<long long>::max();

            for (size_t i = 0; i < n; ++i) {
                if (remaining[i] > 0 &&
                    processes[i].arrivalTime > currentTime) {

                    nextArrival = min(
                        nextArrival,
                        static_cast<long long>(
                            processes[i].arrivalTime
                        )
                    );
                }
            }

            if (nextArrival ==
                numeric_limits<long long>::max()) {

                throw logic_error(
                    "Invalid priority scheduler state."
                );
            }

            addSlice(
                "IDLE",
                static_cast<int>(currentTime),
                static_cast<int>(nextArrival)
            );

            currentTime = nextArrival;
            continue;
        }

        Process& p = processes[selected];

        if (p.startTime == -1) {
            p.startTime = static_cast<int>(currentTime);
        }

        // Non-preemptive: execute the entire remaining
        // burst without interruption.
        long long duration = remaining[selected];

        if (preemptive) {

            // Preemptive: reconsider scheduling when
            // the next process arrives.
            long long nextArrival =
                numeric_limits<long long>::max();

            for (size_t i = 0; i < n; ++i) {
                if (remaining[i] > 0 &&
                    processes[i].arrivalTime > currentTime) {

                    nextArrival = min(
                        nextArrival,
                        static_cast<long long>(
                            processes[i].arrivalTime
                        )
                    );
                }
            }

            if (nextArrival !=
                numeric_limits<long long>::max()) {

                duration = min(
                    duration,
                    nextArrival - currentTime
                );
            }
        }

        const long long end = currentTime + duration;

        if (end > numeric_limits<int>::max()) {
            throw overflow_error(
                "Simulation time exceeds supported range."
            );
        }

        // Count process-to-process changes.
        // Initial dispatch is excluded.
        if (lastRunning != -1 &&
            lastRunning != selected) {

            ++result.contextSwitches;
        }

        addSlice(
            p.name,
            static_cast<int>(currentTime),
            static_cast<int>(end)
        );

        remaining[selected] -= static_cast<int>(duration);
        busyTime += duration;

        currentTime = end;
        lastRunning = selected;

        if (remaining[selected] == 0) {
            p.completionTime = static_cast<int>(currentTime);

            p.turnaroundTime =
                p.completionTime - p.arrivalTime;

            p.waitingTime =
                p.turnaroundTime - p.burstTime;

            p.responseTime =
                p.startTime - p.arrivalTime;

            ++finished;
        }
    }

    long long totalWaiting = 0;
    long long totalTurnaround = 0;
    long long totalResponse = 0;

    for (const auto& p : processes) {
        totalWaiting += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
        totalResponse += p.responseTime;
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

    result.processes = std::move(processes);

    return result;
}

} // namespace

ScheduleResult schedulePriorityNonPreemptive(
    const vector<Process>& processes
) {
    return runPriority(processes, false);
}

ScheduleResult schedulePriorityPreemptive(
    const vector<Process>& processes
) {
    return runPriority(processes, true);
}
