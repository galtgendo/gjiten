desktopdir = $(datadir)/applications
desktop_DATA =	gjiten.desktop

appregistrydir = $(datadir)/application-registry
appregistry_DATA = gjiten.applications

EXTRA_DIST = $(desktop_DATA) \
             $(appregistry_DATA)
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(desktopdir)
	$(mkinstalldirs) $(DESTDIR)$(appregistrydir)
