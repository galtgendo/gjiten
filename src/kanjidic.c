/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* kanjidic.c

   GJITEN : A GTK+/GNOME BASED JAPANESE DICTIONARY
  
   Copyright (C) 1999-2003 Botond Botyanszki <boti@rocketmail.com>
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
#include <gnome.h>
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

/*====== Prototypes========================================================*/
void kanji_selected(gunichar *kanji);

int get_radical_index(gunichar radical);
void get_rad_of_kanji(gunichar kanji);

static void kanjidic_close();

/* VARIABLES ************************/
gchar *kdic_line = NULL;  /*size = KCFGNUM * 200 */
gchar kanjiselected[2];
gchar *radkfile = NULL;
guint32 radkfile_size;
extern guint32 srchpos;

struct knode {
  gunichar kanji;
  struct knode *next;
} *klinklist, *tmpklinklist;

struct radical_info {
  gunichar radical;
  int strokes;
} radicals[300];

gunichar *radical_kanji = NULL;
int total_radicals, radical_kanji_start[300], radical_kanji_count[300];

KanjiDic *kanjiDic;
extern gchar *strginfo[];
extern GjitenApp *gjitenApp;

static GnomeUIInfo kfile_menu_uiinfo[] = {
  GNOMEUIINFO_MENU_EXIT_ITEM(GTK_SIGNAL_FUNC(kanjidic_close), NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo kedit_menu_uiinfo[] = {
  //GNOMEUIINFO_MENU_CUT_ITEM(NULL, NULL),
  //GNOMEUIINFO_MENU_COPY_ITEM(gjiten_copy, NULL),
  //GNOMEUIINFO_MENU_PASTE_ITEM(gjiten_paste, NULL),
  //  GNOMEUIINFO_MENU_CLEAR_ITEM(NULL, NULL),
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_MENU_PREFERENCES_ITEM(create_dialog_preferences, NULL),
  GNOMEUIINFO_END
};

static GnomeUIInfo ktools_menu_uiinfo[] = {
  {
    GNOME_APP_UI_ITEM, N_("KanjiPad"), NULL, gjiten_start_kanjipad, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL
  },
  {
    GNOME_APP_UI_ITEM, N_("WordDic"), NULL, worddic_create, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL
  },
  GNOMEUIINFO_END
};

static GnomeUIInfo khelp_menu_uiinfo[] = {
	GNOMEUIINFO_HELP("gjiten"),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_MENU_ABOUT_ITEM(gjiten_create_about, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo kmenubar_uiinfo[] = {
  GNOMEUIINFO_MENU_FILE_TREE(kfile_menu_uiinfo),
  GNOMEUIINFO_MENU_EDIT_TREE(kedit_menu_uiinfo),
  {
    GNOME_APP_UI_SUBTREE, N_("Tools"), NULL, ktools_menu_uiinfo, NULL, 
    NULL,  GNOME_APP_PIXMAP_NONE, N_("Tools"), 0, 0, NULL
  },
  GNOMEUIINFO_MENU_HELP_TREE(khelp_menu_uiinfo),
  GNOMEUIINFO_END
};


/* ************************************************************ */
void do_kdicline(gchar *kstr) {
  char tmpstr[200];
  int i, pos;

  //printf("%s\n", kstr);

  for (i = 0; i < KCFGNUM * 200; i++) { //clear it
    kdic_line[i] = 0;
  }

  
  g_unichar_to_utf8(g_utf8_get_char(kstr), &kdic_line[KANJI * 200] ); //KANJI
  get_rad_of_kanji(g_utf8_get_char(kdic_line + KANJI * 200)); //RADICAL
  
  get_word(kstr, kdic_line+JIS*200,3);
  pos = 7;
  //printf("%s\n",kstr);
  while (pos != 0){
    pos = get_word(kstr,tmpstr,pos);
    if ((tmpstr[0] >> 7)) {       // jap char   //FIXME
     	if (strlen(kdic_line+READING*200) != 0) strcat(kdic_line+READING*200,", ");
	strcat(kdic_line+READING*200,tmpstr);
    }
    else switch (tmpstr[0])  {
    case '-' : {
      if (strlen(kdic_line+READING*200) != 0) strcat(kdic_line+READING*200,", ");
      strcat(kdic_line+READING*200,tmpstr);
      break;
    }
    case 'T': {
      if (tmpstr[1] == '1') {
	if (strlen(kdic_line+READING*200) != 0) 
	  {
	    strcat(kdic_line+READING*200,", ");
	    strcat(kdic_line+READING*200,_("Name readings:"));
	  }
	
	else strcat(kdic_line+READING*200,_("Name readings:"));
	pos = get_word(kstr,tmpstr,pos);
	strcat(kdic_line+READING*200,tmpstr);
	break;
      }
      if (tmpstr[1] == '2') {
	if (strlen(kdic_line+READING*200) != 0) strcat(kdic_line+READING*200,", Radical Name: ");
	else strcat(kdic_line+READING*200,_("Radical name:"));
	pos = get_word(kstr,tmpstr,pos);
	strcat(kdic_line+READING*200,tmpstr);
	break;
	}
    }
    case '{': { // english meaning
      if (strlen(kdic_line+ENGLISH*200) != 0 ) strcat(kdic_line+ENGLISH*200," ");
      strcat(kdic_line+ENGLISH*200,tmpstr+1);
      strcat(kdic_line+ENGLISH*200,";"); // put endmark: ;
	break;
      }
    case 'B' : {
      strcpy(kdic_line+BUSHU*200,tmpstr+1);
      break;
    }
    case 'C' : {
      strcpy(kdic_line+CLASSIC*200,tmpstr+1);
      break;
    }
    case 'F' : {
      strcpy(kdic_line+FREQ*200,tmpstr+1);
      break;
    }
    case 'G' : {
      strcpy(kdic_line+JOUYOU*200,tmpstr+1);
      break;
    }
    case 'H' : {
      strcpy(kdic_line+HINDEX*200,tmpstr+1);
      break;
      }
    case 'N' : {
      strcpy(kdic_line+NINDEX*200,tmpstr+1);
      break;
    }
    case 'V' : {
      strcpy(kdic_line+VINDEX*200,tmpstr+1);
      break;
    }
    case 'D' : {
      strcpy(kdic_line+DEROO*200,tmpstr+1);
      break;
    }
    case 'P' : {
      strcpy(kdic_line+SKIP*200,tmpstr+1);
      break;
    }
    case 'S' : {
      if (strlen(kdic_line+STROKES*200) == 0)	strcpy(kdic_line+STROKES*200,tmpstr+1);
      else { 
				strcat(kdic_line+STROKES*200,_(", Common miscount: "));
				strcat(kdic_line+STROKES*200,tmpstr+1);
			}
      break;
    }
    case 'U' : {
      strcpy(kdic_line+UNI*200,tmpstr+1);
      break;
    }
    case 'I' : {
      strcpy(kdic_line+IINDEX*200,tmpstr+1);
      break;
      }
    case 'Q' : {
      strcpy(kdic_line+FOURC*200,tmpstr+1);
      break;
    }
    case 'M' : {
      if (tmpstr[1] == 'N') strcpy(kdic_line+MNINDEX*200,tmpstr+2);
      else if (tmpstr[1] == 'P') strcpy(kdic_line+MPINDEX,tmpstr+2);
      break;
      }
    case 'E' : {
      strcpy(kdic_line+EINDEX*200,tmpstr+1);
      break;
    }
    case 'K' : {
      strcpy(kdic_line+KINDEX*200,tmpstr+1);
      break;
    }
    case 'L' : {
      strcpy(kdic_line+LINDEX*200,tmpstr+1);
      break;
    }
    case 'O' : {
      strcpy(kdic_line+OINDEX*200,tmpstr+1);
      break;
    }
    case 'W' : {
      strcpy(kdic_line+KOREAN*200,tmpstr+1);
      break;
    }
    case 'Y' : {
      strcpy(kdic_line+PINYIN*200,tmpstr+1);
      break;
    }
    case 'X' : {
      strcpy(kdic_line+CREF*200,tmpstr+1);
      break;
    }
    case 'Z' : 
      strcpy(kdic_line+MISSC*200,tmpstr+1);
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
																							 &kanjiDic->kinfo_iter, strginfo[i], -1, "blue_foreground", NULL);
	    gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, 
																							 ": ", -1, "blue_foreground", NULL);
	    if (i == KANJI) {
		    if (gjitenApp->conf->bigkanji == FALSE) {
			    gtk_text_buffer_insert(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, kdic_line + i * 200, -1); 
		    }
		    else {
			    gtk_text_buffer_insert_with_tags_by_name(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), 
																									 &kanjiDic->kinfo_iter, kdic_line + i * 200, -1, "largefont", NULL);
		    }
	    }
	    else {
		    gtk_text_buffer_insert(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, kdic_line + i * 200, -1);
	    }
	    gtk_text_buffer_insert(GTK_TEXT_BUFFER(kanjiDic->text_kanjinfo_buffer), &kanjiDic->kinfo_iter, "\n", -1);
    }
}
  

/*
//this is only for debugging 
void klist_print(struct knode *list) { 
  gchar kanji[6];
  int i;
  struct knode *nodeptr;
  
  i = 0;
  printf("Elements in list:\n");
  nodeptr = list;
  while (nodeptr != NULL) {
    g_unichar_to_utf8(*(nodeptr->kanji), kanji);
    printf("%s ", kanji);
    nodeptr = nodeptr->next;
    i++;
  }
  printf("\n----TOTAL: %d-----\n", i);
}
*/


//
// FIXME: convert this to g_slist
//
void knode_add(struct knode **llist, gunichar kanji) { 
/* adds a node to the linked list with a kanji. */

  struct knode *currnode = NULL, *prevnode = NULL;
  
  currnode = *llist;
  while (currnode != NULL) { 
    if ((currnode->kanji) == kanji) return;
    prevnode = currnode;
    currnode = prevnode->next;
  }
  currnode = g_malloc(sizeof(struct knode));
  if (currnode == NULL) gjiten_abort_with_msg("Malloc failed (in knode_add).\n");
  if (*llist == NULL) *llist = currnode;
  else prevnode->next = currnode;
  currnode->kanji = kanji;
  currnode->next = NULL;
}

void klist_free(struct knode **llist) { // frees up a whole llist 
  struct knode *ptr, *nextptr;

  ptr = *llist;
  if (ptr == NULL) return;
  while (ptr != NULL) {
    nextptr = ptr->next;
    g_free(ptr);
    ptr = nextptr;
  }
  *llist = NULL;
}

void knode_remove(struct knode **llist, struct knode *node2rem)  {
  struct knode *currnode, *prevnode;

  currnode = *llist;
  prevnode = NULL;
  
  while ((currnode != node2rem) && (currnode != NULL)) {
    prevnode = currnode;
    currnode = currnode->next;
  }
  if (currnode == NULL) {
    g_warning("couldn't remove knode at %d\n", (int)node2rem);
    return;
  }
  if (prevnode != NULL) prevnode->next = currnode->next;
  else *llist = currnode->next;
  //  printf("removing: %s at %d\n",currnode->kanji,currnode);
  g_free(node2rem);
}

// compares two lists and combines the matching kanji into one
void klists_merge(struct knode **llist1, struct knode **llist2) {
  struct knode *ptr1, *ptr2, *nextptr;
  int found;

  ptr1 = *llist1;

  while (ptr1 != NULL) {
    nextptr = ptr1->next;
    found = FALSE;
    ptr2 = *llist2;
    while (ptr2 != NULL) {
      if (ptr1->kanji == ptr2->kanji) {
	found = TRUE;
	break;
      }
      ptr2 = ptr2->next;
    }
    if (found == FALSE) knode_remove(llist1, ptr1);
    ptr1 = nextptr;
  }
  klist_free(llist2); 
}

int count_kanji(struct knode *list) { //count the nodes of llist
  int kanjnum = 0;
  struct knode *nodeptr;
  
  nodeptr = list;
  while (nodeptr != NULL) {
    kanjnum++;
    nodeptr = nodeptr->next;
  }
  return(kanjnum);
}

void findk_by_key(gchar *srchkey, struct knode **llist)  {
  gint srch_resp, roff, rlen;
  gchar repstr[1024];
  guint32 respos, oldrespos; 
  gint loopnum = 0;

  srchpos = 0;
  srch_resp = search4string(SRCH_START, gjitenApp->conf->kanjidic, srchkey, &respos, &roff, &rlen, repstr);
  //printf("F: Returning:srch_resp:%d\n respos:%ld\n roff:%d rlen:%d\n repstr:%s\n", srch_resp,respos,roff,rlen,repstr); 
  if (srch_resp != SRCH_OK) return; // (FALSE);
  oldrespos = srchpos = respos;
  knode_add(llist, g_utf8_get_char(repstr));
  
  while (roff != 0) {
    oldrespos = respos;
    srchpos++;
    loopnum++;
		srch_resp = search4string(SRCH_CONT, gjitenApp->conf->kanjidic, srchkey, &respos, &roff, &rlen, repstr);
    //printf("srch_resp:%d\n respos:%ld\n roff:%d rlen:%d\n repstr:%s\n",srch_resp,respos,roff,rlen,repstr);
    if (srch_resp != SRCH_OK) break;
    if (oldrespos == respos) continue;
    knode_add(llist, g_utf8_get_char(repstr));
  }
}

void findk_by_stroke(int stroke, int plusmin, struct knode **llist) {
  static char srchkey[10];
  int i, lowerlim, upperlim;
 
  upperlim = stroke + plusmin;
  if (upperlim > 30) upperlim = 30;
  lowerlim = stroke - plusmin;
  if (lowerlim < 1) lowerlim = 1;
 
  for (i = lowerlim; i <= upperlim ; i++) {
    sprintf(srchkey,"S%d ", i);
    findk_by_key(srchkey, llist);
  }
}

void findk_by_radical(gchar *radstrg) { 
  int i, rad_index, kanji_index, radnum;
  gchar *radstr_ptr;

  if (g_utf8_strlen(radstrg, -1) == 0) return;
  /*
  if (g_utf8_strlen(radstrg) <= 1) { 
    sprintf(tmpmsg,_("I don't seem to recognize this radical: '%s' !!!\n"),radstrg);
    gnome_warning_dialog_parented(tmpmsg,GTK_WINDOW(kanjiDic->window));
    return;
  }
  */

  rad_index = get_radical_index(g_utf8_get_char(radstrg));
  if (rad_index < 0) {
    gjiten_print_error(_("Invalid radical!\n"));
    return;
  }
  for (kanji_index = radical_kanji_start[rad_index]; 
       kanji_index < radical_kanji_start[rad_index] + radical_kanji_count[rad_index]; 
       kanji_index++) {
    knode_add(&klinklist, radical_kanji[kanji_index]);
  }
  // if we have more than 1 radical 
  radnum = g_utf8_strlen(radstrg, -1); 
  if (radnum > 1) {
    radstr_ptr = radstrg;
    for (i = 1; i <= radnum; i++) {
      rad_index = get_radical_index(g_utf8_get_char(radstr_ptr));
      if (rad_index < 0) {
				gjiten_print_error(_("I don't seem to recognize this radical: '%s' !!!\n"),
													 g_strndup(radstr_ptr, sizeof(gunichar)));
				return;
      }
      for (kanji_index = radical_kanji_start[rad_index]; 
					 kanji_index < radical_kanji_start[rad_index] + radical_kanji_count[rad_index]; 
					 kanji_index++) {
				knode_add(&tmpklinklist, radical_kanji[kanji_index]);
      }
      klists_merge(&klinklist, &tmpklinklist);
      radstr_ptr = g_utf8_next_char(radstr_ptr);
    }
  }
}


void on_kanji_search() {
  static gchar *kentry, *radentry;
  int found;
  int i, j;
  int stroke, plus_min;
  struct knode *node_ptr;
  gchar kappbarmsg[100];
  int push;
  GtkWidget *kanji_result_button;
  gchar kanji_result_str[6];
  gchar kanji_result_labelstr[100];
  GtkWidget *kanji_result_label;
	GtkTextChildAnchor *kanji_results_anchor;

  gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji),_("Searching..."));
  kappbarmsg[0] = 0;

  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(kanjiDic->kanji_results_buffer), "", 0);
  gtk_text_buffer_get_start_iter(kanjiDic->kanji_results_buffer, &kanjiDic->kanji_results_iter);

  push = TRUE;
  if (kentry != NULL) //Check if we need to save the key entry in the history
    if (strcmp(kentry, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(kanjiDic->combo_entry_key)->entry))) == 0) {  
      push = FALSE;
      g_free(kentry);
    }
  kentry = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(kanjiDic->combo_entry_key)->entry)));
  //printf("KEY:%s\n", kentry);
  if (kentry != NULL)
    if ((strlen(kentry) > 0) && (push == TRUE) ) {
      kanjiDic->combo_entry_key_glist = g_list_prepend(kanjiDic->combo_entry_key_glist, kentry);
      gtk_combo_set_popdown_strings(GTK_COMBO(kanjiDic->combo_entry_key),kanjiDic->combo_entry_key_glist);
    }
  push = TRUE;
  if (radentry != NULL) //Check if we need to save the radical entry in the history
    if (strcmp(radentry, gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(kanjiDic->combo_entry_radical)->entry))) == 0) {  
	push = FALSE;
	g_free(radentry);
      }
  
  radentry = g_strdup(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(kanjiDic->combo_entry_radical)->entry)));
  if (radentry) {
    if ((strlen(radentry) > 0) && push) {
      kanjiDic->combo_entry_radical_glist = g_list_prepend(kanjiDic->combo_entry_radical_glist, radentry);
      gtk_combo_set_popdown_strings(GTK_COMBO(kanjiDic->combo_entry_radical),
				    kanjiDic->combo_entry_radical_glist);
    }
  }

  stroke = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(kanjiDic->spinb_strokenum));
  plus_min = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(kanjiDic->spinb_plusmin));

  if (klinklist != NULL) klist_free(&klinklist);
  tmpklinklist = NULL;
  klinklist = NULL;
  found = TRUE;

  //FIND BY RADICAL
  if ((GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)->active) && (g_utf8_strlen(radentry, -1) > 0)) {
    findk_by_radical(radentry); 
    if (klinklist == NULL) {
      gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji),_("No such kanji with this radical combination."));
      return;
    }
  }
  //FIND BY STROKE
  if (GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)->active) {
    if ((stroke < 1) || (stroke > 30)) {
      	gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji),
				_("Invalid stroke count :-P "));
	return;
    }
    if (klinklist == NULL) {
      findk_by_stroke(stroke, plus_min, &klinklist);  // this should! give results
      if (klinklist == NULL) {
	gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji),
				_("Stroke search didn't find any match :-O "));
	return;
      }
    }
    else {
      findk_by_stroke(stroke, plus_min, &tmpklinklist);
      klists_merge(&klinklist,&tmpklinklist); 
      if (klinklist == NULL) {  
	found = FALSE; 
	gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji),
				_("No such kanji with this stroke/radical combination.")); 
	return;
      }
    }
  }
  //FIND BY KEY
  if ((found) && (GTK_TOGGLE_BUTTON(kanjiDic->checkb_ksearch)->active) && (strlen(kentry) >= 1)) {
    if (klinklist == NULL) findk_by_key(kentry, &klinklist);
    else {
      findk_by_key(kentry, &tmpklinklist);
      klists_merge(&klinklist,&tmpklinklist);
    }
    if (klinklist == NULL) {    
      gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji), _("No Matches found!"));
      return;
    }
  }
  
  sprintf(kappbarmsg,_("Kanji found: %d"),count_kanji(klinklist));
  gnome_appbar_set_status(GNOME_APPBAR(kanjiDic->appbar_kanji),kappbarmsg);

  if (count_kanji(klinklist) == 1) print_kanjinfo(klinklist->kanji);
  

  // PRINT OUT KANJI FOUND
  node_ptr = klinklist;
  i = 0;
  while (node_ptr != NULL) { 
    for (j = 0; j < 5; j++) kanji_result_str[j] = 0;
    g_unichar_to_utf8(node_ptr->kanji, kanji_result_str);
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
			     (gpointer) &(node_ptr->kanji));
    gtk_text_view_add_child_at_anchor(GTK_TEXT_VIEW(kanjiDic->kanji_results_view), kanji_result_button, kanji_results_anchor);
    node_ptr = node_ptr->next;
  }
}


