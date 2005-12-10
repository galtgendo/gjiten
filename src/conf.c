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
#include <libgnome/libgnome.h>

#include "conf.h"
#include "constants.h"
#include "config.h"
#include "error.h"
#include "dicutil.h"

extern gchar *kanjidicstrg[];
extern gchar *gnome_dialog_msg;
GjitenConfig conf;
GConfClient *gconf_client;

GjitenConfig *conf_load() {
  gchar *dicprefix = "/apps/gjiten/general/dic";
  gchar *tmpstrg;
  gchar historystr[31];
  gchar *tmpptr, *endptr;
  gchar *gnomekcfg = "/apps/gjiten/kanjidic/";
  int i;
	GjitenDicfile *dicfile;
	GSList *gconf_diclist = NULL;
	GSList *diclist;
	GjitenConfig *conf;

	conf = g_new0(GjitenConfig, 1);

  conf->version = gconf_client_get_string(gconf_client, "/apps/gjiten/general/version", NULL);
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

  conf->autoadjust_enabled = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/autoadjust_enabled", NULL);

  conf->bigwords = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/bigwords", NULL);
  conf->bigkanji = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/bigkanji", NULL);
  conf->largefont = gconf_client_get_string(gconf_client, "/apps/gjiten/general/largefont", NULL);
  conf->normalfont = gconf_client_get_string(gconf_client, "/apps/gjiten/general/normalfont", NULL);
  conf->gdk_use_xft = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/gdk_use_xft", NULL);
  conf->force_ja_JP = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/force_ja_JP", NULL);
  conf->force_language_c = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/force_language_c", NULL);
  conf->envvar_override = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/envvar_override", NULL);

  conf->searchlimit_enabled = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/searchlimit_enabled", NULL);
  conf->maxwordmatches = gconf_client_get_int(gconf_client, "/apps/gjiten/general/maxwordmatches", NULL);
	if (conf->maxwordmatches < 1) {
		conf->searchlimit_enabled = FALSE;
		conf->maxwordmatches = 100;
	}

  conf->dictpath = gconf_client_get_string(gconf_client, "/apps/gjiten/general/dictpath", NULL);
  conf->menubar = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/menubar", NULL);
  conf->toolbar = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/toolbar", NULL);

  conf->search_kata_on_hira = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/search_kata_on_hira", NULL);
  conf->search_hira_on_kata = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/search_hira_on_kata", NULL);
  conf->verb_deinflection = gconf_client_get_bool(gconf_client, "/apps/gjiten/general/deinflection_enabled", NULL);

  if (conf->kanjidic == NULL) conf->kanjidic = g_new0(GjitenDicfile, 1);
  conf->kanjidic->path = gconf_client_get_string(gconf_client, "/apps/gjiten/kanjidic/kanjidicfile", NULL);
	if ((conf->kanjidic->path == NULL) || (strlen(conf->kanjidic->path)) == 0) {
		conf->kanjidic->path = GJITEN_DICDIR"/kanjidic";
	}
  conf->unicode_radicals = gconf_client_get_bool(gconf_client, "/apps/gjiten/kanjidic/unicode_radicals", NULL);
	

  conf->kanjipad = gconf_client_get_string(gconf_client, "/apps/gjiten/general/kanjipad", NULL);
	if (conf->kanjipad == NULL) conf->kanjipad = "";

  conf->numofdics = gconf_client_get_int(gconf_client, "/apps/gjiten/general/numofdics", NULL);

	if (conf->dicfile_list != NULL) {
		dicutil_unload_dic();
		dicfile_list_free(conf->dicfile_list);
		conf->dicfile_list = NULL;
	}

	if (conf->numofdics != 0) {
		//Load dicfiles from old style config [compatibility with older versions]
		for (i = 0; i < conf->numofdics; i++) {
			//if (i == MAXDICFILES - 1) break;
			tmpstrg = g_strdup_printf("%s%d", dicprefix, i); 
			dicfile = g_new0(GjitenDicfile, 1);
			dicfile->name = gconf_client_get_string(gconf_client, tmpstrg, NULL);
			if (conf->dictpath[strlen(conf->dictpath - 1)] == '/') {
				dicfile->path = g_strdup_printf("%s%s", conf->dictpath, dicfile->name);
			}
			else {
				dicfile->path = g_strdup_printf("%s/%s", conf->dictpath, dicfile->name);
			}
			conf->dicfile_list = g_slist_append(conf->dicfile_list, dicfile);
			g_free(tmpstrg);
		}
	}
	else { //new config
		gconf_diclist = gconf_client_get_list(gconf_client, GCONF_PATH_GENERAL"/dictionary_list", GCONF_VALUE_STRING, NULL);
		diclist = gconf_diclist;
		while (diclist != NULL) {
			if (diclist->data == NULL) break;
			tmpstrg = diclist->data;
			if (tmpstrg != NULL) {
				tmpptr = tmpstrg;
				endptr = tmpptr + strlen(tmpstrg);
				while ((tmpptr != endptr) && (*tmpptr != '\n')) tmpptr++;
				if (*tmpptr == '\n') {
					*tmpptr = 0;
					tmpptr++;
				}
				dicfile = g_new0(GjitenDicfile, 1);
				dicfile->path = g_strdup(tmpstrg);
				dicfile->name = g_strdup(tmpptr);
				//				printf("%s\n%s\n", tmpstrg, tmpptr);
				conf->dicfile_list = g_slist_append(conf->dicfile_list, dicfile);
			}
			diclist = g_slist_next(diclist);
		}
	}
	if (conf->dicfile_list != NULL) conf->selected_dic = conf->dicfile_list->data;

  //Load kanji info settings
  for (i = 0; i < KCFGNUM; i++) { 
		tmpptr = g_strdup_printf("%s%s", gnomekcfg, kanjidicstrg[i]);
    if (gconf_client_get_bool(gconf_client, tmpptr, NULL)) {
      conf->kdiccfg[i] = TRUE;
      /* printf("%s : %d\n",kanjidicstrg[i], conf->kdiccfg[i]); */
    }
    else conf->kdiccfg[i] = FALSE; 
    g_free(tmpptr);
  }
 
  //Load gjiten search history
  for (i = 0; i <= 50; i++) {
    snprintf(historystr, 31, "/apps/gjiten/history/history%d", i);
    conf->history[i] = gconf_client_get_string(gconf_client, historystr, NULL);
    if (conf->history[i] == NULL) break;
  }

  return conf;
}

