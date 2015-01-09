bin_PROGRAMS = dss

TESTS = dsstests

check_PROGRAMS = dsstests

EXTRA_DIST =	$(top_srcdir)/src/messages/messaging.proto \
				$(top_srcdir)/src/messages/device-scene-table-update.proto \
				$(top_srcdir)/src/messages/vdc-messages.proto \
				$(top_srcdir)/src/messages/vdcapi.proto

CLEANFILES = \
			build_info.h \
			$(top_builddir)/src/messages/messaging.pb.h \
			$(top_builddir)/src/messages/messaging.pb.cc \
			$(top_builddir)/src/messages/device-scene-table-update.pb.h \
			$(top_builddir)/src/messages/device-scene-table-update.pb.cc \
			$(top_builddir)/src/messages/vdc-messages.pb.h \
	        $(top_builddir)/src/messages/vdc-messages.pb.cc \
			$(top_builddir)/src/messages/vdcapi.pb.h \
	        $(top_builddir)/src/messages/vdcapi.pb.cc


BUILT_SOURCES = \
			build_info.h \
			$(top_builddir)/src/messages/messaging.pb.h \
			$(top_builddir)/src/messages/messaging.pb.cc \
			$(top_builddir)/src/messages/device-scene-table-update.pb.h \
			$(top_builddir)/src/messages/device-scene-table-update.pb.cc \
			$(top_builddir)/src/messages/vdc-messages.pb.h \
			$(top_builddir)/src/messages/vdc-messages.pb.cc \
			$(top_builddir)/src/messages/vdcapi.pb.h \
	        $(top_builddir)/src/messages/vdcapi.pb.cc

noinst_LIBRARIES = libdsscore.a libmongoose.a libunix.a libprotomessages.a

nodist_libprotomessages_a_SOURCES = \
			$(top_builddir)/src/messages/messaging.pb.h \
			$(top_builddir)/src/messages/messaging.pb.cc \
			$(top_builddir)/src/messages/device-scene-table-update.pb.h \
			$(top_builddir)/src/messages/device-scene-table-update.pb.cc \
			$(top_builddir)/src/messages/vdc-messages.pb.h \
			$(top_builddir)/src/messages/vdc-messages.pb.cc \
			$(top_builddir)/src/messages/vdcapi.pb.h \
	        $(top_builddir)/src/messages/vdcapi.pb.cc

libprotomessages_a_CXXFLAGS = \
			-I$(top_builddir) \
			$(LIBPROTOBUF_CFLAGS)

include $(top_srcdir)/build/sources.list

libdsscore_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			-I$(top_builddir)/src \
			-I$(top_srcdir)/external \
			$(JSONC_CFLAGS) \
			$(JS_CFLAGS) \
			$(ICAL_CFLAGS) \
			$(LIBDSM_API_CFLAGS) \
			$(LIBXML2_CFLAGS) \
			$(OPENSSL_CFLAGS) \
			$(AVAHI_CFLAGS) \
			$(CURL_CFLAGS) \
			$(LIBRRD_CFLAGS) \
			$(EXPAT_CFLAGS) \
			$(GCOV_CFLAGS) \
			$(OSSP_UUID_CFLAGS) \
			$(LIBPROTOBUF_CFLAGS) \
			$(LIBCOMMCHANNEL_CFLAGS) \
			$(LIBDSUID_CFLAGS)

libunix_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_builddir)/src \
			$(BOOST_CPPFLAGS) \
			$(GCOV_CFLAGS) \
            $(LIBPROTOBUF_CFLAGS) \
			$(LIBCOMMCHANNEL_CFLAGS) \
			$(LIBDSUID_CFLAGS)


dss_SOURCES = $(top_srcdir)/main.cpp

nodist_dss_SOURCES = $(top_srcdir)/src/build_info.h

dss_CXXFLAGS = \
			-I$(top_builddir)/src \
			$(LIBXML2_CFLAGS) \
			$(GCOV_CFLAGS) \
            $(LIBPROTOBUF_CFLAGS) \
            $(LIBCOMMCHANNEL_CFLAGS) \
			$(LIBDSUID_CFLAGS) \
			$(OSSP_UUID_CFLAGS)

dss_LDADD = libdsscore.a libunix.a libmongoose.a libprotomessages.a \
			$(BOOST_THREAD_LIBS) \
			$(BOOST_SYSTEM_LIBS) \
			$(BOOST_PROGRAM_OPTIONS_LIBS) \
			$(BOOST_FILESYSTEM_LIBS) \
			$(LIBDSM_API_LDFLAGS) \
			$(JS_LIBS) \
			$(ICAL_LIBS) \
			$(LIBDSM_API_LIBS) \
			$(LIBXML2_LIBS) \
			$(OPENSSL_LIBS) \
			$(AVAHI_LIBS) \
			$(CURL_LIBS) \
			$(LIBRRD_LIBS) \
			$(EXPAT_LIBS) \
			$(BACKTRACE_LDFLAGS) \
			$(GCOV_LIBS) \
			$(LIBTCMALLOC_LIBS) \
			$(JSONC_LIBS) \
			$(LIBPROTOBUF_LIBS) \
			$(LIBCOMMCHANNEL_LIBS) \
			$(LIBDSUID_LIBS) \
			$(OSSP_UUID_LIBS)

dsstests_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			-I$(top_builddir)/src \
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
			$(LIBDSUID_LIBS)

build_info.h:
	$(top_srcdir)/tools/dss_gen_version_unix.sh $(top_srcdir) .

$(top_srcdir)/src/dss.cpp: build_info.h

$(top_srcdir)/src/comm-channel.h: $(top_builddir)/src/messages/messaging.pb.h
$(top_srcdir)/src/comm-channel.cpp: $(top_builddir)/src/messages/device-scene-table-update.pb.h


$(top_srcdir)/src/vdc-connection.h: $(top_builddir)/src/messages/vdc-messages.pb.h
%.pb.cc %.pb.h: %.proto
	$(MKDIR_P) $(top_builddir)/src/messages
	protoc -I$(top_srcdir)/src/messages --cpp_out=$(top_builddir)/src/messages $(top_srcdir)/src/messages/$(<F)
