/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* error.c

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


#include <stdarg.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "error.h"

static gchar *gjiten_errors;

int gjiten_print_error(const char *fmt, ... ) {
	GtkWidget *dialog;
	gint ret = -1;
	va_list args;
	gchar *pstr;

	va_start(args, fmt);
	pstr = g_strdup_vprintf(fmt, args);
	va_end(args);

	if (pstr != NULL) {
	  dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,  GTK_BUTTONS_OK, "%s", pstr );

		g_signal_connect_swapped(GTK_OBJECT(dialog), "response", 
														 G_CALLBACK(gtk_widget_destroy),
														 GTK_OBJECT(dialog));
		
		gtk_widget_show_all(dialog);
	  g_free(pstr);
	}
	return ret;
}

void gjiten_print_error_and_wait(const char *fmt, ... ) {
	GtkWidget *dialog;
	va_list args;
	gchar *pstr;

	va_start(args, fmt);
	pstr = g_strdup_vprintf(fmt, args);
	va_end(args);
 
	if (pstr != NULL) {
	  dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,  GTK_BUTTONS_OK, "%s", pstr );

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	  g_free(pstr);
	}
}

void gjiten_add_errormsg(gchar *msg) {
	gchar *tmpmsg;

	if (gjiten_errors != NULL) {
		tmpmsg = g_strdup_printf("%s\n%s", gjiten_errors, msg);

		g_free(gjiten_errors);
		gjiten_errors = tmpmsg;
	}
	else {
		gjiten_errors = g_strdup(msg);
	}
}

void gjiten_flush_errors() {
	if (gjiten_errors != NULL) {
		gjiten_print_error_and_wait("%s", gjiten_errors);
		g_free(gjiten_errors);
		gjiten_errors = NULL;
	}
}

gboolean gjiten_print_question(const char *fmt, ... ) {
	GtkWidget *dialog;
	gint retval = GTK_RESPONSE_REJECT;

	va_list args;
	gchar *pstr;

	va_start(args, fmt);
	pstr = g_strdup_vprintf(fmt, args);
	va_end(args);
 
	if (pstr != NULL) {
	  dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", pstr);

		retval = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	  g_free(pstr);
	}

	if (retval == GTK_RESPONSE_YES) return TRUE;
	else return FALSE;
}

void gjiten_abort_with_msg(const char *fmt, ... ) {
	va_list args;
	gchar *pstr;

	va_start(args, fmt);
	pstr = g_strdup_vprintf(fmt, args);
	va_end(args);

	gjiten_print_error_and_wait(pstr);
  exit(1);
}

