
#include "output/OutputFormatter.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

void printProcessTable(
    const string& algorithm,
    const ScheduleResult& result
) {
    cout << "\n===== " << algorithm
         << " PROCESS RESULTS =====\n";

    cout << left
         << setw(10) << "Process"
         << setw(10) << "Arrival"
         << setw(10) << "Burst"
         << setw(10) << "Start"
         << setw(12) << "Complete"
         << setw(12) << "Turnaround"
         << setw(10) << "Waiting"
         << setw(10) << "Response"
         << '\n';

    cout << string(84, '-') << '\n';

    for (const auto& p : result.processes) {
        cout << left
             << setw(10) << p.name
             << setw(10) << p.arrivalTime
             << setw(10) << p.burstTime
             << setw(10) << p.startTime
             << setw(12) << p.completionTime
             << setw(12) << p.turnaroundTime
             << setw(10) << p.waitingTime
             << setw(10) << p.responseTime
             << '\n';
    }
}

void printGanttChart(
    const string& algorithm,
    const ScheduleResult& result
) {
    cout << "\n===== " << algorithm
         << " GANTT CHART =====\n";

    if (result.timeline.empty()) {
        cout << "No CPU execution to display.\n";
        return;
    }

    for (const auto& slice : result.timeline) {
        cout << "[ " << slice.processName
             << ": " << slice.start
             << " -> " << slice.end
             << " ] ";
    }

    cout << '\n';
}

void printStatistics(
    const string& algorithm,
    const ScheduleResult& result
) {
    cout << "\n===== " << algorithm
         << " STATISTICS =====\n";

    cout << fixed << setprecision(2);

    cout << "Average Waiting Time: "
         << result.averageWaitingTime << '\n';

    cout << "Average Turnaround Time: "
         << result.averageTurnaroundTime << '\n';

    cout << "Average Response Time: "
         << result.averageResponseTime << '\n';

    cout << "CPU Utilization: "
         << result.cpuUtilization << "%\n";

    cout << "Throughput: "
         << result.throughput
         << " processes/time unit\n";

    cout << "Context Switches: "
         << result.contextSwitches << '\n';

    cout << "Total Simulation Time: "
         << result.totalTime << '\n';
}

void displaySchedule(
    const string& algorithm,
    const ScheduleResult& result
) {
    printProcessTable(algorithm, result);
    printGanttChart(algorithm, result);
    printStatistics(algorithm, result);
}

void printComparison(
    const vector<ComparisonEntry>& comparisons
) {
    cout << "\n===== PERFORMANCE COMPARISON =====\n";

    cout << fixed << setprecision(2);

    cout << left
         << setw(17) << "Algorithm"
         << right
         << setw(10) << "Avg Wait"
         << setw(10) << "Avg Turn"
         << setw(10) << "Avg Resp"
         << setw(10) << "CPU %"
         << setw(12) << "Throughput"
         << setw(10) << "Switches"
         << '\n';

    cout << string(79, '-') << '\n';

    for (const auto& entry : comparisons) {
        if (entry.result == nullptr) {
            throw invalid_argument(
                "Comparison contains a null result."
            );
        }

        const auto& r = *entry.result;

        cout << left
             << setw(17) << entry.name
             << right
             << setw(10) << r.averageWaitingTime
             << setw(10) << r.averageTurnaroundTime
             << setw(10) << r.averageResponseTime
             << setw(10) << r.cpuUtilization
             << setw(12) << r.throughput
             << setw(10) << r.contextSwitches
             << '\n';
    }
}
