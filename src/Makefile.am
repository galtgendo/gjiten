## Process this file with automake to produce Makefile.in

SYSCONFDIR = @sysconfdir@
bin_PROGRAMS = gjiten

gjiten_SOURCES = 		\
	gjiten.c gjiten.h	\
	kanjidic.c kanjidic.h	\
	pref.c pref.h		\
	conf.c conf.h		\
	error.c error.h		\
	dicfile.c dicfile.h	\
	worddic.c worddic.h	\
	dicutil.c dicutil.h	\
	radical-convtable.h	\
	constants.h

radical_convtable: radical-convtable.txt
	./convtable.pl

EXTRA_DIST = convtable.pl radical-convtable.txt

gjiten_CFLAGS = $(GJITEN_CFLAGS) -Wall -Wunused @DEBUG_FLAGS@
gjiten_LDADD = $(GJITEN_LIBS)
