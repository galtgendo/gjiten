/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* gjiten.c

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <gconf/gconf-client.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "error.h"
#include "constants.h"
#include "conf.h"
#include "dicfile.h"
#include "worddic.h"
#include "kanjidic.h"
#include "gjiten.h"
#include "dicutil.h"

GjitenApp *gjitenApp = NULL;

static void parse_an_arg(poptContext state,
												 enum poptCallbackReason reason,
												 const struct poptOption *opt,
												 const char *arg, void *data);

/***************** VARIABLES ***********************/

gchar *clipboard_text = NULL;

/* FIXME: GConfEnumPair also in kanjidic.h */
gchar *kanjidicstrg[] = { "kanji", "radical", "strokes", "reading", "korean", 
													"pinyin", "english", "bushu", "classic", "freq", "jouyou",
													"deroo", "skip", "fourc", "hindex", "nindex", "vindex",
													"iindex", "mnindex", "mpindex", "eindex", "kindex", 
													"lindex", "oindex", "cref", "missc", "unicode", "jisascii" };


enum {
  KANJIDIC_KEY        = -1,
  WORD_LOOKUP_KEY     = -2,
  KANJI_LOOKUP_KEY    = -3,
  CLIP_KANJI_KEY      = -4,
  CLIP_WORD_KEY       = -5
};

/* Command line arguments via popt */
static struct poptOption arg_options [] = {
  { NULL, '\0', POPT_ARG_CALLBACK, (gpointer)parse_an_arg, 0,
    NULL, NULL },

  { "kanjidic", 'k', POPT_ARG_NONE, NULL, KANJIDIC_KEY,
    N_("Start up Kanjidic instead of Word dictionary"), NULL },

  { "word-lookup", 'w', POPT_ARG_STRING, NULL, WORD_LOOKUP_KEY,
    N_("Look up WORD in first dictionary"), N_("WORD") },

  { "kanji-lookup", 'l', POPT_ARG_STRING, NULL, KANJI_LOOKUP_KEY,
    N_("Look up KANJI in kanji dictionary"), N_("KANJI") },

  { "clip-kanji", 'c', POPT_ARG_NONE, NULL, CLIP_KANJI_KEY,
    N_("Look up kanji from clipboard"), NULL },

  { "clip-word", 'v', POPT_ARG_NONE, NULL, CLIP_WORD_KEY,
    N_("Look up word from clipboard"), NULL },

  { NULL, '\0', 0, NULL, 0, NULL, NULL }

};



/*================ Functions ===============================================*/

static void parse_an_arg(poptContext state,
												 enum poptCallbackReason reason,
												 const struct poptOption *opt,
												 const char *arg, void *data) {

  
  switch (opt->val) {
  case KANJIDIC_KEY:
    gjitenApp->conf->startkanjidic = TRUE;
    break;
  case WORD_LOOKUP_KEY:
    gjitenApp->conf->word_to_lookup = (gchar *)arg;
    break;
  case KANJI_LOOKUP_KEY:
    gjitenApp->conf->kanji_to_lookup = (gchar *)arg;
    break;
  case CLIP_KANJI_KEY:
    gjitenApp->conf->clip_kanji_lookup = TRUE;
    gjitenApp->conf->clip_word_lookup = FALSE;
    break;
  case CLIP_WORD_KEY:
    gjitenApp->conf->clip_word_lookup = TRUE;
    gjitenApp->conf->clip_kanji_lookup = FALSE;
    break;
  default:
    break;
  }
}

void gjiten_clear_entry_box(gpointer entrybox) {
  gtk_entry_set_text(GTK_ENTRY(entrybox), "");
}

void gjiten_exit() {
	if ((gjitenApp->worddic == NULL) && (gjitenApp->kanjidic == NULL)) {
		GJITEN_DEBUG("gjiten_exit()\n");
		conf_save_options(gjitenApp->conf);
		dicutil_unload_dic();
		dicfile_list_free(gjitenApp->conf->dicfile_list);
		conf_close_handler();
		gtk_main_quit();
	}
}

