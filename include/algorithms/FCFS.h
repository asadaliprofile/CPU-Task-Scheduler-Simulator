
#ifndef FCFS_H
#define FCFS_H

#include "SchedulingTypes.h"

#include <vector>

ScheduleResult scheduleFCFS(
    const std::vector<Process>& processes
);

#endif