int radical_selected(gunichar *radical) {
  int i, j;
  gchar radical_selected[6];
  gchar tmpchar[6];
  gchar *radline_ptr, *newradline;
  int removed;
  int radline_length = 0;

  for (j = 0; j < 6; j++) radical_selected[j] = 0; 
  g_unichar_to_utf8(*radical, radical_selected);

  radline_ptr = (gchar*) gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(kanjiDic->combo_entry_radical)->entry));
  newradline = g_strndup(radline_ptr, strlen(radline_ptr) + 6); //Enough space for one more character
  radline_length = g_utf8_strlen(newradline, -1);

  for (i = 0; i < strlen(newradline) + 6; i++) newradline[i] = 0; //clear newradline

  removed = FALSE;
  for (i = 0; i < radline_length; i++) {  //Check if we already have the radical in the line
    if (g_utf8_get_char(radline_ptr) != *radical) {
      for (j = 0; j < 6; j++) tmpchar[j] = 0; 
      g_unichar_to_utf8(g_utf8_get_char(radline_ptr), tmpchar);
      strcat(newradline, tmpchar);
    }
    else removed = TRUE;  //if it's there then remove it
    radline_ptr = g_utf8_next_char(radline_ptr);
  }
  
  if (removed == FALSE) strcat(newradline, radical_selected); //Add the radical to the line
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(kanjiDic->combo_entry_radical)->entry), newradline);

  g_free(newradline);
  on_kanji_search();
  
  return 0;
}

