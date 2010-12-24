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
	intltool-merge.in \
	intltool-update.in \
	intltool-extract.in

dist-hook:
	rm -rf $(distdir)/m4/Makefile

ACLOCAL_AMFLAGS = -I m4
