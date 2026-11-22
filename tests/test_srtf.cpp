
#include "algorithms/SRTF.h"

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

    // Test 1: Verify preemptive execution.
    vector<Process> sample = {
        {1, "P1", 0, 8, 2},
        {2, "P2", 2, 3, 1},
        {3, "P3", 3, 1, 3}
    };

    ScheduleResult result = scheduleSRTF(sample);

    check(
        result.timeline.size() == 5 &&
        result.timeline[0].processName == "P1" &&
        result.timeline[0].start == 0 &&
        result.timeline[0].end == 2 &&
        result.timeline[1].processName == "P2" &&
        result.timeline[1].end == 3 &&
        result.timeline[2].processName == "P3" &&
        result.timeline[2].end == 4 &&
        result.timeline[3].processName == "P2" &&
        result.timeline[3].end == 6 &&
        result.timeline[4].processName == "P1" &&
        result.timeline[4].end == 12,
        "SRTF correctly preempts running processes"
    );

    // Test 2: Completion and waiting times.
    check(
        result.processes[0].completionTime == 12 &&
        result.processes[0].waitingTime == 4 &&
        result.processes[1].completionTime == 6 &&
        result.processes[1].waitingTime == 1 &&
        result.processes[2].completionTime == 4 &&
        result.processes[2].waitingTime == 0,
        "SRTF calculates completion and waiting times"
    );

    // Test 3: Average metrics.
    check(
        approximatelyEqual(
            result.averageWaitingTime, 5.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageTurnaroundTime, 17.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageResponseTime, 0.0
        ),
        "SRTF calculates correct averages"
    );

    // Test 4: Process transitions.
    check(
        result.contextSwitches == 4 &&
        result.totalTime == 12 &&
        approximatelyEqual(
            result.cpuUtilization, 100.0
        ) &&
        approximatelyEqual(
            result.throughput, 3.0 / 12.0
        ),
        "SRTF calculates performance statistics"
    );

    // Test 5: Initial CPU idle period.
    ScheduleResult idleResult = scheduleSRTF({
        {1, "P1", 4, 3, 1}
    });

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
        "SRTF handles initial CPU idle time"
    );

    // Test 6: An equal remaining time should
    // not unnecessarily preempt the running job.
    ScheduleResult equalResult = scheduleSRTF({
        {1, "P1", 0, 5, 1},
        {2, "P2", 2, 3, 1}
    });

    check(
        equalResult.timeline.size() == 2 &&
        equalResult.timeline[0].processName == "P1" &&
        equalResult.timeline[0].end == 5 &&
        equalResult.timeline[1].processName == "P2" &&
        equalResult.timeline[1].start == 5,
        "SRTF avoids preemption on equal remaining times"
    );

    // Test 7: CPU idle between processes.
    ScheduleResult gapResult = scheduleSRTF({
        {1, "P1", 0, 2, 1},
        {2, "P2", 6, 1, 1}
    });

    check(
        gapResult.timeline.size() == 3 &&
        gapResult.timeline[1].processName == "IDLE" &&
        gapResult.timeline[1].start == 2 &&
        gapResult.timeline[1].end == 6 &&
        gapResult.contextSwitches == 1,
        "SRTF handles gaps between arrivals"
    );

    // Test 8: Reject invalid process data.
    bool invalidRejected = false;

    try {
        scheduleSRTF({
            {1, "Invalid", 0, 0, 1}
        });
    }
    catch (const invalid_argument&) {
        invalidRejected = true;
    }

    check(
        invalidRejected,
        "SRTF rejects zero burst time"
    );

    // Test 9: Reject duplicate IDs.
    bool duplicateRejected = false;

    try {
        scheduleSRTF({
            {1, "P1", 0, 3, 1},
            {1, "P2", 1, 2, 1}
        });
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "SRTF rejects duplicate process IDs"
    );

    // Test 10: Empty workload.
    ScheduleResult emptyResult = scheduleSRTF({});

    check(
        emptyResult.processes.empty() &&
        emptyResult.timeline.empty() &&
        emptyResult.totalTime == 0 &&
        emptyResult.contextSwitches == 0,
        "SRTF handles empty workloads"
    );

    // Test 11: Original workload must remain unchanged.
    check(
        sample[0].startTime == -1 &&
        sample[0].completionTime == -1 &&
        sample[1].waitingTime == 0,
        "SRTF preserves input workload"
    );

    // Test 12: Reject simulation-time overflow.
    bool overflowRejected = false;

    try {
        scheduleSRTF({
            {1, "P1", 0, numeric_limits<int>::max(), 1},
            {2, "P2", 1, 2, 1}
        });
    }
    catch (const overflow_error&) {
        overflowRejected = true;
    }

    check(
        overflowRejected,
        "SRTF detects simulation-time overflow"
    );

    cout << "\n===== SRTF TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
