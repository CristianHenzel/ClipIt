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

#include <glib.h>
#include <gtk/gtk.h>
#include "main.h"
#include "utils.h"
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "clipit-i18n.h"

GtkListStore* search_list;
GtkWidget *search_entry;
GtkTreeSelection* search_selection;

static void
search_doubleclick()
{
  GtkTreeIter sel_iter;
  /* Check if selected */
  if (gtk_tree_selection_get_selected(search_selection, NULL, &sel_iter))
  {
    gchar *selected_item;
    gtk_tree_model_get((GtkTreeModel*)search_list, &sel_iter, 0, &selected_item, -1);
    GString* s_selected_item = g_string_new(selected_item);
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clip, (gchar*)selected_item, -1);
    g_free(selected_item);
    g_string_free(s_selected_item, TRUE);
  }
}

gint
search_click(GtkWidget *widget, GdkEventButton *event, GtkWidget *search_window)
{
  if(event->type==GDK_2BUTTON_PRESS || event->type==GDK_3BUTTON_PRESS)
  {
    search_doubleclick();
    gtk_widget_destroy(search_window);
  }
  return FALSE;
}

/* Search through the history */
static void
search_history()
{
  guint16 search_len = gtk_entry_get_text_length((GtkEntry*)search_entry);
  gchar* search_input = g_strdup(gtk_entry_get_text((GtkEntry*)search_entry));
  gchar* search_reg = g_regex_escape_string(search_input, search_len);
  gchar* search_regex = g_strconcat (".*", search_reg, ".*", NULL);
  GRegex* regex = g_regex_new(search_regex, G_REGEX_CASELESS, 0, NULL);
  GtkTreeIter search_iter;
  while(gtk_tree_model_get_iter_first((GtkTreeModel*)search_list, &search_iter))
    gtk_list_store_remove(search_list, &search_iter);
  /* Test if there is text in the search box */
  if(search_len > 0)
  {
    if ((history != NULL) && (history->data != NULL))
    {
      /* Declare some variables */
      GSList* element;
      gint element_number = 0;
      /* Go through each element and adding each */
      for (element = history; element != NULL; element = element->next)
      {
        GString* string = g_string_new((gchar*)element->data);
	gchar* compare = string->str;
        gboolean result = g_regex_match(regex, compare, 0, NULL);
        if(result)
        {
          GtkTreeIter row_iter;
          gtk_list_store_append(search_list, &row_iter);
          gtk_list_store_set(search_list, &row_iter, 0, string->str, -1);
        }
        /* Prepare for next item */
        g_string_free(string, TRUE);
        element_number++;
      }
    }
    g_regex_unref(regex);
  }
  g_free(search_regex);
}

static gint
search_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  /* Check if [Return] key was pressed */
  if ((event->keyval == 0xff0d) || (event->keyval == 0xff8d))
    search_history();
  return FALSE;
}

/* Shows the search dialog */
gboolean
show_search()
{
  /* Declare some variables */
  GtkWidget *frame,     *label,
            *alignment, *hbox,
            *vbox;

  GtkTreeViewColumn *tree_column;
  
  /* Create the dialog */
  GtkWidget* search_dialog = gtk_dialog_new_with_buttons(_("Search"),     NULL,
                                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_STOCK_CANCEL,   GTK_RESPONSE_CANCEL, NULL);

  gtk_window_set_icon((GtkWindow*)search_dialog, gtk_widget_render_icon(search_dialog, GTK_STOCK_FIND, GTK_ICON_SIZE_MENU, NULL));
  gtk_window_set_resizable((GtkWindow*)search_dialog, TRUE);
  gtk_window_set_position((GtkWindow*)search_dialog, GTK_WIN_POS_CENTER);

  GtkWidget* vbox_search = gtk_vbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(search_dialog))), vbox_search, TRUE, TRUE, 2);
  gtk_widget_set_size_request((GtkWidget*)vbox_search, 400, 600);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_box_pack_start((GtkBox*)vbox_search, hbox, FALSE, FALSE, 0);
  search_entry = gtk_entry_new();
  gtk_box_pack_end((GtkBox*)hbox, search_entry, TRUE, TRUE, 0);
  g_signal_connect((GObject*)search_entry, "key-press-event", (GCallback)search_key_pressed, NULL);

  /* Build the exclude treeview */
  GtkWidget* scrolled_window_search = gtk_scrolled_window_new(
                               (GtkAdjustment*)gtk_adjustment_new(0, 0, 0, 0, 0, 0),
                               (GtkAdjustment*)gtk_adjustment_new(0, 0, 0, 0, 0, 0));
  
  gtk_scrolled_window_set_policy((GtkScrolledWindow*)scrolled_window_search, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type((GtkScrolledWindow*)scrolled_window_search, GTK_SHADOW_ETCHED_OUT);
  GtkWidget* treeview_search = gtk_tree_view_new();
  gtk_tree_view_set_reorderable((GtkTreeView*)treeview_search, TRUE);
  gtk_tree_view_set_rules_hint((GtkTreeView*)treeview_search, TRUE);
  search_list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model((GtkTreeView*)treeview_search, (GtkTreeModel*)search_list);
  GtkCellRenderer* name_renderer_exclude = gtk_cell_renderer_text_new();
  g_object_set(name_renderer_exclude, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_object_set(name_renderer_exclude, "single-paragraph-mode", TRUE, NULL);
  tree_column = gtk_tree_view_column_new_with_attributes(_("Results"), name_renderer_exclude, "text", 0, NULL);
  gtk_tree_view_column_set_resizable(tree_column, TRUE);
  gtk_tree_view_append_column((GtkTreeView*)treeview_search, tree_column);
  gtk_container_add((GtkContainer*)scrolled_window_search, treeview_search);
  gtk_box_pack_start((GtkBox*)vbox_search, scrolled_window_search, TRUE, TRUE, 0);

  search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  gtk_tree_selection_set_mode(search_selection, GTK_SELECTION_BROWSE);
  g_signal_connect((GObject*)treeview_search, "button_press_event", (GCallback)search_click, search_dialog);

  g_signal_connect((GtkDialog*)search_dialog, "response", (GCallback)gtk_widget_destroy, search_dialog);
  gtk_widget_show_all(vbox_search);

  /* show the dialog */
  gtk_widget_show_all((GtkWidget*)search_dialog);

  return FALSE;
}
