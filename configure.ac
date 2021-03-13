# -------------------------------------------------------------------------------
# Autoconf/automake.
# -------------------------------------------------------------------------------
AC_PREREQ([2.5])
AC_INIT([clipit], [1.4.5], [oss@rspwn.com])
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([src/main.c])
AM_CONFIG_HEADER([config.h])

# -------------------------------------------------------------------------------
# pkg-config.
# -------------------------------------------------------------------------------

AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes, no)
if test x$HAVE_PKG_CONFIG = xno ; then
  AC_MSG_ERROR([Requirement not met: pkg-config not installed])
fi
PKG_PROG_PKG_CONFIG

# -------------------------------------------------------------------------------
# Gettext.
# -------------------------------------------------------------------------------

GETTEXT_PACKAGE=clipit
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext package])

ALL_LINGUAS="ca cs da de es es_ES et fr hr hu it ja nb pl pl pt_BR ro ru sv tr zh_CN"
AM_GLIB_GNU_GETTEXT()
IT_PROG_INTLTOOL([0.40.0], [no-xml])

clipitlocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(clipitlocaledir)

# -------------------------------------------------------------------------------
# Config options.
# -------------------------------------------------------------------------------

AM_MAINTAINER_MODE([disable])

# -------------------------------------------------------------------------------
# Main settings.
# -------------------------------------------------------------------------------
# prefix=/usr

# -------------------------------------------------------------------------------
# Checks for programs.
# -------------------------------------------------------------------------------
AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_MAKE_SET()

# -------------------------------------------------------------------------------
# Checks for libraries.
# -------------------------------------------------------------------------------
AC_ARG_WITH([gtk3], AS_HELP_STRING([--with-gtk3], [Build with the GTK+ 3 interface]))

if test "x$with_gtk3" = "xyes"; then
  PKG_CHECK_MODULES([GTK], [gtk+-3.0])
  APPINDICATOR_MODULE="ayatana-appindicator3-0.1"
else
  PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.10])
  APPINDICATOR_MODULE="ayatana-appindicator-0.1"
fi

# -------------------------------------------------------------------------------
# Application indicator.
# -------------------------------------------------------------------------------

APPINDICATOR_REQUIRED_VERSION=0.2.4

AC_ARG_ENABLE([appindicator],
        AS_HELP_STRING([--enable-appindicator[=@<:@no/auto/yes@:>@]],[Build support for application indicators]),
        [enable_appindicator=$enableval],
        [enable_appindicator="auto"])

if test x$enable_appindicator = xauto ; then
        PKG_CHECK_EXISTS($APPINDICATOR_MODULE= >= $APPINDICATOR_REQUIRED_VERSION,
                [enable_appindicator="yes"],
                [enable_appindicator="no"])
fi

if test x$enable_appindicator = xyes ; then
        PKG_CHECK_MODULES(APPINDICATOR,
                [$APPINDICATOR_MODULE >= $APPINDICATOR_REQUIRED_VERSION],
                [AC_DEFINE(HAVE_APPINDICATOR, 1, [Have AppIndicator])])
fi

AM_CONDITIONAL(HAVE_APPINDICATOR, test x$enable_appindicator = xyes)
AC_SUBST(APPINDICATOR_CFLAGS)
AC_SUBST(APPINDICATOR_LIBS)

AC_SUBST(X11_LIBS, -lX11)


AM_PATH_GLIB_2_0(2.14.0, [], AC_MSG_ERROR(Requirement not met: glib >= 2.14.0))

# -------------------------------------------------------------------------------
# Checks for header files.
# -------------------------------------------------------------------------------
AC_CHECK_HEADERS([stdlib.h string.h])

# -------------------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
# -------------------------------------------------------------------------------
AC_C_CONST()

# -------------------------------------------------------------------------------
# Checks for library functions.
# -------------------------------------------------------------------------------
AC_TYPE_SIGNAL()

AC_CONFIG_FILES([Makefile
		doc/Makefile
		src/Makefile
		data/Makefile])

AC_OUTPUT([po/Makefile.in])
