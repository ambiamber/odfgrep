bin_PROGRAMS = odfgrep
odfgrep_SOURCES = odfgrep.cpp xml.cpp zip.cpp action.cpp unicode.cpp

# set the include path found by configure
AM_CPPFLAGS = $(all_includes) -I/usr/include/libxml2

# the library search path.
odfgrep_LDFLAGS = $(all_libraries) 
odfgrep_LDADD = -lboost_regex -lxml2 -lzip
noinst_HEADERS = xml.hpp zip.hpp action.hpp unicode.hpp
