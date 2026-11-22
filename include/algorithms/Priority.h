
#ifndef PRIORITY_H
#define PRIORITY_H

#include "SchedulingTypes.h"

#include <vector>

// Smaller numeric value means higher priority.

ScheduleResult schedulePriorityNonPreemptive(
    const std::vector<Process>& processes
);

ScheduleResult schedulePriorityPreemptive(
    const std::vector<Process>& processes
);

#endif