void history_add(gunichar unicharkanji) {
  //GSList *tmp_list_ptr = NULL;
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

  //strcpy(last_kanji, kanji);
  
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
			   (gpointer) (unichar_list_elem));
  gtk_box_pack_start(GTK_BOX(kanjiDic->vbox_history), history_kanji_button, FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(kanjiDic->vbox_history), history_kanji_button, 0);
  gtk_widget_show(history_kanji_button);

}


void kanji_selected(gunichar* kanji) {
  print_kanjinfo(*kanji);
  history_add(*kanji);
  //printf("KANJI_SELECTED\n");
}

static void radical_window_close() {
	if (GTK_IS_WIDGET(kanjiDic->window_radicals) == TRUE) {
		gtk_widget_destroy(kanjiDic->window_radicals);
		kanjiDic->window_radicals = NULL;
	}
}


int get_radical_index(gunichar radical) {
  int rad_index;
  
  for (rad_index = 0; rad_index < total_radicals + 1; rad_index++) {
    if (radicals[rad_index].radical == radical) {
      return rad_index; //we found it
    }
  }
  return -1; //didn't find it
}

//get all radicals that the kanji has
void get_rad_of_kanji(gunichar kanji) {
  int rad_index, kanji_index, i;
  gchar *kdicline_ptr;
  
  kdicline_ptr = kdic_line + RADICAL * 200;
  for (rad_index = 0; rad_index < total_radicals; rad_index++) {
    for (kanji_index = radical_kanji_start[rad_index]; 
	 kanji_index < radical_kanji_start[rad_index] + radical_kanji_count[rad_index]; 
	 kanji_index++) {
      if (kanji == radical_kanji[kanji_index]) {
	for (i = 0; i < 6; i++) kdicline_ptr[i] = 0;
	g_unichar_to_utf8(radicals[rad_index].radical, kdicline_ptr);
	kdicline_ptr = g_utf8_next_char(kdicline_ptr);
	g_unichar_to_utf8(' ', kdicline_ptr);
	kdicline_ptr = g_utf8_next_char(kdicline_ptr);
      }
    }
  }
}


