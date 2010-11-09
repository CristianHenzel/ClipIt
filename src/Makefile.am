AM_CFLAGS = -I$(top_srcdir) -DCLIPITLOCALEDIR=\""$(clipitlocaledir)"\"
INCLUDES = $(GTK_CFLAGS)
LDADD = $(GTK_LIBS)

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