void gjiten_start_kanjipad() {
  FILE *kanjipad_binary;
  char *kpad_cmd;
	int32_t len;

  kanjipad_binary = fopen(gjitenApp->conf->kanjipad, "r");
  if (kanjipad_binary == NULL) {
    gjiten_print_error(_("Couldn't find the KanjiPad executable!\n"
												 "Please make sure you have it installed on your system \n"
												 "and set the correct path to it in the Preferences.\n"
												 "See the Documentation for more details about KanjiPad."));
  }
  else {
		len = strlen(gjitenApp->conf->kanjipad) + 2;
    fclose(kanjipad_binary);
    kpad_cmd = g_malloc(len);
    strncpy(kpad_cmd, gjitenApp->conf->kanjipad, len);
    strncat(kpad_cmd, "&", 1);
    system(kpad_cmd); /* FIXME */
    g_free(kpad_cmd);
  }
}

void gjiten_display_manual(GtkWidget *widget, void *data) {
  GtkWidget *window = data;
  GError *err = NULL;
  gboolean retval = FALSE;

  retval = gnome_help_display("gjiten.xml", NULL, &err);
  
  if (retval == FALSE) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(GTK_WINDOW(window),
																		GTK_DIALOG_DESTROY_WITH_PARENT,       
																		GTK_MESSAGE_ERROR,
																		GTK_BUTTONS_CLOSE,
																		_("Could not display help: %s"),
																		err->message);
    
    g_signal_connect(G_OBJECT(dialog), "response",
										 G_CALLBACK(gtk_widget_destroy),
										 NULL);
    
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    
    gtk_widget_show(dialog);
    
    g_error_free(err);
  }
}


void gjiten_create_about() {
  const gchar *authors[] = { "Botond Botyanszki <boti@rocketmail.com>", NULL };
  const gchar *documenters[] = { NULL };
  const gchar *translator = _("TRANSLATORS! PUT YOUR NAME HERE");
  static GtkWidget *about = NULL;
  GdkPixbuf *pixbuf = NULL;

  if (about != NULL) {
    gdk_window_show(about->window);
    gdk_window_raise(about->window);
    return;
  }

  if (pixbuf != NULL) {
    GdkPixbuf* temp_pixbuf = NULL;
    
    temp_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 
					   gdk_pixbuf_get_width(pixbuf) / 2,
					   gdk_pixbuf_get_height(pixbuf) / 2,
					   GDK_INTERP_HYPER);
    g_object_unref(pixbuf);
    pixbuf = temp_pixbuf;
  }

  pixbuf = gdk_pixbuf_new_from_file(PIXMAPDIR"/gjiten/gjiten-logo.png", NULL);

  if (strcmp(translator, "translated_by") == 0) translator = NULL;

  /*
    _("Released under the terms of the GNU GPL.\n"
    "Check out http://gjiten.sourceforge.net for updates"), 
  */
  about = gnome_about_new("gjiten", VERSION, "Copyright \xc2\xa9 1999-2005 Botond Botyanszki",
			  _("gjiten is a Japanese dictionary for Gnome"),
			  (const char **)authors,
			  (const char **)documenters,
			  (const char *)translator,
			  pixbuf);

  gtk_window_set_destroy_with_parent(GTK_WINDOW(about), TRUE);
  if (pixbuf != NULL)  g_object_unref (pixbuf);

  g_signal_connect(G_OBJECT(about), "destroy", G_CALLBACK(gtk_widget_destroyed), &about);
  gtk_widget_show(about);

}



