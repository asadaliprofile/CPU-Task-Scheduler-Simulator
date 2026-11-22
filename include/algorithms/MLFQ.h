
#ifndef MLFQ_H
#define MLFQ_H

#include "SchedulingTypes.h"

#include <array>
#include <vector>

ScheduleResult scheduleMLFQ(
    const std::vector<Process>& processes,
    const std::array<int, 3>& timeQuanta = {2, 4, 8},
    int boostInterval = 20
);

#endif
