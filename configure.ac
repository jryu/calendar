AC_INIT([calendar], [0.1])
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_SRCDIR([src/main.cpp])

AC_PROG_CXX
AC_PROG_RANLIB

PKG_CHECK_MODULES([CAIRO], [pangocairo])
PKG_CHECK_MODULES([CPPUNIT], [cppunit])
PKG_CHECK_MODULES([PROTOBUF], [protobuf])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_CHECK_PROG([PROTOC], [protoc], [protoc])

AC_OUTPUT
