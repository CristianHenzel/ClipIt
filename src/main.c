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
#include <stdlib.h>
#include <gtk/gtk.h>
#include <X11/keysym.h>
#ifdef HAVE_APPINDICATOR
#include <libappindicator/app-indicator.h>
#endif
#include "main.h"
#include "utils.h"
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "manage.h"
#include "clipit-i18n.h"


static gchar* primary_text;
static gchar* clipboard_text;
static gchar* synchronized_text;
static GtkClipboard* primary;
static GtkClipboard* clipboard;
#ifdef HAVE_APPINDICATOR
static AppIndicator *indicator;
static GtkWidget *indicator_menu = NULL;
#else
static GtkStatusIcon *status_icon;
static GtkWidget *statusicon_menu = NULL;
static gboolean status_menu_lock = FALSE;
#endif

static gboolean actions_lock = FALSE;

/* Init preferences structure */
prefs_t prefs = {DEF_USE_COPY,         DEF_USE_PRIMARY,      DEF_SYNCHRONIZE,
                 DEF_AUTOMATIC_PASTE,  DEF_SHOW_INDEXES,     DEF_SAVE_URIS,
                 DEF_USE_RMB_MENU,     DEF_SAVE_HISTORY,     DEF_HISTORY_LIMIT,
                 DEF_ITEMS_MENU,       DEF_STATICS_SHOW,     DEF_STATICS_ITEMS,
                 DEF_HYPERLINKS_ONLY,  DEF_CONFIRM_CLEAR,    DEF_SINGLE_LINE,
                 DEF_REVERSE_HISTORY,  DEF_ITEM_LENGTH,      DEF_ELLIPSIZE,
                 INIT_HISTORY_KEY,     INIT_ACTIONS_KEY,     INIT_MENU_KEY,
                 INIT_SEARCH_KEY,      INIT_OFFLINE_KEY,     DEF_NO_ICON,
                 DEF_OFFLINE_MODE};

/* Called every CHECK_INTERVAL seconds to check for new items */
static gboolean item_check(gpointer data) {
  /* Immediately return in offline mode */
  if (prefs.offline_mode)
    return TRUE;

  /* Grab the current primary and clipboard text */
  gchar* primary_temp = gtk_clipboard_wait_for_text(primary);
  gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);
  
  /* What follows is an extremely confusing system of tests and crap... */
  
  /* Check if primary contents were lost */
  if ((primary_temp == NULL) && (primary_text != NULL))
  {
    /* Check contents */
    gint count;
    GdkAtom *targets;
    gboolean contents = gtk_clipboard_wait_for_targets(primary, &targets, &count);
    g_free(targets);
    /* Only recover lost contents if there isn't any other type of content in the clipboard */
    if (!contents)
    {
      if(prefs.use_primary)
      {
        /* if use_primary is enabled, we restore from primary */
        gtk_clipboard_set_text(primary, primary_text, -1);
      }
      /* else
       * {
       *  // else, we restore from history
       *  GList* element = g_list_nth(history, 0);
       *  gtk_clipboard_set_text(primary, (gchar*)element->data, -1);
       * }
       */
    }
  }
  else
  {
    GdkModifierType button_state;
    gdk_window_get_pointer(NULL, NULL, NULL, &button_state);
    /* Proceed if mouse button not being held */
    if ((primary_temp != NULL) && !(button_state & GDK_BUTTON1_MASK))
    {
      /* Check if primary is the same as the last entry */
      if (g_strcmp0(primary_temp, primary_text) != 0)
      {
        /* New primary entry */
        g_free(primary_text);
        primary_text = g_strdup(primary_temp);
        /* Check if primary option is enabled and if there's text to add */
        if (prefs.use_primary && primary_text)
        {
          check_and_append(primary_text);
        }
      }
    }
  }

  /* Check if clipboard contents were lost */
  if ((clipboard_temp == NULL) && (clipboard_text != NULL))
  {
    /* Check contents */
    gint count;
    GdkAtom *targets;
    gboolean contents = gtk_clipboard_wait_for_targets(clipboard, &targets, &count);
    g_free(targets);
    /* Only recover lost contents if there isn't any other type of content in the clipboard */
    if (!contents)
    {
      g_print("Clipboard is null, recovering ...\n");
      gtk_clipboard_set_text(clipboard, clipboard_text, -1);
    }
  }
  else
  {
    /* Check if clipboard is the same as the last entry */
    if (g_strcmp0(clipboard_temp, clipboard_text) != 0)
    {
      /* New clipboard entry */
      g_free(clipboard_text);
      clipboard_text = g_strdup(clipboard_temp);
      /* Check if clipboard option is enabled and if there's text to add */
      if (prefs.use_copy && clipboard_text)
      {
        check_and_append(clipboard_text);
      }
    }
  }

  /* Synchronization */
  if (prefs.synchronize)
  {
    if (g_strcmp0(synchronized_text, primary_text) != 0)
    {
      g_free(synchronized_text);
      synchronized_text = g_strdup(primary_text);
      gtk_clipboard_set_text(clipboard, primary_text, -1);
    }
    else if (g_strcmp0(synchronized_text, clipboard_text) != 0)
    {
      g_free(synchronized_text);
      synchronized_text = g_strdup(clipboard_text);
      gtk_clipboard_set_text(primary, clipboard_text, -1);
    }
  }

  /* Cleanup */
  g_free(primary_temp);
  g_free(clipboard_temp);

  return TRUE;
}

