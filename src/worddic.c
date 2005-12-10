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

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
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

static GnomeUIInfo file_menu_uiinfo[] = {
  GNOMEUIINFO_MENU_EXIT_ITEM(G_CALLBACK(worddic_destroy_window), NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo edit_menu_uiinfo[] = {
  //GNOMEUIINFO_MENU_CUT_ITEM(NULL, NULL),
  GNOMEUIINFO_MENU_COPY_ITEM(worddic_copy, NULL),
  GNOMEUIINFO_MENU_PASTE_ITEM(worddic_paste, NULL),
  // GNOMEUIINFO_MENU_CLEAR_ITEM(clear_entry_box, GTK_OBJECT(GTK_COMBO(wordDic->combo_entry)->entry)),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(create_dialog_preferences, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo tools_menu_uiinfo[] = {
  {
    GNOME_APP_UI_ITEM, N_("KanjiDic"), NULL, kanjidic_create, 
    NULL, NULL, GNOME_APP_PIXMAP_FILENAME, "kanjidic.png", 0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("KanjiPad"), NULL, gjiten_start_kanjipad, NULL, NULL,
    GNOME_APP_PIXMAP_FILENAME, "kanjipad.png", 0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu_uiinfo[] =  {
  {
    GNOME_APP_UI_ITEM, N_("_Manual"), N_("Display the Gjiten Manual"),
    gjiten_display_manual, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GTK_STOCK_HELP,
    0, 0, NULL },
  
  {    
    GNOME_APP_UI_ITEM, N_("_About"), N_("Information about the program"),
    gjiten_create_about, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_ABOUT,
    0, 0, NULL },
  
  GNOMEUIINFO_END
};

static GnomeUIInfo menubar_uiinfo[] = {
	GNOMEUIINFO_MENU_FILE_TREE(file_menu_uiinfo),
	GNOMEUIINFO_MENU_EDIT_TREE(edit_menu_uiinfo), 
	{
		GNOME_APP_UI_SUBTREE, N_("_Tools"), NULL, tools_menu_uiinfo, NULL, 
		NULL, GNOME_APP_PIXMAP_NONE, N_("Tools"), 0, 0, NULL
	},
	GNOMEUIINFO_MENU_HELP_TREE(help_menu_uiinfo),
	GNOMEUIINFO_END
};


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
  if (selection != NULL) gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry), selection);
  else {
    // if we didn't get anything, try the default clipboard
    selection = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
      if (selection != NULL) gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry), selection);
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
	if ((dicname_printed == FALSE) && (GTK_TOGGLE_BUTTON(wordDic->radiob_searchall)->active)) {
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
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_jpexact)->active) match_criteria = EXACT_MATCH;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_startw)->active) match_criteria = START_WITH_MATCH;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_endw)->active) match_criteria = END_WITH_MATCH;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_any)->active) match_criteria = ANY_MATCH;
  }
  else {
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_engexact)->active) match_criteria = EXACT_MATCH;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_words)->active) match_criteria = WORD_MATCH;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_partial)->active) match_criteria = ANY_MATCH;
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
  if (!(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust)->active))
    return FALSE;
  if (jpsrch) { //Japanese srting
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_any)->active)
      return FALSE;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_jpexact)->active) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_startw), TRUE);
      return TRUE;
    }
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_startw)->active) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_endw), TRUE);
      return TRUE;
    }
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_endw)->active) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_any), TRUE);
      return TRUE;
    }
  }
  else if (engsrch) { //English
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_partial)->active)
      return FALSE;
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_engexact)->active) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->radiob_words), TRUE);
      return TRUE;
    }
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_words)->active) {
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
    gnome_appbar_set_status(GNOME_APPBAR(wordDic->appbar_mainwin),appbarmsg);
    return;
  } 

	// remove leading and trailing spaces	
  while (g_ascii_isspace(srchstrg[0])) srchstrg++; 
  while (g_ascii_isspace(srchstrg[strlen(srchstrg)-1]) != 0) srchstrg[strlen(srchstrg)-1] = 0;

  if (strlen(srchstrg) == 0) return;
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry), srchstrg);

  truncated = 0;
  while (TRUE) {
    if (GTK_TOGGLE_BUTTON(wordDic->radiob_searchall)->active) {
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
    gnome_appbar_set_status(GNOME_APPBAR(wordDic->appbar_mainwin), appbarmsg);
  }
  else gnome_appbar_set_status(GNOME_APPBAR(wordDic->appbar_mainwin), _("No match found!"));
}

