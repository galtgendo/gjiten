/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* kanjidic.c

   GJITEN : A GTK+/GNOME BASED JAPANESE DICTIONARY
  
   Copyright (C) 1999-2005 Botond Botyanszki <boti@rocketmail.com>
   xjdic code Copyright (C) 1998 Jim Breen 

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "constants.h"
#include "kanjidic.h"
#include "conf.h"
#include "dicfile.h"
#include "dicutil.h"
#include "worddic.h"
#include "gjiten.h"
#include "pref.h"
#include "gjiten.h"
#include "error.h"
#include "radical-convtable.h"

/*====== Prototypes========================================================*/
void get_rad_of_kanji(gunichar kanji);

static void kanjidic_close();
static void on_kanji_search();

/* VARIABLES ************************/
gchar *kdic_line = NULL;  /*size = KCFGNUM * KBUFSIZE */
gchar kanjiselected[2];
gchar *radkfile = NULL;
guint32 radkfile_size;
int radkfile_fd = -1;
extern guint32 srchpos;

GList *klinklist = NULL, *tmpklinklist = NULL;

KanjiDic *kanjiDic;
extern gchar *strginfo[];
extern GjitenApp *gjitenApp;

static const GtkActionEntry entries[] = {
	{"FileMenu", NULL, N_("File")},
	{"EditMenu", NULL, N_("Edit")},
	{"ToolsMenu", NULL, N_("Tools")},
	{"HelpMenu", NULL, N_("Help")},
	{"Quit", GTK_STOCK_QUIT, NULL, NULL, N_("Close Gjiten"), G_CALLBACK(kanjidic_close)},
	{"Worddic", "my-gjiten-icon", N_("Worddic"), NULL, N_("Launch WordDic"), G_CALLBACK(worddic_create)},
	{"Kanjipad", "my-kanjipad-icon", N_("Kanjipad"), NULL, N_("Launch KanjiPad"), G_CALLBACK(gjiten_start_kanjipad)},
	{"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK(create_dialog_preferences)},
	{"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(gjiten_create_about)},
	{"Help", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK(gjiten_display_manual)},
	{"Find", GTK_STOCK_FIND, NULL, NULL, N_("Search entered Kanji"), G_CALLBACK(on_kanji_search)},
};

static const char *ui_description =
"<ui>"
"<toolbar name='ToolBar' >"
"<toolitem action='Quit' />"
"<toolitem action='Worddic' />"
"<toolitem action='Kanjipad' />"
"<toolitem action='Find' />"
"</toolbar>"
"<menubar name='MenuBar'>"
"<menu action='FileMenu' >"
"<menuitem action='Quit' />"
"</menu>"
"<menu action='EditMenu' >"
"<menuitem action='Preferences' />"
"</menu>"
"<menu action='ToolsMenu' >"
"<menuitem action='Worddic' />"
"<menuitem action='Kanjipad' />"
"</menu>"
"<menu action='HelpMenu' >"
"<menuitem action='Help' />"
"<menuitem action='About' />"
"</menu>"
"</menubar>"
"</ui>";


/* ************************************************************ */
void do_kdicline(gchar *kstr) {
  char tmpstr[KBUFSIZE];
  int i, pos;

  if (kdic_line == NULL) kdic_line = (gchar *)g_malloc(KCFGNUM * KBUFSIZE);
  if (kdic_line == NULL) gjiten_abort_with_msg("Couldn't allocate memory\n");

  for (i = 0; i < KCFGNUM * KBUFSIZE; i++) { //clear it
    kdic_line[i] = 0;
  }
  
  g_unichar_to_utf8(g_utf8_get_char(kstr), &kdic_line[KANJI * KBUFSIZE] ); //KANJI
  get_rad_of_kanji(g_utf8_get_char(kdic_line + KANJI * KBUFSIZE)); //RADICAL

  get_word(kdic_line + JIS * KBUFSIZE, kstr, KBUFSIZE, 3);
  pos = 7;

  while (pos != 0) {
    pos = get_word(tmpstr, kstr, sizeof(tmpstr), pos);

    if ((tmpstr[0] >> 7)) {       // jap char   //FIXME
     	if (strlen(kdic_line + READING * KBUFSIZE) != 0) {
				strncat(kdic_line + READING * KBUFSIZE, ", ", KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
			}
			strncat(kdic_line + READING * KBUFSIZE, tmpstr, KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
    }
    else switch (tmpstr[0]) {
				case '-' : 
					if (strlen(kdic_line + READING * KBUFSIZE) != 0) {
						strncat(kdic_line + READING * KBUFSIZE, ", ", KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
					}
					strncat(kdic_line + READING * KBUFSIZE, tmpstr, KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
					break;

        case 'T':
					if (tmpstr[1] == '1') {
						if (strlen(kdic_line + READING * KBUFSIZE) != 0) {
							strncat(kdic_line + READING * KBUFSIZE, ", ", KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
							strncat(kdic_line + READING * KBUFSIZE, _("Name readings:"), KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
						}
						else {
							strncat(kdic_line + READING * KBUFSIZE, _("Name readings:"), KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
						}
						pos = get_word(tmpstr, kstr, sizeof(tmpstr), pos);
						strncat(kdic_line + READING * KBUFSIZE, tmpstr, KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
						break;
					}
					if (tmpstr[1] == '2') {
						if (strlen(kdic_line + READING * KBUFSIZE) != 0) {
							strncat(kdic_line + READING * KBUFSIZE, ", Radical Name: ", KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
						}
						else {
							strncat(kdic_line + READING * KBUFSIZE, _("Radical name:"), KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
						}
						pos = get_word(tmpstr, kstr, sizeof(tmpstr), pos);
						strncat(kdic_line + READING * KBUFSIZE, tmpstr, KBUFSIZE - strlen(kdic_line + READING * KBUFSIZE) - 1);
						break;
					}

				case '{': // english meaning
					if (strlen(kdic_line + ENGLISH * KBUFSIZE) != 0 ) {
						strncat(kdic_line + ENGLISH * KBUFSIZE, " ", KBUFSIZE - strlen(kdic_line + ENGLISH * KBUFSIZE) - 1);
					}
					strncat(kdic_line + ENGLISH * KBUFSIZE, tmpstr + 1, KBUFSIZE - strlen(kdic_line + ENGLISH * KBUFSIZE) - 1);
					strncat(kdic_line + ENGLISH * KBUFSIZE, ";", KBUFSIZE - strlen(kdic_line + ENGLISH * KBUFSIZE) - 1); // put endmark: ;
					break;
				
				case 'B':
					strncpy(kdic_line + BUSHU * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
				
				case 'C':
					strncpy(kdic_line + CLASSIC * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
			 
				case 'F':
					strncpy(kdic_line + FREQ * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
				
				case 'G':
					strncpy(kdic_line + JOUYOU * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
				
				case 'H':
					strncpy(kdic_line + HINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
				
				case 'N':
					strncpy(kdic_line + NINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
				
				case 'V':
					strncpy(kdic_line + VINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
				
				case 'D':
					strncpy(kdic_line + DEROO * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
					
				case 'P':
					strncpy(kdic_line + SKIP * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
					
				case 'S':
					if (strlen(kdic_line + STROKES * KBUFSIZE) == 0) {
						strncpy(kdic_line + STROKES * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					}
					else { 
						strncat(kdic_line + STROKES * KBUFSIZE, _(", Common miscount: "), KBUFSIZE - strlen(kdic_line + STROKES * KBUFSIZE) - 1);
						strncat(kdic_line + STROKES * KBUFSIZE, tmpstr + 1, KBUFSIZE - strlen(kdic_line + STROKES * KBUFSIZE) - 1);
					}
					break;
				
				case 'U':
					strncpy(kdic_line + UNI * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
					
				case 'I':
					if (tmpstr[1] == 'N') {
						strncpy(kdic_line + ININDEX * KBUFSIZE, tmpstr + 2, KBUFSIZE);
					}
					else
					{
						strncpy(kdic_line + IINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					}
					break;
      
				case 'Q':
					strncpy(kdic_line + FOURC * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
    
				case 'M':
					if (tmpstr[1] == 'N') strncpy(kdic_line + MNINDEX * KBUFSIZE, tmpstr + 2, KBUFSIZE);
					else if (tmpstr[1] == 'P') strncpy(kdic_line + MPINDEX * KBUFSIZE, tmpstr + 2, KBUFSIZE);
					break;
      
				case 'E':
					strncpy(kdic_line + EINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
    
				case 'K':
					strncpy(kdic_line + KINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
					
				case 'L':
					strncpy(kdic_line + LINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
    
				case 'O':
					strncpy(kdic_line + OINDEX * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
					
				case 'W':
					strncpy(kdic_line + KOREAN * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
					
				case 'Y':
					strncpy(kdic_line + PINYIN * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
    
				case 'X':
					strncpy(kdic_line + CREF * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
    
				case 'Z': 
					strncpy(kdic_line + MISSC * KBUFSIZE, tmpstr + 1, KBUFSIZE);
					break;
		}
	}
}


void print_kanjinfo(gunichar kanji) {
  gint i;
  gint srch_resp, roff, rlen;
  gchar repstr[1024];
  guint32 respos;
  gchar kanjistr[6];

  for (i = 0; i < 6; i++) kanjistr[i] = 0;
  g_unichar_to_utf8(kanji, kanjistr);

  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), "", 0);
  gtk_text_buffer_get_start_iter(kanjiDic->text_kanjinfo_buffer, &kanjiDic->kinfo_iter);

  srchpos = 0;
  srch_resp = search4string(SRCH_START, gjitenApp->conf->kanjidic, kanjistr, &respos, &roff, &rlen, repstr);
  do_kdicline(repstr);

  for (i = 0; i < KCFGNUM; i++)
    if (gjitenApp->conf->kdiccfg[i] == TRUE) {
			gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), 
																							 &kanjiDic->kinfo_iter, _(strginfo[i]), -1, "blue_foreground", NULL);
	    gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, 
																							 ": ", -1, "blue_foreground", NULL);
	    if (i == KANJI) {
		    if (gjitenApp->conf->bigkanji == FALSE) {
			    gtk_text_buffer_insert(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, kdic_line + i * KBUFSIZE, -1); 
		    }
		    else {
			    gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), 
																									 &kanjiDic->kinfo_iter, kdic_line + i * KBUFSIZE, -1, "largefont", NULL);
		    }
	    }
	    else {
		    gtk_text_buffer_insert(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, kdic_line + i * KBUFSIZE, -1);
	    }
	    gtk_text_buffer_insert(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, "\n", -1);
    }
}
  

/* compares two lists and combines the matching kanji into one */
void klists_merge(void) {
  GList *ptr1, *ptr2, *nextptr;
  int found;

  ptr1 = klinklist;
  while (ptr1 != NULL) {
    nextptr = g_list_next(ptr1);
    found = FALSE;
    ptr2 = tmpklinklist;
    while (ptr2 != NULL) {
      if ((gunichar) ptr1->data == (gunichar) ptr2->data) {
				found = TRUE;
				break;
      }
      ptr2 = g_list_next(ptr2);
    }
    if (found == FALSE) {
			klinklist = g_list_remove(klinklist, ptr1->data);
		}
    ptr1 = nextptr;
  }
  g_list_free(tmpklinklist);
	tmpklinklist = NULL;
}

void findk_by_key(gchar *srchkey, GList **list)  {
  gint srch_resp, roff, rlen;
  gchar repstr[1024];
  guint32 respos, oldrespos; 
  gint loopnum = 0;

  srchpos = 0;
  srch_resp = search4string(SRCH_START, gjitenApp->conf->kanjidic, srchkey, &respos, &roff, &rlen, repstr);
  //printf("F: Returning:srch_resp:%d\n respos:%ld\n roff:%d rlen:%d\n repstr:%s\n", srch_resp,respos,roff,rlen,repstr); 
  if (srch_resp != SRCH_OK) return; // (FALSE);
  oldrespos = srchpos = respos;
  *list = g_list_prepend(*list, (gpointer) g_utf8_get_char(repstr));
  
  while (roff != 0) {
    oldrespos = respos;
    srchpos++;
    loopnum++;
		srch_resp = search4string(SRCH_CONT, gjitenApp->conf->kanjidic, srchkey, &respos, &roff, &rlen, repstr);
    //printf("srch_resp:%d\n respos:%ld\n roff:%d rlen:%d\n repstr:%s\n",srch_resp,respos,roff,rlen,repstr);
    if (srch_resp != SRCH_OK) break;
    if (oldrespos == respos) continue;
    *list = g_list_prepend(*list, (gpointer) g_utf8_get_char(repstr));
  }
}

void findk_by_stroke(int stroke, int plusmin, GList **list) {
  static char srchkey[10];
  int i, lowerlim, upperlim;
 
  upperlim = stroke + plusmin;
  if (upperlim > 30) upperlim = 30;
  lowerlim = stroke - plusmin;
  if (lowerlim < 1) lowerlim = 1;
 
  for (i = lowerlim; i <= upperlim ; i++) {
    snprintf(srchkey, 10, " S%d ", i);
    findk_by_key(srchkey, list);
  }
}

void findk_by_radical(gchar *radstrg) { 
  gint i, radnum;
	RadInfo *rad_info;
	GList *kanji_info_list;
	gchar *tmprad;
  gchar *radstr_ptr;

  if (g_utf8_strlen(radstrg, -1) == 0) return;

	radstr_ptr = radstrg;
	rad_info = g_hash_table_lookup(kanjiDic->rad_info_hash, (gpointer) g_utf8_get_char(radstr_ptr));
  if (rad_info == NULL) {
    gjiten_print_error(_("Invalid radical!\n"));
    return;
  }

  for (kanji_info_list = rad_info->kanji_info_list;
			 kanji_info_list != NULL;
			 kanji_info_list = g_list_next(kanji_info_list)) {
    klinklist = g_list_prepend(klinklist, (gpointer) ((KanjiInfo *) kanji_info_list->data)->kanji);
  }

  // if we have more than 1 radical 
  radnum = g_utf8_strlen(radstrg, -1); 
  if (radnum > 1) {
    for (i = 1; i <= radnum; i++) {
      rad_info = g_hash_table_lookup(kanjiDic->rad_info_hash, (gpointer) g_utf8_get_char(radstr_ptr));
      if (rad_info == NULL) {
				tmprad = g_strndup(radstr_ptr, sizeof(gunichar));
				gjiten_print_error(_("I don't seem to recognize this radical: '%s' !!!\n"), tmprad);
				g_free(tmprad);
				return;
      }
			for (kanji_info_list = rad_info->kanji_info_list;
					 kanji_info_list != NULL;
					 kanji_info_list = g_list_next(kanji_info_list)) {
				tmpklinklist = g_list_prepend(tmpklinklist, (gpointer) ((KanjiInfo *) kanji_info_list->data)->kanji);
      }
      klists_merge();
      radstr_ptr = g_utf8_next_char(radstr_ptr);
    }
  }
}

void set_radical_button_sensitive(gpointer radical, RadInfo *rad_info, gpointer user_data) {
	GtkWidget *rad_button = g_hash_table_lookup(kanjiDic->rad_button_hash, radical);
	if (GTK_IS_WIDGET(rad_button) && (gtk_widget_get_sensitive(rad_button) != TRUE)) {
		gtk_widget_set_sensitive(rad_button, TRUE);
	}
}

void set_radical_button_unsensitive(gunichar radical, GtkWidget *rad_button, gboolean sensitive) {
	if (GTK_IS_WIDGET(rad_button) && (gtk_widget_get_sensitive(rad_button) != sensitive)) {
		gtk_widget_set_sensitive(rad_button, sensitive);
	}
}


static void on_kanji_search() {
  static gchar *kentry, *radentry;
  int found;
  int i;
  int stroke, plus_min;
  GList *node_ptr;
  gchar kappbarmsg[100];
  int push;
  GtkWidget *kanji_result_button;
  gchar kanji_result_str[6];
  gchar kanji_result_labelstr[100];
  GtkWidget *kanji_result_label;
	GtkTextChildAnchor *kanji_results_anchor;
	gint result_num = 0;
	GList *rad_info_list = NULL;
	GList *kanji_list = NULL;
	GList *kanji_list_ptr = NULL;
	GHashTable *rad_info_hash = NULL;
	KanjiInfo *kanji_info;

  gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
  gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), _("Searching..."));
  kappbarmsg[0] = 0;

  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(kanjiDic->kanji_results_buffer), "", 0);
  gtk_text_buffer_get_start_iter(kanjiDic->kanji_results_buffer, &kanjiDic->kanji_results_iter);

  push = TRUE;
  if (kentry != NULL) { //Check if we need to save the key entry in the history
    if (strcmp(kentry, gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_key))))) == 0) {
      push = FALSE;
      g_free(kentry);
    }
	}
  kentry = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_key)))));
  if (kentry != NULL) {
    if ((strlen(kentry) > 0) && (push == TRUE) ) {
      kanjiDic->combo_entry_key_glist = g_list_prepend(kanjiDic->combo_entry_key_glist, kentry);
    }
	}
  push = TRUE;
  if (radentry != NULL) { //Check if we need to save the radical entry in the history
    if (strcmp(radentry, gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_radical))))) == 0) {
			push = FALSE;
			g_free(radentry);
		}
  }

  radentry = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_radical)))));
  if (radentry) {
    if ((strlen(radentry) > 0) && push) {
      kanjiDic->combo_entry_radical_glist = g_list_prepend(kanjiDic->combo_entry_radical_glist, radentry);
    }
  }

  stroke = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(kanjiDic->spinb_strokenum));
  plus_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(kanjiDic->spinb_plusmin));

  if (klinklist != NULL) g_list_free(klinklist);
  tmpklinklist = NULL;
  klinklist = NULL;
  found = TRUE;

	if (GTK_IS_WIDGET(kanjiDic->window_radicals)) {
		g_hash_table_foreach(kanjiDic->rad_button_hash, (GHFunc) set_radical_button_unsensitive, (gpointer) TRUE);
	}

  //FIND BY RADICAL
  if ((gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical))) && (g_utf8_strlen(radentry, -1) > 0)) {
    findk_by_radical(radentry); 
    if (klinklist == NULL) {
      gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
      gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), _("No such kanji with this radical combination."));
      return;
    }
  }

  //FIND BY STROKE
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke))) {
    if ((stroke < 1) || (stroke > 30)) {
	gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
	gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), _("Invalid stroke count :-P "));
	return;
    }
    if (klinklist == NULL) {
      findk_by_stroke(stroke, plus_min, &klinklist);  // this should! give results
      if (klinklist == NULL ) {
				gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
				gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), _("Stroke search didn't find any match :-O "));
				return;
      }
    }
    else {
      findk_by_stroke(stroke, plus_min, &tmpklinklist);
      klists_merge(); 
      if (klinklist == NULL) {  
				found = FALSE; 
				gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
				gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), _("No such kanji with this stroke/radical combination."));
				return;
      }
    }
  }

  //FIND BY KEY
  if ((found) && (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_ksearch))) && (strlen(kentry) >= 1)) {
    if (klinklist == NULL) findk_by_key(kentry, &klinklist);
    else {
      findk_by_key(kentry, &tmpklinklist);
      klists_merge();
    }
    if (klinklist == NULL) {    
      gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
      gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), _("No Matches found!"));
      return;
    }
  }
  
	result_num = g_list_length(klinklist);
  snprintf(kappbarmsg, 100, _("Kanji found: %d"), result_num);
  gtk_statusbar_pop(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"));
  gtk_statusbar_push(GTK_STATUSBAR(kanjiDic->appbar_kanji),gtk_statusbar_get_context_id(GTK_STATUSBAR(kanjiDic->appbar_kanji),"Search"), kappbarmsg);

  if (result_num == 1) print_kanjinfo((gunichar) klinklist->data);
  

  // PRINT OUT KANJI FOUND
  node_ptr = klinklist;
  i = 0;
  while (node_ptr != NULL) { 
		kanji_list = g_list_prepend(kanji_list, node_ptr->data);
		memset(kanji_result_str, 0, sizeof(kanji_result_str));
    g_unichar_to_utf8((gunichar) node_ptr->data, kanji_result_str);
    //printf("%s\n", kanji_result_str);
    g_snprintf(kanji_result_labelstr, 100, "<span size=\"xx-large\">%s</span>", kanji_result_str); 
    kanji_results_anchor = gtk_text_buffer_create_child_anchor(kanjiDic->kanji_results_buffer, &kanjiDic->kanji_results_iter);
    
    if (gjitenApp->conf->bigkanji == TRUE) {
      kanji_result_label = gtk_label_new(NULL);
      gtk_label_set_markup(GTK_LABEL(kanji_result_label), kanji_result_labelstr);
      gtk_widget_show(kanji_result_label);
      kanji_result_button = gtk_button_new();
      gtk_container_add(GTK_CONTAINER(kanji_result_button), kanji_result_label);
    }
    else {
     kanji_result_label = gtk_label_new(kanji_result_str);
     gtk_widget_show(kanji_result_label);
     kanji_result_button = gtk_button_new();
     gtk_container_add(GTK_CONTAINER(kanji_result_button), kanji_result_label);
    }
    if ((gjitenApp->conf->normalfont != NULL) && gjitenApp->conf->normalfont_desc != NULL) {
      gtk_widget_modify_font(kanji_result_label, gjitenApp->conf->normalfont_desc);
    }
    gtk_widget_show(kanji_result_button);
    g_signal_connect_swapped(G_OBJECT(kanji_result_button), "clicked", G_CALLBACK(kanji_selected), 
														 node_ptr->data);
    gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(kanjiDic->kanji_results_view), kanji_result_button, kanji_results_anchor);
    node_ptr = g_list_next(node_ptr);
  }

	// find all different radicals in all the kanji found
	if (GTK_IS_WIDGET(kanjiDic->window_radicals)) {
		if (kanji_list != NULL) {
			rad_info_hash = g_hash_table_new(NULL, NULL);
			for (kanji_list_ptr = kanji_list;
					 kanji_list_ptr != NULL;
					 kanji_list_ptr = g_list_next(kanji_list_ptr)) {
				kanji_info = g_hash_table_lookup(kanjiDic->kanji_info_hash, kanji_list_ptr->data);
				for (rad_info_list = kanji_info->rad_info_list;
						 rad_info_list != NULL;
						 rad_info_list = g_list_next(rad_info_list)) {
					g_hash_table_insert(rad_info_hash, (gpointer) ((RadInfo *) rad_info_list->data)->radical, rad_info_list->data);
				}
			}
			g_hash_table_foreach(kanjiDic->rad_button_hash, (GHFunc) set_radical_button_unsensitive, (gpointer) FALSE);
			
			g_hash_table_foreach(rad_info_hash, (GHFunc) set_radical_button_sensitive, NULL);
			g_hash_table_destroy(rad_info_hash);
			g_list_free(kanji_list);
		}
		else { // none found, so set everything sensitive
			g_hash_table_foreach(kanjiDic->rad_button_hash, (GHFunc) set_radical_button_unsensitive, (gpointer) TRUE);
		}
	}
}


