TESTS += dsstests.sh
EXTRA_DIST += \
			$(top_srcdir)/build/dsstests.sh \
			$(top_srcdir)/tests/data/speed_triggers.xml \
			$(top_srcdir)/tests/data/rate_triggers.xml

check_PROGRAMS += dsstests

dsstests_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			-I$(top_builddir)/src \
			$(JSONC_CFLAGS) \
			$(JS_CFLAGS) \
			$(BOOST_CPPFLAGS) \
			$(LIBDSM_API_CFLAGS) \
			$(GCOV_CFLAGS) \
			-DABS_SRCDIR=\"@abs_top_srcdir@\" \
			-DSRCDIR=\"@top_srcdir@\" \
			-DABS_BUILDDIR=\"@abs_top_builddir@\" \
			-DBUILDDIR=\"@top_builddir@\"

dsstests_LDADD = \
			libdsscore.a libunix.a libcivetweb.a libprotomessages.a \
			$(libds_a_LIB) \
			$(BOOST_LDFLAGS) \
			$(BOOST_THREAD_LIBS) \
			$(BOOST_SYSTEM_LIBS) \
			$(BOOST_PROGRAM_OPTIONS_LIBS) \
			$(BOOST_FILESYSTEM_LIBS) \
			$(BOOST_UNIT_TEST_FRAMEWORK_LIBS) \
			$(LIBDSM_API_LDFLAGS) \
			$(JSONC_LIBS) \
			$(JS_LIBS) \
			$(ICAL_LIBS) \
			$(LIBDSM_API_LIBS) \
			$(LIBXML2_LIBS) \
			$(OPENSSL_LIBS) \
			$(AVAHI_LIBS) \
			$(CURL_LIBS) \
			$(LIBRRD_LIBS) \
			$(EXPAT_LIBS) \
			$(GCOV_LIBS) \
			$(LIBTCMALLOC_LIBS) \
			$(JSONC_LIBS) \
			$(LIBPROTOBUF_LIBS) \
			$(OSSP_UUID_CFLAGS) \
			$(LIBCOMMCHANNEL_LIBS) \
			$(OSSP_UUID_LIBS) \
			$(LIBDSUID_LIBS) \
			$(LTLIBICONV) \
			$(BOOST_FILESYSTEM_LIB) \
			$(BOOST_PROGRAM_OPTIONS_LIB) \
			$(BOOST_SYSTEM_LIB) \
			$(BOOST_THREAD_LIB) \
			$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
			$(BOOST_CHRONO_LIB) \
			$(SQLITE3_LIBS) \
			$(libdsscore_a_LIB)