// Load the radical data from the file
static void load_radkfile() {
  int rad_index, rad_max, kanji_index;
  int error = FALSE;
  struct stat radk_stat;
  
  gchar *radkfile_name = RADKFILE_NAME;
  gchar *radkfile_ptr;
  gchar *radkfile_end;
  int fd = 0;
  
  if (radkfile != NULL) {
    //printf("radkfile already initialized.\n");
    return;
  }
  if (stat(radkfile_name, &radk_stat) != 0) {
    g_error("**ERROR** radkfile: stat() \n");
    error = TRUE;
  }
  radkfile_size = radk_stat.st_size;
  fd = open(radkfile_name, O_RDONLY);
  if (fd == -1) {
    g_error("**ERROR** radkfile: open()\n");
    error = TRUE;
  }
  // printf("SIZE: %d\n", radkfile_size);
  radkfile = (gchar *) mmap(NULL, radkfile_size, PROT_READ, MAP_SHARED, fd, 0);
  if (radkfile == NULL) gjiten_abort_with_msg("mmap() failed for radkfile\n");

  //  printf("STRLEN: %d\n", strlen(radkfile));

  if (error == TRUE) {
		gjiten_print_error(_("Error opening %s.\n "\
												 "Check your preferences or read the documentation!"),
											 radkfile_name);
		return;
  }

  radical_kanji = g_malloc(radkfile_size * sizeof(gunichar));
  if (radical_kanji==NULL) gjiten_abort_with_msg("malloc failed for radical tables!\n");
  
  radkfile_end = radkfile + strlen(radkfile);
  radkfile_ptr = radkfile;

  rad_index = -1;
  kanji_index = 0;
  rad_max = 0;
  
  while((radkfile_ptr < radkfile_end) && (radkfile_ptr != NULL)) {
    if (*radkfile_ptr == '#') {  //find $ as first char on the line
      radkfile_ptr = get_eof_line(radkfile_ptr, radkfile_end); //Goto next line
      continue;
    }
    if (*radkfile_ptr == '$') {  //Radical info line
      rad_index++;                         //Increase number of radicals found
      radkfile_ptr = g_utf8_next_char(radkfile_ptr);
      while (g_unichar_iswide(g_utf8_get_char(radkfile_ptr)) == FALSE) //Find radical
	radkfile_ptr = g_utf8_next_char(radkfile_ptr);
      radicals[rad_index].radical = g_utf8_get_char(radkfile_ptr); //store radical
      while (g_ascii_isdigit(*radkfile_ptr) == FALSE) //Find stroke number
	radkfile_ptr = g_utf8_next_char(radkfile_ptr);
      radicals[rad_index].strokes = atoi(radkfile_ptr);  //Store the stroke number
      radical_kanji_start[rad_index] = kanji_index;
      radical_kanji_count[rad_index] = 0;
      if (rad_index != 0) {
	if (radical_kanji_count[rad_index - 1] > rad_max) 
	  rad_max = radical_kanji_count[rad_index - 1];  // find largest kanji count for a given radical
      }
      radkfile_ptr = get_eof_line(radkfile_ptr, radkfile_end); //Goto next line
    }
    else {   //Kanji
      while ((*radkfile_ptr != '$') && (radkfile_ptr < radkfile_end)) {
	if (*radkfile_ptr == '\n') {
	  radkfile_ptr++;
	  continue;
	}
	//printf("KANJI_INDEX: %d \n", kanji_index);
	radical_kanji[kanji_index] = g_utf8_get_char(radkfile_ptr); //Store kanji
	radkfile_ptr = g_utf8_next_char(radkfile_ptr);
	radical_kanji_count[rad_index]++;
	kanji_index++; 
      }
    }
  }
  total_radicals = rad_index;
}

