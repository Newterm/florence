bin_PROGRAMS = florence

INCLUDES = -I$(top_srcdir) -DFLORENCELOCALEDIR=\""$(florencelocaledir)"\"

florence_SOURCES = main.c florence.c keyboard.c key.c trace.c settings.c trayicon.c\
                   layoutreader.c style.c view.c status.c tools.c
florence_CPPFLAGS = -DICONDIR="\"$(datadir)/icons\""\
   -DDATADIR="\"$(datadir)/florence\"" $(DEPS_CFLAGS)\
   $(LIBGNOME_CFLAGS) $(INCLUDES)
florence_LDADD = $(DEPS_LIBS) $(LIBGNOME_LIBS)

