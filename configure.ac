AC_PREREQ(2.60)

AC_INIT([gjiten], [2.6])
AC_CONFIG_SRCDIR(src/gjiten.c)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_HEADERS(config.h)

AC_PROG_CC
#AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
IT_PROG_INTLTOOL([0.41.1])
AC_PATH_PROG([GCONFTOOL], [gconftool-2])

GNOME_DOC_INIT
dnl GNOME_PLATFORM_GNOME_2(yes, force)
PKG_CHECK_MODULES(GJITEN, gtk+-2.0 >= 2.24.0)
GLIB_GSETTINGS

# Checks for header files.
AC_CHECK_HEADERS_ONCE([errno.h fcntl.h locale.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS_ONCE([getcwd putenv setlocale strerror strrchr strstr munmap])

# Gettext stuff
GETTEXT_PACKAGE=gjiten
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", ["$GETTEXT_PACKAGE"])
AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE(debug,
                [AS_HELP_STRING([--enable-debug], [Print debugging information. This is a feature which should not be usually enabled.])],
                [debugging=yes; DEBUG_FLAGS="-ggdb -DDEBUG_GJITEN"],
                [debugging=no] )
AC_SUBST(DEBUG_FLAGS)
#DEBUG_FLAGS="-DGNOME_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED -ggdb -std=c89 -pedantic"],


dnl Set GJITEN_LOCALE_DIR in config.h.
gjiten_prefix="$prefix"
test "x$gjiten_prefix" = xNONE && gjiten_prefix=$ac_default_prefix
GJITEN_LOCALE_DIR="$gjiten_prefix/share/locale"
AC_DEFINE_UNQUOTED(GJITEN_LOCALE_DIR, "$GJITEN_LOCALE_DIR", ["$GJITEN_LOCALE_DIR"])
GJITEN_DATADIR="$gjiten_prefix/share/gjiten"
AC_DEFINE_UNQUOTED(GJITEN_DATADIR, "$GJITEN_DATADIR", ["$GJITEN_DATADIR"])
PIXMAPDIR="$gjiten_prefix/share/pixmaps"
AC_DEFINE_UNQUOTED(PIXMAPDIR, "$PIXMAPDIR", ["$PIXMAPDIR"])

AC_CONFIG_FILES([ Makefile
	    pixmaps/Makefile
	    src/Makefile
	    data/Makefile 
	    help/Makefile 
	    po/Makefile.in 
	    gnome/Makefile 
	    help/ja/Makefile
	    dicfiles/Makefile
	  ])
AC_OUTPUT
