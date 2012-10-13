/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* worddic.c

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

/* This file contains the GUI part for WordDic */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>


#include "constants.h"
#include "kanjidic.h"
#include "conf.h"
#include "dicfile.h"
#include "worddic.h"
#include "gjiten.h"
#include "worddic.h"
#include "dicutil.h"
#include "pref.h"
#include "gjiten.h"
#include "error.h"

static void worddic_copy();
static void print_result(gchar *txt2print, int result_offset, gchar *searchstrg);
static void worddic_close();
static void worddic_destroy_window();
static void worddic_show_hide_options();
static void on_back_clicked();
static void on_forward_clicked();

extern GtkWidget *window_kanjidic;
extern GtkWidget *dialog_preferences;
extern GjitenConfig conf;
extern GjitenApp *gjitenApp;

WordDic *wordDic = NULL;

int word_matches;
gchar *vconj_types[40];
GSList *vinfl_list = NULL;
guint32 srchpos;
int engsrch, jpsrch;
int dicname_printed;
int append_to_history = TRUE;
gpointer current_glist_word = NULL;

static const GtkActionEntry entries[] = {
{"FileMenu", NULL, N_("File")},
{"EditMenu", NULL, N_("Edit")},
{"ToolsMenu", NULL, N_("Tools")},
{"HelpMenu", NULL, N_("Help")},
{"Quit", GTK_STOCK_QUIT, NULL, NULL, N_("Close Gjiten"), G_CALLBACK(worddic_destroy_window)},
{"Kanjidic", "my-gjiten-icon", N_("Kanjidic"), NULL, N_("Launch KanjiDic"), G_CALLBACK(kanjidic_create)},
{"Kanjipad", "my-kanjipad-icon", N_("Kanjipad"), NULL, N_("Launch KanjiPad"), G_CALLBACK(gjiten_start_kanjipad)},
{"Copy", GTK_STOCK_COPY, NULL, NULL, NULL, G_CALLBACK(worddic_copy)},
{"Paste", GTK_STOCK_PASTE, NULL, NULL, NULL, G_CALLBACK(worddic_paste)},
{"Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK(create_dialog_preferences)},
{"About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(gjiten_create_about)},
{"Help", GTK_STOCK_HELP, NULL, NULL, NULL, G_CALLBACK(gjiten_display_manual)},
{"Back", GTK_STOCK_GO_BACK, NULL, NULL, N_("Previous search result"), G_CALLBACK(on_back_clicked)},
{"Forward", GTK_STOCK_GO_FORWARD, NULL, NULL, N_("Next search result"), G_CALLBACK(on_forward_clicked)},
{"Find", GTK_STOCK_FIND, NULL, NULL, N_("Search for entered expression"), G_CALLBACK(on_text_entered)},
{"ShowHideOpts", NULL, N_("Hide/Show \nOptions"), NULL, N_("Show/Hide options"), G_CALLBACK(worddic_show_hide_options)},
};

static const char *ui_description =
"<ui>"
"<toolbar name='ToolBar' >"
"<toolitem action='Quit' />"
"<toolitem action='Back' />"
"<toolitem action='Forward' />"
"<toolitem action='Kanjidic' />"
"<toolitem action='Kanjipad' />"
"<toolitem action='Find' />"
"<toolitem action='ShowHideOpts' />"
"</toolbar>"
"<menubar name='MenuBar'>"
"<menu action='FileMenu' >"
"<menuitem action='Quit' />"
"</menu>"
"<menu action='EditMenu' >"
"<menuitem action='Copy' />"
"<menuitem action='Paste' />"
"<separator />"
"<menuitem action='Preferences' />"
"</menu>"
"<menu action='ToolsMenu' >"
"<menuitem action='Kanjidic' />"
"<menuitem action='Kanjipad' />"
"</menu>"
"<menu action='HelpMenu' >"
"<menuitem action='Help' />"
"<menuitem action='About' />"
"</menu>"
"</menubar>"
"</ui>";

static void worddic_copy() {
  gchar *selection = NULL;

  selection = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
  if (selection == NULL) return;
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), selection, -1);
}

void worddic_paste(WordDic *worddic) {
  gchar *selection = NULL;

  // First try the current selection
  selection = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
  if (selection != NULL) gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))), selection);
  else {
    // if we didn't get anything, try the default clipboard
    selection = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
      if (selection != NULL) gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))), selection);
  }
}

