
#ifndef OUTPUT_FORMATTER_H
#define OUTPUT_FORMATTER_H

#include "SchedulingTypes.h"

#include <string>
#include <vector>

// Associates an algorithm name with its scheduling result.
struct ComparisonEntry {
    std::string name;
    const ScheduleResult* result;
};

void printProcessTable(
    const std::string& algorithm,
    const ScheduleResult& result
);

void printGanttChart(
    const std::string& algorithm,
    const ScheduleResult& result
);

void printStatistics(
    const std::string& algorithm,
    const ScheduleResult& result
);

void displaySchedule(
    const std::string& algorithm,
    const ScheduleResult& result
);

void printComparison(
    const std::vector<ComparisonEntry>& comparisons
);

#endif
