noinst_LIBRARIES = libholidays.a
libholidays_a_SOURCES = holidays.c

noinst_PROGRAMS = calendar
calendar_SOURCES = calendar.c
calendar_CFLAGS = $(CAIRO_CFLAGS)
calendar_LDADD = libholidays.a $(CAIRO_LIBS)
