DS_TESTS =

### ds

noinst_LIBRARIES += libds.a
libds_a_SOURCES = \
        ../src/ds/collection.cpp \
        ../src/ds/collection.h \
        ../src/ds/common.cpp \
        ../src/ds/common.h \
        ../src/ds/cstddef.cpp \
        ../src/ds/cstddef.h \
        ../src/ds/enum.cpp \
        ../src/ds/enum.h \
        ../src/ds/log.cpp \
        ../src/ds/log.h \
        ../src/ds/random.cpp \
        ../src/ds/random.h \
        ../src/ds/str.cpp \
        ../src/ds/str.h
libds_a_CPPFLAGS = \
        $(AM_CPPFLAGS) \
        $(BOOST_CPPFLAGS)
libds_a_LIB = \
        libds.a \
        $(BOOST_SYSTEM_LIB) \
        $(BOOST_THREAD_LIB) \
        $(BOOST_CHRONO_LIB) \
        -lpthread

noinst_LIBRARIES += libds-catch-main.a
libds_catch_main_a_SOURCES = \
        ../src/ds/catch/main.cpp
libds_catch_main_a_LIB = \
        libds-catch-main.a

DS_TESTS += ds-tests
ds_tests_SOURCES = \
        ../src/ds/collection-test.cpp \
        ../src/ds/common-test.cpp \
        ../src/ds/enum-test.cpp \
        ../src/ds/log-test.cpp \
        ../src/ds/str-test.cpp \
        ../src/ds/random-test.cpp
ds_tests_LDADD = \
        $(libds_catch_main_a_LIB) \
        $(libds_a_LIB)

### ds-asio

noinst_LIBRARIES += libds-asio.a
libds_asio_a_SOURCES = \
	../src/ds/asio/abortable.cpp \
	../src/ds/asio/abortable.h \
	../src/ds/asio/reconnecting-socket.cpp \
	../src/ds/asio/reconnecting-socket.h \
	../src/ds/asio/io-service.cpp \
	../src/ds/asio/io-service.h \
	../src/ds/asio/timer.cpp \
	../src/ds/asio/timer.h \
	../src/ds/asio/watchdog.cpp \
	../src/ds/asio/watchdog.h
libds_asio_a_CPPFLAGS = \
        $(libds_a_CPPFLAGS)
libds_asio_a_LIB = \
        libds-asio.a \
        $(libds_a_LIB)

DS_TESTS += ds-asio-tests
ds_asio_tests_SOURCES = \
        ../src/ds/asio/abortable-test.cpp \
        ../src/ds/asio/io-service-test.cpp \
        ../src/ds/asio/timer-test.cpp
ds_asio_tests_CPPFLAGS = \
        $(libds_asio_a_CPPFLAGS)
ds_asio_tests_LDADD = \
        $(libds_catch_main_a_LIB) \
        $(libds_asio_a_LIB)


check_PROGRAMS += $(DS_TESTS)
TESTS += $(DS_TESTS)

### dss

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

noinst_LIBRARIES += libdsscore.a libcivetweb.a libunix.a libprotomessages.a

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
			$(BOOST_CPPFLAGS) \
			$(LIBPROTOBUF_CFLAGS) \
			$(LIBCOMMCHANNEL_CFLAGS) \
			$(LIBDSUID_CFLAGS) \
			$(libds_a_CPPFLAGS)

libdsscore_a_LIB = \
        $(libds_asio_a_LIB)

dss_SOURCES = ../main.cpp

dss_CXXFLAGS = \
			-I$(top_srcdir)/src \
			-I$(top_builddir)/src \
			$(LIBXML2_CFLAGS) \
			$(GCOV_CFLAGS) \
            $(LIBPROTOBUF_CFLAGS) \
            $(LIBCOMMCHANNEL_CFLAGS) \
			$(LIBDSUID_CFLAGS) \
			$(OSSP_UUID_CFLAGS) \
			$(BOOST_CPPFLAGS) \
			$(SQLITE3_CFLAGS)

dss_LDADD = libdsscore.a libunix.a libcivetweb.a libprotomessages.a \
			$(LIBDSM_API_LDFLAGS) \
			$(BOOST_LDFLAGS) \
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
			$(OSSP_UUID_LIBS) \
			$(LTLIBICONV) \
			$(BOOST_FILESYSTEM_LIB) \
			$(BOOST_PROGRAM_OPTIONS_LIB) \
			$(BOOST_SYSTEM_LIB) \
			$(BOOST_THREAD_LIB) \
			$(BOOST_CHRONO_LIB) \
			$(SQLITE3_LIBS) \
			$(libdsscore_a_LIB)

../src/dss.cpp: build_info.h

../src/comm-channel.h: ../src/messages/messaging.pb.h
../src/comm-channel.cpp: ../src/messages/device-scene-table-update.pb.h


../src/vdc-connection.h: ../src/messages/vdc-messages.pb.h
%.pb.cc %.pb.h: %.proto
	$(MKDIR_P) ../src/messages
	protoc -I$(top_srcdir)/src/messages --cpp_out=../src/messages $(top_srcdir)/src/messages/$(<F)
