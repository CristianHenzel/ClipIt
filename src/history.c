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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "utils.h"
#include "history.h"

GList *history;

/* Deletes duplicate item in history */
static GList *find_duplicate(gchar *item)
{
	GList *element;
	/* Go through each element compare each */
	for (element = history; element != NULL; element = element->next) {
		history_item *elem_data = element->data;
		if (g_strcmp0((gchar*)elem_data->content, item) == 0) {
			return element;
		}
	}
	return NULL;
}

static history_item *initialize_history_item()
{
	history_item *hitem = g_new0(history_item, 1);
	hitem->is_static = 0;
	return hitem;
}

/* Returns TRUE if text should be excluded from history */
static gboolean is_excluded(gchar *text)
{
	/* Open the file for reading */
	gchar *path = g_build_filename(g_get_user_data_dir(), EXCLUDES_FILE, NULL);
	FILE *excludes_file = fopen(path, "rb");
	g_free(path);
	/* Check that it opened and begin read */
	if (excludes_file)
	{
		/* Read the size of the first item */
		gint size;
		size_t fread_return;
		fread_return = fread(&size, 4, 1, excludes_file);
		/* Continue reading until size is 0 */
		while (size != 0)
		{
			/* Read Regex */
			gchar *regex = (gchar*)g_malloc(size + 1);
			fread_return = fread(regex, size, 1, excludes_file);
			regex[size] = '\0';
			fread_return = fread(&size, 4, 1, excludes_file);
			/* Append the read action */
			GRegex *regexp = g_regex_new(regex, G_REGEX_CASELESS, 0, NULL);
			gboolean result = g_regex_match(regexp, text, 0, NULL);
			g_regex_unref(regexp);
			g_free(regex);
			if(result)
				return result;
		}
		fclose(excludes_file);
	}
	return FALSE;
}

/* Reads history from DATADIR/clipit/history */
void read_history()
{
	/* Build file path */
	gchar *history_path = g_build_filename(g_get_user_data_dir(),
						HISTORY_FILE,
						NULL);

	/* Open the file for reading */
	FILE *history_file = fopen(history_path, "rb");
	g_free(history_path);
	/* Check that it opened and begin read */
	if (history_file) {
		/* Read the size of the first item */
		int size;
		size_t fread_return;
		if (fread(&size, 4, 1, history_file) != 1)
			size = 0;
		if (size == -1) {
			/* If size is -1, we are using the new filetype introduced in 1.4.1 */
			/* We currently aren't using the extra data fields */
			char extra_data[64];
			int data_type;
			fread_return = fread(&extra_data, 64, 1, history_file);
			if (fread(&size, 4, 1, history_file) != 1)
				size = 0;
			if (fread(&data_type, 4, 1, history_file) != 1)
				data_type = 0;
			while (size && data_type) {
				switch (data_type) {
					case 1: {
						gchar *item = (gchar*)g_malloc(size + 1);
						fread_return = fread(item, size, 1, history_file);
						item[size] = '\0';
						history_item *hitem = initialize_history_item();
						hitem->content = item;
						history = g_list_prepend(history, hitem);
						break;
					}
					case 2: {
						int read_static;
						history_item *hitem = history->data;
						fread_return = fread(&read_static, size, 1, history_file);
						hitem->is_static = read_static;
						break;
					}
				}
				if (fread(&size, 4, 1, history_file) != 1)
					size = 0;
				if (fread(&data_type, 4, 1, history_file) != 1)
					data_type = 0;
			}
		} else {
			/* Continue reading until size is 0 */
			while (size) {
				/* Malloc according to the size of the item */
				gchar *item = (gchar*)g_malloc(size + 1);
				/* Read item and add ending character */
				fread_return = fread(item, size, 1, history_file);
				item[size] = '\0';
				/* Prepend item and read next size */
				history_item *hitem = initialize_history_item();
				hitem->content = item;
				history = g_list_prepend(history, hitem);
				if (fread(&size, 4, 1, history_file) != 1)
					size = 0;
			}
		}
		/* Close file and reverse the history to normal */
		fclose(history_file);
		history = g_list_reverse(history);
	}
}