// Load & initialize verb inflection details
static void Verbinit() {
  static int verbinit_done = FALSE;
  gchar *tmp_ptr;
  int vinfl_size = 0;
  struct stat vinfl_stat;
  gchar *vinfl_start, *vinfl_ptr, *vinfl_end;
  int error = FALSE;
  int fd = 0;
  int vinfl_part = 1;
  int conj_type = 40;
  struct vinfl_struct *tmp_vinfl_struct;
  GSList *tmp_list_ptr = NULL;

  if (verbinit_done == TRUE) {
    //printf("Verbinit already done!\n");
    return;
  }

  if (stat(VINFL_FILENAME, &vinfl_stat) != 0) {
    printf("**ERROR** %s: stat() \n", VINFL_FILENAME);
    error = TRUE;
  }
  vinfl_size = vinfl_stat.st_size;
  fd = open(VINFL_FILENAME, O_RDONLY);
  if (fd == -1) {
    printf("**ERROR** %s: open()\n", VINFL_FILENAME);
    error = TRUE;
  }
  // printf("SIZE: %d\n", radkfile_size);
  vinfl_start = (gchar *) mmap(NULL, vinfl_size, PROT_READ, MAP_SHARED, fd, 0);
  if (vinfl_start == NULL) gjiten_abort_with_msg("mmap() failed for "VINFL_FILENAME"\n");

  //  printf("STRLEN: %d\n", strlen(radkfile));

  if (error == TRUE) {
    if (dialog_preferences == NULL) {
      gjiten_print_error(_("Error opening %s.\n "\
													 "Check your preferences or read the documentation!"),
												 VINFL_FILENAME); 
    }
    return;
  }

  vinfl_end = vinfl_start + strlen(vinfl_start);
  vinfl_ptr = vinfl_start;

  vinfl_part = 1;
  while ((vinfl_ptr < vinfl_end) && (vinfl_ptr != NULL)) {
    if (*vinfl_ptr == '#') {  //find $ as first char on the line
      vinfl_ptr = get_eof_line(vinfl_ptr, vinfl_end); //Goto next line
      continue;
    }
    if (*vinfl_ptr == '$') vinfl_part = 2;
    
    switch (vinfl_part) {
    case 1:
      if (g_ascii_isdigit(*vinfl_ptr) == TRUE) { //Conjugation numbers
				conj_type = atoi(vinfl_ptr);
				if ((conj_type < 0) || (conj_type > 39)) break;
				while (g_ascii_isdigit(*vinfl_ptr) == TRUE) vinfl_ptr = g_utf8_next_char(vinfl_ptr); //skip the number
				while (g_ascii_isspace(*vinfl_ptr) == TRUE) vinfl_ptr = g_utf8_next_char(vinfl_ptr); //skip the space
				tmp_ptr = vinfl_ptr; // beginning of conju	gation definition;
				vinfl_ptr = get_eof_line(vinfl_ptr, vinfl_end); //	find end of line
				vconj_types[conj_type] = g_strndup(tmp_ptr, vinfl_ptr - tmp_ptr -1);
				//printf("%d : %s\n", conj_type, vconj_types[conj_	type]);
      }
      break;
    case 2:
      if (g_unichar_iswide(g_utf8_get_char(vinfl_ptr)) == FALSE) {
				vinfl_ptr =  get_eof_line(vinfl_ptr, vinfl_end);
				break;
      }
      tmp_vinfl_struct = (struct vinfl_struct *) malloc (sizeof(struct vinfl_struct));
      tmp_ptr = vinfl_ptr;
      while (g_unichar_iswide(g_utf8_get_char(vinfl_ptr)) == TRUE) {
				vinfl_ptr = g_utf8_next_char(vinfl_ptr); //skip the conjugation
			}
      tmp_vinfl_struct->conj = g_strndup(tmp_ptr, vinfl_ptr - tmp_ptr); //store the conjugation
      while (g_ascii_isspace(*vinfl_ptr) == TRUE) {
				vinfl_ptr = g_utf8_next_char(vinfl_ptr); //skip the space
			}
      tmp_ptr = vinfl_ptr;
      while (g_unichar_iswide(g_utf8_get_char(vinfl_ptr)) == TRUE) {
				vinfl_ptr = g_utf8_next_char(vinfl_ptr); //skip the inflection	
			}
      tmp_vinfl_struct->infl = g_strndup(tmp_ptr, vinfl_ptr - tmp_ptr); //store the inflection
      while (g_ascii_isspace(*vinfl_ptr) == TRUE) {
				vinfl_ptr = g_utf8_next_char(vinfl_ptr); //skip the space
			}
      tmp_vinfl_struct->type = vconj_types[atoi(vinfl_ptr)];
      vinfl_ptr =  get_eof_line(vinfl_ptr, vinfl_end);
      //printf("%s|%s|%s\n", tmp_vinfl_struct->conj, tmp_vinfl_struct->infl, tmp_vinfl_struct->type);
      tmp_list_ptr = g_slist_append(tmp_list_ptr, tmp_vinfl_struct);
      if (vinfl_list == NULL) vinfl_list = tmp_list_ptr;
      break;
    }
  } 
  verbinit_done = TRUE;
}

static inline void print_matches_in(GjitenDicfile *dicfile) {
	//Print dicfile name if all dictionaries are selected
	if ((dicname_printed == FALSE) && (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_searchall)))) {
		gchar *tmpstr, *hl_start_ptr;
		gint hl_start = 0;
		gint hl_end = 0; 
						
		tmpstr = g_strdup_printf(_("Matches in %s:\n"), dicfile->name);
		hl_start_ptr = strstr(tmpstr, dicfile->name);
		hl_start = hl_start_ptr == NULL ? 0 : hl_start_ptr - tmpstr;
		hl_end = hl_start + strlen(dicfile->name);

		if (hl_start > 0)
			gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, tmpstr, hl_start);
		gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter,  
																						 dicfile->name, -1, "brown_foreground", NULL);
		gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, tmpstr + hl_end, -1);
		g_free(tmpstr);
		dicname_printed = TRUE;
	}
}

static void print_verb_inflections(GjitenDicfile *dicfile, gchar *srchstrg) {
	int srchresp, roff, rlen;
  guint32 oldrespos, respos;
  int gjit_search = SRCH_START;
  gchar repstr[1024];
  GSList *tmp_list_ptr;
  struct vinfl_struct *tmp_vinfl_struct;
  gchar *deinflected, *prevresult;
  int printit = TRUE;

  tmp_list_ptr = vinfl_list;
  if (vinfl_list == NULL) { 
    //printf("VINFL LIST == NULL\n");
    return;
  }

  deinflected = (gchar *) g_malloc(strlen(srchstrg) + 20);
  
  do {
    tmp_vinfl_struct = (struct vinfl_struct *) tmp_list_ptr->data;
    if (strg_end_compare(srchstrg, tmp_vinfl_struct->conj) == TRUE) {

      // create deinflected string
      strncpy(deinflected, srchstrg, strlen(srchstrg) - strlen(tmp_vinfl_struct->conj));
      strcpy(deinflected + strlen(srchstrg) - strlen(tmp_vinfl_struct->conj), 
						 tmp_vinfl_struct->infl);

			oldrespos = srchpos = 0;    
      gjit_search = SRCH_START;
      prevresult = NULL;
      do { // search loop
				oldrespos = respos;
    
				srchresp = search4string(gjit_search, dicfile, deinflected, &respos, &roff, &rlen, repstr);
				//    printf("respos:%d, oldrespos:%d, roffset:%d, rlen:%d\nrepstr:%s\n", respos, oldrespos, roff, rlen, repstr);
				if (srchresp != SRCH_OK)  {
					break;   //No more matches
				}
				if (gjit_search == SRCH_START) {
					srchpos = respos;
					gjit_search = SRCH_CONT;
				}
				srchpos++;
				if (oldrespos == respos) continue;

				printit = TRUE;

				if (is_kanji_only(repstr) == TRUE) {
					printit = FALSE;
				}
				else if (strlen(tmp_vinfl_struct->conj) == strlen(srchstrg)) 
					printit = FALSE; // don't display if conjugation is the same length as the srchstrg
				else if (get_jp_match_type(repstr, deinflected, roff) != EXACT_MATCH) 
					printit = FALSE; // Display only EXACT_MATCHes

				if (printit == TRUE) {
					print_matches_in(dicfile);
					gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter,  
																									 _("Possible inflected verb or adjective: "), 
																									 -1, "brown_foreground", NULL);
					gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, tmp_vinfl_struct->type, -1);
					gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, " " , -1);
					gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, tmp_vinfl_struct->conj, -1);
					gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, "\xe2\x86\x92", -1); //arrow
					gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, tmp_vinfl_struct->infl, -1);
					gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, "\n   ", -1);
					print_result(repstr, roff, deinflected);
					word_matches++;
				}
      } while (srchresp == SRCH_OK);
    }
  } while ((tmp_list_ptr = g_slist_next(tmp_list_ptr)) != NULL);
  
  g_free(deinflected);

}


