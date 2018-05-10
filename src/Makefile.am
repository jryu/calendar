%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^

noinst_LIBRARIES = libholidays.a

libholidays_a_SOURCES = holidays.cpp

dist_noinst_DATA = config.proto
BUILT_SOURCES = config.pb.cc

noinst_PROGRAMS = calendar
calendar_SOURCES = main.cpp config.pb.cc config.pb.h
calendar_CPPFLAGS = $(CAIRO_CFLAGS) $(LIBRSVG2_CFLAGS)
calendar_LDADD = $(CAIRO_LIBS) $(PROTOBUF_LIBS) $(LIBRSVG2_LIBS) libholidays.a


check_PROGRAMS = holidays_test
holidays_test_SOURCES = holidays_test.cpp
holidays_test_CPPFLAGS = $(CPPUNIT_CFLAGS)
holidays_test_LDADD = $(CPPUNIT_LIBS) libholidays.a

TESTS=holidays_test
