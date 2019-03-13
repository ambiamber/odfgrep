AC_INIT([odfgrep], [0.1], [odf@tempest-sw.com])
AC_CONFIG_SRCDIR([src/action.cpp])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS(config.h)

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AM_PROG_LIBTOOL
AC_CHECK_PROG([RMAN], [rman], [rman], [NOT FOUND])

# Checks for libraries.
AC_CHECK_LIB([xml2], [xmlRelaxNGInitTypes], [true],
 [AC_MSG_ERROR([libxml2 is missing])])

AC_CHECK_LIB([zip], [zip_file_error_get], [true],
  [AC_MSG_ERROR([libzip is missing])])

AC_OUTPUT(Makefile src/Makefile man/Makefile)
