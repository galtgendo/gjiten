ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src data help po gnome pixmaps dicfiles

README: help/C/gjiten.xml
	xmlto txt help/C/gjiten.xml
	-mv gjiten.txt $(top_srcdir)/README

EXTRA_DIST = README TODO ChangeLog COPYING rpm/gjiten.spec scripts/make_debs scripts/make_release
