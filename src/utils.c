/* Copyright (C) 2010-2012 by Cristian Henzel <oss@rspwn.com>
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

#include <glib.h>
#include <gtk/gtk.h>
#include "main.h"
#include "utils.h"
#include "daemon.h"
#include "history.h"
#include "clipit-i18n.h"

/* Creates program related directories if needed */
void check_dirs()
{
	gchar *data_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_data_dir(), DATA_DIR, NULL);
	gchar *config_dir = g_build_path(G_DIR_SEPARATOR_S, g_get_user_config_dir(), CONFIG_DIR, NULL);
	/* Check if data directory exists */
	if (!g_file_test(data_dir, G_FILE_TEST_EXISTS))
	{
		/* Try to make data directory */
		if (g_mkdir_with_parents(data_dir, 0755) != 0)
			g_warning(_("Couldn't create directory: %s\n"), data_dir);
	}
	/* Check if config directory exists */
	if (!g_file_test(config_dir, G_FILE_TEST_EXISTS))
	{
		/* Try to make config directory */
		if (g_mkdir_with_parents(config_dir, 0755) != 0)
			g_warning(_("Couldn't create directory: %s\n"), config_dir);
	}
	/* Cleanup */
	g_free(data_dir);
	g_free(config_dir);
}

/* Returns TRUE if text is a hyperlink */
gboolean is_hyperlink(gchar *text)
{
	/* TODO: I need a better regex, this one is poor */
	GRegex *regex = g_regex_new("([A-Za-z][A-Za-z0-9+.-]{1,120}:[A-Za-z0-9/]" \
			"(([A-Za-z0-9$_.+!*,;/?:@&~=-])|%[A-Fa-f0-9]{2}){1,333}" \
			"(#([a-zA-Z0-9][a-zA-Z0-9$_.+!*,;/?:@&~=%-]{0,1000}))?)",
			G_REGEX_CASELESS, 0, NULL);

	gboolean result = g_regex_match(regex, text, 0, NULL);
	g_regex_unref(regex);
	return result;
}

/* Ellipsize a string according to the settings */
GString *ellipsize_string(GString *string)
{
    gboolean is_utf8 = g_utf8_validate(string->str, -1, NULL);
    gint str_len = is_utf8 ? g_utf8_strlen(string->str, -1) : string->len;
    if (string->len > prefs.item_length)
    {
        switch (prefs.ellipsize)
        {
            case PANGO_ELLIPSIZE_START:
                if (is_utf8)
                {
                    gchar *end = g_utf8_substring(string->str, str_len - prefs.item_length, str_len);
                    g_string_printf(string, "...%s", end);
                }
                else
                {
                    string = g_string_erase(string, 0, str_len - (prefs.item_length));
                    string = g_string_prepend(string, "...");
                }
                break;
            case PANGO_ELLIPSIZE_MIDDLE:
                if (is_utf8)
                {
                    gchar *start = g_utf8_substring(string->str, 0, prefs.item_length/2);
                    gchar *end = g_utf8_substring(string->str, str_len - prefs.item_length/2, str_len);
                    g_string_printf(string, "%s...%s", start, end);
                }
                else
                {
                    string = g_string_erase(string, (prefs.item_length/2), str_len - (prefs.item_length));
                    string = g_string_insert(string, (string->len/2), "...");
                }
                break;
            case PANGO_ELLIPSIZE_END:
                if (is_utf8)
                {
                    gchar *buff = g_utf8_substring(string->str, 0, prefs.item_length);
                    g_string_assign(string, buff);
                }
                else
                {
                    string = g_string_truncate(string, prefs.item_length);
                }
                string = g_string_append(string, "...");
                break;
        }
    }
    return string;
}

/* Remove newlines from string */
GString *remove_newlines_string(GString *string)
{
	int i = 0;
	while (i < string->len)
	{
		if (string->str[i] == '\n')
		g_string_overwrite(string, i, " ");
		i++;
	}
	return string;
}

