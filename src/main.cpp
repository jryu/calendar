#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <fcntl.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "holidays.h"
#include "config.pb.h"

config::CalendarConfig conf;
auto console = spdlog::stdout_logger_mt("console");

const int days_per_months[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 + 6};

const std::string BLACK_HEX_CODE = "#000000";
const std::string WHITE_HEX_CODE = "#ffffff";

double month_label_x[12];

struct tm* get_next_day(time_t *t) {
	*t += SECS_PER_DAY;
	return localtime(t);
}

bool is_every_tenth_year(int first_year, struct tm const &timeinfo) {
	int this_year = timeinfo.tm_year + 1900;
	return (this_year - first_year) % 10 == 0;
}

const config::SpecialDay* get_special_day(struct tm const &timeinfo) {
	const config::SpecialDay* special_day = nullptr;
	for (int i = 0; i < conf.special_day_size(); i++) {
		const config::SpecialDay& d = conf.special_day(i);
		if (timeinfo.tm_mon == d.month() - 1 &&
				timeinfo.tm_mday == d.day() &&
				(!d.has_year() || timeinfo.tm_year + 1900 == d.year())) {
			if (special_day == nullptr ||
					(d.has_first_year() &&
					 is_every_tenth_year(d.first_year(), timeinfo))) {
				special_day = &d;
			}
		}
	}
	return special_day;
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

PangoLayout* init_pango_layout(cairo_t *cr, const std::string& font_family,
		double font_size, PangoWeight weight) {
	PangoLayout *layout = pango_cairo_create_layout(cr);
	PangoFontDescription *desc =
		pango_font_description_from_string(font_family.c_str());
	pango_font_description_set_weight(desc, weight);
	pango_font_description_set_absolute_size(desc, font_size * PANGO_SCALE);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free(desc);
	return layout;
}

void draw_text_of_year(cairo_t *cr, int y, const char* text, PangoWeight weight) {
	PangoLayout *layout = init_pango_layout(cr, conf.number_font_family(),
			conf.font_size(), weight);
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

double get_day_x(int day_index) {
	return day_index * (conf.cell_size() + conf.cell_margin()) +
		conf.year_label_width();
}

double get_day_y(int year_index) {
	return year_index * (conf.cell_size() + conf.cell_margin()) +
		conf.month_label_height();
}

int render_svg(const std::string& svg, cairo_t *cr, int x, int y)
{
	cairo_save(cr);

	GError *error = NULL;
	RsvgHandle *handle = rsvg_handle_new_from_data(
			reinterpret_cast<const guint8*>(svg.c_str()),
			svg.length(), &error);
	if (handle == NULL) {
		console->error(error->message);
	}

	cairo_translate(cr, get_day_x(x) + 3, get_day_y(y) + 3);

	RsvgDimensionData dimensions;
	rsvg_handle_get_dimensions(handle, &dimensions);

	double dst_size = conf.cell_size() - 6;
	double scale_factor =
		std::min(dst_size / dimensions.width, dst_size / dimensions.height);
	cairo_scale(cr, scale_factor, scale_factor);

	rsvg_handle_render_cairo(handle, cr);

	cairo_restore(cr);
}

double draw_text_of_month(cairo_t *cr, double x, const char* text) {
	PangoLayout *layout = init_pango_layout(cr, conf.header_font_family(),
			conf.bigger_font_size(), PANGO_WEIGHT_SEMIBOLD);
	pango_layout_set_text(layout, text, -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);

	cairo_move_to(cr, x,
			(conf.month_label_height() - ((double)height / PANGO_SCALE) +
			 conf.cell_margin()) / 2);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
	return x + width / PANGO_SCALE;
}

double draw_text_of_day(cairo_t *cr, int x, int y, const char* text,
		const std::string& font_family, PangoWeight weight) {
	PangoLayout *layout = init_pango_layout(cr, font_family, conf.font_size(),
			weight);
	pango_layout_set_text(layout, text, -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	double text_x = x * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)width / PANGO_SCALE)) / 2 +
			conf.year_label_width();
	cairo_move_to(cr,
			text_x,
			y * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)height / PANGO_SCALE)) / 2 +
			conf.month_label_height());
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
	return text_x;
}

void draw_text_on_bottom_left(cairo_t *cr) {
	PangoLayout *layout = init_pango_layout(cr, conf.quote_font_family(),
			conf.font_size(), PANGO_WEIGHT_NORMAL);
	pango_layout_set_text(layout, conf.bottom_left_label().c_str(), -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			conf.year_label_width(),
			(conf.num_years() + 1) * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)height / PANGO_SCALE)) / 2 +
			conf.month_label_height());
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