GtkWidget *create_window_radicals () {
  int i = 0, j = 0, k = 0, rad_index = 0;
  int curr_strokecount = 0;
  GtkWidget *radtable; 
  GtkWidget *tmpwidget = NULL;
  GtkWidget *radical_label;
  gchar strokenum_label[4];
  gchar radical[6];

	load_radkfile(); 

  if (kanjiDic->window_radicals != NULL) {
    gtk_widget_hide(kanjiDic->window_radicals);
    gtk_widget_show(kanjiDic->window_radicals);
    return kanjiDic->window_radicals;
  }
  kanjiDic->window_radicals = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(kanjiDic->window_radicals), _("Radicals"));
  g_signal_connect(GTK_OBJECT(kanjiDic->window_radicals), "destroy", GTK_SIGNAL_FUNC(radical_window_close), NULL);

  radtable = gtk_table_new(11, RADLISTLEN, TRUE); 
  gtk_container_add(GTK_CONTAINER(kanjiDic->window_radicals), radtable);
  gtk_widget_show(radtable);

   
  for (rad_index = 0; rad_index <= total_radicals; rad_index++) {
    if (i == RADLISTLEN) {
      i = 0;
      j++;
    }
    if (curr_strokecount != radicals[rad_index].strokes) {
      curr_strokecount = radicals[rad_index].strokes;
      snprintf((char *)&strokenum_label, 3, "%d", curr_strokecount); //Make a label with the strokenumber
      tmpwidget = gtk_label_new(strokenum_label); //radical stroke number label
      
      gtk_table_attach(GTK_TABLE(radtable), tmpwidget , i, i+1, j, j+1,
		       (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		       (GtkAttachOptions)(0), 0, 0);
      gtk_widget_show(tmpwidget);
      i++;    
    }
    for (k = 0; k < 6; k++) radical[k] = 0;
    g_unichar_to_utf8(radicals[rad_index].radical, radical);
    radical_label = gtk_label_new(radical);
    gtk_widget_show(radical_label);
    tmpwidget = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(tmpwidget), radical_label);
    if ((gjitenApp->conf->normalfont != NULL) && gjitenApp->conf->normalfont_desc != NULL) {
      gtk_widget_modify_font(radical_label, gjitenApp->conf->normalfont_desc);
    }
    
    g_signal_connect_swapped(GTK_OBJECT(tmpwidget), "clicked", GTK_SIGNAL_FUNC(radical_selected), 
			      (GtkObject*)(&(radicals[rad_index].radical)));
    
    gtk_table_attach(GTK_TABLE(radtable), tmpwidget , i, i+1, j, j+1,
		     (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),
		     (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);
    //      gtk_widget_set_usize(tmpwidget,20,20);
    gtk_widget_show(tmpwidget);
    i++;
  }  
  gtk_widget_show(kanjiDic->window_radicals);
  return kanjiDic->window_radicals;
}


