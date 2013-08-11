pkgdata_DATA = vconj.utf8 radkfile.utf8

gladedir	= $(pkgdatadir)
glade_DATA	= gjiten-settings.ui

#schemas_in_files = apps.gjiten.gschema.xml apps.gjiten-dictfiles.gschema.xml
#gsettings_SCHEMAS = $(schemas_in_files:.schemas.in=.schemas)
gsettings_SCHEMAS = apps.gjiten.gschema.xml

@GSETTINGS_RULES@

EXTRA_DIST = $(gsettings_SCHEMAS) $(glade_DATA) radkfile.utf8 vconj.utf8
