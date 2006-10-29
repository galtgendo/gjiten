AC_PREREQ(2.52)

GJITEN_VERSION=2.6

AC_INIT(gjiten, 2.6)
AC_CONFIG_SRCDIR(src/gjiten.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

dnl AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
#AM_PROG_CC_STDC
AC_PROG_INSTALL
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.22])
AC_PATH_PROG([GCONFTOOL], [gconftool-2])

GNOME_COMMON_INIT
dnl GNOME_PLATFORM_GNOME_2(yes, force)
PKG_CHECK_MODULES(GJITEN, libgnome-2.0 >= 2.2.0 libgnomeui-2.0 >= 2.2.0 libglade-2.0 gconf-2.0)
AM_GCONF_SOURCE_2

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h fcntl.h locale.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_STAT
AC_CHECK_FUNCS([getcwd putenv setlocale strerror strrchr strstr munmap])

GNOME_COMPILE_WARNINGS(maximum)

# Gettext stuff
GETTEXT_PACKAGE=gjiten
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", ["$GETTEXT_PACKAGE"])
ALL_LINGUAS="ja de de_CH"
AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE(debug,
		[  --enable-debug  Print debugging information. This is a feature which should not be usually enabled.],
                [debugging=yes; DEBUG_FLAGS="-ggdb -DDEBUG_GJITEN"],
                [debugging=no]
	      )
	      AC_SUBST(DEBUG_FLAGS)
#DEBUG_FLAGS="-DGNOME_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DG_DISABLE_DEPRECATED -ggdb -std=c89 -pedantic"],


AC_SUBST(GJITEN_CFLAGS)
AC_SUBST(GJITEN_LIBS)
AC_SUBST(PACKAGE_VERSION)

dnl Set GJITEN_LOCALE_DIR in config.h.
gjiten_prefix="$prefix"
test "x$gjiten_prefix" = xNONE && gjiten_prefix=$ac_default_prefix
GJITEN_LOCALE_DIR="$gjiten_prefix/share/locale"
AC_DEFINE_UNQUOTED(GJITEN_LOCALE_DIR, "$GJITEN_LOCALE_DIR", ["$GJITEN_LOCALE_DIR"])
GJITEN_BINDIR="$gjiten_prefix/bin"
AC_DEFINE_UNQUOTED(GJITEN_BINDIR, "$GJITEN_BINDIR", ["$GJITEN_BINDIR"])
GJITEN_DATADIR="$gjiten_prefix/share/gjiten"
AC_DEFINE_UNQUOTED(GJITEN_DATADIR, "$GJITEN_DATADIR", ["$GJITEN_DATADIR"])
GNOMEDATADIR="$gjiten_prefix/share"
AC_DEFINE_UNQUOTED(GNOMEDATADIR, "$GNOMEDATADIR", ["$GNOMEDATADIR"])
PIXMAPDIR="$gjiten_prefix/share/pixmaps"
AC_DEFINE_UNQUOTED(PIXMAPDIR, "$PIXMAPDIR", ["$PIXMAPDIR"])
MAINTAINERCLEANFILES=Makefile.in
AC_SUBST(MAINTAINERCLEANFILES)
CLEANFILES='*~'
AC_SUBST(CLEANFILES)

AC_OUTPUT([ Makefile 
	    pixmaps/Makefile
	    rpm/gjiten.spec 
	    src/Makefile
	    data/Makefile 
	    help/Makefile 
	    po/Makefile.in 
	    po/Makefile
	    gnome/Makefile 
	    help/C/Makefile 
	    help/ja/Makefile
	    dicfiles/Makefile
	  ])

