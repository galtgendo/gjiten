/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* dicfile.c
   
   GJITEN : A JAPANESE DICTIONARY FOR GNOME
  
   Copyright (C) 1999-2003 Botond Botyanszki <boti at rocketmail dot com>
   
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

#include <libgnome/libgnome.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dicfile.h"
#include "error.h"

gboolean dicfile_check_all(GSList *dicfile_list) {
	GSList *node;
	GjitenDicfile *dicfile;

	node = dicfile_list;
	while (node != NULL) {
		if (node->data != NULL) {
			dicfile = node->data;
			if (dicfile_init(dicfile) == FALSE) return FALSE;
			if (dicfile_is_utf8(dicfile) == FALSE) {
				dicfile_close(dicfile);
				return FALSE;
			}
			dicfile_close(dicfile);
		}
		node = g_slist_next(node);
	}
	return TRUE;
}

gboolean dicfile_is_utf8(GjitenDicfile *dicfile) {
	gchar *testbuffer;
	gint pos, bytesread;

	if (dicfile->file > 0) {
		testbuffer = (gchar *) g_malloc(3000);
		bytesread = read(dicfile->file, testbuffer, 3000); // read a chunk into buffer
		pos = bytesread - 1;
		while (testbuffer[pos] != '\n') pos--;
		if (g_utf8_validate(testbuffer, pos, NULL) == FALSE) {
			gjiten_print_error(_("Dictionary file is non-UTF: %s\nPlease convert it to UTF-8. See the docs for more."), dicfile->path);
			return FALSE;
		}
		g_free(testbuffer);
	}
	return TRUE;
}

gboolean dicfile_init(GjitenDicfile *dicfile) {

	if (dicfile->status != DICFILE_OK) {
		dicfile->file = open(dicfile->path, O_RDONLY);
		if (dicfile->file == -1) {
			gjiten_print_error(_("Error opening dictfile:  %s\nCheck your preferences!"), dicfile->path);
			dicfile->status = DICFILE_BAD;
			return FALSE;
		}
		else {
			if (stat(dicfile->path, &dicfile->stat) != 0) {
				printf("**ERROR** %s: stat() \n", dicfile->path);
				dicfile->status = DICFILE_BAD;
				return FALSE;
			}
			else {
				dicfile->size = dicfile->stat.st_size;
			}
		}
	}
	else return TRUE;
	
	dicfile->status = DICFILE_OK;
	return TRUE;
}

void dicfile_close(GjitenDicfile *dicfile) {

	if (dicfile->file > 0) {
		close(dicfile->file);
	}
	dicfile->status = DICFILE_NOT_INITIALIZED;
}

void dicfile_list_free(GSList *dicfile_list) {
	GSList *node;
	GjitenDicfile *dicfile;

	node = dicfile_list;
	while (node != NULL) {
		if (node->data != NULL) {
			dicfile = node->data;
			dicfile_close(dicfile);
			g_free(dicfile);
		}
		node = g_slist_next(node);
	}

	g_slist_free(dicfile_list);

}