/* Saves history to DATADIR/clipit/history */
void save_history()
{
	if(prefs.save_history) {
		/* Check that the directory is available */
		check_dirs();
		/* Build file path */
		gchar *history_path = g_build_filename(g_get_user_data_dir(),
							HISTORY_FILE,
							NULL);
		/* Open the file for writing */
		FILE *history_file = fopen(history_path, "wb");
		g_free(history_path);
		/* Check that it opened and begin write */
		if (history_file) {
			GList *element;
			/* Write each element to a binary file */
			int i;
			for (i=1; i<=17; i++) {
				int write_val = -1;
				fwrite(&write_val, 4, 1, history_file);
			}
			for (element = history; element != NULL; element = element->next) {
				/* Create new GString from element data, write its
				 * length (size) to file followed by the element
				 * data itself
				 */
				int write_val = 1;
				history_item *elem_data = element->data;
				GString *item = g_string_new((gchar*)elem_data->content);
				int write_static = elem_data->is_static;
				fwrite(&(item->len), 4, 1, history_file);
				fwrite(&write_val, 4, 1, history_file);
				fputs(item->str, history_file);
				write_val = 4;
				fwrite(&write_val, 4, 1, history_file);
				write_val = 2;
				fwrite(&write_val, 4, 1, history_file);
				fwrite(&write_static, 4, 1, history_file);
				g_string_free(item, TRUE);
			}
			/* Write 0 to indicate end of file */
			gint end = 0;
			fwrite(&end, 4, 1, history_file);
			fwrite(&end, 4, 1, history_file);
			fclose(history_file);
		}
	}
	/* Refresh indicator menu. Temporary solution until
	 * getting the visible status of the menu is supported by the API
	 */
#ifdef HAVE_APPINDICATOR
	create_app_indicator(0);
#endif
}

/* Checks if item should be included in history and calls append */
void check_and_append(gchar *item)
{
	if (item) {
		/* if item is too big, we don't include it in the history */
		if(strlen(item) > ENTRY_MAX_SIZE)
			return;
		GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		/* Check if we have URIs */
		if((gtk_clipboard_wait_for_uris(clip) != NULL) && !prefs.save_uris)
			/* We have URIs but the user doesn't want to save them */
			return;
		if(is_excluded(item))
			/* This item is excluded from the history */
			return;
		if (prefs.hyperlinks_only && !is_hyperlink(item))
			/* User only wants hyperlinks, but this isn't one */
			return;

		GList *duplicate_elem = find_duplicate(item);
		if (duplicate_elem) {
			history = g_list_remove_link(history, duplicate_elem);
			history = g_list_concat(duplicate_elem, history);
		} else {
			append_item(item);
		}
	}
}

/* Adds item to the end of history */
void append_item(gchar *item)
{
	history_item *hitem = initialize_history_item();
	hitem->content = g_strdup(item);
	history = g_list_prepend(history, hitem);
	truncate_history();
}

/* Truncates history to history_limit items */
void truncate_history()
{
	if (history) {
		/* Shorten history if necessary */
		GList *last_element = g_list_nth (history, prefs.history_limit);
		while (last_element) {
			history_item *elem_data = last_element->data;
			last_element = last_element->next;
			if (elem_data->is_static == 0)
				history = g_list_remove(history, elem_data);
		}
		
		last_element = g_list_nth (history, prefs.history_limit - 1);
		/* last_element->next checks if the list is longer than the user
		 * wants it; last_element->prev checks if this isn't the first
		 * item, which we wouldn't want to delete */
		if (last_element) {
			while (last_element->next && last_element->prev) {
				history_item *elem_data = last_element->data;

				while (elem_data->is_static && last_element->prev) {
					if (last_element->prev->prev)
						last_element = last_element->prev;
					elem_data = last_element->data;
				}
				if (elem_data->is_static == 0)
					history = g_list_remove(history, elem_data);
				last_element = g_list_nth (history, prefs.history_limit - 1);
			}
		}
		/* Save changes */
		save_history();
	}
}

/* Returns pointer to last item in history */
gpointer get_last_item()
{
	if (history) {
		history_item *elem_data = history->data;
		if (elem_data->content) {
			/* Return the last element */
			gpointer last_item = elem_data->content;
			return last_item;
		}
		return NULL;
	}
	return NULL;
}

/* Deletes duplicate item in history */
void delete_duplicate(gchar *item)
{
	GList *element;
	/* Go through each element compare each */
	for (element = history; element != NULL; element = element->next) {
		history_item *elem_data = element->data;
		if (g_strcmp0((gchar*)elem_data->content, item) == 0) {
			g_free(elem_data->content);
			history = g_list_delete_link(history, element);
			break;
		}
	}
}

/* Deletes the most recent item in history */
void delete_latest_item()
{
	if (history) {
		GList *element = history;
		history = g_list_delete_link(history, element);
		save_history();
	}
}
