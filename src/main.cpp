#include <cairo.h>
#include <cairo-svg.h>
#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "holidays.h"
#include "config.pb.h"

config::CalendarConfig conf;

struct tm* get_next_day(time_t *t) {
	*t += SECS_PER_DAY;
	return localtime(t);
}

bool is_special_day(struct tm *timeinfo) {
	for (int i = 0; i < conf.special_day_size(); i++) {
		if (timeinfo->tm_mon == conf.special_day(i).month() - 1 &&
				timeinfo->tm_mday == conf.special_day(i).day()) {
			return true;
		}
	}
	return false;
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
			conf.font_size() * PANGO_SCALE);

	pango_layout_set_font_description (layout, font_description);
	pango_font_description_free(font_description);

	pango_layout_set_text(layout, text, -1);

	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			(x + 0.5) * conf.cell_size() - ((double)width / PANGO_SCALE) / 2,
			(y + 0.5) * conf.cell_size() - ((double)height / PANGO_SCALE) / 2);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}


double cairo_color(int color) {
	return (double) color / 255;
}


void set_rgb(cairo_t *cr, const config::RGB& rgb) {
	cairo_set_source_rgb(cr, cairo_color(rgb.red()),
			cairo_color(rgb.green()), cairo_color(rgb.blue()));
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
				i * conf.cell_size(), y * conf.cell_size(),
				conf.cell_size(), conf.cell_size());
		if (is_special_day(timeinfo)) {
			set_rgb(cr, conf.rgb_special_day());
			cairo_fill(cr);
			filled = 1;
		} else if (timeinfo->tm_wday == 0) {
			set_rgb(cr, conf.rgb_sunday());
			cairo_fill(cr);
		} else if (is_holiday(t)) {
			set_rgb(cr, conf.rgb_holiday());
			cairo_fill(cr);
			filled = 1;
		} else {
			set_rgb(cr, conf.rgb_common_day());
			cairo_fill(cr);
		}
		timeinfo = localtime(&t);

		cairo_rectangle(cr,
				i * conf.cell_size(), y * conf.cell_size(),
				conf.cell_size(), conf.cell_size());
		set_rgb(cr, conf.rgb_line());
		cairo_stroke(cr);

		// Label
		if (filled) {
			set_rgb(cr, conf.rgb_number_bright());
		} else {
			set_rgb(cr, conf.rgb_number_dark());
		}
		if (timeinfo->tm_mday == 1) {
			sprintf(buf, "%d", timeinfo->tm_mon + 1);
			draw_text(cr, i, y, buf);
		}

		timeinfo = get_next_day(&t);
	}
}

bool parse_config() {
	// Verify that the version of the library that we linked
	// against is compatible with the version of the headers we
	// compiled against.
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int fd = open("config.txt", O_RDONLY);
	if (fd < 0) {
		return false;
	}
	google::protobuf::io::FileInputStream fileInput(fd);
	fileInput.SetCloseOnDelete( true );

	if (!google::protobuf::TextFormat::Parse(&fileInput, &conf)) {
		return false;
	}
	return true;
}

int main(int argc, char *argv[])
{
	if (!parse_config()) {
		return -1;
	}

	cairo_surface_t *surface = cairo_svg_surface_create("example.svg",
			366 * conf.cell_size(), conf.num_years() * conf.cell_size());
	cairo_t *cr = cairo_create(surface);

	cairo_set_line_width(cr, conf.line_width());

	int this_year = get_this_year();
	for (int i = 0; i < conf.num_years(); i++) {
		year(cr, i, this_year + i);
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}