static void print_result(gchar *txt2print, int result_offset, gchar *searchstrg) {
  gchar *strg_to_roff;
  glong strlen_to_roff;
  gchar *currentchar;
  gchar *kana_start;
  gchar *exp_start;
  GtkTextMark *linestart;
  GtkTextIter startmatch, endmatch;

  linestart = gtk_text_buffer_create_mark(GTK_TEXT_BUFFER(wordDic->text_results_buffer),
																					"linestart", &wordDic->iter, TRUE);

  strg_to_roff = (gchar *) g_strndup(txt2print, result_offset);
  strlen_to_roff = g_utf8_strlen(strg_to_roff, -1);
  
  currentchar = txt2print;

	while (!((*currentchar == '[') || (*currentchar == '/'))) { // find end of [KANJI]
    if ((size_t) (currentchar - txt2print) >= strlen(txt2print)) break;
    currentchar = g_utf8_next_char(currentchar);
  }
  currentchar = g_utf8_prev_char(currentchar); // go back to the space

  //print out japanese word == [KANJI]
  if (gjitenApp->conf->bigwords == FALSE) {
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer),
													 &wordDic->iter, txt2print, currentchar - txt2print); 
  }
  else {
    gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter,
																						 txt2print, currentchar - txt2print, "largefont", NULL);
  }
  
  currentchar = g_utf8_next_char(currentchar);
  if (*(currentchar) == '[')  { //we have a kana reading
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, " (", 2);
    currentchar = kana_start = g_utf8_next_char(currentchar);
    while(*(currentchar) != ']') { //find ending ]
      currentchar = g_utf8_next_char(currentchar);
    }
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer),
													 &wordDic->iter, kana_start, currentchar - kana_start); //print out kana reading
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, ") ", 2);
    currentchar += 3;
  }
  else {
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer),
													 &wordDic->iter, " ", 1); // insert space 
    currentchar++;
  }

  while (currentchar < txt2print + strlen(txt2print)) {
		if (*currentchar == '\n') break;
    exp_start = currentchar;
    while (!((*currentchar == '/') || (*currentchar == '\n'))) { 
      currentchar = g_utf8_next_char(currentchar);  
    }
		if (*currentchar == '\n') break;
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer),
													 &wordDic->iter, exp_start, currentchar - exp_start); //print out expression
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, "; ", 2);
		currentchar = g_utf8_next_char(currentchar);  
  }
  gtk_text_buffer_insert(GTK_TEXT_BUFFER(wordDic->text_results_buffer),
												 &wordDic->iter, "\n", 1); // insert linebreak

  //find searchstrg matches in the line. we print and highlight it.
  gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &endmatch, linestart);
  
  while (gtk_text_iter_forward_search(&endmatch, searchstrg, 0,
																			&startmatch, &endmatch, &wordDic->iter) == TRUE) {
    gtk_text_buffer_apply_tag_by_name(GTK_TEXT_BUFFER(wordDic->text_results_buffer), 
																			"blue_foreground", &startmatch, &endmatch);
  }
} 


static void search_in_dicfile(GjitenDicfile *dicfile, gchar *srchstrg) {
  gint srchresp, roff, rlen;
  gchar repstr[1024];
  guint32 respos, oldrespos;
  gint printit;
  gint match_criteria = EXACT_MATCH;
  gchar *currchar;
  gint match_type = ANY_MATCH;
  gint gjit_search = SRCH_START;

  //Detect Japanese
  engsrch = TRUE;
  jpsrch = FALSE;
  currchar = srchstrg;
  do { 
    if (g_unichar_iswide(g_utf8_get_char(currchar)) == TRUE) { //FIXME: this doesn't detect all Japanese
      engsrch = FALSE;
      jpsrch = TRUE;
      break;
    }
  } while ((currchar = g_utf8_find_next_char(currchar, srchstrg + strlen(srchstrg))) != NULL);
  
  // Verb deinfelction
  if (gjitenApp->conf->verb_deinflection == TRUE) print_verb_inflections(dicfile, srchstrg);

  if (jpsrch == TRUE) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_jpexact))) match_criteria = EXACT_MATCH;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_startw))) match_criteria = START_WITH_MATCH;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_endw))) match_criteria = END_WITH_MATCH;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_any))) match_criteria = ANY_MATCH;
  }
  else {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_engexact))) match_criteria = EXACT_MATCH;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_words))) match_criteria = WORD_MATCH;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_partial))) match_criteria = ANY_MATCH;
  }

  oldrespos = srchpos = 0;
  
  do { // search loop
    oldrespos = respos;
    
    srchresp = search4string(gjit_search, dicfile, srchstrg, &respos, &roff, &rlen, repstr);

    if (srchresp != SRCH_OK)  {
      return;   //No more matches
    }

    if (gjit_search == SRCH_START) {
      srchpos = respos;
      gjit_search = SRCH_CONT;
    }

    srchpos++;
    if (oldrespos == respos) continue;

    // Check match type and search options
    printit = FALSE;
    if (jpsrch) {
      match_type = get_jp_match_type(repstr, srchstrg, roff);
      switch (match_criteria) {
      case EXACT_MATCH : 
				if (match_type == EXACT_MATCH) printit = TRUE;
				break;
      case START_WITH_MATCH:
				if ((match_type == START_WITH_MATCH) || (match_type == EXACT_MATCH)) printit = TRUE;
				break;
      case END_WITH_MATCH:
				if ((match_type == END_WITH_MATCH) || (match_type == EXACT_MATCH)) printit = TRUE;
				break;
      case ANY_MATCH:
				printit = TRUE;
				break;
      }
    }
    else { //Non-japanese search
      switch (match_criteria) {
      case EXACT_MATCH:
				//Must lie between two '/' delimiters
				if ((repstr[roff - 1] == '/') && (repstr[roff + strlen(srchstrg)] == '/')) printit = TRUE;
				//take "/(n) expression/" into accont
				else if ((repstr[roff - 2] == ')') && (repstr[roff + strlen(srchstrg)] == '/')) printit = TRUE;
				//also match verbs starting with a 'to'. eg: "/(n) to do_something/"
				else if ((repstr[roff - 2] == 'o') && (repstr[roff - 3] == 't') && (repstr[roff + strlen(srchstrg)] == '/') 
								 && ((repstr[roff - 5] == ')') || (repstr[roff - 4] == '/'))) printit = TRUE;
				break;
      case WORD_MATCH:
				if ((g_unichar_isalpha(g_utf8_get_char(repstr + roff + strlen(srchstrg))) == FALSE)  &&
						(g_unichar_isalpha(g_utf8_get_char(repstr + roff - 1)) == FALSE)) {
					/*
						printf("---------\n");
						printf("%s", repstr);
						if (g_unichar_isalpha(g_utf8_get_char(repstr + roff - 1)) == FALSE)  printf("beg:%s", repstr + roff - 1);
						if (g_unichar_isalpha(g_utf8_get_char(repstr + roff + strlen(srchstrg))) == FALSE)  
						printf("end:%s", repstr + roff + strlen(srchstrg));
						printf("---------\n");
					*/
					printit = TRUE;
				}
				break;
      case ANY_MATCH:
				printit = TRUE;
				break;
      }
    }
  
    if (printit) {
      /*
      printf("offset: %d: ", roff);
      printf("jptype: %d\n", match_type);
      printf("criteria: %d\n", match_criteria);
      */

      print_matches_in(dicfile);
      print_result(repstr, roff, srchstrg);
      //get_kanji_and_reading(repstr); FIXME
      word_matches++;
    }
    if ((gjitenApp->conf->searchlimit_enabled == TRUE) && (word_matches >= gjitenApp->conf->maxwordmatches)) {
      gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(wordDic->text_results_buffer), &wordDic->iter, _("Results truncated"), -1, "red_foreground", NULL);
      return;
    }
  } while (srchresp == SRCH_OK);
}

