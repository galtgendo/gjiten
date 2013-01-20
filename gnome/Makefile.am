desktopdir = $(datadir)/applications
desktop_in_files = gjiten.desktop.in
EXTRA_DIST = $(desktop_in_files)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(desktopdir)
