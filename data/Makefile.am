dssdatadir = $(pkgdatadir)/data
dist_dssdata_DATA = \
	$(top_srcdir)/data/config.xml \
	$(top_srcdir)/data/subscriptions.xml

dssjsdir = $(pkgdatadir)/data/scripts
dist_dssjs_DATA = \
	$(top_srcdir)/data/scripts/solar_computer.js \
	$(top_srcdir)/data/scripts/system_action.js \
	$(top_srcdir)/data/scripts/system_conditions.js \
	$(top_srcdir)/data/scripts/system_trigger.js \
	$(top_srcdir)/data/scripts/system_register_trigger.js \
	$(top_srcdir)/data/scripts/http.js \
	$(top_srcdir)/data/scripts/system_events.js \
	$(top_srcdir)/data/scripts/system_state.js

webrootdir = $(pkgdatadir)/data/webroot
dist_webroot_DATA = \
	$(top_srcdir)/webservices/dss.wsdl \
	$(top_srcdir)/webservices/dss.nsmap \
	$(top_srcdir)/webservices/dss.xsd
	
install-data-local:
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/config.d
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/subscriptions.d
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/savedprops
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/metering
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/logs


