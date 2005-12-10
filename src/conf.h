/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* conf.h
   
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

#ifndef __CONF_H__
#define __CONF_H__


#include <gconf/gconf-client.h>
#include <pango/pango-font.h>

#include "kanjidic.h"
#include "constants.h"
#include "dicfile.h"

typedef struct _GjitenConfig GjitenConfig;

struct _GjitenConfig {
  gchar *version;

	GjitenDicfile *kanjidic;
	GSList *dicfile_list;

  char *history[60];
  gboolean toolbar;
  gboolean menubar;
  gboolean kdiccfg[KCFGNUM];
  gboolean bigwords;
  gboolean bigkanji;
	gboolean override_env;
  gchar *kanjipad;

  gboolean startkanjidic;
  gchar *kanji_to_lookup;
  gchar *word_to_lookup;
  gboolean clip_kanji_lookup;
  gboolean clip_word_lookup;
  int maxwordmatches;
  gchar *largefont;
  gchar *normalfont;
  gboolean gdk_use_xft;
  gboolean force_ja_JP;
  gboolean force_language_c;
	gboolean envvar_override;
	gboolean search_kata_on_hira;
	gboolean search_hira_on_kata;
  gboolean verb_deinflection;

  gboolean searchlimit_enabled;
  gboolean autoadjust_enabled;

	gboolean unicode_radicals;

  GjitenDicfile *selected_dic;
  GjitenDicfile *mmaped_dicfile;
  PangoFontDescription *normalfont_desc;

	/* DEPRECATED */
  char *dictpath;
  int numofdics;

};


GjitenConfig *conf_load();
void conf_save(GjitenConfig *conf);
void conf_save_history(GList *history, GjitenConfig *conf);
void conf_save_options(GjitenConfig *conf);
gboolean conf_init_handler();
void conf_close_handler();

#endif