int radical_selected(gunichar radical) {
  int i, j;
  gchar radical_selected[6];
  gchar tmpchar[6];
  gchar *radline_ptr, *newradline;
  int removed;
  int radline_length = 0;

  memset(radical_selected, 0, sizeof(radical_selected)); 
  g_unichar_to_utf8(radical, radical_selected);

  radline_ptr = (gchar*) gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_radical))));
  newradline = g_strndup(radline_ptr, strlen(radline_ptr) + 6); //Enough space for one more character
  radline_length = g_utf8_strlen(newradline, -1);

  for (i = 0; i < (int) (strlen(newradline) + 6); i++) newradline[i] = 0; //clear newradline

  removed = FALSE;
  for (i = 0; i < radline_length; i++) {  //Check if we already have the radical in the line
    if (g_utf8_get_char(radline_ptr) != radical) {
      for (j = 0; j < 6; j++) tmpchar[j] = 0; 
      g_unichar_to_utf8(g_utf8_get_char(radline_ptr), tmpchar);
      strncat(newradline, tmpchar, 5);
    }
    else removed = TRUE;  //if it's there then remove it
    radline_ptr = g_utf8_next_char(radline_ptr);
  }
  
	if (removed == FALSE) strncat(newradline, radical_selected, 5); //Add the radical to the line
  gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_radical))), newradline);

  g_free(newradline);

  on_kanji_search();
  
  return 0;
}

