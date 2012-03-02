/* Copyright (C) 2010-2012 by Cristian Henzel <oss@rspwn.com>
 * Copyright (C) 2011 by Eugene Nikolsky <pluton.od@gmail.com>
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

#ifndef PREFERENCES_H
#define PREFERENCES_H

G_BEGIN_DECLS

#define INIT_HISTORY_KEY      NULL
#define INIT_ACTIONS_KEY      NULL
#define INIT_MENU_KEY         NULL
#define INIT_SEARCH_KEY       NULL
#define INIT_OFFLINE_KEY      NULL

#define DEF_USE_COPY          TRUE
#define DEF_USE_PRIMARY       FALSE
#define DEF_SYNCHRONIZE       FALSE
#define DEF_AUTOMATIC_PASTE   FALSE
#define DEF_SHOW_INDEXES      FALSE
#define DEF_SAVE_URIS         TRUE
#define DEF_SAVE_HISTORY      TRUE
#define DEF_USE_RMB_MENU      FALSE
#define DEF_HISTORY_LIMIT     50
#define DEF_ITEMS_MENU        20
#define DEF_STATICS_SHOW      TRUE
#define DEF_STATICS_ITEMS     10
#define DEF_HYPERLINKS_ONLY   FALSE
#define DEF_CONFIRM_CLEAR     FALSE
#define DEF_SINGLE_LINE       TRUE
#define DEF_REVERSE_HISTORY   FALSE
#define DEF_ITEM_LENGTH       50
#define DEF_ELLIPSIZE         2
#define DEF_HISTORY_KEY       "<Ctrl><Alt>H"
#define DEF_ACTIONS_KEY       "<Ctrl><Alt>A"
#define DEF_MENU_KEY          "<Ctrl><Alt>P"
#define DEF_SEARCH_KEY        "<Ctrl><Alt>F"
#define DEF_OFFLINE_KEY       "<Ctrl><Alt>O"
#define DEF_NO_ICON           FALSE
#define DEF_OFFLINE_MODE      FALSE

#define ACTIONS_FILE          "clipit/actions"
#define EXCLUDES_FILE         "clipit/excludes"
#define PREFERENCES_FILE      "clipit/clipitrc"
#define THEMES_FOLDER         "clipit/themes"

#define SAVE_HIST_MESSAGE     "ClipIt can save your history in a plain text file. If you copy \
passwords or other sensitive data and other people have access to the computer, this might be a \
security risk. Do you want to enable history saving?"

#define CHECK_HIST_MESSAGE    "It appears that you have disabled history saving, the current history \
file still exists though. Do you want to empty the current history file?"

void read_preferences();

void save_preferences();

void show_preferences(gint tab);

G_END_DECLS

#endif
