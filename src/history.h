/* Copyright (C) 2010 by Cristian Henzel <oss@rspwn.com>
 *
 * forked from parcellite, which is
 * Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com>
 *
 * This file is part of ClipIt.
 *
 * ClipIt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ClipIt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HISTORY_H
#define HISTORY_H

G_BEGIN_DECLS

#define HISTORY_FILE "clipit/history"

/* Set maximum size of one clipboard entry to 1024KB (1MB) 
 * 1024 pages × 2000 characters per page - should be more than enough.
 * WARNING: if you use all 1000 history items, clipit could use up to
 * 1 GB of RAM. If you don't want that, set this lower.      */
#define ENTRY_MAX_SIZE 1048576

typedef struct {
	gboolean is_static;
	gpointer content;
} history_item;

extern GList *history;

void read_history();

void save_history();

void check_and_append(gchar *item);

void append_item(gchar *item);

void truncate_history();

gpointer get_last_item();

void delete_duplicate(gchar *item);

void delete_latest_item();

G_END_DECLS

#endif