/* Called when execution action exits */
static void action_exit(GPid pid, gint status, gpointer data) {
  g_spawn_close_pid(pid);
  actions_lock = FALSE;
}

/* Called when an action is selected from actions menu */
static void action_selected(GtkButton *button, gpointer user_data) {
  /* Enable lock */
  actions_lock = TRUE;

  /* Insert clipboard into command (user_data), and prepare it for execution */
  gchar* clipboard_text = gtk_clipboard_wait_for_text(clipboard);
  gchar* command = g_markup_printf_escaped((gchar*)user_data, clipboard_text);
  g_free(clipboard_text);
  g_free(user_data);
  gchar* shell_command = g_shell_quote(command);
  g_free(command);
  gchar* cmd = g_strconcat("/bin/sh -c ", shell_command, NULL);
  g_free(shell_command);

  /* Execute action */
  GPid pid;
  gchar **argv;
  g_shell_parse_argv(cmd, NULL, &argv, NULL);
  g_free(cmd);
  g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
  g_child_watch_add(pid, (GChildWatchFunc)action_exit, NULL);
  g_strfreev(argv);
}

/* Called when Edit Actions is selected from actions menu */
static void edit_actions_selected(GtkButton *button, gpointer user_data) {
  /* Show the preferences dialog on the actions tab */
  show_preferences(ACTIONS_TAB);
}

/* Called when an item is selected from history menu */
static void item_selected(GtkMenuItem *menu_item, gpointer user_data) {
  /* Get the text from the right element and set as clipboard */
  GList* element = g_list_nth(history, GPOINTER_TO_INT(user_data));
  history = g_list_remove_link(history, element);
  history = g_list_concat(element, history);
  history_item *elem_data = history->data;
  gtk_clipboard_set_text(clipboard, (gchar*)elem_data->content, -1);
  gtk_clipboard_set_text(primary, (gchar*)elem_data->content, -1);
  save_history();
  /* Paste the clipboard contents automatically if enabled */
  if (prefs.automatic_paste) {
    gchar* cmd = g_strconcat("/bin/sh -c 'xdotool key ctrl+v'", NULL);
    GPid pid;
    gchar **argv;
    g_shell_parse_argv(cmd, NULL, &argv, NULL);
    g_free(cmd);
    g_spawn_async(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL);
    g_child_watch_add(pid, (GChildWatchFunc)action_exit, NULL);
    g_strfreev(argv);
  }
}

/* Clears all local data (specific to main.c) */
void clear_main_data() {
  g_free(primary_text);
  g_free(clipboard_text);
  g_free(synchronized_text);
  primary_text = g_strdup("");
  clipboard_text = g_strdup("");
  synchronized_text = g_strdup("");
  gtk_clipboard_set_text(primary, "", -1);
  gtk_clipboard_set_text(clipboard, "", -1);
}

