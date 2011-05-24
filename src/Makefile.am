AM_CPPFLAGS = -I$(top_srcdir)
AM_CFLAGS = -Wall -g -DCLIPITLOCALEDIR=\""$(clipitlocaledir)"\" \
		-DCLIPITPIXMAPSDIR=\""$(datarootdir)"/pixmaps/clipit\"
INCLUDES = $(GTK_CFLAGS) $(APPINDICATOR_CFLAGS)
LDADD = $(GTK_LIBS) $(APPINDICATOR_LIBS) $(X11_LIBS)
AM_LDFLAGS = -Wl,--as-needed

bin_PROGRAMS = clipit

DISTCLEANFILES = *.bak *.log *~ .deps/*.P

clipit_SOURCES = main.c main.h \
			utils.c utils.h \
			clipit-i18n.h\
			daemon.c daemon.h \
			history.c history.h \
			keybinder.c keybinder.h \
			preferences.c preferences.h \
			manage.c manage.h \
			eggaccelerators.c eggaccelerators.h
