libunix_a_CXXFLAGS = \
			-I$(top_srcdir) \
			-I$(top_srcdir)/src \
			-I$(top_builddir)/src \
			$(BOOST_CPPFLAGS) \
			$(GCOV_CFLAGS) \
            $(LIBPROTOBUF_CFLAGS) \
			$(LIBCOMMCHANNEL_CFLAGS) \
			$(LIBDSUID_CFLAGS)

