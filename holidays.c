#include "holidays.h"

int is_fixed_holiday(time_t t) {
	struct tm timeinfo = *localtime(&t);

	// New Year's Day (January 1)
	if (timeinfo.tm_mon == 0 && timeinfo.tm_mday == 1) {
		return 1;
	}
	// Independence Day (July 4)
	if (timeinfo.tm_mon == 6 && timeinfo.tm_mday == 4) {
		return 1;
	}
	// Veterans Day (November 11)
	if (timeinfo.tm_mon == 10 && timeinfo.tm_mday == 11) {
		return 1;
	}
	// December 25 (Christmas)
	if (timeinfo.tm_mon == 11 && timeinfo.tm_mday == 25) {
		return 1;
	}
	return 0;
}

int is_last_week(time_t t, struct tm *timeinfo_this_week) {
	t += SECS_PER_DAY * 7;
	return timeinfo_this_week->tm_mon + 1 == localtime(&t)->tm_mon;
}

int is_holiday(time_t t) {
	struct tm timeinfo = *localtime(&t);
	int mweek = (timeinfo.tm_mday - 1) / 7 + 1;

	// If today is not a weekend, check if today is a fixed holiday
	if (0 < timeinfo.tm_wday && timeinfo.tm_wday < 6) {
		if (is_fixed_holiday(t)) {
			return 1;
		}
	}
	// If today is friday, see if tomorrow is a fixed holiday
	if (timeinfo.tm_wday == 5 &&
			is_fixed_holiday(t + SECS_PER_DAY)) {
		return 1;
	}
	// If today is monday, see if yesterday was a fixed holiday
	if (timeinfo.tm_wday == 1 &&
			is_fixed_holiday(t - SECS_PER_DAY)) {
		return 1;
	}

	// Birthday of Martin Luther King, Jr. (Third Monday in January)
	if (timeinfo.tm_mon == 0 && timeinfo.tm_wday == 1 && mweek == 3) {
		return 1;
	}
	// Washington's Birthday (Third Monday in February)
	if (timeinfo.tm_mon == 1 && timeinfo.tm_wday == 1 && mweek == 3) {
		return 1;
	}
	// Memorial Day (Last Monday in May)
	if (timeinfo.tm_mon == 4 && timeinfo.tm_wday == 1 &&
			is_last_week(t, &timeinfo)) {
		return 1;
	}
	// Labor Day (First Monday in September)
	if (timeinfo.tm_mon == 8 && timeinfo.tm_wday == 1 && mweek == 1) {
		return 1;
	}
	// Columbus Day (Second Monday in October)
	if (timeinfo.tm_mon == 9 && timeinfo.tm_wday == 1 && mweek == 2) {
		return 1;
	}
	// Thanksgiving Day (Fourth Thursday in November)
	if (timeinfo.tm_mon == 10 && timeinfo.tm_wday == 4 && mweek == 4) {
		return 1;
	}
	return 0;
}
