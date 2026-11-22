
#include "algorithms/FCFS.h"
#include "algorithms/SJF.h"
#include "algorithms/SRTF.h"
#include "algorithms/RoundRobin.h"
#include "algorithms/Priority.h"
#include "algorithms/PriorityAging.h"
#include "algorithms/MLFQ.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {

namespace fs = std::filesystem;
using Clock = chrono::steady_clock;

constexpr unsigned int RANDOM_SEED = 20261122;
constexpr int REPETITIONS = 5;

const vector<int> WORKLOAD_SIZES = {
    100, 250, 500, 1000
};

// Prevent benchmark work from being discarded.
volatile uint64_t benchmarkSink = 0;

struct BenchmarkResult {
    string algorithm;
    int processCount;
    double averageMs;
    double minimumMs;
    double maximumMs;
    double medianMs;
};

vector<Process> generateWorkload(
    int count,
    unsigned int seed
) {
    mt19937 generator(seed);

    uniform_int_distribution<int> arrivalDist(0, 200);
    uniform_int_distribution<int> burstDist(1, 20);
    uniform_int_distribution<int> priorityDist(1, 10);

    vector<Process> processes;
    processes.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        processes.push_back({
            i + 1,
            "P" + to_string(i + 1),
            arrivalDist(generator),
            burstDist(generator),
            priorityDist(generator)
        });
    }

    return processes;
}

// Execute the selected scheduling configuration.
ScheduleResult runAlgorithm(
    int algorithm,
    const vector<Process>& processes
) {
    switch (algorithm) {
        case 0:
            return scheduleFCFS(processes);

        case 1:
            return scheduleSJF(processes);

        case 2:
            return scheduleSRTF(processes);

        case 3:
            return scheduleRoundRobin(processes, 3);

        case 4:
            return schedulePriorityNonPreemptive(
                processes
            );

        case 5:
            return schedulePriorityPreemptive(
                processes
            );

        case 6:
            return schedulePriorityAgingNonPreemptive(
                processes, 5
            );

        case 7:
            return schedulePriorityAgingPreemptive(
                processes, 5
            );

        case 8:
            return scheduleMLFQ(
                processes,
                {2, 4, 8},
                100
            );

        default:
            throw invalid_argument(
                "Unknown benchmark algorithm."
            );
    }
}

const array<string, 9> ALGORITHM_NAMES = {
    "FCFS",
    "SJF",
    "SRTF",
    "Round Robin",
    "Priority NP",
    "Priority P",
    "Priority Aging NP",
    "Priority Aging P",
    "MLFQ"
};

// Measure the execution time of one algorithm.
BenchmarkResult benchmarkAlgorithm(
    int algorithm,
    const vector<Process>& workload
) {
    // Warm-up run, excluded from measurements.
    {
        const auto warmup = runAlgorithm(
            algorithm, workload
        );

        benchmarkSink = benchmarkSink +
            static_cast<uint64_t>(warmup.totalTime);
    }

    vector<double> measurements;
    measurements.reserve(REPETITIONS);

    for (int repetition = 0;
         repetition < REPETITIONS;
         ++repetition) {

        const auto start = Clock::now();

        const ScheduleResult result = runAlgorithm(
            algorithm, workload
        );

        const auto end = Clock::now();

        // Consume the result outside the timed section.
        benchmarkSink = benchmarkSink +
            static_cast<uint64_t>(result.totalTime) +
            static_cast<uint64_t>(
                result.timeline.size()
            );

        const double elapsedMs =
            chrono::duration<double, milli>(
                end - start
            ).count();

        measurements.push_back(elapsedMs);
    }

    const double total = [&]() {
        double sum = 0.0;

        for (double value : measurements) {
            sum += value;
        }

        return sum;
    }();

    sort(measurements.begin(), measurements.end());

    BenchmarkResult result;

    result.algorithm = ALGORITHM_NAMES.at(
        static_cast<size_t>(algorithm)
    );

    result.processCount =
        static_cast<int>(workload.size());

    result.averageMs = total / REPETITIONS;
    result.minimumMs = measurements.front();
    result.maximumMs = measurements.back();
    result.medianMs =
        measurements[measurements.size() / 2];

    return result;
}

void printResults(
    const vector<BenchmarkResult>& results
) {
    cout << "\n===== BENCHMARK RESULTS =====\n\n";

    cout << left
         << setw(22) << "Algorithm"
         << right
         << setw(12) << "Processes"
         << setw(14) << "Avg (ms)"
         << setw(14) << "Median (ms)"
         << setw(14) << "Min (ms)"
         << setw(14) << "Max (ms)"
         << '\n';

    cout << string(90, '-') << '\n';

    cout << fixed << setprecision(6);

    for (const auto& r : results) {
        cout << left
             << setw(22) << r.algorithm
             << right
             << setw(12) << r.processCount
             << setw(14) << r.averageMs
             << setw(14) << r.medianMs
             << setw(14) << r.minimumMs
             << setw(14) << r.maximumMs
             << '\n';
    }
}

void saveResults(
    const vector<BenchmarkResult>& results,
    const fs::path& outputPath
) {
    const fs::path directory =
        outputPath.parent_path();

    if (!directory.empty()) {
        fs::create_directories(directory);
    }

    ofstream file(outputPath);

    if (!file) {
        throw runtime_error(
            "Cannot create benchmark CSV file."
        );
    }

    file << "algorithm,process_count,"
            "average_ms,median_ms,minimum_ms,maximum_ms\n";

    file << fixed << setprecision(6);

    for (const auto& r : results) {
        file << r.algorithm << ','
             << r.processCount << ','
             << r.averageMs << ','
             << r.medianMs << ','
             << r.minimumMs << ','
             << r.maximumMs << '\n';
    }

    file.flush();

    if (!file) {
        throw runtime_error(
            "Failed to write benchmark results."
        );
    }
}

} // namespace

int main() {
    try {
        cout << "\n=======================================\n";
        cout << "    CPU SCHEDULER PERFORMANCE BENCHMARK\n";
        cout << "=======================================\n";

        cout << "Random seed: " << RANDOM_SEED << '\n';
        cout << "Measured repetitions: "
             << REPETITIONS << '\n';
        cout << "Algorithms: "
             << ALGORITHM_NAMES.size() << '\n';

        vector<BenchmarkResult> allResults;

        allResults.reserve(
            WORKLOAD_SIZES.size() *
            ALGORITHM_NAMES.size()
        );

        for (int count : WORKLOAD_SIZES) {
            cout << "\nBenchmarking "
                 << count << " processes...\n";

            // One reproducible workload per size.
            const auto workload = generateWorkload(
                count,
                RANDOM_SEED +
                    static_cast<unsigned int>(count)
            );

            for (int algorithm = 0;
                 algorithm <
                     static_cast<int>(
                         ALGORITHM_NAMES.size()
                     );
                 ++algorithm) {

                BenchmarkResult result =
                    benchmarkAlgorithm(
                        algorithm, workload
                    );

                cout << "  "
                     << left << setw(22)
                     << result.algorithm
                     << fixed << setprecision(6)
                     << result.averageMs
                     << " ms\n";

                allResults.push_back(result);
            }
        }

        printResults(allResults);

        const fs::path outputPath =
            fs::path("data") /
            "benchmark_results.csv";

        saveResults(allResults, outputPath);

        cout << "\nBenchmark CSV saved to: "
             << outputPath.string() << '\n';

        cout << "Benchmark rows: "
             << allResults.size() << '\n';

        return 0;
    }
    catch (const exception& error) {
        cerr << "\nBenchmark failed: "
             << error.what() << '\n';

        return 1;
    }
}
