
#include "algorithms/MLFQ.h"

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
        cout << (condition ? "[PASS] " : "[FAIL] ")
             << name << '\n';

        if (condition)
            ++passed;
        else
            ++failed;
    };

    // Sample workload for multiple queues.
    vector<Process> sample = {
        {1, "P1", 0, 10, 1},
        {2, "P2", 1, 3, 2},
        {3, "P3", 2, 2, 3}
    };

    auto result = scheduleMLFQ(
        sample, {2, 4, 8}, 50
    );

    // Test 1: Verify queue rotation and demotion.
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
        result.timeline[3].end == 10 &&
        result.timeline[4].processName == "P2" &&
        result.timeline[4].start == 10 &&
        result.timeline[4].end == 11 &&
        result.timeline[5].processName == "P1" &&
        result.timeline[5].start == 11 &&
        result.timeline[5].end == 15,
        "MLFQ handles queue rotation and demotion"
    );

    // Test 2: Verify process metrics.
    check(
        result.processes[0].completionTime == 15 &&
        result.processes[0].waitingTime == 5 &&
        result.processes[1].completionTime == 11 &&
        result.processes[1].waitingTime == 7 &&
        result.processes[2].completionTime == 6 &&
        result.processes[2].waitingTime == 2,
        "MLFQ calculates process completion metrics"
    );

    // Test 3: Verify average metrics.
    check(
        approximatelyEqual(
            result.averageWaitingTime, 14.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageTurnaroundTime, 29.0 / 3.0
        ) &&
        approximatelyEqual(
            result.averageResponseTime, 1.0
        ),
        "MLFQ calculates correct averages"
    );

    // Test 4: Verify overall performance.
    check(
        result.contextSwitches == 5 &&
        result.totalTime == 15 &&
        approximatelyEqual(
            result.cpuUtilization, 100.0
        ) &&
        approximatelyEqual(
            result.throughput, 3.0 / 15.0
        ),
        "MLFQ calculates CPU statistics"
    );

    // Test 5: Higher-priority arrivals preempt
    // lower-priority processes.
    auto preempted = scheduleMLFQ({
        {1, "P1", 0, 12, 1},
        {2, "P2", 5, 1, 2}
    }, {2, 4, 8}, 50);

    check(
        preempted.timeline.size() == 3 &&
        preempted.timeline[0].processName == "P1" &&
        preempted.timeline[0].start == 0 &&
        preempted.timeline[0].end == 5 &&
        preempted.timeline[1].processName == "P2" &&
        preempted.timeline[1].start == 5 &&
        preempted.timeline[1].end == 6 &&
        preempted.timeline[2].processName == "P1" &&
        preempted.timeline[2].start == 6 &&
        preempted.timeline[2].end == 13,
        "MLFQ preempts lower-priority processes"
    );

    // Test 6: Preemption preserves unused quantum.
    // P1 has 1 unit of Q1 quantum left at time 5.
    // After P2, it finishes Q1 at time 7.
    check(
        preempted.processes[0].completionTime == 13 &&
        preempted.processes[1].completionTime == 6 &&
        preempted.contextSwitches == 2,
        "MLFQ preserves unfinished process execution"
    );

    // Test 7: Global priority boosting.
    auto boosted = scheduleMLFQ({
        {1, "P1", 0, 10, 1},
        {2, "P2", 0, 10, 1}
    }, {2, 4, 8}, 6);

    check(
        boosted.timeline.size() >= 4 &&
        boosted.timeline[0].processName == "P1" &&
        boosted.timeline[0].start == 0 &&
        boosted.timeline[0].end == 2 &&
        boosted.timeline[1].processName == "P2" &&
        boosted.timeline[1].start == 2 &&
        boosted.timeline[1].end == 4 &&
        boosted.timeline[2].processName == "P1" &&
        boosted.timeline[2].start == 4 &&
        boosted.timeline[2].end == 6 &&
        boosted.timeline[3].processName == "P2" &&
        boosted.timeline[3].start == 6 &&
        boosted.timeline[3].end == 8,
        "MLFQ applies periodic priority boosting"
    );

    // Test 8: Initial CPU idle time.
    auto idle = scheduleMLFQ({
        {1, "P1", 5, 3, 1}
    }, {2, 4, 8}, 20);

    check(
        idle.timeline.size() == 2 &&
        idle.timeline[0].processName == "IDLE" &&
        idle.timeline[0].start == 0 &&
        idle.timeline[0].end == 5 &&
        idle.timeline[1].processName == "P1" &&
        idle.timeline[1].start == 5 &&
        idle.timeline[1].end == 8 &&
        approximatelyEqual(
            idle.cpuUtilization, 37.5
        ),
        "MLFQ handles initial idle time"
    );

    // Test 9: Reject invalid time quantum.
    bool badQuantum = false;

    try {
        scheduleMLFQ(sample, {2, 0, 8}, 20);
    }
    catch (const invalid_argument&) {
        badQuantum = true;
    }

    check(
        badQuantum,
        "MLFQ rejects nonpositive time quantum"
    );

    // Test 10: Reject invalid boost interval.
    bool badBoost = false;

    try {
        scheduleMLFQ(sample, {2, 4, 8}, 0);
    }
    catch (const invalid_argument&) {
        badBoost = true;
    }

    check(
        badBoost,
        "MLFQ rejects invalid boost interval"
    );

    // Test 11: Reject invalid process data.
    bool badBurst = false;

    try {
        scheduleMLFQ({
            {1, "Invalid", 0, 0, 1}
        });
    }
    catch (const invalid_argument&) {
        badBurst = true;
    }

    check(
        badBurst,
        "MLFQ rejects invalid burst time"
    );

    // Test 12: Reject duplicate process IDs.
    bool duplicateRejected = false;

    try {
        scheduleMLFQ({
            {1, "P1", 0, 3, 1},
            {1, "P2", 1, 2, 1}
        });
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "MLFQ rejects duplicate process IDs"
    );

    // Test 13: Empty workload.
    auto empty = scheduleMLFQ({});

    check(
        empty.processes.empty() &&
        empty.timeline.empty() &&
        empty.totalTime == 0 &&
        empty.contextSwitches == 0,
        "MLFQ handles empty workloads"
    );

    // Test 14: Preserve original workload.
    check(
        sample[0].startTime == -1 &&
        sample[1].completionTime == -1 &&
        sample[2].waitingTime == 0,
        "MLFQ preserves original process data"
    );

    // Test 15: Detect simulation time overflow.
    bool overflowRejected = false;

    try {
        scheduleMLFQ({
            {1, "P1",
             numeric_limits<int>::max() - 1,
             5, 1}
        });
    }
    catch (const overflow_error&) {
        overflowRejected = true;
    }

    check(
        overflowRejected,
        "MLFQ detects simulation-time overflow"
    );

    cout << "\n===== MLFQ TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