void history_add(gunichar unicharkanji) {
  GtkWidget *history_kanji_button;
  GtkWidget *history_kanji_label;
  gchar history_kanji_labelstr[100];
  gchar kanji[6];
  gunichar *unichar_list_elem;
  int i;

  if (kanjiDic->kanji_history_list != NULL) {
    if ((*((gunichar *)(kanjiDic->kanji_history_list->data))) == unicharkanji) return;
  }
  unichar_list_elem = (gunichar *) malloc(sizeof(gunichar));
  *unichar_list_elem = unicharkanji;

  for (i = 0; i < 6; i++) kanji[i] = 0;
  g_unichar_to_utf8(unicharkanji, kanji);
 
  kanjiDic->kanji_history_list = g_slist_prepend(kanjiDic->kanji_history_list, unichar_list_elem);

  if (gjitenApp->conf->bigkanji == TRUE) {
    g_snprintf(history_kanji_labelstr, 100, "<span size=\"large\">%s</span>", kanji); 
    history_kanji_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(history_kanji_label), history_kanji_labelstr);
    gtk_widget_show(history_kanji_label);
    history_kanji_button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(history_kanji_button), history_kanji_label);
  }
  else {
      history_kanji_label = gtk_label_new(kanji);
      gtk_widget_show(history_kanji_label);
      history_kanji_button = gtk_button_new();
      gtk_container_add(GTK_CONTAINER(history_kanji_button), history_kanji_label);
  }
  if ((gjitenApp->conf->normalfont != NULL) && gjitenApp->conf->normalfont_desc != NULL) {
    gtk_widget_modify_font(history_kanji_label, gjitenApp->conf->normalfont_desc);
  }

  g_signal_connect_swapped(G_OBJECT(history_kanji_button), "clicked", G_CALLBACK(kanji_selected), 
			   (gpointer) (*unichar_list_elem));
  gtk_box_pack_start(GTK_BOX(kanjiDic->vbox_history), history_kanji_button, FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(kanjiDic->vbox_history), history_kanji_button, 0);
  gtk_widget_show(history_kanji_button);

	if (gtk_widget_get_visible(kanjiDic->scrolledwin_history) != TRUE)
		gtk_widget_show(kanjiDic->scrolledwin_history);
}


