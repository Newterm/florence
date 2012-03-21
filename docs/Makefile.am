include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = florence
DOC_ENTITIES = gfdl.dbk
DOC_INCLUDES =
DOC_FIGURES = figures/behaviour.png figures/florence.png figures/layout.png \
              figures/style.png figures/window.png figures/minimize.png \
              figures/configuration.png figures/move.png figures/bigger.png \
              figures/smaller.png figures/close.png figures/switch.png
DOC_LINGUAS = fr ru

man_MANS = florence.1 florence_applet.1
EXTRA_DIST = $(man_MANS)

install-data-hook:
        @list_install='$(man_MANS)';                              \
        for i in $$list_install; do                               \
          s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`;      \
          n=`echo $$i | sed -e 's/-[^-]*$$//'`;           \
          f="$$n.$$s";                                    \
          echo "cd $(DESTDIR)$(mandir)/man$$s";           \
          cd $(DESTDIR)$(mandir)/man$$s;                  \
          echo "rm -f $$f";                               \
          rm -f $$f;                                      \
          echo "ln -s $$i $$f";                           \
          ln -s $$i $$f;                                  \
        done

uninstall-local:
        @list_uninst='$(man_MANS)';                              \
        for i in $$list_uninst; do                               \
          s=`echo $$i | sed -e 's/.*\\(.\\)$$/\1/'`;      \
          n=`echo $$i | sed -e 's/-[^-]*$$//'`;           \
          f="$$n.$$s";                                    \
          echo " rm -f $(DESTDIR)$(mandir)/man$$s/$$f";   \
          rm -f $(DESTDIR)$(mandir)/man$$s/$$f;           \
        done

