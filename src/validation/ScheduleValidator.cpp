
#include "validation/ScheduleValidator.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace {

void require(
    bool condition,
    const string& message
) {
    if (!condition) {
        throw runtime_error(
            "Schedule validation failed: " + message
        );
    }
}

bool approximatelyEqual(double a, double b) {
    return isfinite(a) &&
           isfinite(b) &&
           fabs(a - b) < 1e-7;
}

} // namespace

void validateSchedule(
    const vector<Process>& workload,
    const ScheduleResult& result
) {
    // Empty workload should produce an empty result.
    if (workload.empty()) {
        require(
            result.processes.empty(),
            "Unexpected processes in empty workload."
        );

        require(
            result.timeline.empty(),
            "Unexpected timeline for empty workload."
        );

        require(
            result.totalTime == 0 &&
            result.contextSwitches == 0,
            "Invalid empty-workload statistics."
        );

        require(
            approximatelyEqual(result.averageWaitingTime, 0) &&
            approximatelyEqual(result.averageTurnaroundTime, 0) &&
            approximatelyEqual(result.averageResponseTime, 0) &&
            approximatelyEqual(result.cpuUtilization, 0) &&
            approximatelyEqual(result.throughput, 0),
            "Invalid empty-workload averages."
        );

        return;
    }

    require(
        result.processes.size() == workload.size(),
        "Process count does not match workload."
    );

    // Identify each input process by ID and name.
    unordered_map<int, const Process*> inputById;
    unordered_map<string, int> idByName;

    for (const auto& p : workload) {
        require(
            p.arrivalTime >= 0 && p.burstTime > 0,
            "Invalid input process."
        );

        require(
            !p.name.empty() && p.name != "IDLE",
            "Invalid or reserved process name."
        );

        require(
            inputById.emplace(p.id, &p).second,
            "Duplicate input process ID."
        );

        require(
            idByName.emplace(p.name, p.id).second,
            "Duplicate input process name."
        );
    }

    // Check that every output process matches
    // its original input process.
    unordered_map<int, const Process*> resultById;

    long long totalWaiting = 0;
    long long totalTurnaround = 0;
    long long totalResponse = 0;
    long long totalBurst = 0;

    int latestCompletion = 0;

    for (const auto& p : result.processes) {
        const auto it = inputById.find(p.id);

        require(
            it != inputById.end(),
            "Result contains unknown process ID."
        );

        const Process& original = *it->second;

        require(
            resultById.emplace(p.id, &p).second,
            "Duplicate result process ID."
        );

        require(
            p.name == original.name &&
            p.arrivalTime == original.arrivalTime &&
            p.burstTime == original.burstTime &&
            p.priority == original.priority,
            "Input process properties were changed."
        );

        require(
            p.startTime >= p.arrivalTime,
            "Process starts before arrival."
        );

        require(
            p.completionTime >= p.startTime &&
            p.completionTime > p.arrivalTime,
            "Invalid process completion time."
        );

        require(
            p.turnaroundTime ==
                p.completionTime - p.arrivalTime,
            "Incorrect turnaround time."
        );

        require(
            p.waitingTime ==
                p.turnaroundTime - p.burstTime,
            "Incorrect waiting time."
        );

        require(
            p.responseTime ==
                p.startTime - p.arrivalTime,
            "Incorrect response time."
        );

        require(
            p.waitingTime >= 0 &&
            p.responseTime >= 0,
            "Negative waiting or response time."
        );

        totalWaiting += p.waitingTime;
        totalTurnaround += p.turnaroundTime;
        totalResponse += p.responseTime;
        totalBurst += p.burstTime;

        latestCompletion = max(
            latestCompletion, p.completionTime
        );
    }

    // Reconstruct CPU execution from the timeline.
    unordered_map<int, long long> executedTime;
    unordered_map<int, int> firstExecution;
    unordered_map<int, int> lastExecution;

    long long busyTime = 0;
    long long idleTime = 0;

    int previousEnd = 0;
    int previousProcess = -1;
    int countedSwitches = 0;

    require(
        !result.timeline.empty(),
        "Nonempty workload has an empty timeline."
    );

    for (const auto& slice : result.timeline) {
        require(
            slice.start == previousEnd,
            "Timeline contains a gap or overlap."
        );

        require(
            slice.start >= 0 &&
            slice.end > slice.start,
            "Invalid timeline interval."
        );

        previousEnd = slice.end;

        const long long duration =
            static_cast<long long>(slice.end) -
            slice.start;

        if (slice.processName == "IDLE") {
            idleTime += duration;
            continue;
        }

        const auto nameIt =
            idByName.find(slice.processName);

        require(
            nameIt != idByName.end(),
            "Timeline references an unknown process."
        );

        const int id = nameIt->second;
        const Process& original = *inputById.at(id);

        require(
            slice.start >= original.arrivalTime,
            "CPU executes process before arrival."
        );

        executedTime[id] += duration;
        busyTime += duration;

        if (firstExecution.find(id) ==
            firstExecution.end()) {
            firstExecution[id] = slice.start;
        }

        lastExecution[id] = slice.end;

        // Matches our simulator's convention:
        // process-to-process transitions are counted,
        // but idle periods and initial dispatch are not.
        if (previousProcess != -1 &&
            previousProcess != id) {
            ++countedSwitches;
        }

        previousProcess = id;
    }

    require(
        result.totalTime == previousEnd,
        "Simulation duration does not match timeline."
    );

    require(
        result.totalTime == latestCompletion,
        "Simulation ends before or after final completion."
    );

    require(
        busyTime + idleTime == result.totalTime,
        "CPU busy and idle time are inconsistent."
    );

    require(
        busyTime == totalBurst,
        "Total CPU execution differs from total bursts."
    );

    require(
        result.contextSwitches == countedSwitches,
        "Incorrect context-switch count."
    );

    for (const auto& p : result.processes) {
        const int id = p.id;

        require(
            executedTime[id] == p.burstTime,
            "Process received incorrect CPU time: " +
                p.name
        );

        require(
            firstExecution.at(id) == p.startTime,
            "Incorrect first execution time: " +
                p.name
        );

        require(
            lastExecution.at(id) == p.completionTime,
            "Incorrect last execution time: " +
                p.name
        );
    }

    const double count =
        static_cast<double>(workload.size());

    require(
        approximatelyEqual(
            result.averageWaitingTime,
            static_cast<double>(totalWaiting) / count
        ),
        "Incorrect average waiting time."
    );

    require(
        approximatelyEqual(
            result.averageTurnaroundTime,
            static_cast<double>(totalTurnaround) / count
        ),
        "Incorrect average turnaround time."
    );

    require(
        approximatelyEqual(
            result.averageResponseTime,
            static_cast<double>(totalResponse) / count
        ),
        "Incorrect average response time."
    );

    require(
        result.totalTime > 0,
        "Nonempty workload has zero simulation time."
    );

    const double expectedUtilization =
        100.0 * static_cast<double>(busyTime) /
        result.totalTime;

    const double expectedThroughput =
        count / result.totalTime;

    require(
        approximatelyEqual(
            result.cpuUtilization,
            expectedUtilization
        ),
        "Incorrect CPU utilization."
    );

    require(
        approximatelyEqual(
            result.throughput,
            expectedThroughput
        ),
        "Incorrect throughput."
    );
}
