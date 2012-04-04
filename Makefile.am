ACLOCAL_AMFLAGS = -I m4

SUBDIRS = build tools data

EXTRA_DIST = \
	$(top_srcdir)/gen_lists.sh \
	$(top_srcdir)/doc/doxygen.conf \
	$(top_srcdir)/doc/doxygen.conf.in \
	$(top_srcdir)/doc/doxygen_webservices.conf \
	$(top_srcdir)/doc/doxygen_webservices.conf.in \
	$(top_srcdir)/doc/doc.css \
	$(top_srcdir)/doc/images/block.eps \
	$(top_srcdir)/doc/images/block.png \
	$(top_srcdir)/doc/images/digitalSTROM.png

if HAVE_DOXYGEN
doxygen-doc:
	$(DOXYGEN) $(top_srcdir)/doc/doxygen.conf
	$(DOXYGEN) $(top_srcdir)/doc/doxygen_webservices.conf
else
doxygen-doc:
	@echo
	@echo "Not generating documentation: doxygen is not available!"
	@echo
endif

clean-local: clean-doc

clean-doc:
	rm -rf $(top_srcdir)/doc/inetrnal
	rm -rf $(top_srcdir)/doc/webservices

if WITH_CURL
gencurlheader:
	python src/scripting/curl-js-constants.py < /usr/include/curl/curl.h > src/scripting/jscurl-constants.h
endif