void kanji_selected(gunichar kanji) {
  print_kanjinfo(kanji);
  history_add(kanji);
  GJITEN_DEBUG("KANJI_SELECTED\n");
}

static void radical_window_close() {
	if (GTK_IS_WIDGET(kanjiDic->window_radicals) == TRUE) {
		gtk_widget_destroy(kanjiDic->window_radicals);
		kanjiDic->window_radicals = NULL;
	}
}


//get all radicals that the kanji has
void get_rad_of_kanji(gunichar kanji) {
  gchar *kdicline_ptr;
	KanjiInfo *kanji_info;
	GList *rad_info_list;

  kdicline_ptr = kdic_line + RADICAL * KBUFSIZE;

	kanji_info = g_hash_table_lookup(kanjiDic->kanji_info_hash, (gconstpointer) kanji);
	if (kanji_info != NULL) {
		for (rad_info_list = kanji_info->rad_info_list;
				 rad_info_list != NULL;
				 rad_info_list = g_list_next(rad_info_list)) {
			memset(kdicline_ptr, 0, 6);
			if (kdicline_ptr >= kdic_line + RADICAL * KBUFSIZE + KBUFSIZE - 7) return;
			g_unichar_to_utf8(((RadInfo *) rad_info_list->data)->radical, kdicline_ptr);
			kdicline_ptr = g_utf8_next_char(kdicline_ptr);
			g_unichar_to_utf8(' ', kdicline_ptr);
			kdicline_ptr = g_utf8_next_char(kdicline_ptr);
		}
	}
}

