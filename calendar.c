#include <cairo.h>
#include <stdio.h>
#include <time.h>

#define CELL_WIDTH 20
#define CELL_HEIGHT 20
#define FONT_SIZE 10
#define MAX_YEAR 30

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
		cairo_set_source_rgb(cr, 0, 0, 0);

		// Rectangle
		cairo_rectangle(cr,
				i * CELL_WIDTH, y * CELL_HEIGHT,
				CELL_WIDTH, CELL_HEIGHT);
		if (timeinfo->tm_wday == 0) {
			cairo_fill(cr);
			cairo_set_source_rgb(cr, 1, 1, 1);
		} else {
			cairo_stroke(cr);
		}

		// Label
		if (timeinfo->tm_mday == 1) {
			sprintf(buf, "%d", timeinfo->tm_mon + 1);
			cairo_move_to(cr,
					i * CELL_WIDTH + CELL_WIDTH / 2 - FONT_SIZE / 3,
					y * CELL_HEIGHT + FONT_SIZE + (CELL_HEIGHT - FONT_SIZE) / 2);
			cairo_show_text(cr, buf);
		}

		t += 60 * 60 * 24;
		timeinfo = localtime(&t);
	}
}

int main(int argc, char *argv[])
{
	int i;
	cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
			365 * CELL_WIDTH, MAX_YEAR * CELL_HEIGHT);
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