static void kanjidic_close() {

	if (kanjiDic != NULL) {
		radical_window_close();

		gtk_widget_destroy(kanjiDic->window);
		g_free(kanjiDic);
		kanjiDic = NULL;
		gjitenApp->kanjidic = NULL;
	}
	gjiten_exit();
}

void shade_kanjidic_widgets() {
  gtk_widget_set_sensitive(kanjiDic->spinb_strokenum,GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)->active);
  gtk_widget_set_sensitive(kanjiDic->spinb_plusmin,GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)->active);
  gtk_widget_set_sensitive(kanjiDic->label_plusmin,GTK_TOGGLE_BUTTON(kanjiDic->checkb_stroke)->active);

  gtk_widget_set_sensitive(kanjiDic->button_clearrad,GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)->active);
  gtk_widget_set_sensitive(kanjiDic->button_radtable,GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)->active);
  gtk_widget_set_sensitive(kanjiDic->combo_entry_radical,GTK_TOGGLE_BUTTON(kanjiDic->checkb_radical)->active);
     
  gtk_widget_set_sensitive(kanjiDic->button_cleark,GTK_TOGGLE_BUTTON(kanjiDic->checkb_ksearch)->active);
  gtk_widget_set_sensitive(kanjiDic->combo_entry_key,GTK_TOGGLE_BUTTON(kanjiDic->checkb_ksearch)->active);
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
    gtk_widget_modify_font(GTK_COMBO(kanjiDic->combo_entry_key)->entry, gjitenApp->conf->normalfont_desc);
    gtk_widget_modify_font(GTK_COMBO(kanjiDic->combo_entry_radical)->entry, gjitenApp->conf->normalfont_desc);
  }

}

