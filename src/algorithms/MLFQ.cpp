
#include "algorithms/MLFQ.h"

#include <algorithm>
#include <array>
#include <deque>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include <vector>

using namespace std;

ScheduleResult scheduleMLFQ(
    const vector<Process>& input,
    const array<int, 3>& timeQuanta,
    int boostInterval
) {
    for (int quantum : timeQuanta) {
        if (quantum <= 0) {
            throw invalid_argument(
                "All time quanta must be positive."
            );
        }
    }

    if (boostInterval <= 0) {
        throw invalid_argument(
            "Boost interval must be positive."
        );
    }

    ScheduleResult result;

    if (input.empty())
        return result;

    result.processes = input;
    auto& processes = result.processes;

    const size_t n = processes.size();
    const long long maxTime = numeric_limits<int>::max();

    unordered_set<int> ids;

    vector<int> remaining(n);
    vector<int> levels(n, 0);
    vector<int> quantumLeft(n, timeQuanta[0]);

    for (size_t i = 0; i < n; ++i) {
        Process& p = processes[i];

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

        remaining[i] = p.burstTime;
    }

    // Sort process indices by arrival time.
    // Stable sorting preserves input order on ties.
    vector<size_t> arrivalOrder(n);
    iota(arrivalOrder.begin(), arrivalOrder.end(), 0);

    stable_sort(
        arrivalOrder.begin(),
        arrivalOrder.end(),
        [&](size_t a, size_t b) {
            return processes[a].arrivalTime <
                   processes[b].arrivalTime;
        }
    );

    // Three FIFO ready queues.
    array<deque<size_t>, 3> ready;

    size_t nextArrivalIndex = 0;
    size_t finished = 0;

    long long now = 0;
    long long busyTime = 0;
    long long nextBoost = boostInterval;

    // -1 means no process is currently running.
    int running = -1;
    int lastRunning = -1;

    // Store the most recently recorded process index.
    // -2 represents an idle slice.
    int lastSliceOwner = -2;

    auto addSlice = [&](int owner,
                        long long start,
                        long long end) {
        if (start == end)
            return;

        const string name = owner == -1
            ? "IDLE"
            : processes[static_cast<size_t>(owner)].name;

        if (!result.timeline.empty() &&
            lastSliceOwner == owner &&
            result.timeline.back().end == start) {

            result.timeline.back().end =
                static_cast<int>(end);
        }
        else {
            result.timeline.push_back({
                name,
                static_cast<int>(start),
                static_cast<int>(end)
            });
        }

        lastSliceOwner = owner;
    };

    // Newly arrived processes always enter Q0.
    auto enqueueArrivals = [&]() {
        while (nextArrivalIndex < n) {
            const size_t index =
                arrivalOrder[nextArrivalIndex];

            if (processes[index].arrivalTime > now)
                break;

            levels[index] = 0;
            quantumLeft[index] = timeQuanta[0];
            ready[0].push_back(index);

            ++nextArrivalIndex;
        }
    };

    auto highestReadyQueue = [&]() -> int {
        for (int q = 0; q < 3; ++q) {
            if (!ready[q].empty())
                return q;
        }

        return -1;
    };

    while (finished < n) {
        enqueueArrivals();

        // Periodic global priority boost.
        if (now >= nextBoost) {
            deque<size_t> boosted;

            // Preserve the current ready-queue order,
            // highest queue first.
            for (int q = 0; q < 3; ++q) {
                while (!ready[q].empty()) {
                    boosted.push_back(ready[q].front());
                    ready[q].pop_front();
                }
            }

            // The previously running process goes
            // behind processes already waiting.
            if (running != -1) {
                boosted.push_back(
                    static_cast<size_t>(running)
                );
                running = -1;
            }

            for (size_t index : boosted) {
                levels[index] = 0;
                quantumLeft[index] = timeQuanta[0];
                ready[0].push_back(index);
            }

            nextBoost =
                (now / boostInterval + 1) * boostInterval;
        }

        // A higher-priority ready process interrupts
        // a lower-priority running process.
        if (running != -1) {
            const int highest = highestReadyQueue();

            if (highest != -1 &&
                highest < levels[running]) {

                // Preserve unused quantum on preemption.
                ready[levels[running]].push_front(
                    static_cast<size_t>(running)
                );

                running = -1;
            }
        }

        // Dispatch the next ready process.
        if (running == -1) {
            const int queueLevel = highestReadyQueue();

            if (queueLevel == -1) {
                // CPU has no work: jump to next arrival.
                if (nextArrivalIndex >= n) {
                    throw logic_error(
                        "Invalid MLFQ scheduler state."
                    );
                }

                const long long nextArrival =
                    processes[arrivalOrder[nextArrivalIndex]]
                        .arrivalTime;

                addSlice(-1, now, nextArrival);
                now = nextArrival;

                // No unfinished process is waiting
                // while the CPU is completely idle.
                // Skip boosts in the idle interval.
                nextBoost =
                    (now / boostInterval + 1) *
                    boostInterval;

                continue;
            }

            const size_t selected =
                ready[queueLevel].front();

            ready[queueLevel].pop_front();
            running = static_cast<int>(selected);

            if (lastRunning != -1 &&
                lastRunning != running) {
                ++result.contextSwitches;
            }

            if (processes[selected].startTime == -1) {
                processes[selected].startTime =
                    static_cast<int>(now);
            }

            lastRunning = running;
        }

        const size_t index =
            static_cast<size_t>(running);

        // Run at most until the current process
        // completes or exhausts its remaining quantum.
        long long duration = min(
            static_cast<long long>(remaining[index]),
            static_cast<long long>(quantumLeft[index])
        );

        // Stop at the next new process arrival.
        if (nextArrivalIndex < n) {
            const long long arrival =
                processes[arrivalOrder[nextArrivalIndex]]
                    .arrivalTime;

            duration = min(duration, arrival - now);
        }

        // Stop at the next priority boost.
        duration = min(duration, nextBoost - now);

        if (duration <= 0) {
            throw logic_error(
                "Invalid MLFQ event duration."
            );
        }

        const long long end = now + duration;

        if (end > maxTime) {
            throw overflow_error(
                "Simulation time exceeds supported range."
            );
        }

        addSlice(running, now, end);

        remaining[index] -= static_cast<int>(duration);
        quantumLeft[index] -= static_cast<int>(duration);

        busyTime += duration;
        now = end;

        // Process has completed.
        if (remaining[index] == 0) {
            Process& p = processes[index];

            p.completionTime = static_cast<int>(now);
            p.turnaroundTime =
                p.completionTime - p.arrivalTime;

            p.waitingTime =
                p.turnaroundTime - p.burstTime;

            p.responseTime =
                p.startTime - p.arrivalTime;

            ++finished;
            running = -1;
        }
        // Full quantum used: demote one queue level.
        else if (quantumLeft[index] == 0) {
            levels[index] = min(levels[index] + 1, 2);
            quantumLeft[index] =
                timeQuanta[levels[index]];

            ready[levels[index]].push_back(index);
            running = -1;
        }

        // Otherwise, the process remains running
        // until the next event is handled.
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

    return result;
}
