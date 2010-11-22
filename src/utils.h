/* Copyright (C) 2010 by Cristian Henzel <oss@web-tm.com>
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

#ifndef UTILS_H
#define UTILS_H

G_BEGIN_DECLS

#define CONFIG_DIR  ".local/share/clipit"
#define DATA_DIR    ".config/clipit"

void check_dirs();

gboolean is_hyperlink(gchar* link);

GString *ellipsize_string(GString *string);

GString *remove_newlines_string(GString *string);

gboolean parse_options(int argc, char* argv[]);

G_END_DECLS

#endif
