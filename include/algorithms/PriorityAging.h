
#ifndef PRIORITY_AGING_H
#define PRIORITY_AGING_H

#include "SchedulingTypes.h"

#include <vector>

ScheduleResult schedulePriorityAgingNonPreemptive(
    const std::vector<Process>& processes,
    int agingInterval
);

ScheduleResult schedulePriorityAgingPreemptive(
    const std::vector<Process>& processes,
    int agingInterval
);

#endif
