
#include "algorithms/RoundRobin.h"

#include <algorithm>
#include <limits>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <unordered_set>
#include <vector>

using namespace std;

ScheduleResult scheduleRoundRobin(
    const vector<Process>& inputProcesses,
    int timeQuantum
) {
    if (timeQuantum <= 0) {
        throw invalid_argument(
            "Time quantum must be positive."
        );
    }

    ScheduleResult result;

    if (inputProcesses.empty())
        return result;

    // Preserve the original workload.
    vector<Process> processes = inputProcesses;

    const size_t n = processes.size();
    unordered_set<int> ids;

    // Validate processes and clear old simulation results.
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

        process.startTime = -1;
        process.completionTime = -1;
        process.turnaroundTime = 0;
        process.waitingTime = 0;
        process.responseTime = 0;
    }

    // Store the remaining CPU burst of each process.
    vector<int> remainingTime(n);

    for (size_t i = 0; i < n; ++i) {
        remainingTime[i] = processes[i].burstTime;
    }

    // Sort process indices by arrival time without
    // changing the order of the process vector.
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

    // FIFO ready queue stores process indices.
    queue<size_t> readyQueue;

    size_t nextArrivalIndex = 0;
    size_t finishedCount = 0;

    long long currentTime = 0;
    long long busyTime = 0;

    int lastRunning = -1;

    // Merge consecutive CPU slices of the same process.
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

    // Enqueue every process that has arrived.
    auto enqueueArrivals = [&]() {
        while (nextArrivalIndex < n) {
            size_t index = arrivalOrder[nextArrivalIndex];

            if (processes[index].arrivalTime > currentTime)
                break;

            readyQueue.push(index);
            ++nextArrivalIndex;
        }
    };

    while (finishedCount < n) {
        enqueueArrivals();

        // If there are no ready processes, skip to
        // the next arrival and record CPU idle time.
        if (readyQueue.empty()) {
            if (nextArrivalIndex >= n) {
                throw logic_error(
                    "Invalid Round Robin scheduler state."
                );
            }

            const int nextTime =
                processes[arrivalOrder[nextArrivalIndex]]
                    .arrivalTime;

            addSlice(
                "IDLE",
                static_cast<int>(currentTime),
                nextTime
            );

            currentTime = nextTime;
            continue;
        }

        // Pick the next process from the front.
        size_t selected = readyQueue.front();
        readyQueue.pop();

        Process& process = processes[selected];

        // Record its first CPU execution.
        if (process.startTime == -1) {
            process.startTime =
                static_cast<int>(currentTime);
        }

        // Run for one quantum or until completion.
        const int duration = min(
            timeQuantum,
            remainingTime[selected]
        );

        const long long finish =
            currentTime + duration;

        if (finish > numeric_limits<int>::max()) {
            throw overflow_error(
                "Simulation time exceeds supported range."
            );
        }

        // Count transitions between different processes.
        // The first dispatch is not counted.
        if (lastRunning != -1 &&
            lastRunning != static_cast<int>(selected)) {

            ++result.contextSwitches;
        }

        addSlice(
            process.name,
            static_cast<int>(currentTime),
            static_cast<int>(finish)
        );

        remainingTime[selected] -= duration;
        busyTime += duration;
        currentTime = finish;

        lastRunning = static_cast<int>(selected);

        // Arrivals during the time slice join the queue
        // BEFORE the interrupted process is requeued.
        enqueueArrivals();

        if (remainingTime[selected] == 0) {
            // The process has completed.
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
        else {
            // Still has work: place it at the back.
            readyQueue.push(selected);
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

    result.processes = std::move(processes);

    return result;
}