void on_text_entered() {
  static gchar *new_entry_text = NULL;

	gdk_window_set_cursor(gtk_text_view_get_window(GTK_TEXT_VIEW(wordDic->text_results_view), GTK_TEXT_WINDOW_TEXT), wordDic->regular_cursor);
	wordDic->is_cursor_regular = TRUE;

  new_entry_text = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry)));
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

  gnome_appbar_set_status(GNOME_APPBAR(wordDic->appbar_mainwin), _("Searching..."));

  worddic_search(new_entry_text);
  
  if (append_to_history == FALSE)
    g_free(new_entry_text);
  else 
    gtk_combo_set_popdown_strings(GTK_COMBO(wordDic->combo_entry), wordDic->combo_entry_glist);
}

static void on_forward_clicked() { 
  append_to_history = FALSE;
  current_glist_word = (gchar*) g_list_previous(g_list_find(wordDic->combo_entry_glist, current_glist_word))->data;
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry), current_glist_word);
  on_text_entered();  
  append_to_history = TRUE;
}

static void on_back_clicked() {
  append_to_history = FALSE;
  current_glist_word = (gchar*) g_list_next(g_list_find(wordDic->combo_entry_glist, current_glist_word))->data;
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry), current_glist_word);
  on_text_entered();
  append_to_history = TRUE;
}

static void on_dicselection_clicked(GjitenDicfile *selected) {
  gjitenApp->conf->selected_dic = selected;
}

static void checkb_searchlimit_toggled() {
  int state = GTK_TOGGLE_BUTTON(wordDic->checkb_searchlimit)->active;
  if (wordDic->spinb_searchlimit != NULL) gtk_widget_set_sensitive(wordDic->spinb_searchlimit, state);
  gjitenApp->conf->searchlimit_enabled = state;
  if (gjitenApp->conf->maxwordmatches == 0) gjitenApp->conf->searchlimit_enabled = FALSE;
}

static void shade_worddic_widgets() {
  if ((wordDic->menu_selectdic != NULL) && (wordDic->radiob_searchdic != NULL)) 
    gtk_widget_set_sensitive(wordDic->menu_selectdic, GTK_TOGGLE_BUTTON(wordDic->radiob_searchdic)->active);

  if (wordDic->checkb_autoadjust != NULL)
		gjitenApp->conf->autoadjust_enabled = (GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust)->active);
}


static void get_searchlimit() {
  gjitenApp->conf->maxwordmatches = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wordDic->spinb_searchlimit));
  if (gjitenApp->conf->maxwordmatches == 0) gjitenApp->conf->searchlimit_enabled = FALSE;
}

static gboolean set_focus_on_entry(GtkWidget *window, GdkEventKey *key, GtkWidget *entry) {
	//Only set focus on the entry for real input
	if (key->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK)) return FALSE; 
	if ((key->keyval >= GDK_exclam && key->keyval <= GDK_overline) ||
			(key->keyval >= GDK_KP_Space && key->keyval <= GDK_KP_9)) { 
		if (GTK_WIDGET_HAS_FOCUS(entry) != TRUE) {
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
	}
	gjiten_exit();

}

static void worddic_show_hide_options() {
	GJITEN_DEBUG("worddic_show_hide_options()\n");
	if (GTK_WIDGET_VISIBLE(wordDic->hbox_options) == TRUE) {
		gtk_widget_hide(wordDic->hbox_options);
	}
	else gtk_widget_show(wordDic->hbox_options);
}

