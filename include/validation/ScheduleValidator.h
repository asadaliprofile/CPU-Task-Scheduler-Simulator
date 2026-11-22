
#ifndef SCHEDULE_VALIDATOR_H
#define SCHEDULE_VALIDATOR_H

#include "SchedulingTypes.h"

#include <vector>

// Verify that a scheduling result is mathematically
// and temporally consistent with its input workload.
//
// Throws std::runtime_error if any invariant fails.
void validateSchedule(
    const std::vector<Process>& workload,
    const ScheduleResult& result
);

#endif
