#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([dss], [1.31.2], [http://redmine.digitalstrom.org])
AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_SRCDIR([src/dss.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.10 -Wall subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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

MT_OPTION([debug], [enable],
          [enable debug flags during compilation],
          [
            CFLAGS="-g -Wall -O0"
            CXXFLAGS="-g -Wall -O0"
          ],
          [])

GCOV_REPORT=
MT_OPTION([test-coverage], [enable],
          [enable gcov test coverage report (implies debug build)],
          [
            GCOV_CFLAGS="-fprofile-arcs -ftest-coverage"
            GCOV_LIBS="-lgcov"
            CFLAGS="-g -Wall -O0"
            CXXFLAGS="-g -Wall -O0"
            GCOV_REPORT=yes
          ],
          [])

AC_SUBST(GCOV_CFLAGS)
AC_SUBST(GCOV_LIBS)
AM_CONDITIONAL(BUILD_GCOV_REPORT, test "x$GCOV_CFLAGS" != "x")

MT_OPTION([stack-protect], [enable],
          [emit code to check for buffer overflows],
          [
            CXXFLAGS+=' -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4'
            CXXFLAGS+=" -D_FORTIFY_SOURCE=2"
            CFLAGS+=' -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4'
            CFLAGS+=" -D_FORTIFY_SOURCE=2"
            LDFLAGS+=" -Wl,-z,relro,-z,now"
          ],
          [])
AM_CONDITIONAL(BUILD_STACK_PROTECT, test "x$STACK_PROTECT_OPTION_ENABLED" = "xyes")

MT_OPTION([libtcmalloc], [disable],
          [build dSS without libtcmalloc support (default: enabled)],
          [],[])

MT_OPTION([http], [enable],
          [build with HTTP support (default: HTTPS only)],
          [
            AC_DEFINE([WITH_HTTP], [1], [ build with HTTP support ])
          ],[])

MT_OPTION([insecure-cookie], [enable],
          [allow cookies also over HTTP (default: HTTPS only)],
          [
            AC_DEFINE([WITH_INSECURE_COOKIE], [1], [ allow insecure cookies ])
          ],[])

MT_OPTION([bonjour], [disable],
          [build with bonjour support (default: enabled)],
          [
            AC_DEFINE([WITH_BONJOUR], [1], [ build with bonjour support ])
          ],[])

MT_OPTION([sendmail], [disable],
          [build dSS without sendmail support (default: enabled)],
          [], [])

MT_OPTION([mail-spool], [enable],
          [build dSS with mail spool directory support (default: disabled)],
          [
            AC_DEFINE([HAVE_MAILSPOOL], [1], [ build with mail spool support ])
          ], [])

if (test "x$SENDMAIL_OPTION_ENABLED" = "xyes") &&
   (test "x$SENDMAIL_OPTION_REQUESTED" = "xyes") &&
   (test "x$MAIL_SPOOL_OPTION_ENABLED" = "xyes") &&
   (test "x$MAIL_SPOOL_OPTION_REQUESTED" = "xyes"); then
    AC_MSG_ERROR([You enabled the sendmail client option and also the mail spool directory option, please select one of the two!])
fi

if (test "x$MAIL_SPOOL_OPTION_ENABLED" = "xyes"); then
    SENDMAIL_OPTION_ENABLED=no
    AC_MSG_NOTICE([Disabling sendmail because mail-spool feature was requested])
fi

SENDMAIL=
AC_ARG_WITH(sendmail-location,
    AC_HELP_STRING([--with-sendmail-location=SENDMAIL], [hardcode location of sendmail binary, no checks will be performed (useful for cross compiling)]),
    [
        if test "x$SENDMAIL_OPTION_ENABLED" = "xyes"; then
            if test "x$withval" != "x"; then
                AC_MSG_NOTICE([Hardcoding sendmail location to $withval])
                SENDMAIL="$withval"
                AC_DEFINE([HAVE_SENDMAIL], [1], [sendmail program is available])
            fi
        fi
    ])

MAILDIR=
AC_ARG_WITH(mail-dir,
    AC_HELP_STRING([--with-mail-dir=MAILDIR], [location of mail directory)]),
    [
        if test "x$withval" != "x"; then
            AC_MSG_NOTICE([Setting mail directory to $withval])
            MAILDIR="$withval"
        fi
    ])
AC_DEFINE_UNQUOTED([MAILDIR], ["$MAILDIR"], [mail queue directory])

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

DISTR_VERSION_FILE=
AC_ARG_WITH(distro-version-file,
    AC_HELP_STRING([--with-distro-version-file=FILENAME], [read distro version information from specified file (must be a a text file, first line will be interpreted as a distro version)]),
    [
        if test "x$withval" != "x"; then
            AC_MSG_NOTICE([Will read distro version information from  $withval])
            DISTRO_VERSION_FILE="$withval"
            AC_DEFINE_UNQUOTED([DISTRO_VERSION_FILE], ["$withval"], [read distro version from here])
        fi
    ])

AC_ARG_WITH(dsstests-files,
    AC_HELP_STRING([--with-dsstests-files=DIR], [read dsstests input files from DIR]),
    [
        if test "x$withval" != "x"; then
            AC_DEFINE([WITH_DSSTEST_FILES], [1], [custom dsstest files directory set])
            AC_DEFINE_UNQUOTED([WITH_DSSTEST_FILES], ["$withval"], [dsstest files directory])
        fi
    ])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h sys/resource.h], [], [AC_MSG_ERROR([required system header not found])])

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
AC_CHECK_FUNCS(dup2 gethostbyname gettimeofday inet_ntoa localtime_r memchr memmove memset select setlocale socket strchr strcspn strdup strerror strrchr strspn strstr strtol strtoul setrlimit, [],
        [ AC_MSG_ERROR([required function not available]) ])