/* Clear history and free history-related variables */
void clear_main_history() {
  g_list_free(history);
  history = NULL;
  save_history();
  clear_main_data();
}

/* Called when About is selected from right-click menu */
static void show_about_dialog(GtkMenuItem *menu_item, gpointer user_data) {
  /* This helps prevent multiple instances */
  if (!gtk_grab_get_current()) {
    const gchar* authors[] = {"Cristian Henzel <oss@rspwn.com>\n"
				"Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>\n"
				"Eugene Nikolsky <pluton.od@gmail.com>", NULL};
    const gchar* license =
      "This program is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU General Public License as published by\n"
      "the Free Software Foundation; either version 3 of the License, or\n"
      "(at your option) any later version.\n"
      "\n"
      "This program is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program.  If not, see <http://www.gnu.org/licenses/>.";

    /* Create the about dialog */
    GtkWidget* about_dialog = gtk_about_dialog_new();
    gtk_window_set_icon((GtkWindow*)about_dialog,
                        gtk_widget_render_icon(about_dialog, GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU, NULL));

    gtk_about_dialog_set_name((GtkAboutDialog*)about_dialog, "ClipIt");
    #ifdef HAVE_CONFIG_H
    gtk_about_dialog_set_version((GtkAboutDialog*)about_dialog, VERSION);
    #endif
    gtk_about_dialog_set_comments((GtkAboutDialog*)about_dialog,
                                _("Lightweight GTK+ clipboard manager."));

    gtk_about_dialog_set_website((GtkAboutDialog*)about_dialog,
                                 "http://clipit.rspwn.com/");

    gtk_about_dialog_set_copyright((GtkAboutDialog*)about_dialog, "Copyright (C) 2010-2012 Cristian Henzel");
    gtk_about_dialog_set_authors((GtkAboutDialog*)about_dialog, authors);
    gtk_about_dialog_set_translator_credits ((GtkAboutDialog*)about_dialog,
                                             "Guido Tabbernuk <boamaod@gmail.com>\n"
                                             "Miloš Koutný <milos.koutny@gmail.com>\n"
                                             "Kim Jensen <reklamepost@energimail.dk>\n"
                                             "Eckhard M. Jäger <bart@neeneenee.de>\n"
                                             "Michael Stempin <mstempin@web.de>\n"
                                             "Benjamin 'sphax3d' Danon <sphax3d@gmail.com>\n"
                                             "Németh Tamás <ntomasz@vipmail.hu>\n"
                                             "Davide Truffa <davide@catoblepa.org>\n"
                                             "Jiro Kawada <jiro.kawada@gmail.com>\n"
                                             "Øyvind Sæther <oyvinds@everdot.org>\n"
                                             "pankamyk <pankamyk@o2.pl>\n"
                                             "Tomasz Rusek <tomek.rusek@gmail.com>\n"
                                             "Phantom X <megaphantomx@bol.com.br>\n"
                                             "Ovidiu D. Niţan <ov1d1u@sblug.ro>\n"
                                             "Alexander Kazancev <kazancas@mandriva.ru>\n"
                                             "Daniel Nylander <po@danielnylander.se>\n"
                                             "Hedef Türkçe <iletisim@hedefturkce.com>\n"
                                             "Lyman Li <lymanrb@gmail.com>\n"
                                             "Gilberto \"Xyhthyx\" Miralla <xyhthyx@gmail.com>");

    gtk_about_dialog_set_license((GtkAboutDialog*)about_dialog, license);
    gtk_about_dialog_set_logo_icon_name((GtkAboutDialog*)about_dialog, GTK_STOCK_PASTE);
    /* Run the about dialog */
    gtk_dialog_run((GtkDialog*)about_dialog);
    gtk_widget_destroy(about_dialog);
  } else {
    /* A window is already open, so we present it to the user */
    GtkWidget *toplevel = gtk_widget_get_toplevel(gtk_grab_get_current());
    gtk_window_present((GtkWindow*)toplevel);
  }
}

/* Called when "Clear history" is selected */
static void clear_history_selected(GtkMenuItem *menu_item, gpointer user_data) {
    clear_main_history();
}