int lower_search_option() {
  if (!(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust))))
    return FALSE;
  if (jpsrch) { //Japanese srting
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_any)))
      return FALSE;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_jpexact))) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_startw), TRUE);
      return TRUE;
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_startw))) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_endw), TRUE);
      return TRUE;
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_endw))) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_any), TRUE);
      return TRUE;
    }
  }
  else if (engsrch) { //English
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_partial)))
      return FALSE;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_engexact))) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_words), TRUE);
      return TRUE;
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_words))) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_partial), TRUE);
      return TRUE;
    }
  }
  return FALSE;
}

static void worddic_hira_kata_search(GjitenDicfile *dicfile, gchar *srchstrg) {
	gchar *hirakata;
	if (gjitenApp->conf->search_kata_on_hira) {
		if (isKatakanaString(srchstrg) == TRUE) {
			hirakata = kata2hira(srchstrg);
			search_in_dicfile(dicfile, hirakata);
			g_free(hirakata);
		}
	}
	if (gjitenApp->conf->search_hira_on_kata) {
		if (isHiraganaString(srchstrg) == TRUE) {
			hirakata = hira2kata(srchstrg);
			search_in_dicfile(dicfile, hirakata);
			g_free(hirakata);
		}
	}
}

static void worddic_search(gchar *srchstrg) {
  gchar appbarmsg[50];
  int truncated;
	GjitenDicfile *dicfile;
	GSList *dicfile_node;

  word_matches = 0;

	if (gjitenApp->conf->dicfile_list == NULL) {
    snprintf(appbarmsg, 50, _("No dicfiles specified! Set your preferences first."));
    gtk_statusbar_pop(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Prefs"));
    gtk_statusbar_push(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Prefs"), appbarmsg);
    return;
  } 

	// remove leading and trailing spaces	
  while (g_ascii_isspace(srchstrg[0])) srchstrg++; 
  while (g_ascii_isspace(srchstrg[strlen(srchstrg)-1]) != 0) srchstrg[strlen(srchstrg)-1] = 0;

  if (strlen(srchstrg) == 0) return;
  gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))), srchstrg);

  truncated = 0;
  while (TRUE) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_searchall))) {
			dicfile_node = gjitenApp->conf->dicfile_list;
			while (dicfile_node != NULL) {
				if (dicfile_node->data != NULL ) {
					dicfile = dicfile_node->data;
					dicname_printed = FALSE;
					search_in_dicfile(dicfile, srchstrg);
					worddic_hira_kata_search(dicfile, srchstrg);
					if ((gjitenApp->conf->searchlimit_enabled == TRUE) && (word_matches >= gjitenApp->conf->maxwordmatches)) break;
				}
				dicfile_node = g_slist_next(dicfile_node);
      }
    }
    else {
			search_in_dicfile(gjitenApp->conf->selected_dic, srchstrg);
			worddic_hira_kata_search(gjitenApp->conf->selected_dic, srchstrg);
		}

    if ((gjitenApp->conf->searchlimit_enabled == TRUE) && (word_matches >= gjitenApp->conf->maxwordmatches)) truncated = 1;
    if (word_matches > 0) break;  // No need to search anymore
    if (lower_search_option() == FALSE) break;
  }

  if (word_matches) {
    if (truncated) snprintf(appbarmsg, 50, _("Matches found (truncated): %d"), word_matches);
    else snprintf(appbarmsg, 50, _("Matches found: %d"), word_matches);
	gtk_statusbar_pop(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Search"));
	gtk_statusbar_push(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Search"), appbarmsg);
  }
  else {
	gtk_statusbar_pop(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Search"));
	gtk_statusbar_push(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Search"), _("No match found!"));
  }
}

void on_text_entered() {
  static gchar *new_entry_text = NULL;

	gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(wordDic->text_results_view), GTK_TEXT_WINDOW_TEXT), wordDic->regular_cursor);
	wordDic->is_cursor_regular = TRUE;

  new_entry_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry)))));
  if (g_utf8_validate(new_entry_text, -1, NULL) == FALSE) {
    gjiten_print_error_and_wait(_("Invalid input: non-utf8\n")); 
    g_free(new_entry_text);
    return;
  }
  if (strlen(new_entry_text) == 0) return;
  if (append_to_history == TRUE) {
    if (current_glist_word != NULL) {
      if (!(strcmp((char*) g_list_find(wordDic->combo_entry_glist, current_glist_word)->data, new_entry_text) == 0)) {
				current_glist_word = new_entry_text;
				wordDic->combo_entry_glist = g_list_prepend(wordDic->combo_entry_glist, new_entry_text);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wordDic->combo_entry), new_entry_text);
      }
    }
    else {
      current_glist_word = new_entry_text;
      wordDic->combo_entry_glist = g_list_prepend(wordDic->combo_entry_glist, new_entry_text);
    }
  }

  if (g_list_next(g_list_find(wordDic->combo_entry_glist, current_glist_word)) != NULL) 
    gtk_widget_set_sensitive(wordDic->button_back, TRUE);
  else gtk_widget_set_sensitive(wordDic->button_back, FALSE);

  if (g_list_previous(g_list_find(wordDic->combo_entry_glist, current_glist_word)) != NULL) {
    gtk_widget_set_sensitive(wordDic->button_forward, TRUE);
	}
  else gtk_widget_set_sensitive(wordDic->button_forward, FALSE);

  gtk_text_buffer_set_text (GTK_TEXT_BUFFER(wordDic->text_results_buffer), "", 0);
  gtk_text_buffer_get_start_iter(wordDic->text_results_buffer, &wordDic->iter);

  gtk_statusbar_pop(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Search"));
  gtk_statusbar_push(GTK_STATUSBAR(wordDic->appbar_mainwin),gtk_statusbar_get_context_id(GTK_STATUSBAR(wordDic->appbar_mainwin),"Search"), _("Searching..."));

  worddic_search(new_entry_text);
  
  if (append_to_history == FALSE)
    g_free(new_entry_text);
}

