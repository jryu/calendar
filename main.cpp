#include <cairo.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "holidays.h"

#define CELL_WIDTH 20
#define CELL_HEIGHT 20
#define FONT_SIZE 10
#define MAX_YEAR 30

int special_days[10][2];

struct tm* get_next_day(time_t *t) {
	*t += SECS_PER_DAY;
	return localtime(t);
}

int is_special_day(struct tm *timeinfo) {
	int i = 0;
	while (special_days[i][0] != -1) {
		if (timeinfo->tm_mon == special_days[i][0] - 1 &&
				timeinfo->tm_mday == special_days[i][1]) {
			return 1;
		}
		i++;
	}
	return 0;
}

int get_this_year() {
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	return timeinfo->tm_year;
}

time_t get_first_day_of_year_in_sec(int year) {
	struct tm timeinfo = {0};

	timeinfo.tm_year = year;
	timeinfo.tm_mday = 1;

	return mktime(&timeinfo);
}

void year(cairo_t *cr, int y, int year) {
	int i;
	char buf[4];
	time_t t = get_first_day_of_year_in_sec(year);
	struct tm *timeinfo = localtime(&t);

	for (i = 0; timeinfo->tm_year == year; i++) {
		// Rectangle
		int filled = 0;
		cairo_rectangle(cr,
				i * CELL_WIDTH, y * CELL_HEIGHT,
				CELL_WIDTH, CELL_HEIGHT);
		if (is_special_day(timeinfo)) {
			cairo_set_source_rgb(cr, 0, 0, 1);
			cairo_fill(cr);
			filled = 1;
		} else if (timeinfo->tm_wday == 0) {
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_fill(cr);
			filled = 1;
		} else if (is_holiday(t)) {
			cairo_set_source_rgb(cr, 1, 0, 0);
			cairo_fill(cr);
			filled = 1;
		}
		timeinfo = localtime(&t);

		cairo_rectangle(cr,
				i * CELL_WIDTH, y * CELL_HEIGHT,
				CELL_WIDTH, CELL_HEIGHT);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_stroke(cr);

		// Label
		if (filled) {
			cairo_set_source_rgb(cr, 1, 1, 1);
		}
		if (timeinfo->tm_mday == 1) {
			sprintf(buf, "%d", timeinfo->tm_mon + 1);
			cairo_move_to(cr,
					i * CELL_WIDTH + CELL_WIDTH / 2 - FONT_SIZE / 3,
					y * CELL_HEIGHT + FONT_SIZE + (CELL_HEIGHT - FONT_SIZE) / 2);
			cairo_show_text(cr, buf);
		}

		timeinfo = get_next_day(&t);
	}
}

int main(int argc, char *argv[])
{
	int i = 0;
	while (scanf("%d/%d\n", &special_days[i][0], &special_days[i][1]) == 2) {
		i++;
	}
	special_days[i][0] = -1;

	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			366 * CELL_WIDTH, MAX_YEAR * CELL_HEIGHT);
	cairo_t *cr = cairo_create(surface);

	cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, FONT_SIZE);

	cairo_set_line_width(cr, 1);

	int this_year = get_this_year();
	for (i = 0; i < MAX_YEAR; i++) {
		year(cr, i, this_year + i);
	}

	cairo_destroy(cr);

	cairo_status_t status = cairo_surface_write_to_png(surface, "a.png");
	if (status != CAIRO_STATUS_SUCCESS) {
		puts(cairo_status_to_string(status));
	}

	cairo_surface_destroy(surface);
	return status;
}
