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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "preferences.h"

#ifndef MAIN_H
#define MAIN_H

G_BEGIN_DECLS

#define ACTIONS_TAB    2
#define POPUP_DELAY    30
#define CHECK_INTERVAL 500

typedef struct {
  gboolean  use_copy;         /* Use copy */
  gboolean  use_primary;      /* Use primary */
  gboolean  synchronize;      /* Synchronize copy and primary */
  gboolean  automatic_paste;  /* Automatically paste entry after selecting it */
  gboolean  show_indexes;     /* Show index numbers in history menu */
  gboolean  save_uris;        /* Save URIs in history */
  gboolean  use_rmb_menu;     /* Use Right-Mouse-Button Menu */

  gboolean  save_history;     /* Save history */
  gint      history_limit;    /* Items in history */

  gint      items_menu;       /* Items in history menu */
  gboolean  statics_show;     /* Show statics items in history menu */
  gint      statics_items;    /* Static items in history menu */

  gboolean  hyperlinks_only;  /* Hyperlinks only */
  gboolean  confirm_clear;    /* Confirm clear */

  gboolean  single_line;      /* Show in a single line */
  gboolean  reverse_history;  /* Show in reverse order */
  gint      item_length;      /* Length of items */

  gint      ellipsize;        /* Omitting */

  gchar*    history_key;      /* History menu hotkey */
  gchar*    actions_key;      /* Actions menu hotkey */
  gchar*    menu_key;         /* ClipIt menu hotkey */
  gchar*    search_key;       /* ClipIt search hotkey */

  gboolean  no_icon;          /* No icon */
} prefs_t;

extern prefs_t prefs;

void history_hotkey(char *keystring, gpointer user_data);

void actions_hotkey(char *keystring, gpointer user_data);

void menu_hotkey(char *keystring, gpointer user_data);

void search_hotkey(char *keystring, gpointer user_data);

void clear_main_data();

#ifdef HAVE_APPINDICATOR
void create_app_indicator(gint create);
#endif

G_END_DECLS

#endif
