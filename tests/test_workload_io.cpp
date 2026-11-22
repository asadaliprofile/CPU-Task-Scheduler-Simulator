
#include "io/WorkloadIO.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace fs = std::filesystem;

void writeTestFile(
    const fs::path& path,
    const string& contents
) {
    ofstream file(path, ios::binary | ios::trunc);

    if (!file) {
        throw runtime_error("Cannot create test file.");
    }

    file << contents;

    if (!file) {
        throw runtime_error("Cannot write test file.");
    }
}

bool failsToLoad(const fs::path& path) {
    try {
        (void)loadWorkloadCSV(path);
        return false;
    }
    catch (const exception&) {
        return true;
    }
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

    const fs::path directory =
        "workload_io_test_data";

    fs::create_directories(directory);

    const fs::path csv = directory / "workload.csv";

    // Test 1: Save and load a normal workload.
    vector<Process> sample = {
        {1, "P1", 0, 8, 2},
        {2, "P2", 2, 3, 1},
        {3, "P3", 3, 1, 3}
    };

    saveWorkloadCSV(sample, csv);
    const auto loaded = loadWorkloadCSV(csv);

    check(
        loaded.size() == 3 &&
        loaded[0].id == 1 &&
        loaded[0].name == "P1" &&
        loaded[1].arrivalTime == 2 &&
        loaded[1].burstTime == 3 &&
        loaded[2].priority == 3,
        "CSV save/load round trip"
    );

    // Test 2: Preserve workload ordering.
    check(
        loaded[0].id == 1 &&
        loaded[1].id == 2 &&
        loaded[2].id == 3,
        "CSV preserves process order"
    );

    // Test 3: Quoted commas and quotation marks.
    const fs::path quoted = directory / "quoted.csv";

    saveWorkloadCSV({
        {7, "Worker, A", 0, 4, 2},
        {8, "Worker \"B\"", 1, 3, 1}
    }, quoted);

    const auto quotedResult = loadWorkloadCSV(quoted);

    check(
        quotedResult.size() == 2 &&
        quotedResult[0].name == "Worker, A" &&
        quotedResult[1].name == "Worker \"B\"",
        "CSV handles quoted process names"
    );

    // Test 4: Empty workload is valid.
    const fs::path empty = directory / "empty.csv";

    saveWorkloadCSV({}, empty);
    const auto emptyResult = loadWorkloadCSV(empty);

    check(
        emptyResult.empty(),
        "CSV supports empty workloads"
    );

    // Test 5: Reject an incorrect header.
    writeTestFile(
        csv,
        "id,name,burstTime\n"
        "1,P1,3\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects incorrect headers"
    );

    // Test 6: Reject missing columns.
    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "1,P1,0,5\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects missing columns"
    );

    // Test 7: Reject duplicate process IDs.
    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "1,P1,0,5,2\n"
        "1,P2,1,3,1\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects duplicate IDs"
    );

    // Test 8: Reject duplicate process names.
    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "1,P1,0,5,2\n"
        "2,P1,1,3,1\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects duplicate names"
    );

    // Test 9: Reject invalid numerical fields.
    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "1,P1,0,abc,2\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects nonnumeric burst times"
    );

    // Test 10: Reject zero burst time.
    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "1,P1,0,0,2\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects zero burst time"
    );

    // Test 11: Reject malformed quoted fields.
    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "1,\"Unclosed,0,5,2\n"
    );

    check(
        failsToLoad(csv),
        "CSV rejects malformed quotations"
    );

    // Test 12: Failed loading preserves original data.
    vector<Process> current = sample;

    writeTestFile(
        csv,
        "id,name,arrivalTime,burstTime,priority\n"
        "9,Valid,0,3,1\n"
        "10,Invalid,1,-5,2\n"
    );

    bool rejected = false;

    try {
        // The same replacement pattern used by
        // the interactive console application.
        vector<Process> replacement =
            loadWorkloadCSV(csv);

        current = std::move(replacement);
    }
    catch (const exception&) {
        rejected = true;
    }

    check(
        rejected &&
        current.size() == 3 &&
        current[0].id == 1 &&
        current[1].name == "P2" &&
        current[2].burstTime == 1,
        "Failed CSV load preserves active workload"
    );


    // Test 13: Saving valid data replaces an
    // existing CSV with the new workload.
    {
        saveWorkloadCSV(sample, csv);

        vector<Process> replacement = {
            {10, "NewP1", 0, 4, 1},
            {11, "NewP2", 2, 6, 3}
        };

        saveWorkloadCSV(replacement, csv);

        const auto reloaded = loadWorkloadCSV(csv);

        check(
            reloaded.size() == 2 &&
            reloaded[0].id == 10 &&
            reloaded[0].name == "NewP1" &&
            reloaded[1].id == 11 &&
            reloaded[1].burstTime == 6,
            "Safe saving replaces an existing workload"
        );
    }

    // Test 14: Invalid input must not overwrite
    // an existing valid CSV file.
    {
        saveWorkloadCSV(sample, csv);

        bool rejected = false;

        try {
            saveWorkloadCSV({
                {1, "P1", 0, 5, 1},
                {1, "Duplicate", 1, 3, 2}
            }, csv);
        }
        catch (const exception&) {
            rejected = true;
        }

        const auto preserved = loadWorkloadCSV(csv);

        check(
            rejected &&
            preserved.size() == 3 &&
            preserved[0].name == "P1" &&
            preserved[1].name == "P2" &&
            preserved[2].name == "P3",
            "Invalid save preserves existing CSV"
        );
    }

    // Test 15: A blocked temporary file path
    // must not damage the original CSV.
    {
        saveWorkloadCSV(sample, csv);

        fs::path blockedPath = csv;
        blockedPath += ".tmp";

        // Create a directory where the temporary
        // file would normally be written.
        fs::create_directory(blockedPath);

        bool rejected = false;

        try {
            saveWorkloadCSV({
                {20, "Replacement", 0, 2, 1}
            }, csv);
        }
        catch (const exception&) {
            rejected = true;
        }

        const auto preserved = loadWorkloadCSV(csv);

        check(
            rejected &&
            preserved.size() == 3 &&
            preserved[0].id == 1 &&
            preserved[1].id == 2 &&
            preserved[2].id == 3,
            "Blocked temporary path preserves CSV"
        );

        fs::remove_all(blockedPath);
    }

    // Clean up test files.
    fs::remove_all(directory);

    cout << "\n===== WORKLOAD I/O TEST SUMMARY =====\n";
    cout << "Passed: " << passed << '\n';
    cout << "Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
