
#include "validation/ScheduleValidator.h"

#include "algorithms/FCFS.h"
#include "algorithms/SJF.h"
#include "algorithms/SRTF.h"
#include "algorithms/RoundRobin.h"
#include "algorithms/Priority.h"
#include "algorithms/PriorityAging.h"
#include "algorithms/MLFQ.h"

#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

vector<Process> generateWorkload(
    mt19937& generator
) {
    uniform_int_distribution<int> processCount(1, 12);
    uniform_int_distribution<int> arrivals(0, 20);
    uniform_int_distribution<int> bursts(1, 12);
    uniform_int_distribution<int> priorities(-3, 7);

    const int count = processCount(generator);

    vector<Process> processes;
    processes.reserve(count);

    for (int i = 0; i < count; ++i) {
        processes.push_back({
            i + 1,
            "P" + to_string(i + 1),
            arrivals(generator),
            bursts(generator),
            priorities(generator)
        });
    }

    return processes;
}

void validateAllAlgorithms(
    const vector<Process>& workload
) {
    validateSchedule(
        workload,
        scheduleFCFS(workload)
    );

    validateSchedule(
        workload,
        scheduleSJF(workload)
    );

    validateSchedule(
        workload,
        scheduleSRTF(workload)
    );

    validateSchedule(
        workload,
        scheduleRoundRobin(workload, 3)
    );

    validateSchedule(
        workload,
        schedulePriorityNonPreemptive(workload)
    );

    validateSchedule(
        workload,
        schedulePriorityPreemptive(workload)
    );

    validateSchedule(
        workload,
        schedulePriorityAgingNonPreemptive(workload, 3)
    );

    validateSchedule(
        workload,
        schedulePriorityAgingPreemptive(workload, 3)
    );

    validateSchedule(
        workload,
        scheduleMLFQ(workload, {2, 4, 8}, 11)
    );
}

int main() {
    constexpr int RANDOM_WORKLOADS = 200;
    constexpr unsigned int RANDOM_SEED = 20261122;

    mt19937 generator(RANDOM_SEED);

    int passed = 0;
    int failed = 0;

    // Test 1: Empty workload for every algorithm.
    try {
        validateAllAlgorithms({});
        cout << "[PASS] Empty workload consistency\n";
        ++passed;
    }
    catch (const exception& error) {
        cout << "[FAIL] Empty workload consistency: "
             << error.what() << '\n';
        ++failed;
    }

    // Test 2: A workload with substantial idle time.
    try {
        const vector<Process> idleWorkload = {
            {1, "P1", 5, 2, 3},
            {2, "P2", 15, 3, 1},
            {3, "P3", 30, 1, 2}
        };

        validateAllAlgorithms(idleWorkload);

        cout << "[PASS] Idle-period consistency\n";
        ++passed;
    }
    catch (const exception& error) {
        cout << "[FAIL] Idle-period consistency: "
             << error.what() << '\n';
        ++failed;
    }

    // Test 3: Processes arriving simultaneously.
    try {
        const vector<Process> simultaneous = {
            {1, "P1", 0, 8, 3},
            {2, "P2", 0, 2, 1},
            {3, "P3", 0, 5, 2},
            {4, "P4", 0, 1, 4}
        };

        validateAllAlgorithms(simultaneous);

        cout << "[PASS] Simultaneous-arrival consistency\n";
        ++passed;
    }
    catch (const exception& error) {
        cout << "[FAIL] Simultaneous-arrival consistency: "
             << error.what() << '\n';
        ++failed;
    }

    // Test 4: A single process.
    try {
        validateAllAlgorithms({
            {1, "Only", 0, 20, 1}
        });

        cout << "[PASS] Single-process consistency\n";
        ++passed;
    }
    catch (const exception& error) {
        cout << "[FAIL] Single-process consistency: "
             << error.what() << '\n';
        ++failed;
    }

    // Randomized consistency testing.
    for (int test = 0; test < RANDOM_WORKLOADS; ++test) {
        const auto workload = generateWorkload(generator);

        try {
            validateAllAlgorithms(workload);
            ++passed;
        }
        catch (const exception& error) {
            ++failed;

            cout << "[FAIL] Random workload "
                 << test + 1 << ": "
                 << error.what() << '\n';

            // Print the workload for debugging.
            for (const auto& p : workload) {
                cout << "  "
                     << p.name
                     << " arrival=" << p.arrivalTime
                     << " burst=" << p.burstTime
                     << " priority=" << p.priority
                     << '\n';
            }
        }
    }

    cout << "\n===== CONSISTENCY TEST SUMMARY =====\n";
    cout << "Random seed: " << RANDOM_SEED << '\n';
    cout << "Random workloads: " << RANDOM_WORKLOADS
         << '\n';
    cout << "Algorithms per workload: 9\n";
    cout << "Random simulation validations: "
         << RANDOM_WORKLOADS * 9 << '\n';

    cout << "Passed workload checks: "
         << passed << '\n';

    cout << "Failed workload checks: "
         << failed << '\n';

    return failed == 0 ? 0 : 1;
}