static gunichar jis_radical_to_unicode(gchar *radical) {
	gint i;

	gchar jisutf8[6]; 

	if (gjitenApp->conf->unicode_radicals == TRUE) {
		g_unichar_to_utf8(g_utf8_get_char(radical), jisutf8);

		for (i = 0; i < sizeof(radicaltable) / sizeof(radpair); i++) {
			if (strcmp(radicaltable[i].jis, jisutf8) == 0) {
				return g_utf8_get_char(radicaltable[i].uni);
			}
		}
	}

	return g_utf8_get_char(radical);
}

// Load the radical data from the file
void load_radkfile() {
  int error = FALSE;
  struct stat radk_stat;
  gint rad_cnt = 0;
  gchar *radkfile_name = RADKFILE_NAME;
  gchar *radkfile_ptr;
  gchar *radkfile_end;
  RadInfo *rad_info = NULL;
	KanjiInfo *kanji_info;
	gunichar kanji;

  if (radkfile != NULL) {
    //printf("radkfile already initialized.\n");
    return;
  }
  if (stat(radkfile_name, &radk_stat) != 0) {
    g_error("**ERROR** radkfile: stat() \n");
    error = TRUE;
  }
  radkfile_size = radk_stat.st_size;
  radkfile_fd = open(radkfile_name, O_RDONLY);
  if (radkfile_fd == -1) {
    g_error("**ERROR** radkfile: open()\n");
    error = TRUE;
  }
  radkfile = (gchar *) mmap(NULL, radkfile_size, PROT_READ, MAP_SHARED, radkfile_fd, 0);
  if (radkfile == NULL) gjiten_abort_with_msg("mmap() failed for radkfile\n");
	if ( radkfile_fd != -1 ) {
		close(radkfile_fd);
	}
	radkfile_fd = -1;

  if (error == TRUE) {
		gjiten_print_error(_("Error opening %s.\n "\
												 "Check your preferences or read the documentation!"),
											 radkfile_name);
		return;
  }

  radkfile_end = radkfile + strlen(radkfile); //FIXME: lseek
  radkfile_ptr = radkfile;
  
	if (kanjiDic->kanji_info_hash == NULL) {
		kanjiDic->kanji_info_hash = g_hash_table_new(NULL, NULL);
	}
	if (kanjiDic->rad_info_hash == NULL) {
		kanjiDic->rad_info_hash = g_hash_table_new(NULL, NULL);
	}

  while((radkfile_ptr < radkfile_end) && (radkfile_ptr != NULL)) {
    if (*radkfile_ptr == '#') {  //find $ as first char on the line
      radkfile_ptr = get_eof_line(radkfile_ptr, radkfile_end); //Goto next line
      continue;
    }
    if (*radkfile_ptr == '$') {  //Radical info line
      rad_cnt++;          //Increase number of radicals found
      radkfile_ptr = g_utf8_next_char(radkfile_ptr);
      while (g_unichar_iswide(g_utf8_get_char(radkfile_ptr)) == FALSE) { //Find radical
				radkfile_ptr = g_utf8_next_char(radkfile_ptr);
			}

			rad_info = g_new0(RadInfo, 1);
			rad_info->radical = jis_radical_to_unicode(radkfile_ptr); //store radical

      while (g_ascii_isdigit(*radkfile_ptr) == FALSE) { //Find stroke number
				radkfile_ptr = g_utf8_next_char(radkfile_ptr);
			}

      rad_info->strokes = atoi(radkfile_ptr);  //Store the stroke number
			kanjiDic->rad_info_list = g_list_append(kanjiDic->rad_info_list, rad_info);
			g_hash_table_insert(kanjiDic->rad_info_hash, (gpointer) rad_info->radical, rad_info);
			radkfile_ptr = get_eof_line(radkfile_ptr, radkfile_end); //Goto next line
    }
    else {   //Kanji
      while ((*radkfile_ptr != '$') && (radkfile_ptr < radkfile_end)) {
				if (*radkfile_ptr == '\n') {
					radkfile_ptr++;
					continue;
				}
				kanji = g_utf8_get_char(radkfile_ptr);
				kanji_info = g_hash_table_lookup(kanjiDic->kanji_info_hash, (gconstpointer) kanji);
				if (kanji_info == NULL) {
					kanji_info = g_new0(KanjiInfo, 1);
					kanji_info->kanji = kanji;
					g_hash_table_insert(kanjiDic->kanji_info_hash, (gpointer) kanji, (gpointer) kanji_info);
				}
				kanji_info->rad_info_list = g_list_prepend(kanji_info->rad_info_list, rad_info);
				rad_info->kanji_info_list = g_list_prepend(rad_info->kanji_info_list, kanji_info);
				radkfile_ptr = g_utf8_next_char(radkfile_ptr);
      }
    }
  }
}