void conf_save(GjitenConfig *conf) {
  gchar *gnomekcfg = "/apps/gjiten/kanjidic/";
  int i;
  gchar *confpath, *tmpstrg;
	GConfValue *gconfList;
	GSList *gconf_diclist = NULL;
	GSList *diclist;
	GjitenDicfile *dicfile;

  gconf_client_set_string(gconf_client, "/apps/gjiten/general/version", VERSION, NULL);
  //Save kanjidic display options
  for (i = 0; i < KCFGNUM; i++) { 
		confpath = g_strdup_printf("%s%s", gnomekcfg, kanjidicstrg[i]);
    gconf_client_set_bool(gconf_client, confpath, conf->kdiccfg[i], NULL);
    g_free(confpath);
  }
  
  //gconf_client_set_bool(gconf_client, "/apps/gjiten/general/tooltips", conf->tooltips, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/menubar", conf->menubar, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/toolbar", conf->toolbar, NULL);
  gconf_client_set_string(gconf_client, "/apps/gjiten/general/dictpath", conf->dictpath == NULL ? "" : conf->dictpath, NULL);
  gconf_client_set_string(gconf_client, "/apps/gjiten/kanjidic/kanjidicfile", conf->kanjidic->path, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/kanjidic/unicode_radicals", conf->unicode_radicals, NULL);

	if (conf->kanjipad == NULL) conf->kanjipad = "";
  gconf_client_set_string(gconf_client, "/apps/gjiten/general/kanjipad", conf->kanjipad, NULL);

	//Deprecated dictionary file number, zero it out.
	//gconf_client_set_int(gconf_client, "/apps/gjiten/general/numofdics", conf->numofdics, NULL);
	gconf_client_set_int(gconf_client, "/apps/gjiten/general/numofdics", 0, NULL);

  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/bigwords", conf->bigwords, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/bigkanji", conf->bigkanji, NULL);
  gconf_client_set_string(gconf_client, "/apps/gjiten/general/largefont", conf->largefont == NULL ? "" : conf->largefont, NULL);
  gconf_client_set_string(gconf_client, "/apps/gjiten/general/normalfont", conf->normalfont == NULL ? "" : conf->normalfont, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/gdk_use_xft", conf->gdk_use_xft, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/force_ja_JP", conf->force_ja_JP, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/force_language_c", conf->force_language_c, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/envvar_override", conf->envvar_override, NULL);

  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/search_kata_on_hira", conf->search_kata_on_hira, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/search_hira_on_kata", conf->search_hira_on_kata, NULL);
  gconf_client_set_bool(gconf_client, "/apps/gjiten/general/deinflection_enabled", conf->verb_deinflection, NULL);

  //Save dicfiles [path and name seperated with linebreak]
	gconfList = gconf_value_new(GCONF_VALUE_LIST);
	diclist = conf->dicfile_list;
	while (diclist != NULL) {
		if (diclist->data == NULL) break;
		dicfile = diclist->data;
		tmpstrg = g_strdup_printf("%s\n%s", dicfile->path, dicfile->name);
		gconf_diclist = g_slist_append(gconf_diclist, tmpstrg);
		diclist = g_slist_next(diclist);
	}
	gconf_value_set_list_type(gconfList, GCONF_VALUE_STRING);
	gconf_client_set_list(gconf_client, GCONF_PATH_GENERAL"/dictionary_list", GCONF_VALUE_STRING, gconf_diclist, NULL);
}

