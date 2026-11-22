
#include "algorithms/PriorityAging.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

namespace {

ScheduleResult runAging(
    const vector<Process>& input,
    int agingInterval,
    bool preemptive
) {
    if (agingInterval <= 0) {
        throw invalid_argument(
            "Aging interval must be positive."
        );
    }

    ScheduleResult result;

    if (input.empty())
        return result;

    vector<Process> processes = input;
    const size_t n = processes.size();

    unordered_set<int> ids;
    vector<int> remaining(n);

    for (size_t i = 0; i < n; ++i) {
        Process& p = processes[i];

        if (p.arrivalTime < 0)
            throw invalid_argument("Negative arrival time.");

        if (p.burstTime <= 0)
            throw invalid_argument("Invalid burst time.");

        if (!ids.insert(p.id).second)
            throw invalid_argument("Duplicate process ID.");

        p.startTime = -1;
        p.completionTime = -1;
        p.turnaroundTime = 0;
        p.waitingTime = 0;
        p.responseTime = 0;

        remaining[i] = p.burstTime;
    }

    long long now = 0;
    long long busyTime = 0;
    size_t finished = 0;
    int lastRunning = -1;

    const long long maxTime =
        numeric_limits<int>::max();

    // Total waiting time accumulated by process i.
    auto waitingAt = [&](size_t i, long long time) {
        const Process& p = processes[i];

        const long long executed =
            static_cast<long long>(p.burstTime) -
            remaining[i];

        return time - p.arrivalTime - executed;
    };

    // Higher priority means a smaller value.
    auto effectivePriority = [&](size_t i) {
        const long long waiting = waitingAt(i, now);

        return static_cast<long long>(
            processes[i].priority
        ) - waiting / agingInterval;
    };

    auto addSlice = [&](const string& name,
                        int start, int end) {
        if (start == end)
            return;

        if (!result.timeline.empty() &&
            result.timeline.back().processName == name &&
            result.timeline.back().end == start) {
            result.timeline.back().end = end;
        }
        else {
            result.timeline.push_back({name, start, end});
        }
    };

    while (finished < n) {
        int selected = -1;

        // Choose the highest effective-priority
        // process that has already arrived.
        for (size_t i = 0; i < n; ++i) {
            if (remaining[i] == 0 ||
                processes[i].arrivalTime > now)
                continue;

            if (selected == -1) {
                selected = static_cast<int>(i);
                continue;
            }

            const long long candidate = effectivePriority(i);
            const long long best = effectivePriority(selected);

            if (candidate < best) {
                selected = static_cast<int>(i);
            }
            else if (candidate == best) {
                // Keep the previously running process
                // when effective priorities are equal.
                if (preemptive &&
                    static_cast<int>(i) == lastRunning) {
                    selected = static_cast<int>(i);
                }
                else if (!preemptive ||
                         selected != lastRunning) {
                    if (processes[i].arrivalTime <
                        processes[selected].arrivalTime) {
                        selected = static_cast<int>(i);
                    }
                }
            }
        }

        // No ready process: advance to next arrival.
        if (selected == -1) {
            long long nextArrival =
                numeric_limits<long long>::max();

            for (size_t i = 0; i < n; ++i) {
                if (remaining[i] > 0 &&
                    processes[i].arrivalTime > now) {
                    nextArrival = min(
                        nextArrival,
                        static_cast<long long>(
                            processes[i].arrivalTime
                        )
                    );
                }
            }

            if (nextArrival ==
                numeric_limits<long long>::max())
                throw logic_error("Invalid scheduler state.");

            addSlice(
                "IDLE",
                static_cast<int>(now),
                static_cast<int>(nextArrival)
            );

            now = nextArrival;
            continue;
        }

        const size_t running =
            static_cast<size_t>(selected);

        Process& p = processes[running];

        if (p.startTime == -1)
            p.startTime = static_cast<int>(now);

        long long duration = remaining[running];

        if (preemptive) {
            long long nextEvent =
                numeric_limits<long long>::max();

            // Reconsider scheduling when a new
            // process arrives.
            for (size_t i = 0; i < n; ++i) {
                if (remaining[i] > 0 &&
                    processes[i].arrivalTime > now) {
                    nextEvent = min(
                        nextEvent,
                        static_cast<long long>(
                            processes[i].arrivalTime
                        )
                    );
                }
            }

            // The running process does not accumulate
            // waiting time, so its effective priority
            // remains constant during its CPU slice.
            const long long runningPriority =
                effectivePriority(running);

            // Find the earliest instant at which a
            // waiting process can obtain a strictly
            // higher effective priority.
            for (size_t i = 0; i < n; ++i) {
                if (i == running ||
                    remaining[i] == 0 ||
                    processes[i].arrivalTime > now)
                    continue;

                const long long requiredSteps =
                    static_cast<long long>(
                        processes[i].priority
                    ) - runningPriority + 1;

                if (requiredSteps <= 0)
                    continue;

                // Avoid arithmetic overflow and
                // events beyond our supported clock.
                if (requiredSteps >
                    maxTime / agingInterval)
                    continue;

                const long long thresholdWaiting =
                    requiredSteps * agingInterval;

                const long long wait = waitingAt(i, now);

                const long long delay =
                    thresholdWaiting - wait;

                if (delay > 0 &&
                    delay <= maxTime - now) {
                    nextEvent = min(
                        nextEvent, now + delay
                    );
                }
            }

            if (nextEvent !=
                numeric_limits<long long>::max()) {
                duration = min(duration, nextEvent - now);
            }
        }

        const long long end = now + duration;

        if (end > maxTime)
            throw overflow_error(
                "Simulation time exceeds supported range."
            );

        if (lastRunning != -1 &&
            lastRunning != selected) {
            ++result.contextSwitches;
        }

        addSlice(
            p.name,
            static_cast<int>(now),
            static_cast<int>(end)
        );

        remaining[running] -= static_cast<int>(duration);
        busyTime += duration;
        now = end;
        lastRunning = selected;

        if (remaining[running] == 0) {
            p.completionTime = static_cast<int>(now);

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

    result.totalTime = static_cast<int>(now);

    if (now > 0) {
        result.cpuUtilization =
            100.0 * static_cast<double>(busyTime) /
            static_cast<double>(now);

        result.throughput =
            count / static_cast<double>(now);
    }

    result.processes = std::move(processes);

    return result;
}

} // namespace

ScheduleResult schedulePriorityAgingNonPreemptive(
    const vector<Process>& processes,
    int agingInterval
) {
    return runAging(processes, agingInterval, false);
}

ScheduleResult schedulePriorityAgingPreemptive(
    const vector<Process>& processes,
    int agingInterval
) {
    return runAging(processes, agingInterval, true);
}
