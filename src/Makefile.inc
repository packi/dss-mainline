bin_PROGRAMS += dss

EXTRA_DIST +=	../src/messages/messaging.proto \
				../src/messages/device-scene-table-update.proto \
				../src/messages/vdc-messages.proto \
				../src/messages/vdcapi.proto

CLEANFILES += \
			../src/messages/messaging.pb.h \
			../src/messages/messaging.pb.cc \
			../src/messages/device-scene-table-update.pb.h \
			../src/messages/device-scene-table-update.pb.cc \
			../src/messages/vdc-messages.pb.h \
	        ../src/messages/vdc-messages.pb.cc \
			../src/messages/vdcapi.pb.h \
	        ../src/messages/vdcapi.pb.cc


BUILT_SOURCES += \
			../src/messages/messaging.pb.h \
			../src/messages/messaging.pb.cc \
			../src/messages/device-scene-table-update.pb.h \
			../src/messages/device-scene-table-update.pb.cc \
			../src/messages/vdc-messages.pb.h \
			../src/messages/vdc-messages.pb.cc \
			../src/messages/vdcapi.pb.h \
	        ../src/messages/vdcapi.pb.cc

noinst_LIBRARIES += libdsscore.a libmongoose.a libunix.a libprotomessages.a

nodist_libprotomessages_a_SOURCES = \
			../src/messages/messaging.pb.h \
			../src/messages/messaging.pb.cc \
			../src/messages/device-scene-table-update.pb.h \
			../src/messages/device-scene-table-update.pb.cc \
			../src/messages/vdc-messages.pb.h \
			../src/messages/vdc-messages.pb.cc \
			../src/messages/vdcapi.pb.h \
	        ../src/messages/vdcapi.pb.cc

libprotomessages_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_builddir)/src \
			$(LIBPROTOBUF_CFLAGS)

libdsscore_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			-I$(top_srcdir)/external \
			-I$(top_builddir)/src \
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

dss_SOURCES = ../main.cpp

dss_CXXFLAGS = \
			-I$(top_srcdir)/src \
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

../src/dss.cpp: build_info.h

../src/comm-channel.h: ../src/messages/messaging.pb.h
../src/comm-channel.cpp: ../src/messages/device-scene-table-update.pb.h


../src/vdc-connection.h: ../src/messages/vdc-messages.pb.h
%.pb.cc %.pb.h: %.proto
	$(MKDIR_P) ../src/messages
	protoc -I$(top_srcdir)/src/messages --cpp_out=../src/messages $(top_srcdir)/src/messages/$(<F)
