
#ifndef SJF_H
#define SJF_H

#include "SchedulingTypes.h"

#include <vector>

ScheduleResult scheduleSJF(
    const std::vector<Process>& processes
);

#endif