static void on_forward_clicked() { 
  append_to_history = FALSE;
  current_glist_word = (gchar*) g_list_previous(g_list_find(wordDic->combo_entry_glist, current_glist_word))->data;
  gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))), current_glist_word);
  on_text_entered();  
  append_to_history = TRUE;
}

static void on_back_clicked() {
  append_to_history = FALSE;
  current_glist_word = (gchar*) g_list_next(g_list_find(wordDic->combo_entry_glist, current_glist_word))->data;
  gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))), current_glist_word);
  on_text_entered();
  append_to_history = TRUE;
}

static void on_dicselection_clicked(GtkComboBox* my_combo, gpointer data) {
  GSList *dicfile_node;
  gint position = gtk_combo_box_get_active(my_combo);
  if (position < 0) return;
  dicfile_node = gjitenApp->conf->dicfile_list;
  for (; position > 0; position--) dicfile_node = g_slist_next(dicfile_node);
	gjitenApp->conf->selected_dic = dicfile_node->data;
}

static void checkb_searchlimit_toggled() {
  int state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->checkb_searchlimit));
  if (wordDic->spinb_searchlimit != NULL) gtk_widget_set_sensitive(wordDic->spinb_searchlimit, state);
  gjitenApp->conf->searchlimit_enabled = state;
  if (gjitenApp->conf->maxwordmatches == 0) gjitenApp->conf->searchlimit_enabled = FALSE;
}

static void shade_worddic_widgets() {
  if ((wordDic->menu_selectdic != NULL) && (wordDic->radiob_searchdic != NULL)) 
    gtk_widget_set_sensitive(wordDic->menu_selectdic, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->radiob_searchdic)));

  if (wordDic->checkb_autoadjust != NULL)
		gjitenApp->conf->autoadjust_enabled = (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust)));
}


static void get_searchlimit() {
  gjitenApp->conf->maxwordmatches = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wordDic->spinb_searchlimit));
  if (gjitenApp->conf->maxwordmatches == 0) gjitenApp->conf->searchlimit_enabled = FALSE;
}

static gboolean set_focus_on_entry(GtkWidget *window, GdkEventKey *key, GtkWidget *entry) {
	//Only set focus on the entry for real input
	if (key->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK)) return FALSE; 
	if ((key->keyval >= GDK_KEY_exclam && key->keyval <= GDK_KEY_overline) ||
			(key->keyval >= GDK_KEY_KP_Space && key->keyval <= GDK_KEY_KP_9)) {
		if (gtk_widget_has_focus(entry) != TRUE) {
			gtk_widget_grab_focus(entry);
		}
	}
	return FALSE;
}

static void worddic_init_history() {
	gint i;

	for (i = 0; i <= 50; i++) {
    if (gjitenApp->conf->history[i] == NULL) break;
    if (g_utf8_validate(gjitenApp->conf->history[i], -1, NULL) == TRUE)
      wordDic->combo_entry_glist = g_list_append(wordDic->combo_entry_glist, g_strdup(gjitenApp->conf->history[i]));
    //	 printf("Read: %s: %s\n", historystr, tmpptr);
  } 
}

static void worddic_destroy_window() {
	if ((wordDic != NULL) && (GTK_IS_WIDGET(wordDic->window))) {
		gtk_widget_destroy(wordDic->window);
	}
}

static void worddic_close() {

	GJITEN_DEBUG("WORDDIC_CLOSE\n");
	if (wordDic != NULL) {
		conf_save_history(g_list_first(wordDic->combo_entry_glist), gjitenApp->conf);
		if (GTK_IS_WIDGET(wordDic->window) == TRUE) gtk_widget_destroy(wordDic->window);
		g_free(wordDic);
		//FIXME: clear combo_entry_glist
		wordDic = NULL;
		gjitenApp->worddic = NULL;
		current_glist_word = NULL;
	}
	gjiten_exit();

}

static void worddic_show_hide_options() {
	GtkWidget *widget = gtk_widget_get_parent(wordDic->hbox_options);
	GJITEN_DEBUG("worddic_show_hide_options()\n");
	gtk_expander_set_expanded(GTK_EXPANDER(widget),
		!gtk_expander_get_expanded(GTK_EXPANDER(widget)));
}

void worddic_update_dic_menu() {
	GSList *dicfile_node;
	GjitenDicfile *dicfile;

	if (wordDic == NULL) return;

	GJITEN_DEBUG("worddic_update_dic_menu()\n");

	/*
	if (GTK_IS_WIDGET(wordDic->menu_selectdic)) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(wordDic->dicselection_menu));
		gtk_widget_destroy(wordDic->menu_selectdic);
	}
	*/

	dicfile_node = gjitenApp->conf->dicfile_list;
	while (dicfile_node != NULL) {
		if (dicfile_node->data != NULL) {
			dicfile = dicfile_node->data;
			gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wordDic->dicselection_menu), dicfile->name);
		}
		dicfile_node = g_slist_next(dicfile_node);
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(wordDic->dicselection_menu), 0);
	gtk_widget_show_all(wordDic->dicselection_menu);
	if (gjitenApp->conf->dicfile_list != NULL) gjitenApp->conf->selected_dic = gjitenApp->conf->dicfile_list->data;
}

