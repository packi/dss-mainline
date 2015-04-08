TESTS += dsstests.sh
EXTRA_DIST += dsstests.sh

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
			-DTEST_TRIGGERS_PATH="\"$(abs_top_srcdir)/\""

dsstests_LDADD = \
			libdsscore.a libunix.a libmongoose.a libprotomessages.a \
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
			$(LTLIBICONV)
