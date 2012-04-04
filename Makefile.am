AUTOMAKE_OPTIONS = gnu

SUBDIRS = data doc src po

DISTCLEANFILES = *.bak *.log *~

EXTRA_DIST = \
	m4 \
	autogen.sh \
	config.rpath \
	m4/ChangeLog \
	$(desktop_DATA) \
	$(autostart_DATA) \
	$(pixmaps_DATA) \
	intltool-merge.in \
	intltool-update.in \
	intltool-extract.in

dist-hook:
	update-po
	rm -rf $(distdir)/m4/Makefile

ACLOCAL_AMFLAGS = -I m4

gtk_update_icon_cache = gtk-update-icon-cache -f -t
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

check-gettext:
	@if test x$(USE_NLS) != "xyes" ; then echo "Missing gettext. Rerun configure and check for" \
	"'checking whether to use NLS... yes'!" ; exit 1 ; fi

update-po: check-gettext
	@find $(srcdir)/ -name '*.c' -o -name '*.desktop.in' | sed "s/^.\///" | grep -v "eggaccelerators" | sort > $(srcdir)/po/POTFILES.in.2 ; \
	cat $(srcdir)/po/POTFILES.in.2 ; \
	if diff $(srcdir)/po/POTFILES.in $(srcdir)/po/POTFILES.in.2 >/dev/null 2>&1 ; then \
		rm -f $(srcdir)/po/POTFILES.in.2 ; \
	else \
		mv $(srcdir)/po/POTFILES.in.2 $(srcdir)/po/POTFILES.in ; \
	fi
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache) $(datadir)/icons/hicolor; \
	else \
		echo "*** Icon cache not updated.  After (un)install, run these:"; \
		echo "***   $(gtk_update_icon_cache) $(datadir)/icons/hicolor"; \
	fi
