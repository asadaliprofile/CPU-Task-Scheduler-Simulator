
#include "algorithms/Priority.h"

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

    vector<Process> sample = {
        {1, "P1", 0, 8, 2},
        {2, "P2", 2, 3, 1},
        {3, "P3", 3, 1, 3}
    };

    ScheduleResult np =
        schedulePriorityNonPreemptive(sample);

    ScheduleResult p =
        schedulePriorityPreemptive(sample);

    // Test 1: Non-preemptive execution order.
    check(
        np.timeline.size() == 3 &&
        np.timeline[0].processName == "P1" &&
        np.timeline[0].start == 0 &&
        np.timeline[0].end == 8 &&
        np.timeline[1].processName == "P2" &&
        np.timeline[1].end == 11 &&
        np.timeline[2].processName == "P3" &&
        np.timeline[2].end == 12,
        "Non-preemptive priority execution order"
    );

    // Test 2: Preemptive execution order.
    check(
        p.timeline.size() == 4 &&
        p.timeline[0].processName == "P1" &&
        p.timeline[0].start == 0 &&
        p.timeline[0].end == 2 &&
        p.timeline[1].processName == "P2" &&
        p.timeline[1].start == 2 &&
        p.timeline[1].end == 5 &&
        p.timeline[2].processName == "P1" &&
        p.timeline[2].start == 5 &&
        p.timeline[2].end == 11 &&
        p.timeline[3].processName == "P3" &&
        p.timeline[3].end == 12,
        "Preemptive priority interrupts lower priority"
    );

    // Test 3: Non-preemptive metrics.
    check(
        np.processes[0].waitingTime == 0 &&
        np.processes[1].waitingTime == 6 &&
        np.processes[2].waitingTime == 8 &&
        approximatelyEqual(
            np.averageWaitingTime, 14.0 / 3.0
        ) &&
        approximatelyEqual(
            np.averageTurnaroundTime, 26.0 / 3.0
        ),
        "Non-preemptive priority metrics"
    );

    // Test 4: Preemptive metrics.
    check(
        p.processes[0].waitingTime == 3 &&
        p.processes[1].waitingTime == 0 &&
        p.processes[2].waitingTime == 8 &&
        approximatelyEqual(
            p.averageWaitingTime, 11.0 / 3.0
        ) &&
        approximatelyEqual(
            p.averageTurnaroundTime, 23.0 / 3.0
        ) &&
        approximatelyEqual(
            p.averageResponseTime, 8.0 / 3.0
        ),
        "Preemptive priority metrics"
    );

    // Test 5: Context switches and CPU statistics.
    check(
        np.contextSwitches == 2 &&
        p.contextSwitches == 3 &&
        np.totalTime == 12 &&
        p.totalTime == 12 &&
        approximatelyEqual(np.cpuUtilization, 100.0) &&
        approximatelyEqual(p.cpuUtilization, 100.0) &&
        approximatelyEqual(p.throughput, 0.25),
        "Priority scheduling performance statistics"
    );

    // Test 6: Equal priority does not cause preemption.
    ScheduleResult equal = schedulePriorityPreemptive({
        {1, "P1", 0, 5, 2},
        {2, "P2", 2, 2, 2}
    });

    check(
        equal.timeline.size() == 2 &&
        equal.timeline[0].processName == "P1" &&
        equal.timeline[0].end == 5 &&
        equal.timeline[1].processName == "P2" &&
        equal.timeline[1].start == 5,
        "Equal priority does not preempt"
    );

    // Test 7: Lower-priority arrivals do not preempt.
    ScheduleResult lower = schedulePriorityPreemptive({
        {1, "P1", 0, 5, 1},
        {2, "P2", 2, 2, 3}
    });

    check(
        lower.timeline.size() == 2 &&
        lower.timeline[0].end == 5 &&
        lower.timeline[1].start == 5,
        "Lower-priority arrival does not preempt"
    );

    // Test 8: Initial and intermediate CPU idle time.
    ScheduleResult idle = schedulePriorityPreemptive({
        {1, "P1", 3, 2, 2},
        {2, "P2", 8, 1, 1}
    });

    check(
        idle.timeline.size() == 4 &&
        idle.timeline[0].processName == "IDLE" &&
        idle.timeline[0].start == 0 &&
        idle.timeline[0].end == 3 &&
        idle.timeline[2].processName == "IDLE" &&
        idle.timeline[2].start == 5 &&
        idle.timeline[2].end == 8 &&
        idle.contextSwitches == 1,
        "Priority scheduling handles CPU idle periods"
    );

    // Test 9: Tie-breaking by arrival time.
    ScheduleResult ties = schedulePriorityNonPreemptive({
        {1, "First", 0, 5, 1},
        {2, "Later", 3, 2, 2},
        {3, "Earlier", 2, 2, 2}
    });

    check(
        ties.timeline.size() == 3 &&
        ties.timeline[1].processName == "Earlier" &&
        ties.timeline[2].processName == "Later",
        "Equal priority prefers earlier arrival"
    );

    // Test 10: Invalid arrival time.
    bool invalidArrival = false;

    try {
        schedulePriorityNonPreemptive({
            {1, "Invalid", -1, 3, 1}
        });
    }
    catch (const invalid_argument&) {
        invalidArrival = true;
    }

    check(
        invalidArrival,
        "Priority scheduler rejects negative arrival"
    );

    // Test 11: Invalid burst time.
    bool invalidBurst = false;

    try {
        schedulePriorityPreemptive({
            {1, "Invalid", 0, 0, 1}
        });
    }
    catch (const invalid_argument&) {
        invalidBurst = true;
    }

    check(
        invalidBurst,
        "Priority scheduler rejects zero burst"
    );

    // Test 12: Duplicate process IDs.
    bool duplicateRejected = false;

    try {
        schedulePriorityPreemptive({
            {1, "P1", 0, 3, 1},
            {1, "P2", 1, 2, 2}
        });
    }
    catch (const invalid_argument&) {
        duplicateRejected = true;
    }

    check(
        duplicateRejected,
        "Priority scheduler rejects duplicate IDs"
    );

    // Test 13: Empty workload.
    ScheduleResult emptyNP =
        schedulePriorityNonPreemptive({});

    ScheduleResult emptyP =
        schedulePriorityPreemptive({});

    check(
        emptyNP.processes.empty() &&
        emptyNP.timeline.empty() &&
        emptyP.processes.empty() &&
        emptyP.timeline.empty() &&
        emptyP.totalTime == 0,
        "Both policies handle empty workloads"
    );

    // Test 14: Preserve original input data.
    check(
        sample[0].startTime == -1 &&
        sample[1].completionTime == -1 &&
        sample[2].waitingTime == 0,
        "Priority scheduling preserves input workload"
    );

    // Test 15: Simulation-time overflow.
    bool overflowRejected = false;

    try {
        schedulePriorityPreemptive({
            {1, "P1", 0,
             numeric_limits<int>::max(), 1},
            {2, "P2", 1, 2, 2}
        });
    }
    catch (const overflow_error&) {
        overflowRejected = true;
    }

    check(
        overflowRejected,
        "Priority scheduling detects time overflow"
    );

    cout << "\nPRIORITY TEST SUMMARY\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
