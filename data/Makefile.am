dssdatadir = $(pkgdatadir)/data
dist_dssdata_DATA = \
	$(top_srcdir)/data/config.xml.sample \
	$(top_srcdir)/data/subscriptions.xml \
	$(top_srcdir)/data/vdc-db.sql

dssdataimagesdir = $(pkgdatadir)/data/images
include $(top_srcdir)/data/images.list

# TODO
# sqldumpsdir = $(pkgdatadir)/data/sqldumps

dssjsdir = $(pkgdatadir)/data/scripts
dist_dssjs_DATA = \
	$(top_srcdir)/data/scripts/solar_computer.js \
	$(top_srcdir)/data/scripts/system_action.js \
	$(top_srcdir)/data/scripts/system_conditions.js \
	$(top_srcdir)/data/scripts/system_register_trigger.js \
	$(top_srcdir)/data/scripts/http.js \
	$(top_srcdir)/data/scripts/system_hail.js

webrootdir = $(pkgdatadir)/data/webroot

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/config.d
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/subscriptions.d
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/savedprops
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/metering
	$(MKDIR_P) $(DESTDIR)$(dssdatadir)/logs


