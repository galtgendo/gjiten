sgml_ents = legal.xml
docname = gjiten
lang = C
omffile = gjiten-C.omf
entities = legal.xml
include $(top_srcdir)/help/xmldocs.make

all: $(top_srcdir)/README
dist-hook: app-dist-hook

$(top_srcdir)/README: gjiten.xml
	docbook2txt -u gjiten.xml >gjiten.txt
	-mv gjiten.txt $(top_srcdir)/README

