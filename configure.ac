AC_INIT([Florence Gnome virtual keyboard], [0.1.3], [f.agrech@gmail.com], [florence])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(no-dist-gzip dist-bzip2)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_GCONF_SOURCE_2

# Checks for libraries.
DEP_MODULES="libxml-2.0 gconf-2.0 gmodule-2.0 libglade-2.0 cspi-1.0 libgnomecanvas-2.0 gtk+-2.0 >= 2.10.0"
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)
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

AC_CONFIG_FILES([Makefile src/Makefile data/Makefile])
AC_OUTPUT