static GtkWidget *create_window_radicals () {
  int i = 0, j = 0;
  int curr_strokecount = 0;
  GtkWidget *radtable; 
  GtkWidget *tmpwidget = NULL;
  GtkWidget *radical_label;
  gchar *strokenum_label;
  gchar radical[6];
	RadInfo *rad_info = NULL;
	GList *rad_info_list;

	load_radkfile(); 

  if (kanjiDic->window_radicals != NULL) {
    gtk_widget_hide(kanjiDic->window_radicals);
    gtk_widget_show(kanjiDic->window_radicals);
    return kanjiDic->window_radicals;
  }
	if (kanjiDic->rad_button_hash != NULL)	{
		g_hash_table_destroy(kanjiDic->rad_button_hash);
		kanjiDic->rad_button_hash = NULL;
	}
	
	kanjiDic->rad_button_hash = g_hash_table_new_full(NULL, NULL, NULL, NULL);
	
  kanjiDic->window_radicals = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(kanjiDic->window_radicals), _("Radicals"));
  g_signal_connect(G_OBJECT(kanjiDic->window_radicals), "destroy", G_CALLBACK(radical_window_close), NULL);

  radtable = gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(radtable), TRUE);
  gtk_grid_set_column_homogeneous(GTK_GRID(radtable), TRUE);
  gtk_container_add(GTK_CONTAINER(kanjiDic->window_radicals), radtable);
  gtk_widget_show(radtable);

	for (rad_info_list = kanjiDic->rad_info_list; rad_info_list != NULL; rad_info_list = g_list_next(rad_info_list)) {
    if (i == RADLISTLEN) {
      i = 0;
      j++;
    }
		rad_info = (RadInfo *) rad_info_list->data;
    if (curr_strokecount != rad_info->strokes) {
			if (i == RADLISTLEN - 1) {
				i = 0;
				j++;
			}
      curr_strokecount = rad_info->strokes;
      strokenum_label = g_strdup_printf("<b>%d</b>", curr_strokecount); //Make a label with the strokenumber
      tmpwidget = gtk_label_new(""); //radical stroke number label
      gtk_widget_set_hexpand(tmpwidget, TRUE);
      gtk_label_set_markup(GTK_LABEL(tmpwidget), strokenum_label);
			g_free(strokenum_label);

      gtk_grid_attach(GTK_GRID(radtable), tmpwidget, i, j, 1, 1);
      gtk_widget_show(tmpwidget);
      i++;
    }
    memset(radical, 0, sizeof(radical));
    g_unichar_to_utf8(rad_info->radical, radical);
    radical_label = gtk_label_new(radical);
    gtk_widget_show(radical_label);
    tmpwidget = gtk_button_new();
    gtk_widget_set_hexpand(tmpwidget, TRUE);
    gtk_widget_set_vexpand(tmpwidget, TRUE);
    gtk_container_add(GTK_CONTAINER(tmpwidget), radical_label);
    if ((gjitenApp->conf->normalfont != NULL) && gjitenApp->conf->normalfont_desc != NULL) {
      gtk_widget_modify_font(radical_label, gjitenApp->conf->normalfont_desc);
    }
    g_signal_connect_swapped(G_OBJECT(tmpwidget), "clicked", G_CALLBACK(radical_selected),
														 (gpointer)(rad_info->radical));
    
    gtk_grid_attach(GTK_GRID(radtable), tmpwidget , i, j, 1, 1);
    //      gtk_widget_set_usize(tmpwidget,20,20);
    gtk_widget_show(tmpwidget);
		g_hash_table_insert(kanjiDic->rad_button_hash, (gpointer) rad_info->radical, tmpwidget);
    i++;
  }  
  gtk_widget_show(kanjiDic->window_radicals);
  return kanjiDic->window_radicals;
}


static void kanjidic_close() {

	if (kanjiDic != NULL) {
		KanjiDic *tmp;
		radical_window_close();
		if (radkfile != NULL) {
			munmap(radkfile, radkfile_size);
			radkfile = NULL;
			if ( radkfile_fd != -1 ) {
				close(radkfile_fd);
			}
		}                                                                                                             

		/* Avoid recursion */
		tmp = kanjiDic;
		kanjiDic = NULL;
		gtk_widget_destroy(tmp->window);
		g_free(tmp);
		gjitenApp->kanjidic = NULL;
		gjiten_exit();
	}
}

void shade_kanjidic_widgets() {
  gtk_widget_set_sensitive(kanjiDic->spinb_strokenum, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)));
  gtk_widget_set_sensitive(kanjiDic->spinb_plusmin, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)));
  gtk_widget_set_sensitive(kanjiDic->label_plusmin, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)));

  gtk_widget_set_sensitive(kanjiDic->button_clearrad, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)));
  gtk_widget_set_sensitive(kanjiDic->button_radtable, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)));
  gtk_widget_set_sensitive(kanjiDic->combo_entry_radical, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)));
     
  gtk_widget_set_sensitive(kanjiDic->button_cleark, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_ksearch)));
  gtk_widget_set_sensitive(kanjiDic->combo_entry_key, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(kanjiDic->checkb_ksearch)));
}


void history_init() {
  GSList *tmp_list_ptr = NULL;
  GtkWidget *history_kanji_button;
  GtkWidget *history_kanji_label;
  gchar history_kanji_labelstr[100];
  gchar kanji[6];
  int i;
  
  tmp_list_ptr = kanjiDic->kanji_history_list;
  
  while (tmp_list_ptr != NULL) {
    for (i = 0; i < 6; i++) kanji[i] = 0;
    g_unichar_to_utf8((*(gunichar *)tmp_list_ptr->data), kanji);
 
    if (gjitenApp->conf->bigkanji == TRUE) {
      g_snprintf(history_kanji_labelstr, 100, "<span size=\"large\">%s</span>", kanji); 
      history_kanji_label = gtk_label_new(NULL);
      gtk_label_set_markup(GTK_LABEL(history_kanji_label), history_kanji_labelstr);
      gtk_widget_show(history_kanji_label);
      history_kanji_button = gtk_button_new();
      gtk_container_add(GTK_CONTAINER(history_kanji_button), history_kanji_label);
    }
    else {
      history_kanji_label = gtk_label_new(kanji);
      gtk_widget_show(history_kanji_label);
      history_kanji_button = gtk_button_new();
      gtk_container_add(GTK_CONTAINER(history_kanji_button), history_kanji_label);
    }
    if ((gjitenApp->conf->normalfont != NULL) && gjitenApp->conf->normalfont_desc != NULL) {
      gtk_widget_modify_font(history_kanji_label, gjitenApp->conf->normalfont_desc);
    }
    g_signal_connect_swapped(G_OBJECT(history_kanji_button), "clicked", G_CALLBACK(kanji_selected), 
			     (gpointer)(tmp_list_ptr->data));
    gtk_box_pack_start(GTK_BOX(kanjiDic->vbox_history), history_kanji_button, FALSE, FALSE, 0);
    // gtk_box_reorder_child(GTK_BOX(kanjiDic->vbox_history), history_kanji_button, 0);
    gtk_widget_show(history_kanji_button);
		if (gtk_widget_get_visible(kanjiDic->scrolledwin_history) != TRUE)
			gtk_widget_show(kanjiDic->scrolledwin_history);
    tmp_list_ptr = g_slist_next(tmp_list_ptr);
  }
}

