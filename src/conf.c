/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* conf.c
   
   GJITEN : A JAPANESE DICTIONARY FOR GNOME
  
   Copyright (C) 1999-2005 Botond Botyanszki <boti at rocketmail dot com>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published  by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <glib/gi18n.h>

#include "conf.h"
#include "constants.h"
#include "config.h"
#include "error.h"
#include "dicutil.h"

extern gchar *kanjidicstrg[];
extern gchar *gnome_dialog_msg;
GjitenConfig conf;
GSettings *settings, *ksettings;

GjitenConfig *conf_load() {
  gchar *tmpstrg, *tmpptr;
  int i;
	GjitenDicfile *dicfile;
	GSList *diclist;
	GjitenConfig *conf;

	conf = g_new0(GjitenConfig, 1);

  //conf->version = g_settings_get_string(gconf_client, "/apps/gjiten/general/version", NULL);
	/*
  if (conf->version == NULL) { // FIXME: gconf schema
    conf->kdiccfg[KANJI] = TRUE; 
    conf->kdiccfg[RADICAL] = TRUE;
    conf->kdiccfg[STROKES] = TRUE;
    conf->kdiccfg[READING] = TRUE;
    conf->kdiccfg[ENGLISH] = TRUE;
    conf->kdiccfg[FREQ] = TRUE;   
    conf->kdiccfg[JOUYOU] = TRUE;
    conf->kdiccfg[CREF] = TRUE;
		conf->toolbar = TRUE;
    conf->menubar = TRUE;
    conf->force_ja_JP = TRUE;
    conf->force_language_c = TRUE;
		if (conf->kanjidic == NULL) conf->kanjidic = g_new0(GjitenDicfile, 1);
		conf->kanjidic->path = GJITEN_DICDIR"/kanjidic";
    conf->dictpath = GJITEN_DICDIR;
    conf->searchlimit_enabled = FALSE;
    conf->maxwordmatches = DEFMAXWORDMATCHES;
    conf->autoadjust_enabled = TRUE;
    return conf; 
  }
	*/

  conf->autoadjust_enabled = g_settings_get_boolean(settings, "autoadjust-enabled");

  conf->bigwords = g_settings_get_boolean(settings, "bigwords");
  conf->bigkanji = g_settings_get_boolean(settings, "bigkanji");
  conf->largefont = g_settings_get_string(settings, "largefont");
  conf->normalfont = g_settings_get_string(settings, "normalfont");

  conf->searchlimit_enabled = g_settings_get_boolean(settings, "searchlimit-enabled");
  conf->maxwordmatches = g_settings_get_uint(settings, "maxwordmatches");
	if (conf->maxwordmatches < 1) {
		conf->searchlimit_enabled = FALSE;
		conf->maxwordmatches = 100;
	}

  conf->menubar = g_settings_get_boolean(settings, "menubar");
  conf->toolbar = g_settings_get_boolean(settings, "toolbar");

  conf->search_kata_on_hira = g_settings_get_boolean(settings, "search-kata-on-hira");
  conf->search_hira_on_kata = g_settings_get_boolean(settings, "search-hira-on-kata");
  conf->verb_deinflection = g_settings_get_boolean(settings, "deinflection-enabled");

  if (conf->kanjidic == NULL) conf->kanjidic = g_new0(GjitenDicfile, 1);
  conf->kanjidic->path = g_settings_get_string(ksettings, "kanjidicfile");
	if ((conf->kanjidic->path == NULL) || (strlen(conf->kanjidic->path)) == 0) {
		conf->kanjidic->path = GJITEN_DICDIR"/kanjidic";
	}
  conf->unicode_radicals = g_settings_get_boolean(ksettings, "unicode-radicals");
	

  conf->kanjipad = g_settings_get_string(ksettings, "kanjipad");
	if (conf->kanjipad == NULL) conf->kanjipad = "";

	if (conf->dicfile_list != NULL) {
		dicutil_unload_dic();
		dicfile_list_free(conf->dicfile_list);
		conf->dicfile_list = NULL;
	}

	{ //new config
		GVariantIter iter;
		GVariant *dictionaries;
		dictionaries = g_settings_get_value(settings, "dictionaries");
		g_variant_iter_init(&iter, dictionaries);
		diclist = NULL;
		while (g_variant_iter_next (&iter, "(&s&s)", &tmpstrg, &tmpptr)) {
			if (tmpstrg != NULL) {
				dicfile = g_new0(GjitenDicfile, 1);
				dicfile->path = g_strdup(tmpstrg);
				dicfile->name = g_strdup(tmpptr);
				//				printf("%s\n%s\n", tmpstrg, tmpptr);
				conf->dicfile_list = g_slist_append(conf->dicfile_list, dicfile);
			}
		}
		g_variant_unref(dictionaries);
	}
	if (conf->dicfile_list != NULL) conf->selected_dic = conf->dicfile_list->data;

  //Load kanji info settings
  for (i = 0; i < KCFGNUM; i++) { 
    conf->kdiccfg[i] = g_settings_get_boolean(ksettings, kanjidicstrg[i]);
      /* printf("%s : %d\n",kanjidicstrg[i], conf->kdiccfg[i]); */
  }
 
  {
    char ** history_list = g_settings_get_strv(settings, "history");
    guint history_size = g_strv_length(history_list);
    //Load gjiten search history
    for (i = 0; i <= 50 && i < history_size; i++) {
      conf->history[i] = g_strdup(history_list[i]);
    }
    g_strfreev(history_list);
  }

  return conf;
}