void conf_save_history(GList *history, GjitenConfig *conf) {
  char historystr[40];
  int i;
  if (history != NULL) {
    for (i = 0; i <= 50; i++) {
      snprintf(historystr, 31, "/apps/gjiten/history/history%d", i);
      gconf_client_set_string(gconf_client, historystr, history->data, NULL);
      history = g_list_next(history);
      if (history == NULL) break;
    } 
  }
}

void conf_save_options(GjitenConfig *conf) {
    gconf_client_set_bool(gconf_client, "/apps/gjiten/general/autoadjust_enabled", conf->autoadjust_enabled, NULL);
    gconf_client_set_bool(gconf_client, "/apps/gjiten/general/searchlimit_enabled", conf->searchlimit_enabled, NULL);
    gconf_client_set_int(gconf_client, "/apps/gjiten/general/maxwordmatches", conf->maxwordmatches, NULL);
}

gboolean conf_init_handler() {
  GError *error;

  if (gconf_init(0, NULL, &error) == FALSE) {
    gjiten_abort_with_msg("GConf init failed: %s\n", error->message);
	}
  g_type_init();

  if (gconf_client == NULL) {
		gconf_client = gconf_client_get_default();
	}

  if (gconf_client == NULL) {
    gjiten_print_error(_("Could not get gconf_client.\n"));
    return FALSE;
  }
	return TRUE;

}

void conf_close_handler() {
	if (gconf_client != NULL) {
		GJITEN_DEBUG("calling g_object_unref(G_OBJECT(gconf_client)) [%d]\n", (int) gconf_client);
		g_object_unref(G_OBJECT(gconf_client));
		gconf_client = NULL;
	}
}
