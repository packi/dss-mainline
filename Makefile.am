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

clean-local: clean-doc clean-report

clean-doc:
	rm -rf $(top_srcdir)/doc/inetrnal
	rm -rf $(top_srcdir)/doc/webservices

clean-report:
	rm -rf $(top_builddir)/report
	rm -rf $(top_builddir)/dss.info
#	rm -rf $(top_builddir)/build/*.gc*
	rm -rf $(top_builddir)/dss_processed.info
	rm -rf $(top_builddir)/build/*.gcda
	rm -rf $(top_builddir)/build/..*.gcov

gencurlheader:
	python src/scripting/curl-js-constants.py < /usr/include/curl/curl.h > src/scripting/jscurl-constants.h

if BUILD_GCOV_REPORT
report: $(top_builddir)/build/dsstests
	@echo
	@echo "Generating code coverage report"
	@echo
	$(LCOV) --directory $(top_builddir)/build --zerocounters
	$(top_builddir)/build/dsstests
	$(LCOV) --directory $(top_builddir)/build --capture --output-file $(top_builddir)/dss.info
	$(LCOV) --extract $(top_builddir)/dss.info '*src/*' --extract $(top_builddir)/dss.info '*unix/*' -o $(top_builddir)/dss_processed.info
	$(GENHTML) -o $(top_builddir)/report $(top_builddir)/dss_processed.info
else
report:
	@echo
	@echo "Please run configure with the --enable-test-coverage parameter and"
	@echo "recompile!".
	@echo
endif