/* Called when Preferences is selected from right-click menu */
static void preferences_selected(GtkMenuItem *menu_item, gpointer user_data) {
  /* Show the preferences dialog */
  show_preferences(0);
}

/* Called when Quit is selected from right-click menu */
static void quit_selected(GtkMenuItem *menu_item, gpointer user_data) {
  /* Prevent quit with dialogs open */
  if (!gtk_grab_get_current()) {
    /* Quit the program */
    gtk_main_quit();
  } else {
    /* A window is already open, so we present it to the user */
    GtkWidget *toplevel = gtk_widget_get_toplevel(gtk_grab_get_current());
    gtk_window_present((GtkWindow*)toplevel);
  }
}

/* Called when status icon is control-clicked */
static gboolean show_actions_menu(gpointer data) {
  /* Declare some variables */
  GtkWidget *menu,       *menu_item,
            *menu_image, *item_label;

  /* Create menu */
  menu = gtk_menu_new();
  g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL);
  /* Actions using: */
  menu_item = gtk_image_menu_item_new_with_label(_("Actions using:"));
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
  g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Clipboard contents */
  gchar* text = gtk_clipboard_wait_for_text(clipboard);
  if (text != NULL)
  {
    menu_item = gtk_menu_item_new_with_label(_("None"));
    /* Modify menu item label properties */
    item_label = gtk_bin_get_child((GtkBin*)menu_item);
    gtk_label_set_single_line_mode((GtkLabel*)item_label, TRUE);
    gtk_label_set_ellipsize((GtkLabel*)item_label, prefs.ellipsize);
    gtk_label_set_width_chars((GtkLabel*)item_label, 30);
    /* Making bold... */
    gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", text);
    gtk_label_set_markup((GtkLabel*)item_label, bold_text);
    g_free(bold_text);
    /* Append menu item */
    g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  else
  {
    /* Create menu item for empty clipboard contents */
    menu_item = gtk_menu_item_new_with_label(_("None"));
    /* Modify menu item label properties */
    item_label = gtk_bin_get_child((GtkBin*)menu_item);
    gtk_label_set_markup((GtkLabel*)item_label, _("<b>None</b>"));
    /* Append menu item */
    g_signal_connect((GObject*)menu_item, "select", (GCallback)gtk_menu_item_deselect, NULL);
    
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Actions */
  gchar* path = g_build_filename(g_get_user_data_dir(), ACTIONS_FILE, NULL);
  FILE* actions_file = fopen(path, "rb");
  g_free(path);
  /* Check that it opened and begin read */
  if (actions_file)
  {
    gint size;
    size_t fread_return;
    fread_return = fread(&size, 4, 1, actions_file);
    /* Check if actions file is empty */
    if (!size)
    {
      /* File contained no actions so adding empty */
      menu_item = gtk_menu_item_new_with_label(_("Empty"));
      gtk_widget_set_sensitive(menu_item, FALSE);
      gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
    }
    /* Continue reading items until size is 0 */
    while (size)
    {
      /* Read name */
      gchar* name = (gchar*)g_malloc(size + 1);
      fread_return = fread(name, size, 1, actions_file);
      name[size] = '\0';
      menu_item = gtk_menu_item_new_with_label(name);
      g_free(name);
      fread_return = fread(&size, 4, 1, actions_file);
      /* Read command */
      gchar* command = (gchar*)g_malloc(size + 1);
      fread_return = fread(command, size, 1, actions_file);
      command[size] = '\0';
      fread_return = fread(&size, 4, 1, actions_file);
      /* Append the action */
      gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
      g_signal_connect((GObject*)menu_item,        "activate",
                       (GCallback)action_selected, (gpointer)command);      
    }
    fclose(actions_file);
  }
  else
  {
    /* File did not open so adding empty */
    menu_item = gtk_menu_item_new_with_label(_("Empty"));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  }
  /* -------------------- */
  gtk_menu_shell_append((GtkMenuShell*)menu, gtk_separator_menu_item_new());
  /* Edit actions */
  menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Edit actions"));
  menu_image = gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
  g_signal_connect((GObject*)menu_item, "activate", (GCallback)edit_actions_selected, NULL);
  gtk_menu_shell_append((GtkMenuShell*)menu, menu_item);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  /* Return false so the g_timeout_add() function is called only once */
  return FALSE;
}

static gboolean menu_key_pressed(GtkWidget *history_menu, GdkEventKey *event, gpointer user_data) {
  switch (event->keyval) {
    case XK_1:
    case XK_KP_1:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(0));
      gtk_widget_destroy(history_menu);
      break;
    case XK_2:
    case XK_KP_2:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(1));
      gtk_widget_destroy(history_menu);
      break;
    case XK_3:
    case XK_KP_3:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(2));
      gtk_widget_destroy(history_menu);
      break;
    case XK_4:
    case XK_KP_4:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(3));
      gtk_widget_destroy(history_menu);
      break;
    case XK_5:
    case XK_KP_5:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(4));
      gtk_widget_destroy(history_menu);
      break;
    case XK_6:
    case XK_KP_6:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(5));
      gtk_widget_destroy(history_menu);
      break;
    case XK_7:
    case XK_KP_7:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(6));
      gtk_widget_destroy(history_menu);
      break;
    case XK_8:
    case XK_KP_8:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(7));
      gtk_widget_destroy(history_menu);
      break;
    case XK_9:
    case XK_KP_9:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(8));
      gtk_widget_destroy(history_menu);
      break;
    case XK_0:
    case XK_KP_0:
      item_selected((GtkMenuItem*)history_menu, GINT_TO_POINTER(9));
      gtk_widget_destroy(history_menu);
      break;
  }
  return FALSE;
}

