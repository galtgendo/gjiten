/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* gjiten.h

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

#ifndef __GJITEN_H__
#define __GJITEN_H__

typedef struct _GjitenApp GjitenApp;

struct _GjitenApp {
	WordDic *worddic;
	KanjiDic *kanjidic;
	GjitenConfig *conf;
	GtkWidget *pref_dialog;
};


void gjiten_start_kanjipad();
void gjiten_create_about();
void gjiten_display_manual();
void gjiten_exit();
void gjiten_clear_entry_box(gpointer entrybox);

#endif
