sgml_ents = legal.xml
docname = gjiten
lang = C
omffile = gjiten-C.omf
entities = legal.xml
include $(top_srcdir)/help/xmldocs.make

all: $(top_srcdir)/README
dist-hook: app-dist-hook

$(top_srcdir)/README: gjiten.xml
	xmlto txt gjiten.xml
	-mv gjiten.txt $(top_srcdir)/README

