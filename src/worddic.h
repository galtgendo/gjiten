/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* worddic.h

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
#ifndef __WORDDIC_H__
#define __WORDDIC_H__
typedef struct _WordDic WordDic;

struct _WordDic {
/* GTK variables */
  GtkWidget *hbox_options;
	GtkWidget *window;
	GtkWidget *combo_entry;
	GtkWidget *text_results_view;
	GtkTextBuffer *text_results_buffer;
	GtkTextBuffer *info_buffer;
	GtkWidget *menu_selectdic;
	GtkWidget *combo_entry_dictfile;
	GtkWidget *checkb_verb;
	GtkWidget *checkb_autoadjust;
	GtkWidget *checkb_searchlimit;
	GtkWidget *spinb_searchlimit;
	GtkWidget *radiob_jpexact;
	GtkWidget *radiob_startw;
	GtkWidget *radiob_endw;
	GtkWidget *radiob_any;
	GtkWidget *radiob_engexact;
	GtkWidget *radiob_words;
	GtkWidget *radiob_partial;
	GtkWidget *radiob_searchdic;
	GtkWidget *radiob_searchall;
	GtkWidget *button_back;
	GtkWidget *button_forward;
	GtkTextIter iter;
	GtkWidget *appbar_mainwin;
	GList *combo_entry_glist;
	GtkWidget *dicselection_menu;
	GtkTextTag *tag_large_font;
	GdkCursor *selection_cursor;
	GdkCursor *regular_cursor;
	gboolean is_cursor_regular;
};


/* verb deinflection */
struct vinfl_struct {
  gchar *conj;
  gchar *infl;
  gchar *type;
};



WordDic *worddic_create();
void worddic_paste(WordDic *worddic);
void on_text_entered();
void worddic_update_dic_menu();
void worddic_apply_fonts();

#endif
