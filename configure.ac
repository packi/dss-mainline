#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([dss], [1.4.2], [http://redmine.digitalstrom.org])
AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_SRCDIR([src/dss.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 -Wall])

if test "x${prefix}" = "xNONE"; then
    prefix="${ac_default_prefix}"
fi

MT_SET_SEARCHPATH

if test -n "$MT_SEARCHPATH_PKGCONFIG"; then
    export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$MT_SEARCHPATH_PKGCONFIG
fi

if test -n "$MT_SEARCHPATH_PROGS"; then
    export PATH=$PATH:$MT_SEARCHPATH_PROGS
fi

MT_OPTION([soap], [disable],
          [build dSS without SOAP support (default: enabled)],
          [],[])

MT_OPTION([curl], [disable],
          [build dSS without curl support (default: enabled)],
          [],[])

MT_OPTION([http], [enable],
          [build with HTTP support (default: HTTPS only)],
          [
            AC_DEFINE([WITH_HTTP], [1], [ build with HTTP support ])
          ],[])

MT_OPTION([bonjour], [disable],
          [build with bonjour support (default: enabled)],
          [
            AC_DEFINE([WITH_BONJOUR], [1], [ build with bonjour support ])
          ],[])

AC_ARG_WITH(dss-data,
    AC_HELP_STRING([--with-dss-data=DIR], [use DIR as dSS data directory]),
    [
        if test "x$withval" != "x"; then
            AC_DEFINE([WITH_DATADIR], [1], [custom dSS data directory set])
            AC_DEFINE_UNQUOTED([DSS_DATADIR], ["$withval"], [dSS data directory])
        fi
    ])

AC_ARG_WITH(dss-config,
    AC_HELP_STRING([--with-dss-config=DIR], [use DIR as dSS config directory]),
    [
        if test "x$withval" != "x"; then
            AC_DEFINE([WITH_CONFIGDIR], [1], [custom dSS config directory set])
            AC_DEFINE_UNQUOTED([DSS_CONFIGDIR], ["$withval"], [dSS data directory])
        fi
    ])

AC_ARG_WITH(dss-webroot,
    AC_HELP_STRING([--with-dss-webroot=DIR], [use DIR as dSS webroot directory]),
    [
        if test "x$withval" != "x"; then
            AC_DEFINE([WITH_WEBROOTDIR], [1], [custom dSS webroot directory set])
            AC_DEFINE_UNQUOTED([DSS_WEBROOTDIR], ["$withval"], [dSS webroot directory])
        fi
    ])

AC_ARG_WITH(dss-js-logs,
    AC_HELP_STRING([--with-dss-js-logs=DIR], [use DIR as dSS JavaScript logging directory]),
    [
        if test "x$withval" != "x"; then
            AC_DEFINE([WITH_JSLOGDIR], [1], [custom dSS JavaScript logging directory set])
            AC_DEFINE_UNQUOTED([DSS_JSLOGDIR], ["$withval"], [dSS JS log directory])
        fi
    ])

AC_ARG_WITH(dss-saved-properties,
    AC_HELP_STRING([--with-dss-saved-properties=DIR], [save dSS add-on script properties in DIR]),
    [
        if test "x$withval" != "x"; then
            AC_DEFINE([WITH_SAVEDPROPSDIR], [1], [custom dSS savedprops directory set])
            AC_DEFINE_UNQUOTED([DSS_SAVEDPROPSDIR], ["$withval"], [dSS savedprops directory])
        fi
    ])

AC_ARG_WITH(js-pkgconfig,
    AC_HELP_STRING([--with-js-pkgconfig=DIR], [search for libjs.pc or mozjs185.c in DIR]),
    [
        if test "x$withval" != "x"; then
            AC_MSG_NOTICE([Will also search for packages in $withval])
            export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$withval
        fi
    ])


SENDMAIL=
AC_ARG_WITH(sendmail-location,
    AC_HELP_STRING([--with-sendmail-location=SENDMAIL], [hardcode location of sendmail binary, no checks will be performed (useful for cross compiling)]),
    [
        if test "x$withval" != "x"; then
            AC_MSG_NOTICE([Hardcoding sendmail location to $withval])
            SENDMAIL="$withval"
            AC_DEFINE([HAVE_SENDMAIL], [1], [sendmail program is available])
        fi
    ])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_MKDIR_P

# Checks for libraries.
ACX_PTHREAD(,AC_MSG_ERROR(POSIX threads missing))
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

