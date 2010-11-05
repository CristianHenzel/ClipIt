@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_file = clipit.desktop.in
desktop_DATA = $(desktop_in_file:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_file = clipit-startup.desktop.in
autostart_DATA = $(autostart_in_file:.desktop.in=.desktop)

EXTRA_DIST = \
  $(desktop_in_file) \
  $(autostart_in_file)

CLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA)

DISTCLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA)
