AC_INIT([Florence Gnome virtual keyboard], [0.0.2], [f.agrech@gmail.com], [florence])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(no-dist-gzip dist-bzip2)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
DEP_MODULES="cspi-1.0 libgnomecanvas-2.0"
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

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

