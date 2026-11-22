
#include "algorithms/RoundRobin.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

bool approximatelyEqual(double a, double b) {
    return fabs(a - b) < 1e-9;
}

int main() {
    int passed = 0;
    int failed = 0;

    auto check = [&](bool condition, const string& name) {
        if (condition) {
            cout << "[PASS] " << name << '\n';
            ++passed;
        }
        else {
            cout << "[FAIL] " << name << '\n';
            ++failed;
        }
    };

    // Test 1: Complete Round Robin execution timeline.
    vector<Process> sample = {
        {1, "P1", 0, 5, 2},
        {2, "P2", 1, 3, 1},
        {3, "P3", 2, 2, 3}
    };

    ScheduleResult result = scheduleRoundRobin(sample, 2);

    check(
        result.timeline.size() == 6 &&
        result.timeline[0].processName == "P1" &&
        result.timeline[0].start == 0 &&
        result.timeline[0].end == 2 &&
        result.timeline[1].processName == "P2" &&
        result.timeline[1].start == 2 &&
        result.timeline[1].end == 4 &&
        result.timeline[2].processName == "P3" &&
        result.timeline[2].start == 4 &&
        result.timeline[2].end == 6 &&
        result.timeline[3].processName == "P1" &&
        result.timeline[3].start == 6 &&
        result.timeline[3].end == 8 &&
        result.timeline[4].processName == "P2" &&
        result.timeline[4].start == 8 &&
        result.timeline[4].end == 9 &&
        result.timeline[5].processName == "P1" &&
        result.timeline[5].start == 9 &&
        result.timeline[5].end == 10,
        "Round Robin generates correct execution order"
    );

    // Test 2: Completion and waiting times.
    check(
        result.processes[0].completionTime == 10 &&
        result.processes[0].waitingTime == 5 &&
        result.processes[1].completionTime == 9 &&
        result.processes[1].waitingTime == 5 &&
        result.processes[2].completionTime == 6 &&
        result.processes[2].waitingTime == 2,
        "Round Robin calculates process waiting times"
    );

    // Test 3: Average process metrics.
    check(
        approximatelyEqual(
            result.averageWaitingTime, 4.0
        ) &&
        approximatelyEqual(
            result.averageTurnaroundTime, 22.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageResponseTime, 1.0
        ),
        "Round Robin calculates correct averages"
    );

    // Test 4: CPU and context-switch statistics.
    check(
        result.contextSwitches == 5 &&
        result.totalTime == 10 &&
        approximatelyEqual(
            result.cpuUtilization, 100.0
        ) &&
        approximatelyEqual(
            result.throughput, 0.3
        ),
        "Round Robin calculates performance statistics"
    );

    // Test 5: Initial CPU idle period.
    ScheduleResult idleResult = scheduleRoundRobin({
        {1, "P1", 4, 3, 1}
    }, 2);

    check(
        idleResult.timeline.size() == 2 &&
        idleResult.timeline[0].processName == "IDLE" &&
        idleResult.timeline[0].start == 0 &&
        idleResult.timeline[0].end == 4 &&
        idleResult.timeline[1].processName == "P1" &&
        idleResult.timeline[1].start == 4 &&
        idleResult.timeline[1].end == 7 &&
        idleResult.contextSwitches == 0,
        "Round Robin handles initial CPU idle time"
    );

    // Test 6: New arrivals have priority over
    // requeuing a process at the quantum boundary.
    ScheduleResult boundaryResult = scheduleRoundRobin({
        {1, "P1", 0, 4, 1},
        {2, "P2", 2, 1, 1}
    }, 2);

    check(
        boundaryResult.timeline.size() == 3 &&
        boundaryResult.timeline[0].processName == "P1" &&
        boundaryResult.timeline[0].end == 2 &&
        boundaryResult.timeline[1].processName == "P2" &&
        boundaryResult.timeline[1].start == 2 &&
        boundaryResult.timeline[2].processName == "P1" &&
        boundaryResult.timeline[2].start == 3,
        "Round Robin handles quantum-boundary arrivals"
    );

    // Test 7: Preserve input order for simultaneous
    // arrivals, regardless of CPU burst length.
    ScheduleResult tieResult = scheduleRoundRobin({
        {1, "Long", 0, 5, 1},
        {2, "Short", 0, 1, 1}
    }, 2);

    check(
        tieResult.timeline.size() >= 2 &&
        tieResult.timeline[0].processName == "Long" &&
        tieResult.timeline[1].processName == "Short",
        "Round Robin preserves FIFO arrival order"
    );

    // Test 8: Reject invalid quantum.
    bool quantumRejected = false;

    try {
        scheduleRoundRobin(sample, 0);
    }
    catch (const invalid_argument&) {
        quantumRejected = true;
    }

    check(
        quantumRejected,
        "Round Robin rejects zero time quantum"
    );

    // Test 9: Reject invalid burst.
    bool burstRejected = false;

    try {
        scheduleRoundRobin({
            {1, "Invalid", 0, 0, 1}
        }, 2);
    }
    catch (const invalid_argument&) {
        burstRejected = true;
    }

    check(
        burstRejected,
        "Round Robin rejects invalid burst time"
    );

    // Test 10: Reject duplicate process IDs.
    bool duplicateRejected = false;

    try {
        scheduleRoundRobin({
            {1, "P1", 0, 3, 1},
            {1, "P2", 1, 2, 1}
        }, 2);
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "Round Robin rejects duplicate process IDs"
    );

    // Test 11: Empty workload.
    ScheduleResult emptyResult =
        scheduleRoundRobin({}, 2);

    check(
        emptyResult.processes.empty() &&
        emptyResult.timeline.empty() &&
        emptyResult.totalTime == 0 &&
        emptyResult.contextSwitches == 0,
        "Round Robin handles empty workloads"
    );

    // Test 12: Preserve the original process data.
    check(
        sample[0].startTime == -1 &&
        sample[1].completionTime == -1 &&
        sample[2].waitingTime == 0,
        "Round Robin preserves the original workload"
    );

    // Test 13: Detect integer time overflow.
    bool overflowRejected = false;

    try {
        scheduleRoundRobin({
            {1, "P1", 0,
             numeric_limits<int>::max(), 1},
            {2, "P2", 1, 2, 1}
        }, numeric_limits<int>::max());
    }
    catch (const overflow_error&) {
        overflowRejected = true;
    }

    check(
        overflowRejected,
        "Round Robin detects simulation-time overflow"
    );

    // Test 14: No unnecessary switching when
    // only one process is ready.
    ScheduleResult singleResult = scheduleRoundRobin({
        {1, "P1", 0, 5, 1}
    }, 1);

    check(
        singleResult.timeline.size() == 1 &&
        singleResult.timeline[0].start == 0 &&
        singleResult.timeline[0].end == 5 &&
        singleResult.contextSwitches == 0,
        "Round Robin avoids artificial self-switches"
    );

    cout << "\n===== ROUND ROBIN TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
