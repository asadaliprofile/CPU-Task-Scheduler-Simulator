
#ifndef WORKLOAD_IO_H
#define WORKLOAD_IO_H

#include "Process.h"

#include <filesystem>
#include <vector>

// Saves process input data to a CSV file.
// Throws an exception if validation or writing fails.
void saveWorkloadCSV(
    const std::vector<Process>& processes,
    const std::filesystem::path& filePath
);

// Loads and validates an entire CSV file.
// Returns a new vector only after successful validation.
// Throws an exception on malformed or invalid input.
std::vector<Process> loadWorkloadCSV(
    const std::filesystem::path& filePath
);

#endif
