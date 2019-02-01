@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_file = clipit.desktop.in
desktop_DATA = $(desktop_in_file:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_file = clipit-startup.desktop.in
autostart_DATA = $(autostart_in_file:.desktop.in=.desktop)

pixmapsdir = $(datarootdir)/icons/hicolor/scalable/apps
pixmaps_DATA = clipit-trayicon.svg clipit-trayicon-offline.svg

EXTRA_DIST = \
  $(desktop_in_file) \
  $(autostart_in_file) \
  $(pixmaps_DATA)

CLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA)

DISTCLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA)
