dssdatadir = $(pkgdatadir)/data
dist_dssdata_DATA = \
	$(top_srcdir)/data/config.xml \
	$(top_srcdir)/data/subscriptions.xml

dssdataimagesdir = $(pkgdatadir)/data/images
include $(top_srcdir)/data/images.list

dssjsdir = $(pkgdatadir)/data/scripts
dist_dssjs_DATA = \
	$(top_srcdir)/data/scripts/solar_computer.js \
	$(top_srcdir)/data/scripts/system_action.js \
	$(top_srcdir)/data/scripts/system_conditions.js \
	$(top_srcdir)/data/scripts/system_trigger.js \
	$(top_srcdir)/data/scripts/system_register_trigger.js \
	$(top_srcdir)/data/scripts/http.js \
	$(top_srcdir)/data/scripts/system_events.js \
	$(top_srcdir)/data/scripts/system_hail.js \
	$(top_srcdir)/data/scripts/system_state.js

webrootdir = $(pkgdatadir)/data/webroot

if BUILD_SOAP

dist_webroot_DATA = \
	$(top_builddir)/build/webservices/dss.wsdl \
	$(top_builddir)/build/webservices/dss.nsmap \
	$(top_builddir)/build/webservices/dss.xsd

endif

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/config.d
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/subscriptions.d
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/savedprops
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/metering
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/logs