void worddic_update_dic_menu() {
	GSList *dicfile_node;
  GtkWidget *menu_dictfiles_item;
	GjitenDicfile *dicfile;

	if (wordDic == NULL) return;

	GJITEN_DEBUG("worddic_update_dic_menu()\n");

	/*
	if (GTK_IS_WIDGET(wordDic->menu_selectdic)) {
		gtk_option_menu_remove_menu(GTK_OPTION_MENU(wordDic->dicselection_menu));
		gtk_widget_destroy(wordDic->menu_selectdic);
	}
	*/
  wordDic->menu_selectdic = gtk_menu_new();

	dicfile_node = gjitenApp->conf->dicfile_list;
	while (dicfile_node != NULL) {
		if (dicfile_node->data != NULL) {
			dicfile = dicfile_node->data;
			menu_dictfiles_item = gtk_menu_item_new_with_label(dicfile->name);
			gtk_menu_shell_append(GTK_MENU_SHELL(wordDic->menu_selectdic), menu_dictfiles_item);
			g_signal_connect_swapped(G_OBJECT(menu_dictfiles_item), "activate", 
															 G_CALLBACK(on_dicselection_clicked), (gpointer) dicfile);
			gtk_widget_show(menu_dictfiles_item);
		}
		dicfile_node = g_slist_next(dicfile_node);
	}
  gtk_widget_show(wordDic->dicselection_menu);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(wordDic->dicselection_menu), wordDic->menu_selectdic);
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
    gtk_widget_modify_font(GTK_COMBO(wordDic->combo_entry)->entry, gjitenApp->conf->normalfont_desc);
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
	gdk_window_get_pointer(text_view->window, NULL, NULL, NULL);

  return FALSE;
}



