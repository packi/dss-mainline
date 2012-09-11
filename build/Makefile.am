bin_PROGRAMS = dss

noinst_PROGRAMS = dsstests

CLEANFILES = \
			build_info.h

BUILT_SOURCES = \
			build_info.h

noinst_LIBRARIES = libdsscore.a libmongoose.a libunix.a

if BUILD_SOAP

CLEANFILES += \
			 webservices/soapC.cpp \
			 webservices/soapH.h
			 webservices/dss.nsmap \
			 webservices/dss.wsdl \
			 webservices/dss.xsd \
			 webservices/soapServer.cpp \
			 webservices/soapStub.h \
			 webservices/soapdssObject.h

BUILT_SOURCES += \
			webservices/soapC.cpp \
			webservices/soapH.h \
			webservices/dss.nsmap \
			webservices/dss.wsdl \
			webservices/dss.xsd \
			webservices/soapServer.cpp \
			webservices/soapStub.h \
			webservices/soapdssObject.h

noinst_LIBRARIES += libwebservices.a

endif

include $(top_srcdir)/build/sources.list

libdsscore_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			-I$(top_srcdir)/external \
			$(JS_CFLAGS) \
			$(ICAL_CFLAGS) \
			$(LIBDSM_API_CFLAGS) \
			$(GSOAP_CFLAGS) \
			$(LIBXML2_CFLAGS) \
			$(OPENSSL_CFLAGS) \
			$(AVAHI_CFLAGS) \
			$(CURL_CFLAGS) \
			$(LIBRRD_CFLAGS) \
			$(EXPAT_CFLAGS) \
			$(GCOV_CFLAGS)

libunix_a_CXXFLAGS = \
			-I$(top_srcdir) \
			$(BOOST_CPPFLAGS) \
			$(GCOV_CFLAGS)

if WITH_CURL

libdsscore_a_SOURCES +=  \
	$(top_srcdir)/src/scripting/jscurl.cpp \
	$(top_srcdir)/src/scripting/jscurl.h \
	$(top_srcdir)/src/scripting/jscurl-constants.h

endif

dss_SOURCES = $(top_srcdir)/main.cpp

nodist_dss_SOURCES = $(top_srcdir)/src/build_info.h

dss_CXXFLAGS = \
			$(LIBXML2_CFLAGS) \
			$(GSOAP_CFLAGS) \
			$(GCOV_CFLAGS)

dss_LDADD = libdsscore.a libunix.a libmongoose.a \
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
			$(LIBTCMALLOC_LIBS)

if BUILD_SOAP

dss_SOURCES += $(top_srcdir)/namespaces.cpp

dss_LDADD += \
			libwebservices.a \
			$(GSOAP_LIBS) \
			$(OPENSSL_LIBS)

libwebservices_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			$(JS_CFLAGS) \
			$(GSOAP_CFLAGS) \
			$(LIBXML2_CFLAGS) \
			$(OPENSSL_CFLAGS) \
			$(AVAHI_CFLAGS) \
			$(GCOV_CFLAGS)

nodist_libwebservices_a_SOURCES = \
			webservices/soapC.cpp \
			webservices/soapH.h \
			webservices/soapServer.cpp \
			webservices/soapStub.h \
			webservices/soapdssObject.h

endif

dsstests_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			$(JS_CFLAGS) \
			$(BOOST_CPPFLAGS) \
			$(LIBDSM_API_CFLAGS) \
			$(GCOV_CFLAGS)

dsstests_LDADD = \
			libdsscore.a libunix.a libmongoose.a \
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
			$(LIBTCMALLOC_LIBS)

if BUILD_SOAP

dsstests_LDADD += \
			libwebservices.a \
			$(GSOAP_LIBS) \
			$(OPENSSL_LIBS)

dsstests_SOURCES += $(top_srcdir)/namespaces.cpp

endif


build_info.h:
	$(top_srcdir)/tools/dss_gen_version_unix.sh $(top_srcdir) .

$(top_srcdir)/src/dss.cpp: build_info.h

if BUILD_SOAP

webservices/soapH.h: webservices/soapC.cpp
webservices/soapServer.cpp: webservices/soapC.cpp
webservices/soapStub.h: webservices/soapC.cpp
webservices/soapdssObject.h: webservices/soapC.cpp
webservices/dss.nsmap: webservices/soapC.cpp
webservices/dss.wsdl: webservices/soapC.cpp
webservices/dss.xsd: webservices/soapC.cpp

webservices/soapC.cpp: $(top_srcdir)/webservices/model_soap.cpp $(top_srcdir)/webservices/model_soap.h
	$(MKDIR_P) webservices
	$(GSOAP_SOAPCPP2) -I$(GSOAP_IMPORT_DIR) -SLx -dwebservices $(top_srcdir)/webservices/model_soap.h

endif
