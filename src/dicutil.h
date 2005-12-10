/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* dicutil.h

   GJITEN : A GTK+/GNOME BASED JAPANESE DICTIONARY
  
   Copyright (C) 1999 - 2005 Botond Botyanszki <boti@rocketmail.com>

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
#ifndef __DICUTIL_H__
#define __DICUTIL_H__

gchar *hira2kata(gchar *hirastr);
gchar *kata2hira(gchar *hirastr);
gchar *full2half(gchar *instr);
gboolean isKanaChar(gunichar c);
gboolean isKatakanaChar(gunichar c);
gboolean isHiraganaChar(gunichar c);
gboolean isKanjiChar(gunichar c);
gboolean isJPChar(gunichar c);
gboolean isOtherChar(gunichar c);
gint search4string(gint type, GjitenDicfile *dicfile, gchar *srchstrg,
									 guint32 *res_index, gint *hit_pos, gint *res_len, gchar *res_str);

gchar *get_eof_line(gchar *ptr, gchar *end_ptr);
int get_word(char *dest, char *src, int size, int pos);
int strg_end_compare(gchar *strg1, gchar *strg2);
int get_jp_match_type(gchar *line, gchar *srchstrg, int offset);
gboolean is_kanji_only(gchar *line);
void dicutil_unload_dic();
gboolean isHiraganaString(gchar *strg);
gboolean isKatakanaString(gchar *strg);

#endif
