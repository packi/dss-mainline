bin_PROGRAMS =
check_PROGRAMS =
EXTRA_DIST =
CLEANFILES = build_info.h
TESTS =
noinst_LIBRARIES =
BUILT_SOURCES = build_info.h

build_info.h:
	$(top_srcdir)/tools/dss_gen_version_unix.sh .. .

include sources.list

include ../unix/Makefile.inc
include ../src/Makefile.inc
include ../tests/Makefile.inc