AM_ICONV

AC_CHECK_HEADERS(sys/prctl.h)
AC_CHECK_FUNCS(prctl)

# Check for execinfo.h and backtrace
AC_CHECK_HEADERS(execinfo.h,
    [
        AC_CHECK_FUNCS(backtrace backtrace_symbols,
            [
                BACKTRACE_LDFLAGS="-rdynamic"
                AC_SUBST(BACKTRACE_LDFLAGS)
            ],
            [])
    ]
    []
)

# For mongoose
# Function dlsym() comes from the C library or -ldl
AC_CHECK_FUNCS(
    [dlsym],
    [],
    [ AC_SEARCH_LIBS([dlsym], [dl]) ])

AC_CHECK_FUNCS([clock_gettime])
if test $ac_cv_func_clock_gettime = no; then
    AC_CHECK_LIB(rt, clock_gettime, [LIBS="-lrt $LIBS"])
fi


# Check for boost
AC_CHECK_HEADERS([boost/algorithm/string/finder.hpp boost/algorithm/string.hpp boost/algorithm/string/replace.hpp boost/atomic.hpp boost/bind.hpp boost/chrono.hpp boost/enable_shared_from_this.hpp boost/filesystem.hpp boost/filesystem/operations.hpp boost/filesystem/path.hpp boost/flyweight.hpp boost/foreach.hpp boost/function.hpp boost/lambda/lambda.hpp boost/make_shared.hpp boost/make_shared.hpp boost/noncopyable.hpp boost/ptr_container/ptr_deque.hpp boost/ptr_container/ptr_map.hpp boost/ptr_container/ptr_vector.hpp boost/ref.hpp boost/scoped_ptr.hpp boost/shared_array.hpp boost/shared_ptr.hpp boost/smart_ptr.hpp boost/thread/condition_variable.hpp boost/thread.hpp boost/thread/locks.hpp boost/thread/mutex.hpp boost/thread/recursive_mutex.hpp boost/thread/thread.hpp boost/tuple/tuple.hpp boost/weak_ptr.hpp], [],
    [
        AC_MSG_ERROR([required boost headers not found on your system!])
    ])

AX_BOOST_BASE([1.44.0],[],[ AC_MSG_ERROR([boost was not found on your system])])
AX_BOOST_SYSTEM()
AX_BOOST_FILESYSTEM()
AX_BOOST_PROGRAM_OPTIONS()
AX_BOOST_THREAD()
AX_BOOST_UNIT_TEST_FRAMEWORK()

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
CFLAGS="$CFLAGS $ICAL_CFLAGS"
CXXFLAGS="$CXXFLAGS $ICAL_CFLAGS"

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

MT_CHECK_REQUIRED_PACKAGE([librrd], [rrd], [rrd], [rrd_create_r], [-lrrd_th])

MT_CHECK_REQUIRED_PACKAGE([expat], [expat], [expat], [XML_ParserCreate])

