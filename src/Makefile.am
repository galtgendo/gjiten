## Process this file with automake to produce Makefile.in

SYSCONFDIR = @sysconfdir@
bin_PROGRAMS = gjiten

gjiten_SOURCES = 		\
	gjiten.c gjiten.h	\
	kanjidic.c kanjidic.h	\
	pref.c			\
	conf.c conf.h		\
	error.c error.h		\
	dicfile.c dicfile.h	\
	worddic.c worddic.h	\
	dicutil.c dicutil.h	\
	constants.h

CFLAGS = @CFLAGS@ $(GJITEN_CFLAGS) -O2 -Wall -Wunused -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGNOME_DISABLE_DEPRECATED @DEBUG_FLAGS@
gjiten_LDADD = $(GJITEN_LIBS)