/* Parses the program arguments. Returns TRUE if program needs
 * to exit after parsing is complete
 */
gboolean parse_options(int argc, char* argv[])
{
	/* Declare argument options and argument variables */
	gboolean icon = FALSE, daemon = FALSE,
		clipboard = FALSE, primary = FALSE,
		exit = FALSE;

	GOptionEntry main_entries[] = 
	{
		{
			"daemon", 'd',
			0,
			G_OPTION_ARG_NONE,
			&daemon, _("Run as daemon"),
			NULL
		},
		{
			"no-icon", 'n',
			0,
			G_OPTION_ARG_NONE,
			&icon, _("Do not use status icon (Ctrl-Alt-P for menu)"),
			NULL
		},
		{
			"clipboard", 'c',
			0,
			G_OPTION_ARG_NONE,
			&clipboard, _("Print clipboard contents"),
			NULL
		},
		{
			"primary", 'p',
			0,
			G_OPTION_ARG_NONE,
			&primary, _("Print primary contents"),
			NULL
		},
		{
			NULL
		}
	};

	/* Option parsing */
	GOptionContext *context = g_option_context_new(NULL);
	/* Set summary */
	g_option_context_set_summary(context,
			_("Clipboard CLI usage examples:\n\n"
			"  echo \"copied to clipboard\" | clipit\n"
			"  clipit \"copied to clipboard\"\n"
			"  echo \"copied to clipboard\" | clipit -c"));
	/* Set description */
	g_option_context_set_description(context,
			_("Written by Cristian Henzel.\n"
			"Report bugs to <oss@rspwn.com>."));
	/* Add entries and parse options */
	g_option_context_add_main_entries(context, main_entries, NULL);
	g_option_context_parse(context, &argc, &argv, NULL);
	g_option_context_free(context);

	/* Check which options were parseed */

	/* Do not display icon option */
	if (icon)
	{
		prefs.no_icon = TRUE;
	}
	/* Run as daemon option */
	else if (daemon)
	{
		init_daemon_mode();
		exit = TRUE;
	}
	/* Print clipboard option */
	else if (clipboard)
	{
		/* Grab clipboard */
		GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

		/* Check if stdin has text to copy */
		if (!isatty(fileno(stdin)))
		{
			GString *piped_string = g_string_new(NULL);
			/* Append stdin to string */
			while (1)
			{
				gchar *buffer = (gchar*)g_malloc(256);
				if (fgets(buffer, 256, stdin) == NULL)
				{
					g_free(buffer);
					break;
				}
				g_string_append(piped_string, buffer);
				g_free(buffer);
			}
			/* Check if anything was piped in */
			if (piped_string->len > 0)
			{
				/* Copy to clipboard */
				gtk_clipboard_set_text(clip, piped_string->str, -1);
				gtk_clipboard_store(clip);
			}
			g_string_free(piped_string, TRUE);
		}
		/* Print clipboard text (if any) */
		gchar *clip_text = gtk_clipboard_wait_for_text(clip);
		if (clip_text)
			g_print("%s", clip_text);
		g_free(clip_text);
		
		/* Return true so program exits when finished parsing */
		exit = TRUE;
	}
	else if (primary)
	{
		/* Grab primary */
		GtkClipboard *prim = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
		/* Print primary text (if any) */
		gchar *prim_text = gtk_clipboard_wait_for_text(prim);
		if (prim_text)
			g_print("%s", prim_text);
		g_free(prim_text);

		/* Return true so program exits when finished parsing */
		exit = TRUE;
	}
	else
	{
		/* Copy from unrecognized options */
		gchar *argv_string = g_strjoinv(" ", argv + 1);
		GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_set_text(clip, argv_string, -1);
		gtk_clipboard_store(clip);
		g_free(argv_string);
		/* Return true so program exits when finished parsing */
		exit = TRUE;
	}
	return exit;
}