KanjiDic *kanjidic_create() {
  GtkWidget *vbox_maink;
  GtkWidget *hbox_spinb;
  GtkWidget *table_koptions;
  GtkWidget *toolbar_kanji;
  GtkWidget *button_closek;
  GtkWidget *button_kanjipad;
  GtkWidget *button_worddic;
  GtkWidget *button_searchk;
  GtkWidget *frame_koptions;
  GtkObject *spinb_strokenum_adj;
  GtkObject *spinb_plusmin_adj;
  GtkWidget *hseparator;
  GtkWidget *frame_kresults;
  GtkWidget *scrolledwin_kresults;
  GtkWidget *scrolledwin_kinfo;
  GtkWidget *dock_kanjidic;
  GtkWidget *hbox;
  GtkWidget *frame_kinfo;
  GtkWidget *tmpimage;
 	GtkWidget *scrolledwin_history;
	GtkWidget *vpane;


  load_radkfile();

  if (kanjiDic != NULL) {
		gtk_widget_hide(kanjiDic->window);
		gtk_widget_show(kanjiDic->window);
		return kanjiDic;
  }

	kanjiDic = g_new0(KanjiDic, 1);
	gjitenApp->kanjidic = kanjiDic;

  if (kdic_line == NULL) kdic_line = (gchar *)g_malloc(KCFGNUM * 200);
  if (kdic_line == NULL) gjiten_abort_with_msg("Couldn't allocate memory\n");

  kanjiDic->window = gnome_app_new("gjiten", "Gjiten - KanjiDic");
  GTK_WIDGET_SET_FLAGS(kanjiDic->window, GTK_CAN_DEFAULT);
  g_signal_connect(G_OBJECT(kanjiDic->window), "destroy", G_CALLBACK(kanjidic_close), NULL);
  gtk_window_set_default_size(GTK_WINDOW(kanjiDic->window), 500, 500);

  dock_kanjidic = GNOME_APP(kanjiDic->window)->dock;
  gtk_widget_show(dock_kanjidic);

  if (gjitenApp->conf->menubar) gnome_app_create_menus(GNOME_APP(kanjiDic->window), kmenubar_uiinfo);
  
  vbox_maink = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox_maink);
  gnome_app_set_contents(GNOME_APP(kanjiDic->window), vbox_maink);
   
  if (gjitenApp->conf->toolbar) {
    toolbar_kanji = gtk_toolbar_new();
    gtk_widget_show(toolbar_kanji);

    gnome_app_set_toolbar(GNOME_APP(kanjiDic->window), GTK_TOOLBAR(toolbar_kanji));
    
    button_closek = gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar_kanji), GTK_STOCK_CLOSE,
																						 _("Close"), "Close", 
																						 NULL, NULL, -1);
		g_signal_connect_swapped(G_OBJECT(button_closek), "clicked", 
														 G_CALLBACK(gtk_widget_destroy), kanjiDic->window);

    tmpimage = gtk_image_new_from_file(PIXMAPDIR"/kanjidic.png");
    button_worddic = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar_kanji), _("WordDic"),
																						 _("WordDic"), "WordDic", tmpimage,
																						 G_CALLBACK(worddic_create), NULL);

    tmpimage = gtk_image_new_from_file(PIXMAPDIR"/kanjipad.png");
    button_kanjipad = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar_kanji), _("KanjiPad"),
					      _("KanjiPad"), "KanjiPad", tmpimage,
					      GTK_SIGNAL_FUNC(gjiten_start_kanjipad), NULL);
    
    button_searchk = gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar_kanji), GTK_STOCK_FIND,
					      _("Search"), "Search", 
					      on_kanji_search, NULL, -1);
  }
  
  frame_koptions = gtk_frame_new(_("Kanji Search Options"));
  gtk_widget_show(frame_koptions);
  gtk_box_pack_start(GTK_BOX(vbox_maink), frame_koptions, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame_koptions), 2);

  table_koptions = gtk_table_new(3, 4, FALSE);
  gtk_widget_show(table_koptions);
  gtk_container_add(GTK_CONTAINER(frame_koptions), table_koptions);

  //FIXME: use mnemonics
  kanjiDic->checkb_stroke = gtk_check_button_new_with_mnemonic(_("Search By _Strokes:")); 
  gtk_widget_show(kanjiDic->checkb_stroke);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->checkb_stroke, 0, 1, 0, 1,
									 (GtkAttachOptions)(GTK_FILL),(GtkAttachOptions)(0), 0, 0);
  g_signal_connect(GTK_OBJECT(kanjiDic->checkb_stroke), "toggled", 
									 GTK_SIGNAL_FUNC(shade_kanjidic_widgets), NULL);

  kanjiDic->checkb_radical = gtk_check_button_new_with_mnemonic(_("Search By _Radical:"));  
  gtk_widget_show(kanjiDic->checkb_radical);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->checkb_radical, 0, 1, 1, 2,
                    (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(0), 0, 0);
  g_signal_connect (GTK_OBJECT(kanjiDic->checkb_radical), "toggled", 
		      GTK_SIGNAL_FUNC(shade_kanjidic_widgets), NULL);
 
  kanjiDic->checkb_ksearch = gtk_check_button_new_with_mnemonic(_("Search By _Key:"));
  gtk_widget_show(kanjiDic->checkb_ksearch);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->checkb_ksearch, 0, 1, 2, 3,
                    (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(0), 0, 0);
  g_signal_connect(GTK_OBJECT(kanjiDic->checkb_ksearch), "toggled", 
		      GTK_SIGNAL_FUNC(shade_kanjidic_widgets), NULL);

  hbox_spinb = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox_spinb);
  gtk_table_attach(GTK_TABLE(table_koptions), hbox_spinb, 1, 2, 0, 1,
                    (GtkAttachOptions)(GTK_FILL), (GtkAttachOptions)(GTK_FILL), 0, 0);

  spinb_strokenum_adj = gtk_adjustment_new(1, 1, 30, 1, 2, 2);
  kanjiDic->spinb_strokenum = gtk_spin_button_new(GTK_ADJUSTMENT(spinb_strokenum_adj), 1, 0);
  gtk_widget_show(kanjiDic->spinb_strokenum);
  gtk_box_pack_start(GTK_BOX(hbox_spinb), kanjiDic->spinb_strokenum, FALSE, FALSE, 0);

  kanjiDic->label_plusmin = gtk_label_new("+/-");
  gtk_widget_show(kanjiDic->label_plusmin);
  gtk_box_pack_start(GTK_BOX(hbox_spinb), kanjiDic->label_plusmin, FALSE, FALSE, 0);

  spinb_plusmin_adj = gtk_adjustment_new(0, 0, 10, 1, 10, 10);
  kanjiDic->spinb_plusmin = gtk_spin_button_new(GTK_ADJUSTMENT(spinb_plusmin_adj), 1, 0);
  gtk_widget_show(kanjiDic->spinb_plusmin);
  gtk_box_pack_start(GTK_BOX(hbox_spinb), kanjiDic->spinb_plusmin, FALSE, FALSE, 0);

  kanjiDic->button_radtable = gtk_button_new_with_mnemonic(_("Show Radical _List"));
  gtk_widget_show(kanjiDic->button_radtable);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->button_radtable, 3, 4, 1, 2,
                    (GtkAttachOptions)(0), (GtkAttachOptions)(0), 0, 0);
  g_signal_connect(GTK_OBJECT(kanjiDic->button_radtable), "clicked", 
		      GTK_SIGNAL_FUNC(create_window_radicals), NULL);

  kanjiDic->combo_entry_radical = gtk_combo_new();
  gtk_widget_show(kanjiDic->combo_entry_radical);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->combo_entry_radical, 1, 2, 1, 2,
                    (GtkAttachOptions)(GTK_EXPAND | GTK_FILL),  (GtkAttachOptions)(0), 0, 0);
  g_signal_connect(GTK_OBJECT(GTK_COMBO(kanjiDic->combo_entry_radical)->entry), 
		      "activate", GTK_SIGNAL_FUNC(on_kanji_search), NULL);
  gtk_combo_disable_activate(GTK_COMBO(kanjiDic->combo_entry_radical));

  kanjiDic->combo_entry_key = gtk_combo_new();
  gtk_widget_show(kanjiDic->combo_entry_key);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->combo_entry_key, 1, 2, 2, 3,
                    (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(0), 0, 0);
  g_signal_connect(GTK_OBJECT(GTK_COMBO(kanjiDic->combo_entry_key)->entry), 
		      "activate", GTK_SIGNAL_FUNC(on_kanji_search), NULL);
  gtk_combo_disable_activate(GTK_COMBO(kanjiDic->combo_entry_key));

  kanjiDic->button_clearrad = gtk_button_new_with_label(_("Clear"));
  gtk_widget_show(kanjiDic->button_clearrad);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->button_clearrad, 2, 3, 1, 2,
                    (GtkAttachOptions)(0), (GtkAttachOptions)(0), 5, 0);
  g_signal_connect_swapped(GTK_OBJECT(kanjiDic->button_clearrad), "clicked", 
													 GTK_SIGNAL_FUNC(gjiten_clear_entry_box), 
													 GTK_OBJECT(GTK_COMBO(kanjiDic->combo_entry_radical)->entry));

  kanjiDic->button_cleark = gtk_button_new_with_label(_("Clear"));
  gtk_widget_show(kanjiDic->button_cleark);
  gtk_table_attach(GTK_TABLE(table_koptions), kanjiDic->button_cleark, 2, 3, 2, 3,
									 (GtkAttachOptions)(0), (GtkAttachOptions)(0), 5, 0);
  g_signal_connect_swapped(GTK_OBJECT(kanjiDic->button_cleark), "clicked", 
													 GTK_SIGNAL_FUNC(gjiten_clear_entry_box), 
													 GTK_OBJECT(GTK_COMBO(kanjiDic->combo_entry_key)->entry));

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

  scrolledwin_history = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwin_history),
																 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show(scrolledwin_history);

  kanjiDic->vbox_history = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(kanjiDic->vbox_history);
  history_init();

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwin_history), kanjiDic->vbox_history);
					//  gtk_container_add(GTK_CONTAINER(scrolledwin_history), kanjiDic->vbox_history);
  gtk_box_pack_start(GTK_BOX(hbox), scrolledwin_history, FALSE, FALSE, 0);

  vpane = gtk_vpaned_new();
  gtk_widget_show(vpane);
  gtk_paned_add1(GTK_PANED(vpane), frame_kresults);
  gtk_paned_add2(GTK_PANED(vpane), frame_kinfo);
  gtk_box_pack_start(GTK_BOX(vbox_maink), vpane, TRUE, TRUE, 0);

  kanjiDic->appbar_kanji = gnome_appbar_new(TRUE, TRUE, GNOME_PREFERENCES_NEVER);
  gtk_widget_show(kanjiDic->appbar_kanji);
  gtk_box_pack_start(GTK_BOX(vbox_maink), kanjiDic->appbar_kanji, FALSE, FALSE, 0);

  shade_kanjidic_widgets();

  g_signal_connect(GTK_OBJECT(GTK_ADJUSTMENT(spinb_strokenum_adj)), 
				  "value_changed", GTK_SIGNAL_FUNC(on_kanji_search), NULL);
  g_signal_connect(GTK_OBJECT(GTK_ADJUSTMENT(spinb_plusmin_adj)), 
				  "value_changed", GTK_SIGNAL_FUNC(on_kanji_search), NULL);

  gtk_text_buffer_create_tag(kanjiDic->text_kanjinfo_buffer, "blue_foreground", "foreground", "blue", NULL);  

	kanjidic_apply_fonts();

  gtk_widget_show(kanjiDic->window);

	return kanjiDic;
}
