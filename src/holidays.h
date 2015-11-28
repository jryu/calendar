#ifndef HOLIDAYS_H
#define HOLIDAYS_H

#include <time.h>

#define SECS_PER_DAY (60 * 60 * 24)

int is_holiday(time_t t);

#endif	// HOLIDAYS_H
