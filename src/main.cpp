#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <pango/pangocairo.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "holidays.h"
#include "config.pb.h"

config::CalendarConfig conf;
auto console = spdlog::stdout_logger_mt("console");

struct tm* get_next_day(time_t *t) {
	*t += SECS_PER_DAY;
	return localtime(t);
}

bool is_special_day(struct tm const &timeinfo) {
	for (int i = 0; i < conf.special_day_size(); i++) {
		if (timeinfo.tm_mon == conf.special_day(i).month() - 1 &&
				timeinfo.tm_mday == conf.special_day(i).day()) {
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

PangoLayout* init_pango_layout(cairo_t *cr, int font_size) {
	PangoLayout *layout = pango_cairo_create_layout(cr);
	PangoFontDescription *desc = pango_font_description_from_string("Roboto");
	pango_font_description_set_weight(desc, PANGO_WEIGHT_SEMIBOLD);
	pango_font_description_set_absolute_size(desc, font_size * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free(desc);
	return layout;
}

void draw_text_of_year(cairo_t *cr, int y, const char* text) {
	PangoLayout *layout = init_pango_layout(cr, conf.font_size());
	pango_layout_set_text(layout, text, -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			(conf.year_label_width() - ((double)width / PANGO_SCALE)) / 2,
			(y + 1) * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)height / PANGO_SCALE)) / 2 +
			conf.month_label_height());

	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

int get_day_x(int day_index) {
	return day_index * (conf.cell_size() + conf.cell_margin()) +
		conf.year_label_width();
}

int get_day_y(int year_index) {
	return year_index * (conf.cell_size() + conf.cell_margin()) +
		conf.month_label_height();
}

double draw_text_of_month(cairo_t *cr, int x, const char* text) {
	PangoLayout *layout =
		init_pango_layout(cr, conf.bigger_font_size());
	pango_layout_set_text(layout, text, -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);

	int label_x = get_day_x(x);

	cairo_move_to(cr, label_x,
			(conf.month_label_height() - ((double)height / PANGO_SCALE)) / 2);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
	return label_x + width / PANGO_SCALE;
}

void draw_text_of_day(cairo_t *cr, int x, int y, const char* text) {
	PangoLayout *layout = init_pango_layout(cr, conf.font_size());
	pango_layout_set_text(layout, text, -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			x * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)width / PANGO_SCALE)) / 2 +
			conf.year_label_width(),
			y * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)height / PANGO_SCALE)) / 2 +
			conf.month_label_height());
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

void draw_symbol_of_day(cairo_t *cr, int day_index, int year_index) {
	int x = get_day_x(day_index) + conf.cell_size() / 2;
	int y = get_day_y(year_index) + conf.cell_size() / 2;
	int size = 5;

	cairo_set_line_width(cr, 1);
	cairo_move_to(cr, x - size, y - size);
	cairo_line_to(cr, x + size, y + size);
	cairo_move_to(cr, x + size, y - size);
	cairo_line_to(cr, x - size, y + size);
	cairo_stroke(cr);
}

void draw_rectangle_of_day(cairo_t *cr, int x, int y) {
	cairo_rectangle(cr, get_day_x(x), get_day_y(y),
			conf.cell_size(), conf.cell_size());
}

double cairo_color(int color) {
	return (double) color / 255;
}


void set_rgb(cairo_t *cr, const config::RGB& rgb) {
	cairo_set_source_rgb(cr, cairo_color(rgb.red()),
			cairo_color(rgb.green()), cairo_color(rgb.blue()));
}


void year_label(cairo_t *cr, int this_year) {
	char buf[5];

	set_rgb(cr, conf.rgb_header());

	for (int i = 0; i < conf.num_years(); i++) {
		sprintf(buf, "%d", this_year + i);
		draw_text_of_year(cr, i, buf);
	}
}

