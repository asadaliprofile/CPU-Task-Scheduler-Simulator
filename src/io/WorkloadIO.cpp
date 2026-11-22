
#include "io/WorkloadIO.h"

#include <charconv>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

namespace {

constexpr size_t MAX_PROCESSES = 1000;
constexpr size_t MAX_NAME_LENGTH = 24;
constexpr size_t MAX_LINE_LENGTH = 16384;

constexpr int MAX_INPUT = 1000000;
constexpr int MIN_PRIORITY = -1000;
constexpr int MAX_PRIORITY = 1000;

const string CSV_HEADER =
    "id,name,arrivalTime,burstTime,priority";

string trim(const string& value) {
    size_t first = 0;
    size_t last = value.size();

    while (first < last &&
           isspace(static_cast<unsigned char>(
               value[first]
           ))) {
        ++first;
    }

    while (last > first &&
           isspace(static_cast<unsigned char>(
               value[last - 1]
           ))) {
        --last;
    }

    return value.substr(first, last - first);
}

void validateProcess(
    const Process& p,
    unordered_set<int>& ids,
    unordered_set<string>& names
) {
    if (p.id < 1 || p.id > MAX_INPUT) {
        throw invalid_argument(
            "Process ID is out of range."
        );
    }

    if (p.name.empty() ||
        p.name.size() > MAX_NAME_LENGTH ||
        trim(p.name) != p.name ||
        p.name == "IDLE" ||
        p.name.find('\n') != string::npos ||
        p.name.find('\r') != string::npos) {

        throw invalid_argument(
            "Invalid process name."
        );
    }

    if (p.arrivalTime < 0 ||
        p.arrivalTime > MAX_INPUT) {
        throw invalid_argument(
            "Arrival time is out of range."
        );
    }

    if (p.burstTime < 1 ||
        p.burstTime > MAX_INPUT) {
        throw invalid_argument(
            "Burst time is out of range."
        );
    }

    if (p.priority < MIN_PRIORITY ||
        p.priority > MAX_PRIORITY) {
        throw invalid_argument(
            "Priority is out of range."
        );
    }

    if (!ids.insert(p.id).second) {
        throw invalid_argument(
            "Duplicate process ID."
        );
    }

    if (!names.insert(p.name).second) {
        throw invalid_argument(
            "Duplicate process name."
        );
    }
}

void validateWorkload(
    const vector<Process>& processes
) {
    if (processes.size() > MAX_PROCESSES) {
        throw invalid_argument(
            "Too many processes."
        );
    }

    unordered_set<int> ids;
    unordered_set<string> names;

    for (const auto& p : processes) {
        validateProcess(p, ids, names);
    }
}

int parseInteger(
    const string& field,
    int minimum,
    int maximum
) {
    const string value = trim(field);

    if (value.empty()) {
        throw invalid_argument(
            "Missing integer field."
        );
    }

    int number = 0;

    const char* begin = value.data();
    const char* end = begin + value.size();

    const auto parsed =
        from_chars(begin, end, number);

    if (parsed.ec != errc{} ||
        parsed.ptr != end ||
        number < minimum ||
        number > maximum) {

        throw invalid_argument(
            "Invalid integer field: " + value
        );
    }

    return number;
}

// Parse one CSV record.
//
// Quoted fields may contain commas and escaped
// double quotation marks ("").
//
// Embedded line breaks are not supported.
vector<string> parseCSVLine(const string& line) {
    vector<string> fields;
    size_t position = 0;

    while (true) {
        string field;

        if (position < line.size() &&
            line[position] == '"') {

            // Quoted field
            ++position;
            bool closed = false;

            while (position < line.size()) {
                const char c = line[position++];

                if (c == '"') {
                    if (position < line.size() &&
                        line[position] == '"') {
                        field += '"';
                        ++position;
                    }
                    else {
                        closed = true;
                        break;
                    }
                }
                else {
                    field += c;
                }
            }

            if (!closed) {
                throw invalid_argument(
                    "Unclosed CSV quotation mark."
                );
            }

            if (position < line.size() &&
                line[position] != ',') {
                throw invalid_argument(
                    "Unexpected characters after CSV quote."
                );
            }
        }
        else {
            // Unquoted field
            while (position < line.size() &&
                   line[position] != ',') {

                if (line[position] == '"') {
                    throw invalid_argument(
                        "Unexpected CSV quotation mark."
                    );
                }

                field += line[position++];
            }
        }

        fields.push_back(field);

        if (position == line.size()) {
            break;
        }

        // Skip separator.
        ++position;

        // Preserve an empty trailing field.
        if (position == line.size()) {
            fields.emplace_back("");
            break;
        }
    }

    return fields;
}

string escapeCSV(const string& value) {
    if (value.find_first_of(",\"") ==
        string::npos) {
        return value;
    }

    string escaped = "\"";

    for (char c : value) {
        if (c == '"') {
            escaped += "\"\"";
        }
        else {
            escaped += c;
        }
    }

    escaped += '"';
    return escaped;
}

} // namespace