void conf_save(GjitenConfig *conf) {
  int i;
	GSList *diclist;
	GjitenDicfile *dicfile;

  //gconf_client_set_string(gconf_client, "/apps/gjiten/general/version", VERSION, NULL);
  //Save kanjidic display options
  for (i = 0; i < KCFGNUM; i++) { 
    g_settings_set_boolean(ksettings, kanjidicstrg[i], conf->kdiccfg[i]);
  }
  
  //gconf_client_set_bool(gconf_client, "/apps/gjiten/general/tooltips", conf->tooltips, NULL);
  g_settings_set_boolean(settings, "menubar", conf->menubar);
  g_settings_set_boolean(settings, "toolbar", conf->toolbar);
  g_settings_set_string(ksettings, "kanjidicfile", conf->kanjidic->path);
  g_settings_set_boolean(ksettings, "unicode-radicals", conf->unicode_radicals);

	if (conf->kanjipad == NULL) conf->kanjipad = "";
  g_settings_set_string(ksettings, "kanjipad", conf->kanjipad);

  g_settings_set_boolean(settings, "bigwords", conf->bigwords);
  g_settings_set_boolean(settings, "bigkanji", conf->bigkanji);
  g_settings_set_string(settings, "largefont", conf->largefont == NULL ? "" : conf->largefont);
  g_settings_set_string(settings, "normalfont", conf->normalfont == NULL ? "" : conf->normalfont);

  g_settings_set_boolean(settings, "search-kata-on-hira", conf->search_kata_on_hira);
  g_settings_set_boolean(settings, "search-hira-on-kata", conf->search_hira_on_kata);
  g_settings_set_boolean(settings, "deinflection-enabled", conf->verb_deinflection);

  {
  //Save dicfiles [path and name seperated with linebreak]
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a(ss)"));
	diclist = conf->dicfile_list;
	while (diclist != NULL) {
		if (diclist->data == NULL) break;
		dicfile = diclist->data;
		g_variant_builder_add(&builder, "(ss)", dicfile->path, dicfile->name);
		diclist = g_slist_next(diclist);
	}
	g_settings_set_value(settings, "dictionaries", g_variant_builder_end(&builder));
  }
}

void conf_save_history(GList *history, GjitenConfig *conf) {
  GArray *array;
  int i;
  array = g_array_new(TRUE, TRUE, sizeof(gchar *));
  if (history != NULL) {
    for (i = 0; i <= 50; i++) {
      array = g_array_append_val(array, history->data);
      history = g_list_next(history);
      if (history == NULL) break;
    } 
    g_settings_set_strv(settings, "history", (const gchar **)array->data);
  }
  g_array_free(array, TRUE);
}

void conf_save_options(GjitenConfig *conf) {
    g_settings_set_boolean(settings, "autoadjust-enabled", conf->autoadjust_enabled);
    g_settings_set_boolean(settings, "searchlimit-enabled", conf->searchlimit_enabled);
    g_settings_set_uint(settings, "maxwordmatches", conf->maxwordmatches);
}

gboolean conf_init_handler() {
#if !GLIB_CHECK_VERSION(2, 32, 0)
  g_type_init();
#endif

  if (settings == NULL) {
    settings = g_settings_new("apps.gjiten.general");
    ksettings = g_settings_new("apps.gjiten.kanjidic");
  }

  return TRUE;

}

void conf_close_handler() {
	if (settings != NULL) {
		GJITEN_DEBUG("calling g_object_unref(G_OBJECT(settings)) [%d]\n", (int) settings);
		g_object_unref(G_OBJECT(settings));
		g_object_unref(G_OBJECT(ksettings));
		settings = NULL;
		ksettings = NULL;
	}
}
