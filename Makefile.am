ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	gen_lists.sh \
	doc/doxygen.conf \
	doc/doxygen_webservices.conf \
	doc/doc.css \
	doc/images/block.eps \
	doc/images/block.png \
	doc/images/digitalSTROM.png

SUBDIRS = build tools data

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