AC_LANG_CPLUSPLUS

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h], [], [AC_MSG_ERROR([required system header not found])])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([dup2 gethostbyname gettimeofday inet_ntoa localtime_r memchr memmove memset select setlocale socket strchr strcspn strdup strerror strrchr strspn strstr strtol strtoul])

ASSUMED_BOOST_VER=146

BOOST_REQUIRE([1.46],
    [
        unset boost_cv_version
        unset boost_cv_inc_path
        BOOST_REQUIRE([1.35],
            [
                AC_MSG_ERROR([could not find boost on your system])
            ])
        ASSUMED_BOOST_VER=135
    ])

if test $ASSUMED_BOOST_VER -eq 135; then
    AC_DEFINE([BOOST_VERSION_135], [1], [boost 1.35 or higher])
else
    AC_DEFINE([BOOST_VERSION_146], [1], [boost 1.46 or higher])
fi

BOOST_ASIO
BOOST_BIND
BOOST_FOREACH
BOOST_FUNCTION
BOOST_LAMBDA
BOOST_SMART_PTR
BOOST_TUPLE

BOOST_THREADS([mt-])
BOOST_FILESYSTEM([mt-])
BOOST_SYSTEM([mt-])
BOOST_PROGRAM_OPTIONS([mt-])
BOOST_TEST([mt-])

MT_CHECK_HEADER([poco], [Poco/XML/XML])

dnl unlike C, C++ library checks are not easy to do since AC_CHECK_LIB does not
dnl support it
 
POCO_LIBS="-lPocoXML -lPocoFoundation -lPocoNet"
POCO_LDFLAGS=
AC_ARG_WITH(poco-libs, 
    AC_HELP_STRING([--with-poco-libs=DIR], [search for poco libraries in DIR]),
    [
        if test "x$withval" != "x"; then
            AC_MSG_NOTICE([Will search for poco libraries in $withval])
            if test -d "$withval"; then
                POCO_LDFLAGS="-L$withval"
            fi
        fi
    ])

AC_SUBST(POCO_LIBS)
AC_SUBST(POCO_LDFLAGS)

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([JS], [libjs >= 1.8.5], [],
    [
        AC_MSG_WARN([$JS_PKG_ERRORS])
        AC_MSG_NOTICE([could not find libjs, trying to find mozjs185])
        PKG_CHECK_MODULES([JS], [mozjs185],[],
        [
            echo
            echo "Could not find SpiderMonkey JavaScript library version 1.8.5!"
            echo "Please install the SpiderMonkey development package for your distribution!"
            echo "It's called js-devel for Fedora or libmozjs185-dev for Ubuntu."
            echo "If no package is available for your distribution you can compile libjs from"
            echo "source. The tarball should be available here:"
            echo
            echo "http://ftp.mozilla.org/pub/mozilla.org/js/"
            echo
            echo "If you installed SpiderMonkey manually in a non standard location you can point"
            echo "configure to the directory that is holding your libjs.pc or mozjs185.pc"
            echo "pkg-config files using the --with-js-pkgconfig=/path/ option or by setting the"
            echo "PKG_CONFIG_PATH environment variable prior to running configure."
            echo
            AC_MSG_ERROR([$JS_PKG_ERRORS])
        ])
    ])


PKG_CHECK_MODULES([ICAL], [libical], [],
    [
        AC_MSG_ERROR([$ICAL_PKG_ERRORS])
    ])

CFLAGS_SAVE="$CFLAGS"
CXXFLAGS_SAVE="$CXXFLAGS"
CFLAGS="$CFLAGS $ical_CFLAGS"
CXXFLAGS="$CXXFLAGS $ical_CFLAGS"

AC_CHECK_HEADERS([libical/ical.h], [],
    [
        AC_CHECK_HEADERS([ical.h], [],
        [
            AC_MSG_ERROR([Could not find required ical.h header!])
        ])
    ])

CFLAGS="$CFLAGS_SAVE"
CXXFLAGS="$CXXFLAGS_SAVE"

PKG_CHECK_MODULES([LIBXML2], [libxml-2.0], [],
    [
        AC_MSG_ERROR([$LIBXML2_PKG_ERRORS])
    ])

PKG_CHECK_MODULES([OPENSSL], [openssl], [],
    [
        AC_MSG_ERROR([$OPENSSL_PKG_ERRORS])
    ])

PKG_CHECK_MODULES([AVAHI], [avahi-client],
    [
        AC_DEFINE([HAVE_AVAHI], [1], [avahi support available])
    ],
    [
        AC_MSG_ERROR([$AVAHI_PKG_ERRORS])
    ])

