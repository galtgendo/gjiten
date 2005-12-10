/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* vi: set ts=2 sw=2: */
/* dicfile.h
   
   GJITEN : A JAPANESE DICTIONARY FOR GNOME
  
   Copyright (C) 1999-2005 Botond Botyanszki <boti at rocketmail dot com>
   
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

#ifndef __DICFILE_H__
#define __DICFILE_H__

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


typedef struct _GjitenDicfile GjitenDicfile;

struct _GjitenDicfile {
	gchar *path;
	gchar *name;
	gchar *mem;
	int file;
	gint status;
  struct stat stat;
	gint size;
};

enum {
	DICFILE_NOT_INITIALIZED,
	DICFILE_BAD,
	DICFILE_OK
};


gboolean dicfile_is_utf8(GjitenDicfile *dicfile);
gboolean dicfile_init(GjitenDicfile *dicfile);
void dicfile_close(GjitenDicfile *dicfile);
void dicfile_list_free(GSList *dicfile_list);
gboolean dicfile_check_all(GSList *dicfile_list);

#endif