void kanjidic_apply_fonts() {

	if (kanjiDic == NULL) return;

  if ((gjitenApp->conf->largefont == NULL) || (strlen(gjitenApp->conf->largefont) == 0)) {
		if (kanjiDic->tag_large_font != NULL) {
			g_object_set(kanjiDic->tag_large_font, "size", 20 * PANGO_SCALE, NULL);
		}
		else {
			kanjiDic->tag_large_font = gtk_text_buffer_create_tag(kanjiDic->text_kanjinfo_buffer, "largefont", "size", 20 * PANGO_SCALE, NULL);
		}
  }
  else {
		if (kanjiDic->tag_large_font != NULL) {
			g_object_set(kanjiDic->tag_large_font, "font", gjitenApp->conf->largefont, NULL);
		}
 		else {
			kanjiDic->tag_large_font = gtk_text_buffer_create_tag(kanjiDic->text_kanjinfo_buffer, "largefont", "font", gjitenApp->conf->largefont, NULL);
		}
  }

  if ((gjitenApp->conf->normalfont != NULL) && (strlen(gjitenApp->conf->normalfont) != 0)) {
    gjitenApp->conf->normalfont_desc = pango_font_description_from_string(gjitenApp->conf->normalfont);

    gtk_widget_modify_font(kanjiDic->kanji_results_view, gjitenApp->conf->normalfont_desc);
    gtk_widget_modify_font(kanjiDic->text_kanjinfo_view, gjitenApp->conf->normalfont_desc);
    gtk_widget_modify_font(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_key)), gjitenApp->conf->normalfont_desc);
    gtk_widget_modify_font(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_radical)), gjitenApp->conf->normalfont_desc);
  }
}

void clear_radical_entry_box(gpointer entrybox) {
  gtk_entry_set_text(GTK_ENTRY(entrybox), "");
	if (GTK_IS_WIDGET(kanjiDic->window_radicals)) {
		g_hash_table_foreach(kanjiDic->rad_button_hash, (GHFunc) set_radical_button_unsensitive, (gpointer) TRUE);
	}
}


