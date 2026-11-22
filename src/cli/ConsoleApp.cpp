#include "io/WorkloadIO.h"
#include "cli/ConsoleApp.h"
#include "cli/InputUtils.h"

#include "algorithms/FCFS.h"
#include "algorithms/SJF.h"
#include "algorithms/SRTF.h"
#include "algorithms/RoundRobin.h"
#include "algorithms/Priority.h"
#include "algorithms/PriorityAging.h"
#include "algorithms/MLFQ.h"

#include "output/OutputFormatter.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

const int MAX_TIME_INPUT = 1000000;
const size_t MAX_PROCESSES = 1000;

vector<Process> sampleWorkload() {
    return {
        {1, "P1", 0, 8, 2},
        {2, "P2", 2, 3, 1},
        {3, "P3", 3, 1, 3}
    };
}

void showMenu() {
    cout << "\n********************************************\n";
    cout << "CPU TASK SCHEDULER SIMULATOR\n";
    cout << "**************************************\n";
    cout << "1. View processes\n";
    cout << "2. Add process\n";
    cout << "3. Remove process\n";
    cout << "4. Run a scheduling algorithm\n";
    cout << "5. Compare all algorithms\n";
    cout << "6. Configure scheduling parameters\n";
    cout << "7. Load sample workload\n";
    cout << "8. Save workload to CSV\n";
    cout << "9. Load workload from CSV\n";
    cout << "0. Exit\n";
}

void viewProcesses(const vector<Process>& processes) {
    cout << "\n===== CURRENT PROCESSES =====\n";

    if (processes.empty()) {
        cout << "No processes available.\n";
        return;
    }

    cout << left
         << setw(8) << "ID"
         << setw(25) << "Name"
         << setw(12) << "Arrival"
         << setw(12) << "Burst"
         << setw(12) << "Priority"
         << '\n';

    cout << string(69, '-') << '\n';

    for (const auto& p : processes) {
        cout << left
             << setw(8) << p.id
             << setw(25) << p.name
             << setw(12) << p.arrivalTime
             << setw(12) << p.burstTime
             << setw(12) << p.priority
             << '\n';
    }
}

bool addProcess(vector<Process>& processes) {
    if (processes.size() >= MAX_PROCESSES) {
        cout << "Maximum process count reached.\n";
        return true;
    }

    auto id = readInteger(
        cin, cout, "Process ID: ", 1, 1000000
    );
    if (!id) return false;

    for (const auto& p : processes) {
        if (p.id == *id) {
            cout << "A process with that ID already exists.\n";
            return true;
        }
    }

    auto name = readText(
        cin, cout, "Process name: ", 24
    );
    if (!name) return false;

    if (*name == "IDLE") {
        cout << "'IDLE' is reserved for the CPU timeline.\n";
        return true;
    }

    for (const auto& p : processes) {
        if (p.name == *name) {
            cout << "A process with that name already exists.\n";
            return true;
        }
    }

    auto arrival = readInteger(
        cin, cout, "Arrival time: ",
        0, MAX_TIME_INPUT
    );
    if (!arrival) return false;

    auto burst = readInteger(
        cin, cout, "CPU burst time: ",
        1, MAX_TIME_INPUT
    );
    if (!burst) return false;

    auto priority = readInteger(
        cin, cout,
        "Priority (-1000 to 1000; lower is higher): ",
        -1000, 1000
    );
    if (!priority) return false;

    processes.push_back({
        *id, *name, *arrival, *burst, *priority
    });

    cout << "Process added successfully.\n";
    return true;
}

bool removeProcess(vector<Process>& processes) {
    if (processes.empty()) {
        cout << "There are no processes to remove.\n";
        return true;
    }

    auto id = readInteger(
        cin, cout, "Enter process ID to remove: ",
        1, 1000000
    );
    if (!id) return false;

    auto it = find_if(
        processes.begin(),
        processes.end(),
        [&](const Process& p) {
            return p.id == *id;
        }
    );

    if (it == processes.end()) {
        cout << "Process not found.\n";
        return true;
    }

    processes.erase(it);
    cout << "Process removed successfully.\n";
    return true;
}