PKG_CHECK_MODULES([JSONC], [json-c], [],
    [
        PKG_CHECK_MODULES([JSONC], [json], [],
        [
            AC_MSG_ERROR([$JSON_PKG_ERRORS])
        ])
    ])

PKG_CHECK_MODULES([CURL], [libcurl],
    [
        AC_DEFINE([HAVE_CURL], [1], [libcurl support available])
    ],
    [
        AC_MSG_ERROR([$CURL_PKG_ERRORS])
    ])
AC_SUBST(HAVE_CURL)

if test "x$LIBTCMALLOC_OPTION_ENABLED" = "xyes"; then
    PKG_CHECK_MODULES([LIBTCMALLOC], [libtcmalloc],
        [
            AC_DEFINE([HAVE_LIBTCMALLOC], [1], [libtcmalloc support available])
            WITH_LIBTCMALLOC=yes
        ],
        [
            AC_MSG_NOTICE([libtcmalloc not available: $LIBTCMALLOC_PKG_ERRORS])
        ])
    AC_SUBST(HAVE_LIBTCMALLOC)
fi
AM_CONDITIONAL(WITH_LIBTCMALLOC, test "x$WITH_LIBTCMALLOC" = "xyes")


AC_PATH_PROG(DOXYGEN, doxygen, no)
if test "x$DOXYGEN" = "xno"; then
    DOXYGEN=
fi

AC_SUBST(DOXYGEN)
AM_CONDITIONAL(HAVE_DOXYGEN, test -n "$DOXYGEN")

if test "x$GCOV_REPORT" = "xyes"; then
    AC_PATH_PROG(LCOV, lcov, no)
    if test "x$LCOV" = xno; then
        AC_MSG_ERROR([lcov utility not found])
    fi
    AC_SUBST(LCOV)

    AC_PATH_PROG(GENHTML, genhtml, no)
    if test "x$GENHTML" = xno; then
        AC_MSG_ERROR([genhtml utility not found])
    fi
    AC_SUBST(GENHTML)
fi


if (test "x$SENDMAIL_OPTION_ENABLED" = "xyes") && (test "x$SENDMAIL" = "x"); then
    AC_PATH_PROG(SENDMAIL, sendmail, no)
    if test "x$SENDMAIL" = "xno"; then
        SENDMAIL=
    else
        AC_DEFINE([HAVE_SENDMAIL], [1], [sendmail program is available])
    fi
fi
AC_DEFINE_UNQUOTED([SENDMAIL], ["$SENDMAIL"], [location of the sendmail program])

PKG_CHECK_MODULES([LIBPROTOBUF], [protobuf], [],
    [
        AC_MSG_ERROR([$LIBPROTBUF_PKG_ERRORS])
    ])

PKG_CHECK_MODULES([LIBCOMMCHANNEL], [libcommchannel], [],
    [
        echo
        echo "Get libcommchannel from"
        echo "      http://gitorious.digitalstrom.org/dss-misc/libcommchannel"
        echo
        AC_MSG_ERROR([$LIBCOMMCHANNEL_PKG_ERRORS])
    ])

PKG_CHECK_MODULES([LIBDSUID], [libdsuid], [],
    [
        echo
        echo "Get libdsuid from"
        echo "      http://gitorious.digitalstrom.org/ds485-stack/libdsuid"
        echo
        AC_MSG_ERROR([$LIBDSUID_PKG_ERRORS])
    ])

MT_CHECK_REQUIRED_PACKAGE([libdsm-api], [digitalSTROM/dsm-api-v2/dsm-api],
                          [dsm-api-v2], [DsmApiOpen], [-lds485-client -ldsuid])

PKG_CHECK_MODULES([OSSP_UUID], [ossp-uuid], [],
    [
        PKG_CHECK_MODULES([OSSP_UUID], [uuid], [],
    [
        AC_MSG_ERROR([$OSSP_UUID_PKG_ERRORS])
    ])
    ])

PKG_CHECK_MODULES([RAPIDJSON], [RapidJSON], [],
    [
        AC_MSG_WARN([$RAPIDJSON_PKG_ERRORS])
        AC_CHECK_HEADERS([rapidjson/rapidjson.h], [],
        [
            AC_MSG_ERROR([Could not find required header rapidjson.h!])
        ])
    ])

PKG_CHECK_MODULES([SQLITE3], [sqlite3], [],
    [
        AC_MSG_ERROR([$SQLITE3_PKG_ERRORS])
    ])

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
    doc/doxygen.conf
])

AC_OUTPUT