KanjiDic *kanjidic_create() {
  GtkWidget *vbox_maink;
  GtkWidget *hbox_spinb;
  GtkWidget *table_koptions;
  GtkWidget *toolbar_kanji;
  GtkWidget *menubar;
  GtkWidget *frame_koptions;
  GtkAdjustment *spinb_strokenum_adj;
  GtkAdjustment *spinb_plusmin_adj;
  GtkWidget *hseparator;
  GtkWidget *frame_kresults;
  GtkWidget *scrolledwin_kresults;
  GtkWidget *scrolledwin_kinfo;
  GtkWidget *hbox;
  GtkWidget *frame_kinfo;
	GtkWidget *vpane;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkAccelGroup *accel_group;
	GError *error = NULL;

  if (kanjiDic != NULL) {
		gtk_window_present(GTK_WINDOW(kanjiDic->window));
		return kanjiDic;
  }

	kanjiDic = g_new0(KanjiDic, 1);
	gjitenApp->kanjidic = kanjiDic;

  load_radkfile();

  if (kdic_line == NULL) kdic_line = (gchar *)g_malloc(KCFGNUM * KBUFSIZE);
  if (kdic_line == NULL) gjiten_abort_with_msg("Couldn't allocate memory\n");

  kanjiDic->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(kanjiDic->window), _("Gjiten - KanjiDic"));
  gtk_widget_set_can_default(kanjiDic->window, TRUE);
  g_signal_connect(G_OBJECT(kanjiDic->window), "destroy", G_CALLBACK(kanjidic_close), NULL);
  gtk_window_set_default_size(GTK_WINDOW(kanjiDic->window), 500, 500);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), GTK_WINDOW(kanjiDic->window));

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (kanjiDic->window), accel_group);

  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error))
  {
    g_message ("building menus failed: %s", error->message);
    g_error_free (error);
    exit (EXIT_FAILURE);
  }

  vbox_maink = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_maink);
  gtk_container_add(GTK_CONTAINER(kanjiDic->window), vbox_maink);

  if (gjitenApp->conf->menubar)
  {
    menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");
    gtk_widget_show(menubar);
    gtk_box_pack_start (GTK_BOX (vbox_maink), menubar, FALSE, FALSE, 0);
  }
   
  if (gjitenApp->conf->toolbar) {
    toolbar_kanji = gtk_ui_manager_get_widget (ui_manager, "/ToolBar");
    gtk_widget_show(toolbar_kanji);
    gtk_box_pack_start (GTK_BOX (vbox_maink), toolbar_kanji, FALSE, FALSE, 0);
  }
  
  frame_koptions = gtk_frame_new(_("Kanji Search Options"));
  gtk_widget_show(frame_koptions);
  gtk_box_pack_start(GTK_BOX(vbox_maink), frame_koptions, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame_koptions), 2);

  table_koptions = gtk_grid_new();
  gtk_widget_show(table_koptions);
  gtk_container_add(GTK_CONTAINER(frame_koptions), table_koptions);

  kanjiDic->checkb_stroke = gtk_check_button_new_with_mnemonic(_("Search By _Strokes:")); 
  gtk_widget_show(kanjiDic->checkb_stroke);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->checkb_stroke, 0, 0, 1, 1);
  g_signal_connect(G_OBJECT(kanjiDic->checkb_stroke), "toggled",
									 G_CALLBACK(shade_kanjidic_widgets), NULL);

  kanjiDic->checkb_radical = gtk_check_button_new_with_mnemonic(_("Search By _Radical:"));  
  gtk_widget_show(kanjiDic->checkb_radical);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->checkb_radical, 0, 1, 1, 1);
  g_signal_connect (G_OBJECT(kanjiDic->checkb_radical), "toggled",
		      G_CALLBACK(shade_kanjidic_widgets), NULL);
 
  kanjiDic->checkb_ksearch = gtk_check_button_new_with_mnemonic(_("Search By _Key:"));
  gtk_widget_show(kanjiDic->checkb_ksearch);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->checkb_ksearch, 0, 2, 1, 1);
  g_signal_connect(G_OBJECT(kanjiDic->checkb_ksearch), "toggled",
		      G_CALLBACK(shade_kanjidic_widgets), NULL);

  hbox_spinb = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox_spinb);
  gtk_grid_attach(GTK_GRID(table_koptions), hbox_spinb, 1, 0, 1, 1);

  spinb_strokenum_adj = (GtkAdjustment*)gtk_adjustment_new(1, 1, 30, 1, 2, 0);
  kanjiDic->spinb_strokenum = gtk_spin_button_new(GTK_ADJUSTMENT(spinb_strokenum_adj), 1, 0);
  gtk_widget_show(kanjiDic->spinb_strokenum);
  gtk_box_pack_start(GTK_BOX(hbox_spinb), kanjiDic->spinb_strokenum, FALSE, FALSE, 0);

  kanjiDic->label_plusmin = gtk_label_new("+/-");
  gtk_widget_show(kanjiDic->label_plusmin);
  gtk_box_pack_start(GTK_BOX(hbox_spinb), kanjiDic->label_plusmin, FALSE, FALSE, 0);

  spinb_plusmin_adj = (GtkAdjustment*)gtk_adjustment_new(0, 0, 10, 1, 10, 0);
  kanjiDic->spinb_plusmin = gtk_spin_button_new(GTK_ADJUSTMENT(spinb_plusmin_adj), 1, 0);
  gtk_widget_show(kanjiDic->spinb_plusmin);
  gtk_box_pack_start(GTK_BOX(hbox_spinb), kanjiDic->spinb_plusmin, FALSE, FALSE, 0);

  kanjiDic->button_radtable = gtk_button_new_with_mnemonic(_("Show Radical _List"));
  gtk_widget_show(kanjiDic->button_radtable);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->button_radtable, 3, 1, 1, 1);
  g_signal_connect(G_OBJECT(kanjiDic->button_radtable), "clicked",
		      G_CALLBACK(create_window_radicals), NULL);

  kanjiDic->combo_entry_radical = gtk_combo_box_text_new_with_entry();
  gtk_widget_set_hexpand (kanjiDic->combo_entry_radical, TRUE);
  gtk_widget_show(kanjiDic->combo_entry_radical);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->combo_entry_radical, 1, 1, 1, 1);
  g_signal_connect(G_OBJECT(kanjiDic->combo_entry_radical),
		      "changed", G_CALLBACK(on_kanji_search), NULL);

  kanjiDic->combo_entry_key = gtk_combo_box_text_new_with_entry();
  gtk_widget_set_hexpand (kanjiDic->combo_entry_key, TRUE);
  gtk_widget_show(kanjiDic->combo_entry_key);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->combo_entry_key, 1, 2, 1, 1);
  g_signal_connect(G_OBJECT(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_key))),
		      "activate", G_CALLBACK(on_kanji_search), NULL);

  kanjiDic->button_clearrad = gtk_button_new_with_label(_("Clear"));
  g_object_set (kanjiDic->button_clearrad, "margin-left", 5, NULL);
  g_object_set (kanjiDic->button_clearrad, "margin-right", 5, NULL);
  gtk_widget_show(kanjiDic->button_clearrad);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->button_clearrad, 2, 1, 1, 1);
  g_signal_connect_swapped(G_OBJECT(kanjiDic->button_clearrad), "clicked",
													 G_CALLBACK(clear_radical_entry_box),
													 G_OBJECT(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_radical))));

  kanjiDic->button_cleark = gtk_button_new_with_label(_("Clear"));
  g_object_set (kanjiDic->button_cleark, "margin-left", 5, NULL);
  g_object_set (kanjiDic->button_cleark, "margin-right", 5, NULL);
  gtk_widget_show(kanjiDic->button_cleark);
  gtk_grid_attach(GTK_GRID(table_koptions), kanjiDic->button_cleark, 2, 2, 1, 1);
  g_signal_connect_swapped(G_OBJECT(kanjiDic->button_cleark), "clicked",
													 G_CALLBACK(gjiten_clear_entry_box),
													 G_OBJECT(gtk_bin_get_child(GTK_BIN(kanjiDic->combo_entry_key))));

  hseparator = gtk_hseparator_new();
  gtk_widget_show(hseparator);
  gtk_box_pack_start(GTK_BOX(vbox_maink), hseparator, FALSE, FALSE, 7);

  frame_kresults = gtk_frame_new(_("Search Results :"));
  gtk_widget_show(frame_kresults);
  gtk_container_set_border_width(GTK_CONTAINER(frame_kresults), 2);

  scrolledwin_kresults = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin_kresults),
				 GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin_kresults), GTK_SHADOW_IN);
  gtk_widget_show(scrolledwin_kresults);
  gtk_container_add(GTK_CONTAINER(frame_kresults), scrolledwin_kresults);

  kanjiDic->kanji_results_view = gtk_text_view_new();
  kanjiDic->kanji_results_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(kanjiDic->kanji_results_view));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(kanjiDic->kanji_results_view), GTK_WRAP_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(kanjiDic->kanji_results_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(kanjiDic->kanji_results_view), FALSE);
  gtk_widget_show(kanjiDic->kanji_results_view);
  gtk_container_add(GTK_CONTAINER(scrolledwin_kresults), kanjiDic->kanji_results_view);
  gtk_widget_set_size_request(kanjiDic->kanji_results_view, -1, 66);

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox);
  
  kanjiDic->text_kanjinfo_view = gtk_text_view_new();
  kanjiDic->text_kanjinfo_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(kanjiDic->text_kanjinfo_view));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(kanjiDic->text_kanjinfo_view), GTK_WRAP_WORD);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(kanjiDic->text_kanjinfo_view), FALSE);
  gtk_widget_show(kanjiDic->text_kanjinfo_view);

  frame_kinfo = gtk_frame_new(_("Kanji Info :"));
  gtk_widget_show(frame_kinfo);
  gtk_container_set_border_width(GTK_CONTAINER(frame_kinfo), 2);
  gtk_container_add(GTK_CONTAINER(frame_kinfo), hbox);

  scrolledwin_kinfo = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin_kinfo),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin_kinfo), GTK_SHADOW_IN);
  gtk_widget_show(scrolledwin_kinfo);
  gtk_box_pack_start(GTK_BOX(hbox), scrolledwin_kinfo, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(scrolledwin_kinfo), kanjiDic->text_kanjinfo_view);

  kanjiDic->scrolledwin_history = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(kanjiDic->scrolledwin_history),
																 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  kanjiDic->vbox_history = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(kanjiDic->vbox_history);
  history_init();

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(kanjiDic->scrolledwin_history), kanjiDic->vbox_history);
  gtk_box_pack_start(GTK_BOX(hbox), kanjiDic->scrolledwin_history, FALSE, TRUE, 0);

  vpane = gtk_vpaned_new();
  gtk_widget_show(vpane);
  gtk_paned_add1(GTK_PANED(vpane), frame_kresults);
  gtk_paned_add2(GTK_PANED(vpane), frame_kinfo);
  gtk_box_pack_start(GTK_BOX(vbox_maink), vpane, TRUE, TRUE, 0);

  kanjiDic->appbar_kanji = gtk_statusbar_new();
  gtk_widget_show(kanjiDic->appbar_kanji);
  gtk_box_pack_start(GTK_BOX(vbox_maink), kanjiDic->appbar_kanji, FALSE, FALSE, 0);

  shade_kanjidic_widgets();

  g_signal_connect(G_OBJECT(spinb_strokenum_adj),
				  "value_changed", G_CALLBACK(on_kanji_search), NULL);
  g_signal_connect(G_OBJECT(spinb_plusmin_adj),
				  "value_changed", G_CALLBACK(on_kanji_search), NULL);

  gtk_text_buffer_create_tag(kanjiDic->text_kanjinfo_buffer, "blue_foreground", "foreground", "blue", NULL);  

	kanjidic_apply_fonts();

  gtk_widget_show(kanjiDic->window);

	return kanjiDic;
}