void showAlgorithmMenu() {
    cout << "\n===== SELECT SCHEDULING ALGORITHM =====\n";
    cout << "1. FCFS\n";
    cout << "2. SJF\n";
    cout << "3. SRTF\n";
    cout << "4. Round Robin\n";
    cout << "5. Priority Non-Preemptive\n";
    cout << "6. Priority Preemptive\n";
    cout << "7. Priority Aging Non-Preemptive\n";
    cout << "8. Priority Aging Preemptive\n";
    cout << "9. MLFQ\n";
    cout << "0. Return to main menu\n";
}

string algorithmName(int choice) {
    switch (choice) {
        case 1: return "FCFS";
        case 2: return "SJF";
        case 3: return "SRTF";
        case 4: return "Round Robin";
        case 5: return "Priority NP";
        case 6: return "Priority P";
        case 7: return "Aging NP";
        case 8: return "Aging P";
        case 9: return "MLFQ";
        default:
            throw invalid_argument("Unknown algorithm.");
    }
}

ScheduleResult runAlgorithm(
    int choice,
    const vector<Process>& processes,
    const SchedulerConfig& config
) {
    switch (choice) {
        case 1:
            return scheduleFCFS(processes);

        case 2:
            return scheduleSJF(processes);

        case 3:
            return scheduleSRTF(processes);

        case 4:
            return scheduleRoundRobin(
                processes, config.roundRobinQuantum
            );

        case 5:
            return schedulePriorityNonPreemptive(processes);

        case 6:
            return schedulePriorityPreemptive(processes);

        case 7:
            return schedulePriorityAgingNonPreemptive(
                processes, config.agingInterval
            );

        case 8:
            return schedulePriorityAgingPreemptive(
                processes, config.agingInterval
            );

        case 9:
            return scheduleMLFQ(
                processes,
                config.mlfqQuanta,
                config.boostInterval
            );

        default:
            throw invalid_argument(
                "Invalid scheduling algorithm."
            );
    }
}

bool runSelectedAlgorithm(
    const vector<Process>& processes,
    const SchedulerConfig& config
) {
    if (processes.empty()) {
        cout << "Please add processes first.\n";
        return true;
    }

    showAlgorithmMenu();

    auto choice = readInteger(
        cin, cout, "Select algorithm: ", 0, 9
    );
    if (!choice) return false;

    if (*choice == 0) return true;

    ScheduleResult result = runAlgorithm(
        *choice, processes, config
    );

    displaySchedule(
        algorithmName(*choice), result
    );

    return true;
}

void compareAlgorithms(
    const vector<Process>& processes,
    const SchedulerConfig& config
) {
    if (processes.empty()) {
        cout << "Please add processes first.\n";
        return;
    }

    // Compute each result separately.
    // Keep them alive until comparison is printed.
    vector<ScheduleResult> results;
    results.reserve(9);

    for (int choice = 1; choice <= 9; ++choice) {
        results.push_back(
            runAlgorithm(choice, processes, config)
        );
    }

    vector<ComparisonEntry> comparisons;
    comparisons.reserve(9);

    for (int choice = 1; choice <= 9; ++choice) {
        comparisons.push_back({
            algorithmName(choice),
            &results[static_cast<size_t>(choice - 1)]
        });
    }

    printComparison(comparisons);
}

bool configureScheduling(SchedulerConfig& config) {
    cout << "\n===== SCHEDULING CONFIGURATION =====\n";

    cout << "Current Round Robin quantum: "
         << config.roundRobinQuantum << '\n';

    cout << "Current aging interval: "
         << config.agingInterval << '\n';

    cout << "Current MLFQ quanta: "
         << config.mlfqQuanta[0] << ", "
         << config.mlfqQuanta[1] << ", "
         << config.mlfqQuanta[2] << '\n';

    cout << "Current MLFQ boost interval: "
         << config.boostInterval << '\n';

    cout << "\n1. Change Round Robin quantum\n";
    cout << "2. Change aging interval\n";
    cout << "3. Change MLFQ time quanta\n";
    cout << "4. Change MLFQ boost interval\n";
    cout << "0. Return\n";

    auto choice = readInteger(
        cin, cout, "Your choice: ", 0, 4
    );
    if (!choice) return false;

    switch (*choice) {
        case 0:
            break;

        case 1: {
            auto value = readInteger(
                cin, cout, "New RR quantum: ",
                1, MAX_TIME_INPUT
            );
            if (!value) return false;

            config.roundRobinQuantum = *value;
            break;
        }

        case 2: {
            auto value = readInteger(
                cin, cout, "New aging interval: ",
                1, MAX_TIME_INPUT
            );
            if (!value) return false;

            config.agingInterval = *value;
            break;
        }

        case 3: {
            // Update only after all values are valid.
            array<int, 3> newQuanta;

            for (int i = 0; i < 3; ++i) {
                auto value = readInteger(
                    cin, cout,
                    "Quantum for Q" + to_string(i) + ": ",
                    1, MAX_TIME_INPUT
                );
                if (!value) return false;

                newQuanta[i] = *value;
            }

            config.mlfqQuanta = newQuanta;
            break;
        }

        case 4: {
            auto value = readInteger(
                cin, cout, "New boost interval: ",
                1, MAX_TIME_INPUT
            );
            if (!value) return false;

            config.boostInterval = *value;
            break;
        }
    }

    if (*choice != 0) {
        cout << "Configuration updated successfully.\n";
    }

    return true;
}