void worddic_apply_fonts() {

	if (wordDic == NULL) return;

  if ((gjitenApp->conf->largefont == NULL) || (strlen(gjitenApp->conf->largefont) == 0)) {
		if (wordDic->tag_large_font != NULL) {
			g_object_set(wordDic->tag_large_font, "size", 20 * PANGO_SCALE, NULL);
		}
		else {
			wordDic->tag_large_font = gtk_text_buffer_create_tag(wordDic->text_results_buffer, "largefont", "size", 20 * PANGO_SCALE, NULL);
		}
  }
  else {
		if (wordDic->tag_large_font != NULL) {
			g_object_set(wordDic->tag_large_font, "font", gjitenApp->conf->largefont, NULL);
		}
 		else {
			wordDic->tag_large_font = gtk_text_buffer_create_tag(wordDic->text_results_buffer, "largefont", "font", gjitenApp->conf->largefont, NULL);
		}
  }
  if ((gjitenApp->conf->normalfont != NULL) && (strlen(gjitenApp->conf->normalfont) != 0)) {
    gjitenApp->conf->normalfont_desc = pango_font_description_from_string(gjitenApp->conf->normalfont);
		gtk_widget_modify_font(wordDic->text_results_view, gjitenApp->conf->normalfont_desc);
    gtk_widget_modify_font(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry)), gjitenApp->conf->normalfont_desc);
  }

}


/*
 * Update the cursor image if the pointer is above a kanji. 
 */
static gboolean result_view_motion(GtkWidget *text_view, GdkEventMotion *event) {
  gint x, y;
  GtkTextIter mouse_iter;
	gunichar kanji;
	gint trailing;

  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
																				GTK_TEXT_WINDOW_WIDGET,
																				event->x, event->y, &x, &y);

  gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(text_view), &mouse_iter, &trailing, x , y);
	kanji = gtk_text_iter_get_char(&mouse_iter);

	// Change the cursor if necessary
	if ((isKanjiChar(kanji) == TRUE)) {
		gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(text_view), GTK_TEXT_WINDOW_TEXT), wordDic->selection_cursor);
		wordDic->is_cursor_regular = FALSE;
	}
	else if (wordDic->is_cursor_regular == FALSE) {
		gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(text_view), GTK_TEXT_WINDOW_TEXT), wordDic->regular_cursor);
		wordDic->is_cursor_regular = TRUE;
	}

  return FALSE;
}



static gboolean kanji_clicked(GtkWidget *text_view, GdkEventButton *event, gpointer user_data) {
	GtkTextIter mouse_iter;
	gint x, y;
	gint trailing;
	gunichar kanji;

  if (event->button != 1) return FALSE;

	if (gtk_text_buffer_get_selection_bounds(wordDic->text_results_buffer, NULL, NULL) == TRUE )
	{
		// don't look up kanji if it is in a selection
		return FALSE;
	}

  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW (text_view), 
																				GTK_TEXT_WINDOW_WIDGET,
																				event->x, event->y, &x, &y);

  gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(text_view), &mouse_iter, &trailing, x, y);
	kanji = gtk_text_iter_get_char(&mouse_iter);
	if ((kanji != 0xFFFC) && (kanji != 0) && (isKanjiChar(kanji) == TRUE)) {
		kanjidic_create();
		kanji_selected(kanji);
	}

  return FALSE;
}