PKG_CHECK_MODULES([LIBRRD], [librrd], [],
    [
        AC_MSG_ERROR([$LIBRRD_PKG_ERRORS])
    ])

MT_CHECK_REQUIRED_PACKAGE([libdsm-api], [digitalSTROM/dsm-api-v2/dsm-api],
                          [dsm-api-v2], [DsmApiOpen], [-lds485-client])


if test "x$SOAP_OPTION_ENABLED" = "xyes"; then
    PKG_CHECK_MODULES([GSOAP], [gsoapssl++], [],
        [
            AC_MSG_ERROR([$GSOAP_PKG_ERRORS])
        ])

    AC_PATH_PROG(GSOAP_SOAPCPP2, soapcpp2, no)
    if test "x$GSOAP_SOAPCPP2" = xno; then
        AC_MSG_ERROR([soapcpp2 utility not found])
    fi
    AC_SUBST(GSOAP_SOAPCPP2)

    GSOAP_IMPORT_DIR=
    AC_ARG_WITH(gsoap-import,
        AC_HELP_STRING([--with-gsoap-import=DIR], [search for gSOAP import headers in DIR]),
        [
            if test "x$withval" != "x"; then
                if (test -d "$withval") && (test -f "$withval/wsa.h") ; then
                    GSOAP_IMPORT_DIR="$withval"
                    AC_MSG_NOTICE([gSOAP import headers path set to $withval])
                else
                    AC_MSG_ERROR([you specified $withval as gSOAP import header directory, but this path does not exist or does not contain valid headers!])
                fi
            fi
        ])

    AC_MSG_CHECKING([for gSOAP import header directory])
    if (test -z $GSOAP_IMPORT_DIR); then
        for GSPATH in /usr/share/gsoap/import \
                      /usr/local/share/gsoap/import ; do
            if (test -z $GSOAP_IMPORT_DIR) && (test -s $GSPATH/wsa.h); then
                GSOAP_IMPORT_DIR=$GSPATH
            fi
        done
    fi
    if (test -z $GSOAP_IMPORT_DIR); then
        AC_MSG_ERROR([could not find gSOAP import header directory])
    fi

    AC_MSG_RESULT([$GSOAP_IMPORT_DIR])
    AC_SUBST(GSOAP_IMPORT_DIR)
    AC_DEFINE([WITH_SOAP], [1], [ build with SOAP support ])
fi

AM_CONDITIONAL(BUILD_SOAP, test "x$SOAP_OPTION_ENABLED" = "xyes")

if test "x$CURL_OPTION_ENABLED" = "xyes"; then
    PKG_CHECK_MODULES([CURL], [libcurl],
        [
            AC_DEFINE([HAVE_CURL], [1], [libcurl support available])
            WITH_CURL=yes
        ],
        [
            AC_MSG_ERROR([$CURL_PKG_ERRORS])
        ])
    AC_SUBST(HAVE_CURL)
fi
AM_CONDITIONAL(WITH_CURL, test "x$WITH_CURL" = "xyes")

if test "x$SENDMAIL" = "x"; then
    AC_PATH_PROG(SENDMAIL, sendmail, no)
    if test "x$SENDMAIL" = "xno"; then
        SENDMAIL=
    else
        AC_DEFINE([HAVE_SENDMAIL], [1], [sendmail program is available])
    fi
fi
AC_DEFINE_UNQUOTED([SENDMAIL], ["$SENDMAIL"], [location of the sendmail program])

AC_PATH_PROG(DOXYGEN, doxygen, no)
if test "x$DOXYGEN" = "xno"; then
    DOXYGEN=
fi

AC_SUBST(DOXYGEN)
AM_CONDITIONAL(HAVE_DOXYGEN, test -n "$DOXYGEN")

eval PACKAGE_DATADIR="${datadir}/${PACKAGE}"
eval PACKAGE_DATADIR="${PACKAGE_DATADIR}"
AC_DEFINE_UNQUOTED(PACKAGE_DATADIR, "$PACKAGE_DATADIR", [data directory])

AC_DEFINE_UNQUOTED([DSS_VERSION], ["$VERSION"], [alias for dSS version define that is used in the code])

AC_DEFINE([HAVE_BUILD_INFO_H], [1], [currently we are always generating the build info header])

AC_DEFINE([USE_LIBXML], [1], [TODO: check why this is a define, libxml2 seems to be required anyway])


AC_CONFIG_FILES([
    Makefile
    build/Makefile
    tools/Makefile
    data/Makefile
])

AC_OUTPUT