static gboolean kanji_clicked(GtkWidget *text_view, GdkEventButton *event, gpointer user_data) {
	GtkTextIter mouse_iter;
	gint x, y;
	gint trailing;
	gunichar kanji;

  if (event->button != 1) return FALSE;

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
  GtkWidget *dock_main;
  GtkWidget *toolbar;
  GtkWidget *button_exit;
  //  GtkWidget *button_pref;
  GtkWidget *button_kanjipad;
  GtkWidget *button_kanjidic;
  GtkWidget *button_clear;
  GtkWidget *button_srch;
  //  GtkWidget *button_copy;
  //  GtkWidget *button_paste;
  GtkWidget *frame_japopt;
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
  GtkObject *spinb_searchlimit_adj;
  GtkWidget *tmpimage;
  GdkPixbuf *cursor_pixbuf;

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

  wordDic->window = gnome_app_new("gjiten", _("Gjiten - WordDic"));
  GTK_WIDGET_SET_FLAGS(wordDic->window, GTK_CAN_DEFAULT);
  g_signal_connect(G_OBJECT(wordDic->window), "destroy", G_CALLBACK(worddic_close), NULL);
  gtk_window_set_default_size(GTK_WINDOW(wordDic->window), 500, 500);

  dock_main = GNOME_APP(wordDic->window)->dock;
  gtk_widget_show(dock_main);

  if (gjitenApp->conf->menubar) gnome_app_create_menus(GNOME_APP(wordDic->window), menubar_uiinfo);

  if (gjitenApp->conf->toolbar) {
    toolbar = gtk_toolbar_new();
    gtk_widget_show(toolbar);

    gnome_app_set_toolbar(GNOME_APP(wordDic->window), GTK_TOOLBAR(toolbar));
    
    button_exit = gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_CLOSE,
																					 _("Close Gjiten"), "Close", NULL, NULL, -1);
		g_signal_connect_swapped(G_OBJECT(button_exit), "clicked", 
														 G_CALLBACK(gtk_widget_destroy), wordDic->window);

    wordDic->button_back = gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_GO_BACK,
																										_("Previous search result"), "Back", 
																										on_back_clicked, NULL, -1);
    gtk_widget_set_sensitive(wordDic->button_back, FALSE);

    wordDic->button_forward = gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_GO_FORWARD,
																											 _("Next search result"), "Forward", 
																											 on_forward_clicked, NULL, -1);
    gtk_widget_set_sensitive(wordDic->button_forward, FALSE);


    tmpimage = gtk_image_new_from_file(PIXMAPDIR"/kanjidic.png");
    button_kanjidic = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("KanjiDic"),
																							_("Launch KanjiDic"), "KanjiDic", tmpimage,
																							G_CALLBACK(kanjidic_create), NULL);

    tmpimage = gtk_image_new_from_file(PIXMAPDIR"/kanjipad.png");
    button_kanjipad = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("KanjiPad"),
																							_("Launch KanjiPad"), "KanjiPad", tmpimage,
																							G_CALLBACK(gjiten_start_kanjipad), NULL);

    button_srch = gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_FIND,
																					 _("Search for entered expression"), "Search", 
																					 on_text_entered, NULL, -1);

		/*
		gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), GTK_STOCK_NO, 
														 _("Show/Hide options"), "Show/Hide options",
														 G_CALLBACK(worddic_show_hide_options), NULL, -1);
		*/
		gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), _("Show/Hide\noptions"),
														_("Show/Hide options"), "Show/Hide options", NULL,
														G_CALLBACK(worddic_show_hide_options), NULL);

    /*
    button_srch = gtk_toolbar_insert_item(GTK_TOOLBAR(toolbar), _("Search"), "Search", "Search", 
                                             GtkWidget *icon,
					  on_text_entered, NULL, -1);
    */
    /*

    tmp_toolbar_icon = gnome_stock_pixmap_widget(wordDic->window, GNOME_STOCK_PIXMAP_COPY);
    button_copy = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
					      GTK_TOOLBAR_CHILD_BUTTON,  NULL, _("Copy"),
					      NULL, NULL, tmp_toolbar_icon, NULL, NULL);
    gtk_widget_show(button_copy);
    g_signal_connect(G_OBJECT(button_copy), "clicked",G_CALLBACK(worddic_copy), NULL);
  
    tmp_toolbar_icon = gnome_stock_pixmap_widget(wordDic->window, GNOME_STOCK_PIXMAP_PASTE);
    button_paste = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
					       GTK_TOOLBAR_CHILD_BUTTON, NULL,_("Paste"),
					       NULL, NULL, tmp_toolbar_icon, NULL, NULL);
    gtk_widget_show(button_paste);
    g_signal_connect(G_OBJECT(button_paste), "clicked",G_CALLBACK(worddic_paste), NULL);

    */   
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);

  }    

  vbox_main = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_main);
  gnome_app_set_contents(GNOME_APP(wordDic->window), vbox_main);

  wordDic->hbox_options = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(wordDic->hbox_options);
  gtk_box_pack_start(GTK_BOX(vbox_main), wordDic->hbox_options, FALSE, TRUE, 0);

  frame_japopt = gtk_frame_new(_("Japanese Search Options: "));
  gtk_widget_show(frame_japopt);
  gtk_box_pack_start(GTK_BOX(wordDic->hbox_options), frame_japopt, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame_japopt), 5);

  vbox_japopt = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_japopt);
  gtk_container_add(GTK_CONTAINER(frame_japopt), vbox_japopt);

  wordDic->radiob_jpexact = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("E_xact Matches"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_jpexact));
  gtk_widget_show(wordDic->radiob_jpexact);
  gtk_box_pack_start(GTK_BOX(vbox_japopt), wordDic->radiob_jpexact, FALSE, FALSE, 0);

  wordDic->radiob_startw = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("_Start With Expression"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_startw));
  gtk_widget_show(wordDic->radiob_startw);
  gtk_box_pack_start(GTK_BOX(vbox_japopt), wordDic->radiob_startw, FALSE, FALSE, 0);

  wordDic->radiob_endw = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("E_nd With Expression"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_endw));
  gtk_widget_show(wordDic->radiob_endw);
  gtk_box_pack_start(GTK_BOX(vbox_japopt), wordDic->radiob_endw, FALSE, FALSE, 0);

  wordDic->radiob_any = gtk_radio_button_new_with_mnemonic(vbox_japopt_group, _("_Any Matches"));
  vbox_japopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_any));
  gtk_widget_show(wordDic->radiob_any);
  gtk_box_pack_start(GTK_BOX(vbox_japopt), wordDic->radiob_any, FALSE, FALSE, 0);

  frame_engopt = gtk_frame_new(_("English Search Options: "));
  gtk_widget_show(frame_engopt);
  gtk_box_pack_start(GTK_BOX(wordDic->hbox_options), frame_engopt, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame_engopt), 5);

  vbox_engopt = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_engopt);
  gtk_container_add(GTK_CONTAINER(frame_engopt), vbox_engopt);

  wordDic->radiob_engexact = gtk_radio_button_new_with_mnemonic(vbox_engopt_group, _("Wh_ole Expressions"));
  vbox_engopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_engexact));
  gtk_widget_show(wordDic->radiob_engexact);
  gtk_box_pack_start(GTK_BOX(vbox_engopt), wordDic->radiob_engexact, FALSE, FALSE, 0);

  wordDic->radiob_words = gtk_radio_button_new_with_mnemonic(vbox_engopt_group, _("_Whole Words"));
  vbox_engopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_words));
  gtk_widget_show(wordDic->radiob_words);
  gtk_box_pack_start(GTK_BOX(vbox_engopt), wordDic->radiob_words, FALSE, FALSE, 0);

  wordDic->radiob_partial = gtk_radio_button_new_with_mnemonic(vbox_engopt_group, _("Any _Matches"));
  vbox_engopt_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_partial));
  gtk_widget_show(wordDic->radiob_partial);
  gtk_box_pack_start(GTK_BOX(vbox_engopt), wordDic->radiob_partial, FALSE, FALSE, 0);

  frame_gopt = gtk_frame_new(_("General Options: "));
  gtk_widget_show(frame_gopt);
  gtk_box_pack_start(GTK_BOX(wordDic->hbox_options), frame_gopt, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame_gopt), 5);

  table_gopt = gtk_table_new(3, 2, FALSE);
  gtk_widget_show(table_gopt);
  gtk_container_add(GTK_CONTAINER(frame_gopt), table_gopt);

  wordDic->radiob_searchdic = gtk_radio_button_new_with_mnemonic(dicssearch_group, _("Search _Dic:"));
  dicssearch_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(wordDic->radiob_searchdic));
  gtk_widget_show(wordDic->radiob_searchdic);
  gtk_table_attach(GTK_TABLE(table_gopt), wordDic->radiob_searchdic, 0, 1, 0, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 
									 (GtkAttachOptions)(0), 0, 0);
  g_signal_connect_swapped(G_OBJECT(wordDic->radiob_searchdic), "clicked", 
													 G_CALLBACK(shade_worddic_widgets), NULL);

  // DICTFILE SELECTION MENU
	
  wordDic->dicselection_menu = gtk_option_menu_new();
	worddic_update_dic_menu();
 
  gtk_table_attach(GTK_TABLE(table_gopt), wordDic->dicselection_menu, 1, 2, 0, 1,
                   (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
									 (GtkAttachOptions)(0), 0, 0);

  wordDic->radiob_searchall = gtk_radio_button_new_with_mnemonic(dicssearch_group, _("Sea_rch All Dictionaries"));
  gtk_widget_show(wordDic->radiob_searchall);
  gtk_table_attach(GTK_TABLE(table_gopt), wordDic->radiob_searchall, 0, 2, 1, 2,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 
									 (GtkAttachOptions)(0), 0, 0);
  g_signal_connect_swapped(G_OBJECT(wordDic->radiob_searchall), "clicked", 
													 G_CALLBACK(shade_worddic_widgets), NULL);


  wordDic->checkb_autoadjust = gtk_check_button_new_with_mnemonic(_("A_uto Adjust Options"));
  gtk_widget_show(wordDic->checkb_autoadjust);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust), TRUE);
  gtk_table_attach(GTK_TABLE(table_gopt), wordDic->checkb_autoadjust, 0, 2, 2, 3,
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 
									 (GtkAttachOptions)(0), 0, 0);
  g_signal_connect(G_OBJECT(wordDic->checkb_autoadjust), "toggled", 
									 G_CALLBACK(shade_worddic_widgets), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->checkb_autoadjust), gjitenApp->conf->autoadjust_enabled);

  
  hbox_searchlimit = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox_searchlimit);
  gtk_table_attach(GTK_TABLE(table_gopt), hbox_searchlimit, 0, 2, 3, 4,
                   (GtkAttachOptions)(GTK_FILL), 
									 (GtkAttachOptions)(0), 0, 0);
  wordDic->checkb_searchlimit = gtk_check_button_new_with_mnemonic(_("_Limit Results:"));
  gtk_widget_show(wordDic->checkb_searchlimit);
  gtk_box_pack_start(GTK_BOX(hbox_searchlimit), wordDic->checkb_searchlimit, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(wordDic->checkb_searchlimit), "toggled", 
									 G_CALLBACK(checkb_searchlimit_toggled), NULL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wordDic->checkb_searchlimit), gjitenApp->conf->searchlimit_enabled);

	spinb_searchlimit_adj = gtk_adjustment_new(gjitenApp->conf->maxwordmatches, 1, G_MAXFLOAT, 1, 2, 2);
  wordDic->spinb_searchlimit = gtk_spin_button_new(GTK_ADJUSTMENT(spinb_searchlimit_adj), 1, 0);
  gtk_widget_show(wordDic->spinb_searchlimit);
  gtk_box_pack_start(GTK_BOX(hbox_searchlimit), wordDic->spinb_searchlimit, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(wordDic->spinb_searchlimit, (GTK_TOGGLE_BUTTON(wordDic->checkb_searchlimit)->active));
  g_signal_connect(G_OBJECT(spinb_searchlimit_adj), "value_changed", 
									 G_CALLBACK(get_searchlimit), NULL);
  

  hbox_entry = gtk_hbox_new(FALSE, 0);   
  gtk_widget_show(hbox_entry);
  gtk_box_pack_start(GTK_BOX(vbox_main), hbox_entry, FALSE, TRUE, 14);
  gtk_container_set_border_width(GTK_CONTAINER(hbox_entry), 3);

  label_enter = gtk_label_new(_("Enter expression :"));
  gtk_widget_show(label_enter);
  gtk_box_pack_start(GTK_BOX(hbox_entry), label_enter, FALSE, TRUE, 5);
  gtk_label_set_justify(GTK_LABEL(label_enter), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment(GTK_MISC(label_enter), 1, 0.5);
  gtk_misc_set_padding(GTK_MISC(label_enter), 7, 0);

  wordDic->combo_entry = gtk_combo_new();
  gtk_widget_show(wordDic->combo_entry);
  gtk_box_pack_start(GTK_BOX(hbox_entry), wordDic->combo_entry, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(GTK_COMBO(wordDic->combo_entry)->entry), 
									 "activate", G_CALLBACK(on_text_entered), NULL);
  g_signal_connect(G_OBJECT(wordDic->window), "key_press_event",
									 G_CALLBACK(set_focus_on_entry), GTK_COMBO(wordDic->combo_entry)->entry);

  gtk_combo_disable_activate(GTK_COMBO(wordDic->combo_entry));
  gtk_combo_set_case_sensitive(GTK_COMBO(wordDic->combo_entry), TRUE);
  if (wordDic->combo_entry_glist != NULL) {
    gtk_combo_set_popdown_strings(GTK_COMBO(wordDic->combo_entry), wordDic->combo_entry_glist);
    gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wordDic->combo_entry)->entry), "");
  }
  GTK_WIDGET_SET_FLAGS(GTK_COMBO(wordDic->combo_entry)->entry, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus(GTK_COMBO(wordDic->combo_entry)->entry);
  gtk_widget_grab_default(GTK_COMBO(wordDic->combo_entry)->entry);
  

  button_search = gtk_button_new_with_label(_("Search"));
  gtk_widget_show(button_search);
  gtk_box_pack_start(GTK_BOX(hbox_entry), button_search, FALSE, FALSE, 7);
  g_signal_connect(G_OBJECT(button_search), "clicked", G_CALLBACK(on_text_entered), NULL);

  button_clear = gtk_button_new_with_mnemonic(_("_Clear"));
  gtk_widget_show(button_clear);
  gtk_box_pack_start(GTK_BOX(hbox_entry), button_clear, FALSE, FALSE, 0);
  g_signal_connect_swapped(G_OBJECT(button_clear), "clicked", 
													 G_CALLBACK(gjiten_clear_entry_box), 
													 G_OBJECT(GTK_COMBO(wordDic->combo_entry)->entry));

  frame_results = gtk_frame_new(_("Search results :"));
  gtk_widget_show(frame_results);
  gtk_box_pack_start(GTK_BOX(vbox_main), frame_results, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame_results), 5);
  gtk_frame_set_label_align(GTK_FRAME(frame_results), 0.03, 0.5);

  vbox_results = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_results);
  gtk_container_add(GTK_CONTAINER(frame_results), vbox_results);

  wordDic->text_results_view = gtk_text_view_new();
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
  gtk_box_pack_start(GTK_BOX(vbox_results), scrolledwin_results, TRUE, TRUE, 0);
  gtk_widget_show(scrolledwin_results);

  wordDic->appbar_mainwin = gnome_appbar_new(TRUE, TRUE, GNOME_PREFERENCES_NEVER);
  gtk_widget_show(wordDic->appbar_mainwin);
  gtk_box_pack_end(GTK_BOX(vbox_results), wordDic->appbar_mainwin, FALSE, FALSE, 0);
 
  gtk_widget_show(wordDic->window);

  gjiten_flush_errors();

	return wordDic;
}