void saveWorkloadCSV(
    const vector<Process>& processes,
    const filesystem::path& filePath
) {
    namespace fs = std::filesystem;

    // Validate before performing filesystem changes.
    validateWorkload(processes);

    if (filePath.empty()) {
        throw invalid_argument(
            "Output file path cannot be empty."
        );
    }

    const fs::path parent = filePath.parent_path();

    if (!parent.empty()) {
        fs::create_directories(parent);
    }

    // Keep temporary and backup files alongside
    // the destination, on the same filesystem.
    fs::path temporaryPath = filePath;
    temporaryPath += ".tmp";

    fs::path backupPath = filePath;
    backupPath += ".bak";

    // Never overwrite files left by another save
    // operation or an interrupted replacement.
    if (fs::exists(temporaryPath) ||
        fs::exists(backupPath)) {
        throw runtime_error(
            "Temporary or backup file already exists. "
            "Check it before retrying."
        );
    }

    const bool destinationExists = fs::exists(filePath);

    if (destinationExists &&
        !fs::is_regular_file(filePath)) {
        throw runtime_error(
            "Destination is not a regular file."
        );
    }

    // Step 1: Write the complete CSV to a
    // temporary file, leaving the original untouched.
    try {
        ofstream file(
            temporaryPath,
            ios::out | ios::trunc
        );

        if (!file) {
            throw runtime_error(
                "Cannot create temporary CSV file."
            );
        }

        file << CSV_HEADER << '\n';

        for (const auto& p : processes) {
            file << p.id << ','
                 << escapeCSV(p.name) << ','
                 << p.arrivalTime << ','
                 << p.burstTime << ','
                 << p.priority << '\n';
        }

        file.flush();

        if (!file) {
            throw runtime_error(
                "Failed while writing temporary CSV."
            );
        }

        file.close();

        if (!file) {
            throw runtime_error(
                "Failed to close temporary CSV."
            );
        }
    }
    catch (...) {
        // Remove an incomplete temporary file.
        std::error_code error;
        fs::remove(temporaryPath, error);
        throw;
    }

    bool originalMoved = false;

    try {
        // Step 2: Preserve the previous destination.
        if (destinationExists) {
            fs::rename(filePath, backupPath);
            originalMoved = true;
        }

        // Step 3: Install the completed CSV.
        fs::rename(temporaryPath, filePath);
    }
    catch (...) {
        // Attempt to restore the previous destination.
        bool restorationFailed = false;

        if (originalMoved) {
            try {
                fs::rename(backupPath, filePath);
            }
            catch (...) {
                restorationFailed = true;
            }
        }

        std::error_code cleanupError;
        fs::remove(temporaryPath, cleanupError);

        if (restorationFailed) {
            throw runtime_error(
                "CSV replacement and restoration failed. "
                "The original may remain at: " +
                backupPath.string()
            );
        }

        throw;
    }

    // Step 4: After installing the new file,
    // remove the old backup.
    if (originalMoved) {
        std::error_code cleanupError;
        fs::remove(backupPath, cleanupError);

        if (cleanupError) {
            // The new CSV is already installed.
            // Preserve the backup for manual cleanup.
            // Do not report that writing itself failed.
        }
    }
}

vector<Process> loadWorkloadCSV(
    const filesystem::path& filePath
) {
    ifstream file(filePath, ios::binary);

    if (!file) {
        throw runtime_error(
            "Cannot open input CSV file."
        );
    }

    string line;

    if (!getline(file, line)) {
        throw invalid_argument(
            "CSV file is empty."
        );
    }

    // Accept Windows CRLF line endings.
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    if (line != CSV_HEADER) {
        throw invalid_argument(
            "Invalid CSV header."
        );
    }

    vector<Process> loaded;
    unordered_set<int> ids;
    unordered_set<string> names;

    size_t lineNumber = 1;

    while (getline(file, line)) {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.size() > MAX_LINE_LENGTH) {
            throw invalid_argument(
                "CSV row is too long."
            );
        }

        try {
            const auto fields = parseCSVLine(line);

            if (fields.size() != 5) {
                throw invalid_argument(
                    "Expected exactly five columns."
                );
            }

            Process p{};

            p.id = parseInteger(
                fields[0], 1, MAX_INPUT
            );

            p.name = fields[1];

            p.arrivalTime = parseInteger(
                fields[2], 0, MAX_INPUT
            );

            p.burstTime = parseInteger(
                fields[3], 1, MAX_INPUT
            );

            p.priority = parseInteger(
                fields[4],
                MIN_PRIORITY,
                MAX_PRIORITY
            );

            validateProcess(p, ids, names);
            loaded.push_back(std::move(p));

            if (loaded.size() > MAX_PROCESSES) {
                throw invalid_argument(
                    "Too many processes."
                );
            }
        }
        catch (const invalid_argument& error) {
            throw invalid_argument(
                "CSV line " + to_string(lineNumber) +
                ": " + error.what()
            );
        }
    }

    if (file.bad()) {
        throw runtime_error(
            "Error while reading CSV file."
        );
    }

    // Return only after the entire CSV is validated.
    return loaded;
}
