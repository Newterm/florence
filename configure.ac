AC_INIT([Florence Gnome virtual keyboard], [0.4.0], [f.agrech@gmail.com], [florence])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(no-dist-gzip dist-bzip2)

AC_ARG_WITH([docs], AS_HELP_STRING([--without-docs], [Build without docs (default: enabled)]))
AM_CONDITIONAL([WITH_DOCS], [test "x$with_docs" != "xno"])

# Internationalization
AC_PROG_INTLTOOL([0.23])
GETTEXT_PACKAGE=florence
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE(GETTEXT_PACKAGE, ["florence"], [Package name for gettext])
ALL_LINGUAS="fr"
AM_GLIB_GNU_GETTEXT
florencelocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(florencelocaledir)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_GREP
AC_PROG_SED
AM_GCONF_SOURCE_2

if test "x$with_docs" != "xno"; then
	AC_PATH_PROG(SK_CONFIG,scrollkeeper-config,no)
	if test x$SK_CONFIG = xno; then
		AC_MSG_ERROR(Couldn't find scrollkeeper-config. Please install the scrollkeeper package: http://scrollkeeper.sourceforge.net)
	fi
	SCROLLKEEPER_BUILD_REQUIRED=0.3.5
	AC_SUBST(SCROLLKEEPER_BUILD_REQUIRED)
fi

# Checks for libraries.
DEP_MODULES="gmodule-2.0 cairo librsvg-2.0 libxml-2.0 gconf-2.0 libglade-2.0 cspi-1.0 gtk+-2.0 >= 2.10.0"
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

if test "x$with_docs" != "xno"; then
	PKG_CHECK_MODULES([LIBGNOME], [libgnome-2.0], AC_DEFINE([ENABLE_HELP], [], [Help enabled.]), AC_MSG_ERROR(Couldn't find libgnome.
Please either install libgnome package or disable docs: --without-docs configure option))
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
AC_CHECK_HEADERS([math.h fcntl.h libintl.h locale.h memory.h stdio.h stdlib.h string.h strings.h sys/param.h sys/stat.h sys/types.h dirent.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STAT
AC_C_CONST
AC_CHECK_MEMBERS([struct stat.st_blksize])

# Checks for library functions.
AC_CHECK_FUNCS([memset sscanf strrchr strcpy strncpy strcat atoi putenv fopen fclose fgets fputs lstat opendir readdir closedir])

# Files generated
AC_CONFIG_FILES([Makefile src/Makefile data/Makefile data/styles/Makefile data/styles/default/Makefile data/styles/bright/Makefile data/styles/hard/Makefile data/relaxng/Makefile data/svg11/Makefile data/florence.xml.in docs/Makefile docs/C/Makefile docs/fr/Makefile po/Makefile.in])
AC_OUTPUT