/*********************** MAIN ***********************************/
int main (int argc, char **argv) {
  char *icon_path = PIXMAPDIR"/jiten.png";
 
	gjitenApp = g_new0(GjitenApp, 1);
  conf_init_handler();
	gjitenApp->conf = conf_load();

	if (gjitenApp->conf->envvar_override == TRUE) {
		if (gjitenApp->conf->gdk_use_xft == TRUE) putenv("GDK_USE_XFT=1");
		else putenv("GDK_USE_XFT=0");
		/* if (gjitenApp->conf->force_ja_JP == TRUE) putenv("LC_CTYPE=ja_JP"); */
		if (gjitenApp->conf->force_ja_JP == TRUE) putenv("LC_ALL=ja_JP");
		if (gjitenApp->conf->force_language_c == TRUE) putenv("LANGUAGE=C");
	}

  gtk_set_locale();  

#ifdef ENABLE_NLS
  bindtextdomain(PACKAGE, GJITEN_LOCALE_DIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);
#endif

  gnome_program_init("gjiten", VERSION, LIBGNOMEUI_MODULE, argc, argv, 
										 GNOME_PARAM_POPT_TABLE, arg_options, 
										 GNOME_PARAM_HUMAN_READABLE_NAME, _("gjiten"), 
										 GNOME_PARAM_APP_DATADIR, GNOMEDATADIR,
										 NULL);

  if (! g_file_test (icon_path, G_FILE_TEST_EXISTS)) {
    g_warning ("Could not find %s", icon_path);
  }
  else {
    gnome_window_icon_set_default_from_file(icon_path);
  }

  /* the following is for clipboard lookup. */
  if ((gjitenApp->conf->clip_kanji_lookup == TRUE) || (gjitenApp->conf->clip_word_lookup == TRUE)) {
    if (gjitenApp->conf->clip_word_lookup) {
      gjitenApp->worddic = worddic_create();
      worddic_paste(gjitenApp->worddic);
      on_text_entered();
    }
    else {
      if (gjitenApp->conf->clip_kanji_lookup){
				clipboard_text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
				/* validate	*/
				/* FIXME: try to convert EUC-JP to UTF8 if it's non-utf8 */
				if (g_utf8_validate(clipboard_text, -1, NULL) == FALSE) {
					gjiten_print_error(_("Unable to look up kanji: NON-UTF8 string received from clipboard!\n"));
					exit(0);
				}
				else if (isKanjiChar(g_utf8_get_char(clipboard_text)) == FALSE) {
					gjiten_print_error(_("Non-kanji string received from clipboard: %s\n"), clipboard_text);
					exit(0);
				}
				else {
					if (gjitenApp->kanjidic == NULL) kanjidic_create();
					print_kanjinfo(g_utf8_get_char(clipboard_text));
					/* if (clipboard_text !	= NULL) gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo_entry)->entry), clipboard_text); */
				}
      }
    }
  }

  if (argc > 1) {
    if (gjitenApp->conf->startkanjidic) {
      kanjidic_create();
    }
    else  
      if (gjitenApp->conf->word_to_lookup) {
				gjitenApp->worddic = worddic_create();
				gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(gjitenApp->worddic->combo_entry)->entry), gjitenApp->conf->word_to_lookup);
				on_text_entered();
      }
      else if (gjitenApp->conf->kanji_to_lookup != NULL) {
				if (g_utf8_validate(gjitenApp->conf->kanji_to_lookup, -1, NULL) == FALSE) {
					gjiten_print_error(_("Unable to look up kanji: NON-UTF8 string received from clipboard!\n"));
					exit(0); /* FIXME */
				}
				else if (isKanjiChar(g_utf8_get_char(gjitenApp->conf->kanji_to_lookup)) == FALSE) {
					gjiten_print_error(_("Non-kanji string received from clipboard: %s\n"), gjitenApp->conf->kanji_to_lookup);
					exit(0); /* FIXME */
				}
				else {
					if (gjitenApp->kanjidic == NULL) kanjidic_create();
					print_kanjinfo(g_utf8_get_char(gjitenApp->conf->kanji_to_lookup));
				}
			}
			else if (!gjitenApp->conf->clip_kanji_lookup && !gjitenApp->conf->clip_word_lookup) 
				gjitenApp->worddic = worddic_create();
  }
  else {
    gjitenApp->worddic = worddic_create();
  }
	gjiten_flush_errors();
  gtk_main();
  return 0;
}
