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
#include <locale.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
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

/***************** VARIABLES ***********************/

gchar *clipboard_text = NULL;

/* FIXME: GConfEnumPair also in kanjidic.h */
gchar *kanjidicstrg[] = { "kanji", "radical", "strokes", "reading", "korean", 
													"pinyin", "english", "bushu", "classic", "freq", "jouyou",
													"deroo", "skip", "fourc", "hindex", "nindex", "vindex",
													"iindex", "inindex", "mnindex", "mpindex", "eindex", "kindex", 
													"lindex", "oindex", "cref", "missc", "unicode", "jisascii" };


enum {
  KANJIDIC_KEY        = -1,
  WORD_LOOKUP_KEY     = -2,
  KANJI_LOOKUP_KEY    = -3,
  CLIP_KANJI_KEY      = -4,
  CLIP_WORD_KEY       = -5
};

static gboolean run_kdic = FALSE;
static gboolean run_kdic_clip = FALSE;
static gboolean run_wdic_clip = FALSE;
static gchar *k_to_lookup;
static gchar *w_to_lookup;
static gchar *remaining_args;

/* Command line arguments via popt */
static GOptionEntry arg_options [] = {
	{ "kanjidic", 'k', 0, G_OPTION_ARG_NONE, &run_kdic,
		N_("Start up Kanjidic instead of Word dictionary"), NULL },

	{ "word-lookup", 'w', 0, G_OPTION_ARG_STRING, &w_to_lookup,
		N_("Look up WORD in first dictionary"), N_("WORD") },

	{ "kanji-lookup", 'l', 0, G_OPTION_ARG_STRING, &k_to_lookup,
		N_("Look up KANJI in kanji dictionary"), N_("KANJI") },

	{ "clip-kanji", 'c', 0, G_OPTION_ARG_NONE, &run_kdic_clip,
		N_("Look up kanji from clipboard"), NULL },

	{ "clip-word", 'v', 0, G_OPTION_ARG_NONE, &run_wdic_clip,
		N_("Look up word from clipboard"), NULL },

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
		&remaining_args, NULL},

	{ NULL }

};

static struct {
	gchar *filename;
	gchar *stock_id;
} stock_icons[] = {
	{ PIXMAPDIR"/kanjidic.png", "my-gjiten-icon" },
	{ PIXMAPDIR"/kanjipad.png", "my-kanjipad-icon" },
};

static gint n_stock_icons = G_N_ELEMENTS (stock_icons);

static void
register_my_stock_icons(void)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set;
	GtkIconSource *icon_source;
	gint i;

	icon_factory = gtk_icon_factory_new();

	for (i = 0; i < n_stock_icons; i++)
	{
		icon_set = gtk_icon_set_new();
		icon_source = gtk_icon_source_new();
		gtk_icon_source_set_filename(icon_source, stock_icons[i].filename);
		gtk_icon_set_add_source(icon_set, icon_source);
		gtk_icon_source_free(icon_source);
		gtk_icon_factory_add(icon_factory, stock_icons[i].stock_id, icon_set);
		gtk_icon_set_unref(icon_set);
	}

	gtk_icon_factory_add_default(icon_factory);

	g_object_unref(icon_factory);
}


/*================ Functions ===============================================*/

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

  gtk_show_uri(NULL, "ghelp:gjiten", gtk_get_current_event_time (), &err);
  
  if (err != NULL) {
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

    gtk_window_present(GTK_WINDOW(dialog));

    g_error_free(err);
  }
}


void gjiten_create_about() {
  const gchar *authors[] = { "Botond Botyanszki <boti@rocketmail.com>", NULL };
  const gchar *documenters[] = { NULL };
  const gchar *translator = _("TRANSLATORS! PUT YOUR NAME HERE");
  GdkPixbuf *pixbuf = NULL;

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
  gtk_show_about_dialog(NULL, "program-name", "gjiten", "version", VERSION,
		"copyright", "Copyright \xc2\xa9 1999-2005 Botond Botyanszki",
		"comments", _("gjiten is a Japanese dictionary for Gnome"),
		"authors", (const char **)authors,
		"documenters", (const char **)documenters,
		"translator-credits", (const char *)translator,
		"logo", pixbuf, NULL);

  if (pixbuf != NULL)  g_object_unref (pixbuf);

}



/*********************** MAIN ***********************************/
int main (int argc, char **argv) {
  char *icon_path = PIXMAPDIR"/jiten.png";
  GError *error = NULL;
 
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


#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, GJITEN_LOCALE_DIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
  textdomain(PACKAGE);
#endif

  if (!gtk_init_with_args(&argc, &argv, NULL, arg_options, PACKAGE, &error))
    {printf("%s\n", error->message); exit(EXIT_FAILURE);}

  register_my_stock_icons();

  gjitenApp->conf->clip_kanji_lookup = run_kdic_clip;
  gjitenApp->conf->clip_word_lookup = run_wdic_clip;
  gjitenApp->conf->startkanjidic = run_kdic;
  gjitenApp->conf->word_to_lookup = w_to_lookup;
  gjitenApp->conf->kanji_to_lookup = k_to_lookup;

  if (!g_file_test(icon_path, G_FILE_TEST_EXISTS)) {
    g_warning ("Could not find %s", icon_path);
  }
  else {
    gtk_window_set_default_icon_from_file(icon_path, &error);
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
	gjiten_flush_errors();
  gtk_main();
  return 0;
}