static void toggle_offline_mode() {
	if (prefs.offline_mode) {
		/* Restore clipboard contents before turning offline mode off */
		gtk_clipboard_set_text(clipboard, clipboard_text != NULL ? clipboard_text : "", -1);
	}

	prefs.offline_mode = !prefs.offline_mode;
	/* Save the change */
	save_preferences();
}

static GtkWidget *create_history_menu(GtkWidget *history_menu) {
	GtkWidget *menu_item, *item_label;
	history_menu = gtk_menu_new();
	g_signal_connect((GObject*)history_menu, "key-press-event", (GCallback)menu_key_pressed, NULL);

	/* Items */
	if ((history != NULL) && (history->data != NULL))
	{
		/* Declare some variables */
		GList* element;
		gint element_number = 0;
		gint element_number_small = 0;
		gchar* primary_temp = gtk_clipboard_wait_for_text(primary);
		gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);
		/* Reverse history if enabled */
		if (prefs.reverse_history)
		{
			history = g_list_reverse(history);
			element_number = g_list_length(history) - 1;
		}
		/* Go through each element and adding each */
		for (element = history; (element != NULL) && (element_number_small < prefs.items_menu); element = element->next)
		{
			history_item *elem_data = element->data;
			GString* string = g_string_new((gchar*)elem_data->content);
			/* Ellipsize text */
			string = ellipsize_string(string);
			/* Remove control characters */
			string = remove_newlines_string(string);
			/* Make new item with ellipsized text */
			gchar* list_item;
			if (prefs.show_indexes)
			{
				list_item = g_strdup_printf("%d. %s", (element_number_small+1), string->str);
			} else {
				list_item = g_strdup(string->str);
			}
			menu_item = gtk_menu_item_new_with_label(list_item);
			g_signal_connect((GObject*)menu_item, "activate", (GCallback)item_selected, GINT_TO_POINTER(element_number));

			/* Modify menu item label properties */
			item_label = gtk_bin_get_child((GtkBin*)menu_item);
			gtk_label_set_single_line_mode((GtkLabel*)item_label, prefs.single_line);

			/* Check if item is also clipboard text and make bold */
			if ((clipboard_temp) && (g_strcmp0((gchar*)elem_data->content, clipboard_temp) == 0))
			{
				gchar* bold_text = g_markup_printf_escaped("<b>%s</b>", list_item);
				gtk_label_set_markup((GtkLabel*)item_label, bold_text);
				g_free(bold_text);
			}
			else if ((primary_temp) && (g_strcmp0((gchar*)elem_data->content, primary_temp) == 0))
			{
				gchar* italic_text = g_markup_printf_escaped("<i>%s</i>", list_item);
				gtk_label_set_markup((GtkLabel*)item_label, italic_text);
				g_free(italic_text);
			}
			g_free(list_item);
			/* Append item */
			gtk_menu_shell_append((GtkMenuShell*)history_menu, menu_item);
			/* Prepare for next item */
			g_string_free(string, TRUE);
			if (prefs.reverse_history)
				element_number--;
			else
				element_number++;
				element_number_small++;
		}
		/* Cleanup */
		g_free(primary_temp);
		g_free(clipboard_temp);
		/* Return history to normal if reversed */
		if (prefs.reverse_history)
			history = g_list_reverse(history);
	}
	else
	{
		/* Nothing in history so adding empty */
		menu_item = gtk_menu_item_new_with_label(_("Empty"));
		gtk_widget_set_sensitive(menu_item, FALSE);
		gtk_menu_shell_append((GtkMenuShell*)history_menu, menu_item);
	}
	if (prefs.statics_show) {
		/* -------------------- */
		gtk_menu_shell_append((GtkMenuShell*)history_menu, gtk_separator_menu_item_new());

		/* Items */
		GList *elem = history;
		int elem_num = 0;
		int elem_num_static = 1;
		while (elem && (elem_num_static <= prefs.statics_items)) {
			history_item *hitem = elem->data;
			if (hitem->is_static) {
				GString* string = g_string_new((gchar*)hitem->content);
				/* Ellipsize text */
				string = ellipsize_string(string);
				/* Remove control characters */
				string = remove_newlines_string(string);
				gchar* list_item;
				if (prefs.show_indexes)
				{
					list_item = g_strdup_printf("%d. %s", (elem_num_static), string->str);
				} else {
					list_item = g_strdup(string->str);
				}
				menu_item = gtk_menu_item_new_with_label(list_item);
				g_signal_connect((GObject*)menu_item, "activate", (GCallback)item_selected, GINT_TO_POINTER(elem_num));
				/* Modify menu item label properties */
				item_label = gtk_bin_get_child((GtkBin*)menu_item);
				gtk_label_set_single_line_mode((GtkLabel*)item_label, prefs.single_line);
				g_free(list_item);
				g_string_free(string, TRUE);
				gtk_menu_shell_append((GtkMenuShell*)history_menu, menu_item);
				elem_num_static++;
			}
			elem_num++;
			elem = elem->next;
		}
	}
	/* Show a notice in offline mode */
	if (prefs.offline_mode) {
		gtk_menu_shell_append((GtkMenuShell*)history_menu, gtk_separator_menu_item_new());

		menu_item = gtk_menu_item_new_with_label("");
		item_label = gtk_bin_get_child((GtkBin*)menu_item);
		gtk_label_set_markup((GtkLabel*)item_label, "<b>Offline mode is ON</b>");
		gtk_label_set_single_line_mode((GtkLabel*)item_label, TRUE);
		gtk_widget_set_sensitive(item_label, FALSE);
		gtk_menu_shell_append((GtkMenuShell*)history_menu, menu_item);
	}
	return history_menu;
}