bool saveWorkload(
    const vector<Process>& processes
) {
    auto filename = readText(
        cin, cout,
        "CSV filename (without path): ",
        100
    );

    if (!filename) return false;

    // Restrict input to simple filenames.
    // Prevent directory traversal and unexpected paths.
    if (*filename == "." ||
        *filename == ".." ||
        filename->find_first_of("/\\:") != string::npos ||
        filename->find("..") != string::npos) {

        cout << "Invalid filename.\n";
        return true;
    }

    string name = *filename;

    // Add .csv if the user omitted it.
    if (name.size() < 4 ||
        name.substr(name.size() - 4) != ".csv") {
        name += ".csv";
    }

    const filesystem::path path =
        filesystem::path("data") / name;

    saveWorkloadCSV(processes, path);

    cout << "Workload saved to: "
         << path.string() << '\n';

    return true;
}

bool loadWorkload(
    vector<Process>& processes
) {
    auto filename = readText(
        cin, cout,
        "CSV filename (without path): ",
        100
    );

    if (!filename) return false;

    if (*filename == "." ||
        *filename == ".." ||
        filename->find_first_of("/\\:") != string::npos ||
        filename->find("..") != string::npos) {

        cout << "Invalid filename.\n";
        return true;
    }

    string name = *filename;

    if (name.size() < 4 ||
        name.substr(name.size() - 4) != ".csv") {
        name += ".csv";
    }

    const filesystem::path path =
        filesystem::path("data") / name;

    // Parse and validate before replacing current data.
    vector<Process> loaded = loadWorkloadCSV(path);

    // This happens only after a successful load.
    processes = std::move(loaded);

    cout << "Successfully loaded "
         << processes.size()
         << " processes from "
         << path.string() << '\n';

    return true;
}

} // namespace

void runConsoleApp() {
    vector<Process> processes = sampleWorkload();
    SchedulerConfig config;

    cout << "\nWelcome to CPU Task Scheduler Simulator!\n";

    while (true) {
        showMenu();

        auto choice = readInteger(
            cin, cout, "Enter your choice: ", 0, 9
        );

        if (!choice) {
            cout << "\nInput ended. Exiting.\n";
            break;
        }

        if (*choice == 0) {
            cout << "Goodbye!\n";
            break;
        }

        try {
            bool inputAvailable = true;

            switch (*choice) {
                case 1:
                    viewProcesses(processes);
                    break;

                case 2:
                    inputAvailable = addProcess(processes);
                    break;

                case 3:
                    inputAvailable = removeProcess(processes);
                    break;

                case 4:
                    inputAvailable = runSelectedAlgorithm(
                        processes, config
                    );
                    break;

                case 5:
                    compareAlgorithms(processes, config);
                    break;

                case 6:
                    inputAvailable = configureScheduling(
                        config
                    );
                    break;

                case 7:
                    processes = sampleWorkload();
                    cout << "Sample workload loaded.\n";
                    break;

                case 8:
                    inputAvailable = saveWorkload(processes);
                    break;

                case 9:
                    inputAvailable = loadWorkload(processes);
                    break;
            }

            if (!inputAvailable) {
                cout << "\nInput ended. Exiting.\n";
                break;
            }
        }
        catch (const exception& error) {
            cout << "Operation failed: "
                 << error.what() << '\n';
        }
    }
}
