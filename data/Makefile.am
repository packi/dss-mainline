dssdatadir = $(pkgdatadir)/data
dist_dssdata_DATA = \
	$(top_srcdir)/data/config.xml \
	$(top_srcdir)/data/sim.xml \
	$(top_srcdir)/data/subscriptions.xml \
	$(top_srcdir)/data/solar_computer.js \
	$(top_srcdir)/data/handle_temperature_sensor_events.js \
	$(top_srcdir)/data/read_temperature.js \
	$(top_srcdir)/data/simulated_device.js

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


