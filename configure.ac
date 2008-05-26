AC_INIT([Florence Gnome virtual keyboard], [0.2.2], [f.agrech@gmail.com], [florence])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(no-dist-gzip dist-bzip2)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_GCONF_SOURCE_2

AC_PATH_PROG(SK_CONFIG,sed,no)
if test x$SK_CONFIG = xno; then
	AC_MSG_ERROR(Couldn't find the sed command. Please install the sed package: ftp://ftp.gnu.org/gnu/sed)
fi

AC_PATH_PROG(SK_CONFIG,scrollkeeper-config,no)
if test x$SK_CONFIG = xno; then
	AC_MSG_ERROR(Couldn't find scrollkeeper-config. Please install the scrollkeeper package: http://scrollkeeper.sourceforge.net)
fi
SCROLLKEEPER_BUILD_REQUIRED=0.3.5
AC_SUBST(SCROLLKEEPER_BUILD_REQUIRED)

# Checks for libraries.
DEP_MODULES="libxml-2.0 gconf-2.0 gmodule-2.0 libglade-2.0 cspi-1.0 libgnomecanvas-2.0 gtk+-2.0 >= 2.10.0"
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_ARG_WITH([help], AS_HELP_STRING([--without-help], [Build without help (default: enabled)]))
if test "x$with_help" != "xno"; then
	PKG_CHECK_MODULES([LIBGNOME], [libgnome-2.0], AC_DEFINE([WITHHELP], [], [Help enabled.]), AC_MSG_ERROR(Couldn't find libgnome.
Please either install libgnome package or disable help: --without-help configure option))
fi

AC_SUBST(LIBGNOME_CFLAGS)
AC_SUBST(LIBGNOME_LIBS)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_MAJOR
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h memory.h stdlib.h string.h strings.h sys/file.h sys/param.h sys/time.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_C_CONST
AC_CHECK_MEMBERS([struct stat.st_blksize])

# Checks for library functions.
AC_CHECK_FUNCS([memchr memset])

AC_CONFIG_FILES([Makefile src/Makefile data/Makefile docs/Makefile docs/C/Makefile docs/fr/Makefile])
AC_OUTPUT

