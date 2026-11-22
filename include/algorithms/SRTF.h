
#ifndef SRTF_H
#define SRTF_H

#include "SchedulingTypes.h"

#include <vector>

ScheduleResult scheduleSRTF(
    const std::vector<Process>& processes
);

#endif
