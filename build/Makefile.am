bin_PROGRAMS = dss

noinst_PROGRAMS = dsstests

CLEANFILES = \
			$(top_srcdir)/src/build_info.h

BUILT_SOURCES = \
			$(top_srcdir)/src/build_info.h

noinst_LIBRARIES = libdsscore.a libmongoose.a libunix.a

if BUILD_SOAP

CLEANFILES += \
			 $(top_srcdir)/webservices/soapC.cpp \
			 $(top_srcdir)/webservices/soapH.h
			 $(top_srcdir)/webservices/dss.nsmap \
			 $(top_srcdir)/webservices/dss.wsdl \
			 $(top_srcdir)/webservices/dss.xsd \
			 $(top_srcdir)/webservices/soapServer.cpp \
			 $(top_srcdir)/webservices/soapStub.h \
			 $(top_srcdir)/webservices/soapdssObject.h

BUILT_SOURCES += \
			$(top_srcdir)/webservices/soapC.cpp \
			$(top_srcdir)/webservices/soapH.h \
			$(top_srcdir)/webservices/dss.nsmap \
			$(top_srcdir)/webservices/dss.wsdl \
			$(top_srcdir)/webservices/dss.xsd \
			$(top_srcdir)/webservices/soapServer.cpp \
			$(top_srcdir)/webservices/soapStub.h \
			$(top_srcdir)/webservices/soapdssObject.h

noinst_LIBRARIES += libwebservices.a

endif

include $(top_srcdir)/build/sources.list

libdsscore_a_CXXFLAGS = \
			-I$(top_srcdir)/src \
			-I$(top_srcdir)/external \
			$(JS_CFLAGS) \
			$(ICAL_CFLAGS) \
			$(LIBDSM_API_CFLAGS) \
			$(GSOAP_CFLAGS) \
			$(POCO_CFLAGS) \
			$(LIBXML2_CFLAGS) \
			$(OPENSSL_CFLAGS) \
			$(AVAHI_CFLAGS) \
			$(CURL_CFLAGS) \
			$(LIBRRD_CFLAGS) \
			$(EXPAT_CFLAGS)

libunix_a_CXXFLAGS = \
			$(BOOST_CPPFLAGS)

if WITH_CURL

libdsscore_a_SOURCES +=  \
	$(top_srcdir)/src/scripting/jscurl.cpp \
	$(top_srcdir)/src/scripting/jscurl.h \
	$(top_srcdir)/src/scripting/jscurl-constants.h

endif

dss_SOURCES = $(top_srcdir)/main.cpp $(top_srcdir)/namespaces.cpp

nodist_dss_SOURCES = $(top_srcdir)/src/build_info.h

dss_CXXFLAGS = \
			$(LIBXML2_CFLAGS) \
			$(GSOAP_CFLAGS)

dss_LDADD = libdsscore.a libunix.a libmongoose.a \
			$(BOOST_THREAD_LIBS) \
			$(BOOST_SYSTEM_LIBS) \
			$(BOOST_PROGRAM_OPTIONS_LIBS) \
			$(BOOST_FILESYSTEM_LIBS) \
			$(LIBDSM_API_LDFLAGS) \
			$(POCO_LDFLAGS) \
			$(JS_LIBS) \
			$(ICAL_LIBS) \
			$(LIBDSM_API_LIBS) \
			$(POCO_LIBS) \
			$(LIBXML2_LIBS) \
			$(OPENSSL_LIBS) \
			$(AVAHI_LIBS) \
			$(CURL_LIBS) \
			$(LIBRRD_LIBS) -lrrd_th \
			$(EXPAT_LIBS)

if BUILD_SOAP

dss_LDADD += \
			libwebservices.a \
			$(GSOAP_LIBS) \
			$(OPENSSL_LIBS)

libwebservices_a_CXXFLAGS = \
			-I$(top_srcdir)/src \
			$(JS_CFLAGS) \
			$(GSOAP_CFLAGS) \
			$(LIBXML2_CFLAGS) \
			$(OPENSSL_CFLAGS) \
			$(AVAHI_CFLAGS)

nodist_libwebservices_a_SOURCES = \
			$(top_srcdir)/webservices/soapC.cpp \
			$(top_srcdir)/webservices/soapH.h \
			$(top_srcdir)/webservices/soapServer.cpp \
			$(top_srcdir)/webservices/soapStub.h \
			$(top_srcdir)/webservices/soapdssObject.h

endif

dsstests_CXXFLAGS = \
			-I$(top_srcdir)/src \
			$(JS_CFLAGS) \
			$(BOOST_CPPFLAGS) \
			$(LIBDSM_API_CFLAGS)

dsstests_LDADD = \
			libdsscore.a libunix.a libmongoose.a \
			$(BOOST_THREAD_LIBS) \
			$(BOOST_SYSTEM_LIBS) \
			$(BOOST_PROGRAM_OPTIONS_LIBS) \
			$(BOOST_FILESYSTEM_LIBS) \
			$(BOOST_UNIT_TEST_FRAMEWORK_LIBS) \
			$(LIBDSM_API_LDFLAGS) \
			$(POCO_LDFLAGS) \
			$(JS_LIBS) \
			$(ICAL_LIBS) \
			$(LIBDSM_API_LIBS) \
			$(POCO_LIBS) \
			$(LIBXML2_LIBS) \
			$(OPENSSL_LIBS) \
			$(AVAHI_LIBS) \
			$(CURL_LIBS) \
			$(LIBRRD_LIBS) -lrrd_th \
			$(EXPAT_LIBS)

if BUILD_SOAP

dsstests_LDADD += \
			libwebservices.a \
			$(GSOAP_LIBS) \
			$(OPENSSL_LIBS)

endif

dsstests_SOURCES += $(top_srcdir)/namespaces.cpp

$(top_srcdir)/src/build_info.h:
	$(top_srcdir)/tools/dss_gen_version_unix.sh $(top_srcdir) $(top_srcdir)

$(top_srcdir)/src/dss.cpp: $(top_srcdir)/src/build_info.h

if BUILD_SOAP

$(top_srcdir)/webservices/soapH.h: $(top_srcdir)/webservices/soapC.cpp
$(top_srcdir)/webservices/soapServer.cpp: $(top_srcdir)/webservices/soapC.cpp
$(top_srcdir)/webservices/soapStub.h: $(top_srcdir)/webservices/soapC.cpp
$(top_srcdir)/webservices/soapdssObject.h: $(top_srcdir)/webservices/soapC.cpp
$(top_srcdir)/webservices/dss.nsmap: $(top_srcdir)/webservices/soapC.cpp
$(top_srcdir)/webservices/dss.wsdl: $(top_srcdir)/webservices/soapC.cpp
$(top_srcdir)/webservices/dss.xsd: $(top_srcdir)/webservices/soapC.cpp

$(top_srcdir)/webservices/soapC.cpp: $(top_srcdir)/webservices/model_soap.cpp $(top_srcdir)/webservices/model_soap.h
	$(GSOAP_SOAPCPP2) -I$(GSOAP_IMPORT_DIR) -I$(top_srcdir)/webservices -SLx -d$(top_srcdir)/webservices $(top_srcdir)/webservices/model_soap.h

endif
