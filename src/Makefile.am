noinst_LIBRARIES = libholidays.a
libholidays_a_SOURCES = holidays.cpp

noinst_PROGRAMS = calendar
calendar_SOURCES = main.cpp
calendar_CPPFLAGS = $(CAIRO_CFLAGS)
calendar_LDADD = $(CAIRO_LIBS) libholidays.a

check_PROGRAMS = holidays_test
holidays_test_SOURCES = holidays_test.cpp
holidays_test_CPPFLAGS = $(CPPUNIT_CFLAGS)
holidays_test_LDADD = $(CPPUNIT_LIBS) libholidays.a

TESTS=holidays_test
