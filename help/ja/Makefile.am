gjiten_helpdir = $(datadir)/gnome/help/gjiten/ja

gjiten_help_DATA = 	\
	gjiten-doc.ja.html		\
	topic.dat

EXTRA_DIST = gjiten-doc.ja.html topic.dat

#all: ../../README.ja

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gjiten_helpdir)

#../../README.ja:
#	lynx -dump -image_links -nolist gjiten-doc.ja.html >../../README.ja