WordDic *worddic_create() {
  GtkWidget *vbox_main;
  GtkWidget* menubar;
  GtkWidget *toolbar;
  GtkWidget *button_clear;
  GtkWidget *frame_japopt;
  GtkWidget *exp_japopt;
  GtkWidget *vbox_japopt;
  GSList *vbox_japopt_group = NULL;
  GSList *dicssearch_group = NULL;
  GtkWidget *frame_engopt;
  GtkWidget *vbox_engopt;
  GSList *vbox_engopt_group = NULL;
  GtkWidget *table_gopt;
  GtkWidget *frame_gopt;
  GtkWidget *hbox_searchlimit;
  GtkWidget *hbox_entry;
  GtkWidget *label_enter;
  GtkWidget *button_search;
  GtkWidget *frame_results;
  GtkWidget *vbox_results;
  GtkWidget *scrolledwin_results;
  GtkAdjustment *spinb_searchlimit_adj;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  GdkPixbuf *cursor_pixbuf;
  GError *error = NULL;

	if (wordDic == NULL) {
		wordDic = g_new0(WordDic, 1);
		gjitenApp->worddic = wordDic;
	}
	else {
		gtk_window_present(GTK_WINDOW(wordDic->window));
		return wordDic;
	}

  cursor_pixbuf = gdk_pixbuf_new_from_file(PIXMAPDIR"/left_ptr_question.png", NULL);
  wordDic->selection_cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), cursor_pixbuf, 0, 0);
  wordDic->regular_cursor = gdk_cursor_new(GDK_XTERM);
	wordDic->is_cursor_regular = TRUE;

  wordDic->spinb_searchlimit = NULL;
  wordDic->radiob_searchdic = NULL;
  wordDic->checkb_autoadjust = NULL;
  wordDic->checkb_verb = NULL;

	worddic_init_history();
  Verbinit(); //FIXME: On demand

  wordDic->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(wordDic->window), _("Gjiten - WordDic"));
  gtk_widget_set_can_default(wordDic->window, TRUE);
  g_signal_connect(G_OBJECT(wordDic->window), "destroy", G_CALLBACK(worddic_close), NULL);
  gtk_window_set_default_size(GTK_WINDOW(wordDic->window), 500, 500);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries), GTK_WINDOW(wordDic->window));

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (wordDic->window), accel_group);

  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, &error))
  {
    g_message ("building menus failed: %s", error->message);
    g_error_free (error);
    exit (EXIT_FAILURE);
  }

  menubar = gtk_ui_manager_get_widget (ui_manager, "/MenuBar");

	wordDic->button_back = gtk_ui_manager_get_widget (ui_manager, "/ToolBar/Back");
	gtk_widget_set_sensitive(GTK_WIDGET(wordDic->button_back), FALSE);

	wordDic->button_forward = gtk_ui_manager_get_widget (ui_manager, "/ToolBar/Forward");
	gtk_widget_set_sensitive(GTK_WIDGET(wordDic->button_forward), FALSE);

	//gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

  vbox_main = gtk_grid_new();
  gtk_widget_show(vbox_main);
  gtk_container_add(GTK_CONTAINER(wordDic->window), vbox_main);
  gtk_grid_attach (GTK_GRID (vbox_main), menubar, 0, 0, 1, 1);
  toolbar = gtk_ui_manager_get_widget (ui_manager, "/ToolBar"); //gtk_toolbar_new();
  gtk_widget_show(toolbar);
  gtk_grid_attach (GTK_GRID (vbox_main), toolbar, 0, 1, 1, 1);

  wordDic->hbox_options = gtk_grid_new();
  gtk_widget_show(wordDic->hbox_options);
  exp_japopt = gtk_expander_new("");
  gtk_widget_set_hexpand(exp_japopt, TRUE);
  gtk_widget_set_halign (exp_japopt, GTK_ALIGN_CENTER);
  gtk_container_add(GTK_CONTAINER(exp_japopt), wordDic->hbox_options);
  gtk_grid_attach(GTK_GRID(vbox_main), exp_japopt, 0, 2, 1, 1);

  frame_japopt = gtk_frame_new(_("Japanese Search Options: "));
  gtk_widget_show(frame_japopt);
  gtk_grid_attach(GTK_GRID(wordDic->hbox_options), frame_japopt, 0, 0, 1, 1);
  gtk_container_set_border_width(GTK_CONTAINER(frame_japopt), 5);

  vbox_japopt = gtk_grid_new();
  gtk_widget_show(vbox_japopt);
  gtk_container_add(GTK_CONTAINER(frame_japopt), vbox_japopt);

  wordDic->radiob_jpexact = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("E_xact Matches"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_jpexact));
  gtk_widget_show(wordDic->radiob_jpexact);
  gtk_grid_attach(GTK_GRID(vbox_japopt), wordDic->radiob_jpexact, 0, 0, 1, 1);

  wordDic->radiob_startw = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("_Start With Expression"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_startw));
  gtk_widget_show(wordDic->radiob_startw);
  gtk_grid_attach(GTK_GRID(vbox_japopt), wordDic->radiob_startw, 0, 1, 1, 1);

  wordDic->radiob_endw = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("E_nd With Expression"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_endw));
  gtk_widget_show(wordDic->radiob_endw);
  gtk_grid_attach(GTK_GRID(vbox_japopt), wordDic->radiob_endw, 0, 2, 1, 1);

  wordDic->radiob_any = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("_Any Matches"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_any));
  gtk_widget_show(wordDic->radiob_any);
  gtk_grid_attach(GTK_GRID(vbox_japopt), wordDic->radiob_any, 0, 3, 1, 1);

  frame_engopt = gtk_frame_new(_("English Search Options: "));
  gtk_widget_show(frame_engopt);
  gtk_grid_attach(GTK_GRID(wordDic->hbox_options), frame_engopt, 1, 0, 1, 1);
  gtk_container_set_border_width(GTK_CONTAINER(frame_engopt), 5);

  vbox_engopt = gtk_grid_new();
  gtk_widget_show(vbox_engopt);
  gtk_container_add(GTK_CONTAINER(frame_engopt), vbox_engopt);

  wordDic->radiob_engexact = gtk_radio_button_new_with_mnemonic(vbox_engopt_group, _("Wh_ole Expressions"));
  vbox_engopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_engexact));
  gtk_widget_show(wordDic->radiob_engexact);
  gtk_grid_attach(GTK_GRID(vbox_engopt), wordDic->radiob_engexact, 0, 0, 1, 1);

  wordDic->radiob_words = gtk_radio_button_new_with_mnemonic(vbox_engopt_group, _("_Whole Words"));
  vbox_engopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_words));
  gtk_widget_show(wordDic->radiob_words);
  gtk_grid_attach(GTK_GRID(vbox_engopt), wordDic->radiob_words, 0, 1, 1, 1);

  wordDic->radiob_partial = gtk_radio_button_new_with_mnemonic(vbox_engopt_group, _("Any _Matches"));
  vbox_engopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_partial));
  gtk_widget_show(wordDic->radiob_partial);
  gtk_grid_attach(GTK_GRID(vbox_engopt), wordDic->radiob_partial, 0, 2, 1, 1);

  frame_gopt = gtk_frame_new(_("General Options: "));
  gtk_widget_show(frame_gopt);
  gtk_grid_attach(GTK_GRID(wordDic->hbox_options), frame_gopt, 2, 0, 1, 1);
  gtk_container_set_border_width(GTK_CONTAINER(frame_gopt), 5);

  table_gopt = gtk_grid_new();
  gtk_widget_show(table_gopt);
  gtk_container_add(GTK_CONTAINER(frame_gopt), table_gopt);

  wordDic->radiob_searchdic = gtk_radio_button_new_with_mnemonic(dicssearch_group, _("Search _Dic:"));
  dicssearch_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_searchdic));
  gtk_widget_show(wordDic->radiob_searchdic);
  gtk_grid_attach(GTK_GRID(table_gopt), wordDic->radiob_searchdic, 0, 0, 1, 1);
  g_signal_connect_swapped(G_OBJECT(wordDic->radiob_searchdic), "clicked", 
													 G_CALLBACK(shade_worddic_widgets), NULL);

  // DICTFILE SELECTION MENU
	
  wordDic->dicselection_menu = gtk_combo_box_text_new();
	worddic_update_dic_menu();
  g_signal_connect(G_OBJECT(wordDic->dicselection_menu), "changed", G_CALLBACK(on_dicselection_clicked), NULL);
 
  gtk_grid_attach(GTK_GRID(table_gopt), wordDic->dicselection_menu, 1, 0, 1, 1);

  wordDic->radiob_searchall = gtk_radio_button_new_with_mnemonic(dicssearch_group, _("Sea_rch All Dictionaries"));
  gtk_widget_show(wordDic->radiob_searchall);
  gtk_grid_attach(GTK_GRID(table_gopt), wordDic->radiob_searchall, 0, 1, 2, 1);
  g_signal_connect_swapped(G_OBJECT(wordDic->radiob_searchall), "clicked", 
													 G_CALLBACK(shade_worddic_widgets), NULL);


  wordDic->checkb_autoadjust = gtk_check_button_new_with_mnemonic(_("A_uto Adjust Options"));
  gtk_widget_show(wordDic->checkb_autoadjust);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust), TRUE);
  gtk_grid_attach(GTK_GRID(table_gopt), wordDic->checkb_autoadjust, 0, 2, 2, 1);
  g_signal_connect(G_OBJECT(wordDic->checkb_autoadjust), "toggled", 
									 G_CALLBACK(shade_worddic_widgets), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust), gjitenApp->conf->autoadjust_enabled);

  
  hbox_searchlimit = gtk_grid_new();
  gtk_widget_show(hbox_searchlimit);
  gtk_grid_attach(GTK_GRID(table_gopt), hbox_searchlimit, 0, 3, 2, 1);
  wordDic->checkb_searchlimit = gtk_check_button_new_with_mnemonic(_("_Limit Results:"));
  gtk_widget_show(wordDic->checkb_searchlimit);
  gtk_grid_attach(GTK_GRID(hbox_searchlimit), wordDic->checkb_searchlimit, 0, 0, 1, 1);
  g_signal_connect(G_OBJECT(wordDic->checkb_searchlimit), "toggled", 
									 G_CALLBACK(checkb_searchlimit_toggled), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->checkb_searchlimit), gjitenApp->conf->searchlimit_enabled);

	spinb_searchlimit_adj = (GtkAdjustment*)gtk_adjustment_new(gjitenApp->conf->maxwordmatches, 1, G_MAXFLOAT, 1, 2, 0);
  wordDic->spinb_searchlimit = gtk_spin_button_new(GTK_ADJUSTMENT(spinb_searchlimit_adj), 1, 0);
  gtk_widget_show(wordDic->spinb_searchlimit);
  gtk_grid_attach(GTK_GRID(hbox_searchlimit), wordDic->spinb_searchlimit, 1, 0, 1, 1);
  gtk_widget_set_sensitive(wordDic->spinb_searchlimit, (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wordDic->checkb_searchlimit))));
  g_signal_connect(G_OBJECT(spinb_searchlimit_adj), "value_changed", 
									 G_CALLBACK(get_searchlimit), NULL);
  

  hbox_entry = gtk_grid_new();
  gtk_widget_show(hbox_entry);
  g_object_set(hbox_entry, "margin-top", 14, NULL);
  g_object_set(hbox_entry, "margin-bottom", 14, NULL);
  gtk_grid_attach(GTK_GRID(vbox_main), hbox_entry, 0, 3, 1, 1);
  gtk_container_set_border_width(GTK_CONTAINER(hbox_entry), 3);

  label_enter = gtk_label_new(_("Enter expression :"));
  g_object_set(label_enter, "margin-left", 5, NULL);
  g_object_set(label_enter, "margin-right", 5, NULL);
  gtk_widget_show(label_enter);
  gtk_grid_attach(GTK_GRID(hbox_entry), label_enter, 0, 0, 1, 1);
  gtk_label_set_justify(GTK_LABEL(label_enter), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment(GTK_MISC(label_enter), 1, 0.5);
  gtk_misc_set_padding(GTK_MISC(label_enter), 7, 0);

  wordDic->combo_entry = gtk_combo_box_text_new_with_entry();
  gtk_widget_set_hexpand(wordDic->combo_entry, TRUE);
  gtk_grid_attach(GTK_GRID(hbox_entry), wordDic->combo_entry, 1, 0, 1, 1);
  g_signal_connect(G_OBJECT(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))),
									 "activate", G_CALLBACK(on_text_entered), NULL);
  g_signal_connect(G_OBJECT(wordDic->window), "key_press_event",
									 G_CALLBACK(set_focus_on_entry), gtk_bin_get_child(GTK_BIN(wordDic->combo_entry)));

  if (wordDic->combo_entry_glist != NULL) {
	GList *my_list;
	for (my_list = wordDic->combo_entry_glist; my_list != NULL && my_list->data != NULL; my_list = g_list_next(my_list))
	{
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wordDic->combo_entry), my_list->data);
	}
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))), "");
  }
  gtk_widget_set_can_default(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry)), TRUE);
  gtk_widget_grab_focus(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry)));
  gtk_widget_grab_default(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry)));
  gtk_widget_show(wordDic->combo_entry);

  button_search = gtk_button_new_with_label(_("Search"));
  g_object_set(button_search, "margin-left", 7, NULL);
  g_object_set(button_search, "margin-right", 7, NULL);
  gtk_widget_show(button_search);
  gtk_grid_attach(GTK_GRID(hbox_entry), button_search, 2, 0, 1, 1);
  g_signal_connect(G_OBJECT(button_search), "clicked", G_CALLBACK(on_text_entered), NULL);

  button_clear = gtk_button_new_with_mnemonic(_("_Clear"));
  gtk_widget_show(button_clear);
  gtk_grid_attach(GTK_GRID(hbox_entry), button_clear, 3, 0, 1, 1);
  g_signal_connect_swapped(G_OBJECT(button_clear), "clicked", 
													 G_CALLBACK(gjiten_clear_entry_box), 
													 G_OBJECT(gtk_bin_get_child(GTK_BIN(wordDic->combo_entry))));

  frame_results = gtk_frame_new(_("Search results :"));
  gtk_widget_show(frame_results);
  gtk_grid_attach(GTK_GRID(vbox_main), frame_results, 0, 4, 1, 1);
  gtk_container_set_border_width(GTK_CONTAINER(frame_results), 5);
  gtk_frame_set_label_align(GTK_FRAME(frame_results), 0.03, 0.5);

  vbox_results = gtk_grid_new();
  gtk_widget_show(vbox_results);
  gtk_container_add(GTK_CONTAINER(frame_results), vbox_results);

  wordDic->text_results_view = gtk_text_view_new();
  gtk_widget_set_hexpand (wordDic->text_results_view, TRUE);
  gtk_widget_set_vexpand (wordDic->text_results_view, TRUE);
  wordDic->text_results_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(wordDic->text_results_view));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(wordDic->text_results_view), GTK_WRAP_WORD);

  gtk_widget_show(wordDic->text_results_view);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(wordDic->text_results_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(wordDic->text_results_view), FALSE);

  // enable clickable kanji
  g_signal_connect(G_OBJECT(wordDic->text_results_view), "button-release-event", G_CALLBACK(kanji_clicked), NULL);
  g_signal_connect(G_OBJECT(wordDic->text_results_view), "motion-notify-event", G_CALLBACK(result_view_motion), NULL);
  
  //set up fonts and tags
  gtk_text_buffer_create_tag(wordDic->text_results_buffer, "blue_foreground", "foreground", "blue", NULL);  
  gtk_text_buffer_create_tag(wordDic->text_results_buffer, "red_foreground", "foreground", "red", NULL);  
  gtk_text_buffer_create_tag(wordDic->text_results_buffer, "brown_foreground", "foreground", "brown", NULL);  

  worddic_apply_fonts();

  scrolledwin_results = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwin_results), GTK_SHADOW_IN);

  gtk_container_add(GTK_CONTAINER(scrolledwin_results), wordDic->text_results_view);
  gtk_grid_attach(GTK_GRID(vbox_results), scrolledwin_results, 0, 0, 1, 1);
  gtk_widget_show(scrolledwin_results);

  wordDic->appbar_mainwin = gtk_statusbar_new();
  gtk_widget_show(wordDic->appbar_mainwin);
  gtk_grid_attach(GTK_GRID(vbox_results), wordDic->appbar_mainwin, 0, 1, 1, 1);
 
  gtk_widget_show_all(wordDic->window);

  gjiten_flush_errors();

	return wordDic;
}