void draw_text_on_bottom_right(cairo_t *cr) {
	PangoLayout *layout = init_pango_layout(cr, conf.quote_font_family(),
			conf.font_size(), PANGO_WEIGHT_NORMAL);
	pango_layout_set_text(layout, conf.bottom_right_label().c_str(), -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			(366 + 5) * (conf.cell_size() + conf.cell_margin()) -
			conf.cell_margin() * 2 +
			conf.year_label_width() - ((double)width / PANGO_SCALE),
			(conf.num_years() + 1) * (conf.cell_size() + conf.cell_margin()) +
			(conf.cell_size() - ((double)height / PANGO_SCALE)) / 2 +
			conf.month_label_height());
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

void draw_symbol_of_day(cairo_t *cr, int day_index, int year_index, int month) {
	double x = get_day_x(day_index) + conf.cell_size() / 2;
	double y = get_day_y(year_index) + conf.cell_size() / 2;

	if ((year_index + month) % 2) {
		double size = 1.5;
		cairo_set_line_width(cr, 0.5);
		cairo_move_to(cr, x - size, y - size);
		cairo_line_to(cr, x + size, y + size);
		cairo_move_to(cr, x + size, y - size);
		cairo_line_to(cr, x - size, y + size);
		cairo_stroke(cr);
	} else {
		cairo_move_to(cr, x, y);
		cairo_arc(cr, x, y, 1, 0, 2*M_PI);
		cairo_fill(cr);
	}
}

void draw_rectangle_of_day(cairo_t *cr, int day_index, int year_index) {
	double x = get_day_x(day_index);
	double y = get_day_y(year_index);
	double size = conf.cell_size();
	double r = conf.cell_size() / 8;
	double degrees = M_PI / 180.0;

	cairo_new_sub_path(cr);
	cairo_arc(cr, x + size - r, y + r, r, -90 * degrees, 0 * degrees);
	cairo_arc(cr, x + size - r, y + size - r, r, 0 * degrees, 90 * degrees);
	cairo_arc(cr, x + r, y + size - r, r, 90 * degrees, 180 * degrees);
	cairo_arc(cr, x + r, y + r, r, 180 * degrees, 270 * degrees);
	cairo_close_path(cr);
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

	for (int i = 0; i < conf.num_years(); i++) {
		int year = this_year + i;
		sprintf(buf, "%d", year);
		if (year % 5) {
			set_rgb(cr, conf.rgb_header());
			draw_text_of_year(cr, i, buf, PANGO_WEIGHT_NORMAL);
		} else {
			cairo_set_source_rgb(cr, 0, 0, 0);
			draw_text_of_year(cr, i, buf, PANGO_WEIGHT_SEMIBOLD);
		}
	}
}

void month_label(cairo_t *cr) {
	const char *month_text[] = {
		"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY",
		"AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};

	int d = 0;
	double month_line_y = (conf.month_label_height() + conf.cell_margin()) / 2;
	for (int m = 0; m < 12; m++) {
		set_rgb(cr, conf.rgb_header());
		double end_of_label =
			draw_text_of_month(cr, month_label_x[m], month_text[m]);

		cairo_set_line_width(cr, 1);
		set_rgb(cr, conf.rgb_month_line());
		cairo_move_to(cr,
				end_of_label + conf.cell_size() / 2,
				month_line_y);

		d += days_per_months[m];

		cairo_line_to(cr,
				get_day_x(d) -
				(m < 11 ? conf.cell_size() : conf.cell_margin()),
				month_line_y);
		cairo_stroke(cr);
	}
}

void wday_label(cairo_t *cr) {
	const char *wday_text[] = {"M", "T", "W", "Th", "F", "S", "Su"};

	int month_index = 0;
	int next_month_d = 0;
	for (int d = 0; d < 365 + 6; d++) {
		int wday_index = d % 7;
		double text_x;
		if (wday_index == 6) {
			set_rgb(cr, conf.rgb_header_sunday());
			text_x = draw_text_of_day(cr, d, 0, wday_text[wday_index],
					conf.header_font_family(), PANGO_WEIGHT_SEMIBOLD);
		} else {
			set_rgb(cr, conf.rgb_header());
			text_x = draw_text_of_day(cr, d, 0, wday_text[wday_index],
					conf.header_font_family(), PANGO_WEIGHT_NORMAL);
		}
		if (d == next_month_d) {
			month_label_x[month_index] = text_x;
			next_month_d += days_per_months[month_index];
			month_index++;
		}
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
		const config::SpecialDay* special_day = get_special_day(timeinfo);
		bool draw_label = true;

		if (special_day != nullptr) {
			std::string svg = special_day->svg();
			if (special_day->has_year() || (
						special_day->has_first_year() &&
						is_every_tenth_year(special_day->first_year(), timeinfo))) {
				draw_rectangle_of_day(cr, i, y + 1);
				if (special_day->has_rgb()) {
					set_rgb(cr, special_day->rgb());
				} else {
					set_rgb(cr, conf.rgb_holiday());
				}
				cairo_fill(cr);

				svg.replace(svg.find(BLACK_HEX_CODE), BLACK_HEX_CODE.length(),
						WHITE_HEX_CODE);
			}
			render_svg(svg, cr, i, y + 1);
			draw_label = false;
		} else if (timeinfo.tm_wday == 0) {
			cairo_set_source_rgb(cr, 0, 0, 0);
		} else if (is_holiday(t)) {
			draw_rectangle_of_day(cr, i, y + 1);
			set_rgb(cr, conf.rgb_holiday());
			cairo_fill(cr);

			cairo_set_source_rgb(cr, 1, 1, 1);
		} else {
			cairo_set_source_rgb(cr, 0, 0, 0);
			draw_label = false;
		}

		// Label
		if (draw_label) {
			sprintf(buf, "%d", timeinfo.tm_mday);
			draw_text_of_day(cr, i, y + 1, buf, conf.number_font_family(),
					PANGO_WEIGHT_SEMIBOLD);
		} else if (special_day == nullptr) {
			draw_symbol_of_day(cr, i, y + 1, timeinfo.tm_mon);
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

double calc_offset_width() {
	if (conf.first_month() <= 1) {
		return 0;
	}

	int d = 0;
	for (int m = 0; m < conf.first_month() - 1; m++) {
		d += days_per_months[m];
	}
	return d * (conf.cell_size() + conf.cell_margin()) +
		conf.year_label_width();
}

double calc_visible_width() {
	int d = 0;
	for (int m = conf.first_month() - 1;
			m < conf.first_month() + conf.num_months() - 1; m++) {
		d += days_per_months[m];
		if (m == 11) {
			d++;
		}
	}
	double width = d * (conf.cell_size() + conf.cell_margin());
	if (conf.first_month() == 1) {
		width += conf.year_label_width() - conf.cell_margin();
	}
	width += conf.cell_margin();
	return width;
}

void draw_dashes(cairo_t *cr, double x, double y, double width, double height)
{
	set_rgb(cr, conf.rgb_header());
	cairo_set_line_width(cr, 1);
	double dashes[] = {5, 5};
	cairo_set_dash(cr, dashes, 2, 0);
	if (conf.dotted_line()) {
		cairo_rectangle(cr, x, y, width, height);
		cairo_stroke(cr);
	}
	if (conf.has_vertical_dotted_line_x()) {
		cairo_move_to(cr, x + conf.vertical_dotted_line_x(), 0);
		cairo_line_to(cr, x + conf.vertical_dotted_line_x(), height);
		cairo_stroke(cr);
	}
}

int main(int argc, char *argv[])
{
	if (!parse_config()) {
		console->error("Error");
		return EXIT_FAILURE;
	}

	int surface_width = (366 + 6) * (conf.cell_size() + conf.cell_margin()) +
			conf.year_label_width();

	double offset_width = calc_offset_width();
	double visible_width = calc_visible_width();
	double print_width = visible_width;
	if (conf.has_vertical_dotted_line_x()) {
		print_width = conf.vertical_dotted_line_x();
	}

	int surface_height = (conf.num_years() + 2) *
			(conf.cell_size() + conf.cell_margin()) +
			conf.month_label_height() + conf.cell_margin();
	console->info("Size: {} x {}", surface_width, surface_height);
	console->info("Offset: {}", offset_width);
	console->info("Visible: {}", visible_width);
	cairo_surface_t *surface = NULL;
	switch (conf.output_type()) {
		case config::OutputType::PDF:
			surface = cairo_pdf_surface_create("example.pdf",
					print_width, surface_height);
			break;
		case config::OutputType::PNG:
			surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					print_width, surface_height);
			break;
		default:
			surface = cairo_svg_surface_create("example.svg",
					print_width, surface_height);
			break;
	}
	cairo_t *cr = cairo_create(surface);

	if (offset_width != 0) {
		cairo_translate(cr, -offset_width + conf.cell_margin(), 0);
	}

	int this_year = get_this_year();
	year_label(cr, this_year + 1900);
	wday_label(cr);
	month_label(cr);
	for (int i = 0; i < conf.num_years(); i++) {
		year(cr, i, this_year + i);
	}

	set_rgb(cr, conf.rgb_header());
	draw_text_on_bottom_left(cr);
	draw_text_on_bottom_right(cr);

	draw_dashes(cr,
			std::max(0.0, offset_width - conf.cell_margin()), 0,
			visible_width, surface_height);

	if (conf.output_type() == config::OutputType::PNG) {
		cairo_surface_write_to_png(surface, "example.png");
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	return EXIT_SUCCESS;
}
