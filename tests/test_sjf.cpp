
#include "algorithms/SJF.h"

#include <cmath>
#include <iostream>
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

    // Test 1: SJF execution order.
    vector<Process> sample = {
        {1, "P1", 0, 5, 2},
        {2, "P2", 1, 3, 1},
        {3, "P3", 2, 2, 3}
    };

    ScheduleResult result = scheduleSJF(sample);

    check(
        result.processes.size() == 3 &&
        result.processes[0].name == "P1" &&
        result.processes[1].name == "P3" &&
        result.processes[2].name == "P2" &&
        result.processes[0].completionTime == 5 &&
        result.processes[1].completionTime == 7 &&
        result.processes[2].completionTime == 10,
        "SJF selects the shortest available process"
    );

    // Test 2: Average metrics.
    check(
        approximatelyEqual(
            result.averageWaitingTime, 3.0
        ) &&
        approximatelyEqual(
            result.averageTurnaroundTime, 19.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageResponseTime, 3.0
        ),
        "SJF calculates correct averages"
    );

    // Test 3: CPU idle before the first arrival.
    vector<Process> lateArrival = {
        {1, "P1", 4, 3, 1}
    };

    ScheduleResult idleResult =
        scheduleSJF(lateArrival);

    check(
        idleResult.timeline.size() == 2 &&
        idleResult.timeline[0].processName == "IDLE" &&
        idleResult.timeline[0].start == 0 &&
        idleResult.timeline[0].end == 4 &&
        idleResult.processes[0].startTime == 4 &&
        idleResult.processes[0].completionTime == 7,
        "SJF handles initial CPU idle time"
    );

    // Test 4: A shorter process arriving later
    // cannot interrupt the running process.
    vector<Process> nonPreemptive = {
        {1, "Long", 0, 8, 1},
        {2, "Short", 2, 1, 1}
    };

    ScheduleResult nonPreemptiveResult =
        scheduleSJF(nonPreemptive);

    check(
        nonPreemptiveResult.processes[0].name == "Long" &&
        nonPreemptiveResult.processes[0].completionTime == 8 &&
        nonPreemptiveResult.processes[1].startTime == 8,
        "SJF is non-preemptive"
    );

    // Test 5: Tie-breaking by arrival time.
    vector<Process> equalBursts = {
        {1, "Later", 2, 3, 1},
        {2, "Earlier", 0, 3, 1}
    };

    ScheduleResult tieResult =
        scheduleSJF(equalBursts);

    check(
        tieResult.processes[0].name == "Earlier" &&
        tieResult.processes[1].name == "Later",
        "SJF resolves equal bursts by arrival time"
    );

    // Test 6: Reject invalid burst time.
    bool invalidBurstRejected = false;

    try {
        scheduleSJF({
            {1, "Invalid", 0, 0, 1}
        });
    }
    catch (const invalid_argument&) {
        invalidBurstRejected = true;
    }

    check(
        invalidBurstRejected,
        "SJF rejects invalid burst time"
    );

    // Test 7: Reject duplicate IDs.
    bool duplicateRejected = false;

    try {
        scheduleSJF({
            {1, "A", 0, 2, 1},
            {1, "B", 1, 3, 1}
        });
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "SJF rejects duplicate process IDs"
    );

    // Test 8: Empty workload.
    ScheduleResult emptyResult =
        scheduleSJF({});

    check(
        emptyResult.processes.empty() &&
        emptyResult.timeline.empty() &&
        emptyResult.totalTime == 0 &&
        emptyResult.contextSwitches == 0,
        "SJF handles empty workloads"
    );

    // Test 9: Preserve original workload.
    check(
        sample[0].startTime == -1 &&
        sample[1].completionTime == -1 &&
        sample[2].waitingTime == 0,
        "SJF does not modify input processes"
    );

    cout << "\n===== SJF TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
