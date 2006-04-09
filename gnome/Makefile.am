desktopdir = $(datadir)/applications
desktop_in_files = gjiten.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

appregistrydir = $(datadir)/application-registry
appregistry_DATA = gjiten.desktop
CLEANFILES = gjiten.applications
EXTRA_DIST = $(desktop_DATA) \
             $(appregistry_DATA)
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(desktopdir)
	$(mkinstalldirs) $(DESTDIR)$(appregistrydir)
