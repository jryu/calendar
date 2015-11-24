#include <time.h>

#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "holidays.h"

class TestHolidays : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestHolidays);
	CPPUNIT_TEST(test2015);
	CPPUNIT_TEST(test2016);
	CPPUNIT_TEST_SUITE_END();

protected:
	void test2015() {
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 1, 1)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 1, 19)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 2, 16)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 5, 25)));

		// July 4, 2015 (Independence Day), falls on a Saturday.
		CPPUNIT_ASSERT(!is_holiday(ymd(2015, 7, 4)));
		// Friday, July 3, will be treated as a holiday.
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 7, 3)));

		CPPUNIT_ASSERT(is_holiday(ymd(2015, 9, 7)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 10, 12)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 11, 11)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 11, 26)));
		CPPUNIT_ASSERT(is_holiday(ymd(2015, 12, 25)));
	}

	void test2016() {
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 1, 1)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 1, 18)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 2, 15)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 5, 30)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 7, 4)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 9, 5)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 10, 10)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 11, 11)));
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 11, 24)));

		// December 25, 2016 (Christmas Day), falls on a Sunday.
		CPPUNIT_ASSERT(!is_holiday(ymd(2016, 12, 25)));
		// Monday, December 26, will be treated as a holiday.
		CPPUNIT_ASSERT(is_holiday(ymd(2016, 12, 26)));
	}

private:
	time_t ymd(int year, int month, int day) {
		struct tm timeinfo = {0};
		timeinfo.tm_year = year - 1900;
		timeinfo.tm_mon = month - 1;
		timeinfo.tm_mday = day;
		return mktime(&timeinfo);
	}
};
CPPUNIT_TEST_SUITE_REGISTRATION(TestHolidays);

int main(int argc, char* argv[])
{
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
	return !runner.run();
}
