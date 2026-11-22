
#include "algorithms/MLFQ.h"
#include "algorithms/PriorityAging.h"
#include "algorithms/Priority.h"
#include "algorithms/RoundRobin.h"
#include "validation/ScheduleValidator.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

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

    // Test 1: Arrival exactly at quantum expiration.
    {
        vector<Process> workload = {
            {1, "P1", 0, 5, 1},
            {2, "P2", 2, 1, 2}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 50
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 3 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].start == 0 &&
            result.timeline[0].end == 2 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].start == 2 &&
            result.timeline[1].end == 3 &&
            result.timeline[2].processName == "P1" &&
            result.timeline[2].start == 3 &&
            result.timeline[2].end == 6,
            "MLFQ handles arrival at quantum boundary"
        );
    }

    // Test 2: Higher-priority arrival interrupts
    // a process currently running in Q1.
    {
        vector<Process> workload = {
            {1, "P1", 0, 10, 1},
            {2, "P2", 3, 1, 2}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 50
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 3 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].end == 3 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].start == 3 &&
            result.timeline[1].end == 4 &&
            result.timeline[2].processName == "P1" &&
            result.timeline[2].start == 4 &&
            result.timeline[2].end == 11 &&
            result.contextSwitches == 2,
            "MLFQ preempts a lower queue correctly"
        );
    }

    // Test 3: A priority boost occurs exactly when
    // a running process exhausts its quantum.
    {
        vector<Process> workload = {
            {1, "P1", 0, 10, 1},
            {2, "P2", 0, 10, 1}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 6
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() >= 4 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].end == 2 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].end == 4 &&
            result.timeline[2].processName == "P1" &&
            result.timeline[2].end == 6 &&
            result.timeline[3].processName == "P2" &&
            result.timeline[3].start == 6 &&
            result.timeline[3].end == 8,
            "MLFQ handles boost at quantum expiration"
        );
    }

    // Test 4: Arrival and global boost at the
    // exact same simulation time.
    {
        vector<Process> workload = {
            {1, "P1", 0, 9, 1},
            {2, "P2", 6, 1, 2}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 6
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 3 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].end == 6 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].start == 6 &&
            result.timeline[1].end == 7 &&
            result.timeline[2].processName == "P1" &&
            result.timeline[2].start == 7 &&
            result.timeline[2].end == 10,
            "MLFQ handles arrival coinciding with boost"
        );
    }

    // Test 5: An idle CPU must jump to the next
    // arrival, even across multiple boost intervals.
    {
        vector<Process> workload = {
            {1, "P1", 13, 2, 1}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 5
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 2 &&
            result.timeline[0].processName == "IDLE" &&
            result.timeline[0].start == 0 &&
            result.timeline[0].end == 13 &&
            result.timeline[1].processName == "P1" &&
            result.timeline[1].start == 13 &&
            result.timeline[1].end == 15,
            "MLFQ skips idle intervals safely"
        );
    }

    // Test 6: A CPU idle period does not erase the
    // previous process-to-process switch history.
    {
        vector<Process> workload = {
            {1, "P1", 0, 2, 1},
            {2, "P2", 6, 1, 2}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 50
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 3 &&
            result.timeline[1].processName == "IDLE" &&
            result.timeline[1].start == 2 &&
            result.timeline[1].end == 6 &&
            result.contextSwitches == 1,
            "MLFQ context switches across idle periods"
        );
    }

    // Test 7: A single process may cross many
    // boost intervals without self-switching.
    {
        vector<Process> workload = {
            {1, "P1", 0, 11, 1}
        };

        auto result = scheduleMLFQ(
            workload, {2, 4, 8}, 3
        );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 1 &&
            result.timeline[0].start == 0 &&
            result.timeline[0].end == 11 &&
            result.contextSwitches == 0,
            "MLFQ avoids artificial self-switches"
        );
    }

    // Test 8: Priority aging can preempt even
    // when no new process arrives.
    {
        vector<Process> workload = {
            {1, "P1", 0, 12, 1},
            {2, "P2", 0, 2, 4}
        };

        auto result =
            schedulePriorityAgingPreemptive(
                workload, 2
            );

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 3 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].end == 8 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].start == 8 &&
            result.timeline[1].end == 10 &&
            result.timeline[2].processName == "P1" &&
            result.timeline[2].end == 14,
            "Priority aging triggers threshold preemption"
        );
    }

    // Test 9: Equal priority alone should not
    // interrupt the running process.
    {
        vector<Process> workload = {
            {1, "P1", 0, 5, 2},
            {2, "P2", 2, 2, 2}
        };

        auto result =
            schedulePriorityPreemptive(workload);

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 2 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].end == 5 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].start == 5,
            "Equal static priority does not preempt"
        );
    }

    // Test 10: Round Robin arrival exactly when
    // the current quantum expires.
    {
        vector<Process> workload = {
            {1, "P1", 0, 4, 1},
            {2, "P2", 2, 1, 1}
        };

        auto result =
            scheduleRoundRobin(workload, 2);

        validateSchedule(workload, result);

        check(
            result.timeline.size() == 3 &&
            result.timeline[0].processName == "P1" &&
            result.timeline[0].end == 2 &&
            result.timeline[1].processName == "P2" &&
            result.timeline[1].start == 2 &&
            result.timeline[2].processName == "P1" &&
            result.timeline[2].start == 3 &&
            result.contextSwitches == 2,
            "Round Robin handles boundary arrivals"
        );
    }

    // Test 11: Reject invalid MLFQ parameters.
    {
        bool rejected = false;

        try {
            (void)scheduleMLFQ(
                {{1, "P1", 0, 5, 1}},
                {2, 0, 8},
                10
            );
        }
        catch (const invalid_argument&) {
            rejected = true;
        }

        check(
            rejected,
            "MLFQ rejects a zero quantum"
        );
    }

    // Test 12: Detect an overflow in total
    // simulation time.
    {
        bool rejected = false;

        try {
            (void)scheduleMLFQ({
                {1, "P1",
                 numeric_limits<int>::max() - 1,
                 5, 1}
            });
        }
        catch (const overflow_error&) {
            rejected = true;
        }

        check(
            rejected,
            "MLFQ rejects simulation-time overflow"
        );
    }

    cout << "\n===== SCHEDULER BOUNDARY TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
