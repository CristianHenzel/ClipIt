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
	rm -rf $(distdir)/m4/Makefile

ACLOCAL_AMFLAGS = -I m4

gtk_update_icon_cache = gtk-update-icon-cache -f -t
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache) $(datadir)/icons/hicolor; \
	else \
		echo "*** Icon cache not updated.  After (un)install, run these:"; \
		echo "***   $(gtk_update_icon_cache) $(datadir)/icons/hicolor"; \
	fi