/* Generates the history menu */
static gboolean show_history_menu(gpointer data) {
  /* Declare some variables */
  GtkWidget *menu = gtk_menu_new();
  menu = create_history_menu(menu);
  g_signal_connect((GObject*)menu, "selection-done", (GCallback)gtk_widget_destroy, NULL);
  /* Popup the menu... */
  gtk_widget_show_all(menu);
  gtk_menu_popup((GtkMenu*)menu, NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
  gtk_menu_shell_select_first((GtkMenuShell*)menu, TRUE);
  /* Return FALSE so the g_timeout_add() function is called only once */
  return FALSE;
}

static GtkWidget *create_tray_menu(GtkWidget *tray_menu, int menu_type) {
	GtkWidget *menu_item, *menu_image;

	if ((menu_type == 1) || (menu_type == 2)) {
		tray_menu = create_history_menu(tray_menu);
	} else {
		tray_menu = gtk_menu_new();
	}
	if (!prefs.use_rmb_menu || (menu_type == 2)) {
		/* -------------------- */
		gtk_menu_shell_append((GtkMenuShell*)tray_menu, gtk_separator_menu_item_new());
	}
	/* We show the options only if:
	 * - use_rmb_menu is active and menu_type is right-click, OR
	 * - use_rmb_menu is inactive and menu_type is left-click */
	if ((prefs.use_rmb_menu && (menu_type == 3)) || (!prefs.use_rmb_menu) || (menu_type == 2)) {
		/* Offline mode checkbox */
		menu_item = gtk_check_menu_item_new_with_mnemonic(_("_Offline mode"));
		gtk_check_menu_item_set_active((GtkCheckMenuItem*)menu_item, prefs.offline_mode);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)toggle_offline_mode, NULL);
		gtk_menu_shell_append((GtkMenuShell*)tray_menu, menu_item);
		/* About */
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)show_about_dialog, NULL);
		gtk_menu_shell_append((GtkMenuShell*)tray_menu, menu_item);
		/* Manage history */
		menu_item = gtk_image_menu_item_new_with_mnemonic(_("_Manage history"));
		menu_image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image((GtkImageMenuItem*)menu_item, menu_image);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)show_search, NULL);
		gtk_menu_shell_append((GtkMenuShell*)tray_menu, menu_item);
		/* Preferences */
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)preferences_selected, NULL);
		gtk_menu_shell_append((GtkMenuShell*)tray_menu, menu_item);
		/* Quit */
		menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
		g_signal_connect((GObject*)menu_item, "activate", (GCallback)quit_selected, NULL);
		gtk_menu_shell_append((GtkMenuShell*)tray_menu, menu_item);
	}
	/* Popup the menu... */
	gtk_widget_show_all(tray_menu);
	return tray_menu;
}

