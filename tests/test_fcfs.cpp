
#include "algorithms/FCFS.h"

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

    // Test 1: Verify FCFS order and completion times.
    vector<Process> sample = {
        {1, "P1", 0, 5, 2},
        {2, "P2", 1, 3, 1},
        {3, "P3", 2, 2, 3}
    };

    ScheduleResult result = scheduleFCFS(sample);

    check(
        result.processes.size() == 3 &&
        result.processes[0].name == "P1" &&
        result.processes[1].name == "P2" &&
        result.processes[2].name == "P3" &&
        result.processes[0].completionTime == 5 &&
        result.processes[1].completionTime == 8 &&
        result.processes[2].completionTime == 10,
        "FCFS schedules processes by arrival time"
    );

    // Test 2: Verify average metrics.
    check(
        approximatelyEqual(
            result.averageWaitingTime, 10.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageTurnaroundTime, 20.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageResponseTime, 10.0 / 3.0
        ),
        "FCFS calculates correct average metrics"
    );

    // Test 3: Handle initial CPU idle time.
    vector<Process> lateArrival = {
        {1, "P1", 4, 3, 1}
    };

    ScheduleResult idleResult =
        scheduleFCFS(lateArrival);

    check(
        idleResult.timeline.size() == 2 &&
        idleResult.timeline[0].processName == "IDLE" &&
        idleResult.timeline[0].start == 0 &&
        idleResult.timeline[0].end == 4 &&
        idleResult.processes[0].startTime == 4 &&
        idleResult.processes[0].completionTime == 7 &&
        approximatelyEqual(
            idleResult.cpuUtilization, 300.0 / 7.0
        ),
        "FCFS handles initial CPU idle periods"
    );

    // Test 4: Preserve order on equal arrival times.
    vector<Process> equalArrival = {
        {2, "P2", 0, 2, 1},
        {1, "P1", 0, 3, 2}
    };

    ScheduleResult equalResult =
        scheduleFCFS(equalArrival);

    check(
        equalResult.processes[0].name == "P2" &&
        equalResult.processes[1].name == "P1" &&
        equalResult.processes[0].startTime == 0 &&
        equalResult.processes[1].startTime == 2,
        "FCFS preserves input order for ties"
    );

    // Test 5: Reject invalid burst time.
    bool invalidBurstRejected = false;

    try {
        scheduleFCFS({
            {1, "Invalid", 0, 0, 1}
        });
    }
    catch (const invalid_argument&) {
        invalidBurstRejected = true;
    }

    check(
        invalidBurstRejected,
        "FCFS rejects zero burst time"
    );

    // Test 6: Reject duplicate process IDs.
    bool duplicateRejected = false;

    try {
        scheduleFCFS({
            {1, "P1", 0, 2, 1},
            {1, "P2", 1, 3, 1}
        });
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "FCFS rejects duplicate process IDs"
    );

    // Test 7: Empty workload.
    ScheduleResult emptyResult = scheduleFCFS({});

    check(
        emptyResult.processes.empty() &&
        emptyResult.timeline.empty() &&
        emptyResult.totalTime == 0 &&
        emptyResult.contextSwitches == 0,
        "FCFS handles an empty workload"
    );

    cout << "\n===== FCFS TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
