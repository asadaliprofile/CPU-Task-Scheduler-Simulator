
#include "algorithms/Priority.h"
#include "algorithms/PriorityAging.h"

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

    // Test 1: Aging changes non-preemptive order.
    vector<Process> orderSample = {
        {1, "P1", 0, 6, 1},
        {2, "P2", 0, 2, 5},
        {3, "P3", 5, 2, 2}
    };

    auto withoutAging =
        schedulePriorityNonPreemptive(orderSample);

    auto withAging =
        schedulePriorityAgingNonPreemptive(orderSample, 2);

    check(
        withoutAging.timeline.size() == 3 &&
        withoutAging.timeline[1].processName == "P3" &&
        withAging.timeline.size() == 3 &&
        withAging.timeline[0].processName == "P1" &&
        withAging.timeline[1].processName == "P2" &&
        withAging.timeline[2].processName == "P3",
        "Aging changes non-preemptive execution order"
    );

    // Test 2: Verify non-preemptive metrics.
    check(
        withAging.processes[0].completionTime == 6 &&
        withAging.processes[1].completionTime == 8 &&
        withAging.processes[2].completionTime == 10 &&
        withAging.processes[1].waitingTime == 6 &&
        approximatelyEqual(
            withAging.averageWaitingTime, 3.0
        ),
        "Non-preemptive aging calculates metrics"
    );

    // Test 3: Aging causes preemption without arrivals.
    vector<Process> preemptSample = {
        {1, "P1", 0, 12, 1},
        {2, "P2", 0, 2, 4}
    };

    auto preemptResult =
        schedulePriorityAgingPreemptive(preemptSample, 2);

    check(
        preemptResult.timeline.size() == 3 &&
        preemptResult.timeline[0].processName == "P1" &&
        preemptResult.timeline[0].start == 0 &&
        preemptResult.timeline[0].end == 8 &&
        preemptResult.timeline[1].processName == "P2" &&
        preemptResult.timeline[1].start == 8 &&
        preemptResult.timeline[1].end == 10 &&
        preemptResult.timeline[2].processName == "P1" &&
        preemptResult.timeline[2].start == 10 &&
        preemptResult.timeline[2].end == 14,
        "Aging triggers preemption at a priority threshold"
    );

    // Test 4: Verify preemptive metrics.
    check(
        preemptResult.processes[0].completionTime == 14 &&
        preemptResult.processes[0].waitingTime == 2 &&
        preemptResult.processes[1].completionTime == 10 &&
        preemptResult.processes[1].waitingTime == 8 &&
        approximatelyEqual(
            preemptResult.averageWaitingTime, 5.0
        ) &&
        preemptResult.contextSwitches == 2,
        "Preemptive aging calculates correct metrics"
    );

    // Test 5: Compare the same workload without aging.
    auto normalPriority =
        schedulePriorityPreemptive(preemptSample);

    check(
        normalPriority.timeline.size() == 2 &&
        normalPriority.timeline[0].end == 12 &&
        normalPriority.timeline[1].start == 12 &&
        preemptResult.processes[1].completionTime <
        normalPriority.processes[1].completionTime,
        "Aging lets a low-priority process run earlier"
    );

    // Test 6: Equal priorities do not force preemption.
    auto equalResult = schedulePriorityAgingPreemptive({
        {1, "P1", 0, 5, 1},
        {2, "P2", 2, 2, 1}
    }, 10);

    check(
        equalResult.timeline.size() == 2 &&
        equalResult.timeline[0].processName == "P1" &&
        equalResult.timeline[0].end == 5 &&
        equalResult.timeline[1].start == 5,
        "Equal priorities preserve the running process"
    );

    // Test 7: Handle idle periods.
    auto idleResult = schedulePriorityAgingPreemptive({
        {1, "P1", 3, 2, 2},
        {2, "P2", 8, 1, 1}
    }, 2);

    check(
        idleResult.timeline.size() == 4 &&
        idleResult.timeline[0].processName == "IDLE" &&
        idleResult.timeline[0].start == 0 &&
        idleResult.timeline[0].end == 3 &&
        idleResult.timeline[2].processName == "IDLE" &&
        idleResult.timeline[2].start == 5 &&
        idleResult.timeline[2].end == 8 &&
        idleResult.totalTime == 9,
        "Aging scheduler handles CPU idle periods"
    );

    // Test 8: Reject invalid aging interval.
    bool invalidInterval = false;

    try {
        schedulePriorityAgingPreemptive(preemptSample, 0);
    }
    catch (const invalid_argument&) {
        invalidInterval = true;
    }

    check(
        invalidInterval,
        "Aging scheduler rejects zero interval"
    );

    // Test 9: Reject invalid burst time.
    bool invalidBurst = false;

    try {
        schedulePriorityAgingNonPreemptive({
            {1, "Invalid", 0, 0, 2}
        }, 2);
    }
    catch (const invalid_argument&) {
        invalidBurst = true;
    }

    check(
        invalidBurst,
        "Aging scheduler rejects invalid burst"
    );

    // Test 10: Reject duplicate IDs.
    bool duplicateRejected = false;

    try {
        schedulePriorityAgingPreemptive({
            {1, "P1", 0, 4, 1},
            {1, "P2", 1, 2, 2}
        }, 2);
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "Aging scheduler rejects duplicate IDs"
    );

    // Test 11: Empty workload.
    auto emptyNP =
        schedulePriorityAgingNonPreemptive({}, 2);

    auto emptyP =
        schedulePriorityAgingPreemptive({}, 2);

    check(
        emptyNP.processes.empty() &&
        emptyP.processes.empty() &&
        emptyNP.timeline.empty() &&
        emptyP.timeline.empty() &&
        emptyP.totalTime == 0 &&
        emptyP.contextSwitches == 0,
        "Both aging policies handle empty workloads"
    );

    // Test 12: Preserve input data.
    check(
        preemptSample[0].startTime == -1 &&
        preemptSample[1].completionTime == -1 &&
        preemptSample[1].waitingTime == 0,
        "Aging scheduling preserves original input"
    );

    cout << "\n===== PRIORITY AGING TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}