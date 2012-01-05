ACLOCAL_AMFLAGS = -I m4

SUBDIRS = build tools data

EXTRA_DIST = \
	$(top_srcdir)/gen_lists.sh \
	$(top_srcdir)/CMakeLists.txt \
	$(top_srcdir)/src/CMakeLists.txt \
	$(top_srcdir)/src/security/CMakeLists.txt \
	$(top_srcdir)/src/sim/CMakeLists.txt \
	$(top_srcdir)/src/ds485/CMakeLists.txt \
	$(top_srcdir)/src/model/CMakeLists.txt \
	$(top_srcdir)/src/scripting/CMakeLists.txt \
	$(top_srcdir)/unix/CMakeLists.txt \
	$(top_srcdir)/external/CMakeLists.txt \
	$(top_srcdir)/external/mongoose/CMakeLists.txt \
	$(top_srcdir)/webservices/CMakeLists.txt \
	$(top_srcdir)/cmake/Modules/FindAvahi.cmake \
	$(top_srcdir)/cmake/Modules/FindDNSSD.cmake \
	$(top_srcdir)/cmake/Modules/FindGSOAP.cmake \
	$(top_srcdir)/cmake/Modules/FindLibical.cmake \
	$(top_srcdir)/cmake/Modules/FindPoco.cmake \
	$(top_srcdir)/cmake/Modules/FindRrd.cmake \
	$(top_srcdir)/cmake/Modules/FindSpiderMonkey.cmake \
	$(top_srcdir)/cmake/Modules/GGLCheckFunctionExists.c \
	$(top_srcdir)/cmake/Modules/GGLCheckFunctionExists.cmake \
	$(top_srcdir)/tests/CMakeLists.txt \
	$(top_srcdir)/doc/doxygen.conf \
	$(top_srcdir)/doc/doxygen.conf.in \
	$(top_srcdir)/doc/doxygen_webservices.conf \
	$(top_srcdir)/doc/doxygen_webservices.conf.in \
	$(top_srcdir)/doc/doc.css \
	$(top_srcdir)/doc/images/block.eps \
	$(top_srcdir)/doc/images/block.png \
	$(top_srcdir)/doc/images/digitalSTROM.png \
	$(top_srcdir)/config.cmake.h.in \
	$(top_srcdir)/doc/CMakeLists.txt

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
