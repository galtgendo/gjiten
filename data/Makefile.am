pkgdata_DATA = vconj.utf8 radkfile.utf8

gladedir	= $(pkgdatadir)
glade_DATA	= gjiten-settings.glade

schemasdir       = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = gjiten.schemas.in gjiten-dicfiles.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

CLEANFILES = *.bak *.gladep $(schemas_DATA)

EXTRA_DIST = $(schemas_in_files) $(glade_DATA) radkfile.utf8 vconj.utf8

if GCONF_SCHEMAS_INSTALL
install-data-local: $(schemas_DATA)
        GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schemas_DATA);
endif
