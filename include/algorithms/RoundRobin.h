
#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H

#include "SchedulingTypes.h"

#include <vector>

ScheduleResult scheduleRoundRobin(
    const std::vector<Process>& processes,
    int timeQuantum
);

#endif
