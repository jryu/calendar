#include <stdio.h>
#include <cairo.h>

#define CELL_WIDTH 80
#define CELL_HEIGHT 80
#define FONT_SIZE 20
#define MAX_YEAR 12

void year(cairo_t *cr, int year) {
	int i;
	char buf[4];
	for (i = 0; i < 365; i++) {
		cairo_set_source_rgb(cr, 0, 0, 0);

		// Rectangle
		cairo_rectangle(cr,
				i * CELL_WIDTH, year * CELL_HEIGHT,
				CELL_WIDTH, CELL_HEIGHT);
		if (i % 2) {
			cairo_stroke(cr);
		} else {
			cairo_fill(cr);
		}

		// Label
		sprintf(buf, "%d", i + 1);
		if (i % 2 == 0) {
			cairo_set_source_rgb(cr, 1, 1, 1);
		}
		cairo_move_to(cr,
				i * CELL_WIDTH + CELL_WIDTH / 2 - FONT_SIZE,
				year * CELL_HEIGHT + FONT_SIZE + (CELL_HEIGHT - FONT_SIZE) / 2);
		cairo_show_text(cr, buf);
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

	for (i = 0; i < MAX_YEAR; i++) {
		year(cr, i);
	}

	cairo_destroy(cr);

	cairo_status_t status = cairo_surface_write_to_png(surface, "a.png");
	if (status != CAIRO_STATUS_SUCCESS) {
		puts(cairo_status_to_string(status));
	}

	cairo_surface_destroy(surface);
	return status;
}