void month_label(cairo_t *cr) {
	const char *month_text[] = {
		"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY",
		"AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
	const int days_per_months[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 + 6};

	int d = 0;
	for (int m = 0; m < 12; m++) {
		set_rgb(cr, conf.rgb_header());
		double end_of_label = draw_text_of_month(cr, d, month_text[m]);

		set_rgb(cr, conf.rgb_month_line());
		cairo_move_to(cr,
				end_of_label + conf.cell_size() / 2,
				conf.month_label_height() / 2);

		d += days_per_months[m];

		cairo_line_to(cr,
				get_day_x(d) -
				(m < 11 ? conf.cell_size() : conf.cell_margin()),
				conf.month_label_height() / 2);
		cairo_stroke(cr);
	}
}

void wday_label(cairo_t *cr) {
	const char *wday_text[] = {"M", "T", "W", "Th", "F", "S", "Su"};

	set_rgb(cr, conf.rgb_header());

	for (int d = 0; d < 365 + 6; d++) {
		draw_text_of_day(cr, d, 0, wday_text[d % 7]);
	}
}

int get_wday_index(struct tm const &timeinfo) {
	if (timeinfo.tm_wday == 0) {
		return 6;
	}
	return timeinfo.tm_wday - 1;
}

void year(cairo_t *cr, int y, int year) {
	char buf[4];
	time_t t = get_first_day_of_year_in_sec(year);
	struct tm timeinfo = *localtime(&t);

	int i = get_wday_index(timeinfo);
	while (timeinfo.tm_year == year) {
		// Rectangle
		bool filled = false;
		bool draw_label = true;

		cairo_set_line_width(cr, conf.line_width());
		draw_rectangle_of_day(cr, i, y + 1);
		if (is_special_day(timeinfo)) {
			set_rgb(cr, conf.rgb_header());
			cairo_stroke(cr);
		} else if (timeinfo.tm_wday == 0) {
			set_rgb(cr, conf.rgb_day_background());
			cairo_fill(cr);
		} else if (is_holiday(t)) {
			set_rgb(cr, conf.rgb_header());
			cairo_fill(cr);
			filled = true;
		} else {
			set_rgb(cr, conf.rgb_day_background());
			cairo_fill(cr);
			draw_label = false;
		}

		// Label
		if (filled) {
			set_rgb(cr, conf.rgb_day_background());
		} else {
			set_rgb(cr, conf.rgb_header());
		}
		if (draw_label) {
			sprintf(buf, "%d", timeinfo.tm_mday);
			draw_text_of_day(cr, i, y + 1, buf);
		} else {
			draw_symbol_of_day(cr, i, y + 1);
		}

		timeinfo = *get_next_day(&t);
		i++;
	}
}

bool parse_config() {
	// Verify that the version of the library that we linked
	// against is compatible with the version of the headers we
	// compiled against.
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int fd = open("config.txt", O_RDONLY);
	if (fd < 0) {
		console->error(strerror(errno));
		return false;
	}
	google::protobuf::io::FileInputStream fileInput(fd);
	fileInput.SetCloseOnDelete( true );

	if (!google::protobuf::TextFormat::Parse(&fileInput, &conf)) {
		// protobuf prints error message
		return false;
	}
	return true;
}

int main(int argc, char *argv[])
{
	console->info(cairo_version_string());
	if (!parse_config()) {
		console->error("Error");
		return EXIT_FAILURE;
	}

	cairo_surface_t *surface = NULL;
//	surface = cairo_pdf_surface_create("example.pdf",
	surface = cairo_svg_surface_create("example.svg",
			(366 + 6) * (conf.cell_size() + conf.cell_margin()) +
			conf.year_label_width(),
			(conf.num_years() + 1) *
			(conf.cell_size() + conf.cell_margin()) +
			conf.month_label_height());
	cairo_t *cr = cairo_create(surface);

	cairo_set_line_width(cr, conf.line_width());

	int this_year = get_this_year();
	year_label(cr, this_year + 1900);
	month_label(cr);
	wday_label(cr);
	for (int i = 0; i < conf.num_years(); i++) {
		year(cr, i, this_year + i);
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return EXIT_SUCCESS;
}
