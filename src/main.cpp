#include <cairo.h>
#include <cairo-svg.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "holidays.h"

#define CELL_WIDTH 30
#define CELL_HEIGHT 30
#define FONT_SIZE 15
#define LINE_WIDTH 3
#define MAX_YEAR 30

#define RGB_SPECIAL_DAY 63,87,101
#define RGB_SUNDAY 189,212,222
#define RGB_HOLIDAY 255,83,13
#define RGB_COMMON_DAY 239,239,239
#define RGB_LINE 255,255,255
#define RGB_NUMBER_BRIGHT 255,255,255
#define RGB_NUMBER_DARK 43,58,66

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

void draw_text(cairo_t *cr, int x, int y, const char* text) {
	int width, height;
	PangoLayout *layout = pango_cairo_create_layout(cr);
	PangoFontDescription *font_description =
		pango_font_description_from_string("Source Code Pro Bold");
	pango_font_description_set_absolute_size(font_description,
			FONT_SIZE * PANGO_SCALE);

	pango_layout_set_font_description (layout, font_description);
	pango_font_description_free(font_description);

	pango_layout_set_text(layout, text, -1);

	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			(x + 0.5) * CELL_WIDTH - ((double)width / PANGO_SCALE) / 2,
			(y + 0.5) * CELL_HEIGHT - ((double)height / PANGO_SCALE) / 2);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}


double cairo_color(int color) {
	return (double) color / 255;
}


void set_rgb(cairo_t *cr, int r, int g, int b) {
	cairo_set_source_rgb(cr, cairo_color(r), cairo_color(g), cairo_color(b));
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
			set_rgb(cr, RGB_SPECIAL_DAY);
			cairo_fill(cr);
			filled = 1;
		} else if (timeinfo->tm_wday == 0) {
			set_rgb(cr, RGB_SUNDAY);
			cairo_fill(cr);
		} else if (is_holiday(t)) {
			set_rgb(cr, RGB_HOLIDAY);
			cairo_fill(cr);
			filled = 1;
		} else {
			set_rgb(cr, RGB_COMMON_DAY);
			cairo_fill(cr);
		}
		timeinfo = localtime(&t);

		cairo_rectangle(cr,
				i * CELL_WIDTH, y * CELL_HEIGHT,
				CELL_WIDTH, CELL_HEIGHT);
		set_rgb(cr, RGB_LINE);
		cairo_stroke(cr);

		// Label
		if (filled) {
			set_rgb(cr, RGB_NUMBER_BRIGHT);
		} else {
			set_rgb(cr, RGB_NUMBER_DARK);
		}
		if (timeinfo->tm_mday == 1) {
			sprintf(buf, "%d", timeinfo->tm_mon + 1);
			draw_text(cr, i, y, buf);
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

	cairo_surface_t *surface = cairo_svg_surface_create("example.svg",
			366 * CELL_WIDTH, MAX_YEAR * CELL_HEIGHT);
	cairo_t *cr = cairo_create(surface);

	cairo_set_line_width(cr, LINE_WIDTH);

	int this_year = get_this_year();
	for (i = 0; i < MAX_YEAR; i++) {
		year(cr, i, this_year + i);
	}

	cairo_destroy(cr);

	cairo_surface_destroy(surface);
}
