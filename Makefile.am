AUTOMAKE_OPTIONS = dejagnu

SUBDIRS = src data po

RUNTESTDEFAULTFLAGS = --tool style --srcdir $srcdir/testsuite

EXTRA_DIST = AUTHORS ChangeLog NEWS README COPYING COPYING-DOCS\
             autogen.sh src/florence.server.in.in

if WITH_DOCS
   SUBDIRS += docs
   EXTRA_DIST += gnome-doc-utils.make
endif