#ifdef HAVE_APPINDICATOR

void create_app_indicator(gint create) {
	/* Create the menu */
	indicator_menu = create_tray_menu(indicator_menu, 2);
	/* check if we need to create the indicator or just refresh the menu */
	if(create == 1) {
		indicator = app_indicator_new("clipit", "clipit-trayicon", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
		app_indicator_set_status (indicator, APP_INDICATOR_STATUS_ACTIVE);
		app_indicator_set_attention_icon (indicator, "clipit-trayicon");
	}
	app_indicator_set_menu (indicator, GTK_MENU (indicator_menu));
}

#else

/* Called when status icon is clicked */
static void show_clipit_menu(int menu_type) {
	/* If the menu is visible, we don't do anything, so that it gets hidden */
	if((statusicon_menu != NULL) && gtk_widget_get_visible((GtkWidget *)statusicon_menu))
		return;
	if(status_menu_lock)
		return;
	status_menu_lock = TRUE;
	/* Create the menu */
	statusicon_menu = create_tray_menu(statusicon_menu, menu_type);
	g_signal_connect((GObject*)statusicon_menu, "selection-done", (GCallback)gtk_widget_destroy, NULL);
	/* GENERATE THE MENU*/
	gtk_widget_set_visible(statusicon_menu, TRUE);
	gtk_menu_popup((GtkMenu*)statusicon_menu, NULL, NULL, gtk_status_icon_position_menu, status_icon, 1, gtk_get_current_event_time());
	gtk_menu_shell_select_first((GtkMenuShell*)statusicon_menu, TRUE);

	status_menu_lock = FALSE;
}

/* Called when status icon is clicked */
/* (checks type of click and calls correct function) */
static void status_icon_clicked(GtkStatusIcon *status_icon, GdkEventButton *event ) {
	/* Check what type of click was recieved */
	GdkModifierType state;
	gtk_get_current_event_state(&state);
	/* Control click */
	if (state == GDK_MOD2_MASK+GDK_CONTROL_MASK || state == GDK_CONTROL_MASK)
	{
		if (actions_lock == FALSE)
		{
			g_timeout_add(POPUP_DELAY, show_actions_menu, NULL);
		}
	}
	/* Left click */
	else if (event->button == 1)
	{
		show_clipit_menu(1);
	}
	else if (event->button == 3)
	{
		if (prefs.use_rmb_menu)
			show_clipit_menu(3);
	}
}

#endif

/* Called when history global hotkey is pressed */
void history_hotkey(char *keystring, gpointer user_data) {
	g_timeout_add(POPUP_DELAY, show_history_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void actions_hotkey(char *keystring, gpointer user_data) {
	g_timeout_add(POPUP_DELAY, show_actions_menu, NULL);
}

/* Called when actions global hotkey is pressed */
void menu_hotkey(char *keystring, gpointer user_data) {
#ifndef HAVE_APPINDICATOR
	show_clipit_menu(1);
#endif
}

/* Called when search global hotkey is pressed */
void search_hotkey(char *keystring, gpointer user_data) {
	g_timeout_add(POPUP_DELAY, show_search, NULL);
}

/* Called when offline mode global hotkey is pressed */
void offline_hotkey(char *keystring, gpointer user_data) {
	toggle_offline_mode();
}

/* Startup calls and initializations */
static void clipit_init() {
	/* Create clipboard */
	primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	g_timeout_add(CHECK_INTERVAL, item_check, NULL);

	/* Read preferences */
	read_preferences();

	/* Read history */
	if (prefs.save_history)
		read_history();

	/* Bind global keys */
	keybinder_init();
	keybinder_bind(prefs.history_key, history_hotkey, NULL);
	keybinder_bind(prefs.actions_key, actions_hotkey, NULL);
	keybinder_bind(prefs.menu_key, menu_hotkey, NULL);
	keybinder_bind(prefs.search_key, search_hotkey, NULL);
	keybinder_bind(prefs.offline_key, offline_hotkey, NULL);

	/* Create status icon */
	if (!prefs.no_icon)
	{
#ifdef HAVE_APPINDICATOR
	create_app_indicator(1);
#else
	status_icon = gtk_status_icon_new_from_icon_name("clipit-trayicon");
	gtk_status_icon_set_tooltip((GtkStatusIcon*)status_icon, _("Clipboard Manager"));
	g_signal_connect((GObject*)status_icon, "button_press_event", (GCallback)status_icon_clicked, NULL);
#endif
	}
}

/* This is Sparta! */
int main(int argc, char **argv) {
	bindtextdomain(GETTEXT_PACKAGE, CLIPITLOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	
	/* Initiate GTK+ */
	gtk_init(&argc, &argv);
	
	/* Parse options and exit if returns TRUE */
	if (argc > 1)
	{
		if (parse_options(argc, argv))
			return 0;
	}
	/* Read input from stdin (if any) */
	else
	{
		/* Check if stdin is piped */
		if (!isatty(fileno(stdin)))
		{
			GString* piped_string = g_string_new(NULL);
			/* Append stdin to string */
			while (1)
			{
				gchar* buffer = (gchar*)g_malloc(256);
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
				/* Truncate new line character */
				/* g_string_truncate(piped_string, (piped_string->len - 1)); */
				/* Copy to clipboard */
				GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
				gtk_clipboard_set_text(clip, piped_string->str, -1);
				gtk_clipboard_store(clip);
				/* Exit */
				return 0;
			}
			g_string_free(piped_string, TRUE);
		}
	}

	/* Init ClipIt */
	clipit_init();

	/* Run GTK+ loop */
	gtk_main();
	
	/* Unbind keys */
	keybinder_unbind(prefs.history_key, history_hotkey);
	keybinder_unbind(prefs.actions_key, actions_hotkey);
	keybinder_unbind(prefs.menu_key, menu_hotkey);
	keybinder_unbind(prefs.search_key, search_hotkey);
	keybinder_unbind(prefs.offline_key, offline_hotkey);
	/* Cleanup */
	g_free(prefs.history_key);
	g_free(prefs.actions_key);
	g_free(prefs.menu_key);
	g_free(prefs.search_key);
	g_free(prefs.offline_key);
	g_list_foreach(history, (GFunc)g_free, NULL);
	g_list_free(history);
	g_free(primary_text);
	g_free(clipboard_text);
	g_free(synchronized_text);

	/* Exit */
	return 0;
}
