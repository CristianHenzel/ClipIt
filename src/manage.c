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

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include "main.h"
#include "utils.h"
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "clipit-i18n.h"

GtkListStore* search_list;
GtkWidget *search_entry;
GtkWidget* treeview_search;

/* Search through the history */
static void search_history()
{
  guint16 search_len = gtk_entry_get_text_length((GtkEntry*)search_entry);
  /* Test if there is text in the search box */
  if(search_len > 0)
  {
    if ((history != NULL) && (history->data != NULL))
    {
      char* search_input = (char *)g_strdup(gtk_entry_get_text((GtkEntry*)search_entry));
      GtkTreeIter search_iter;
      while(gtk_tree_model_get_iter_first((GtkTreeModel*)search_list, &search_iter))
        gtk_list_store_remove(search_list, &search_iter);

      /* Declare some variables */
      GSList* element;
      gint element_number = 0;
      /* Go through each element and adding each */
      for (element = history; element != NULL; element = element->next)
      {
        GString* string = g_string_new((gchar*)element->data);
        gchar* strn_cmp_to = g_utf8_strdown(string->str, string->len);
        gchar* strn_find = g_utf8_strdown(search_input, search_len);
        char* result = strstr(strn_cmp_to, strn_find);
        if(result)
        {
          GtkTreeIter row_iter;
          gtk_list_store_append(search_list, &row_iter);
          string = ellipsize_string(string);
          string = remove_newlines_string(string);
          int row_num = g_slist_position(history, element);
          gtk_list_store_set(search_list, &row_iter, 0, row_num, 1, string->str, -1);
        }
        /* Prepare for next item */
        g_string_free(string, TRUE);
        element_number++;
      }
    }
  }
  else
  {
    /* If nothing is searched for, we show all items */
    if ((history != NULL) && (history->data != NULL))
    {
      GtkTreeIter search_iter;
      /* First, we remove all items */
      while(gtk_tree_model_get_iter_first((GtkTreeModel*)search_list, &search_iter))
        gtk_list_store_remove(search_list, &search_iter);

      /* Declare some variables */
      GSList *element;
      /* Go through each element and adding each */
      for (element = history; element != NULL; element = element->next)
      {
        GString *string = g_string_new((gchar*)element->data);
        GtkTreeIter row_iter;
        gtk_list_store_append(search_list, &row_iter);
        string = ellipsize_string(string);
        string = remove_newlines_string(string);
        int row_num = g_slist_position(history, element);
        gtk_list_store_set(search_list, &row_iter, 0, row_num, 1, string->str, -1);
        
        /* Prepare for next item */
        g_string_free(string, TRUE);
      }
    }
  }
}

/* Called when Edit is selected from Manage dialog */
static void edit_selected()
{
  GtkTreeIter sel_iter;
  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  /* This helps prevent multiple instances and checks if there's anything selected */
  if (gtk_tree_selection_get_selected(search_selection, NULL, &sel_iter))
  {
    /* Create clipboard buffer and set its text */
    gint selected_item_nr;
    gtk_tree_model_get((GtkTreeModel*)search_list, &sel_iter, 0, &selected_item_nr, -1);
    GSList* element = g_slist_nth(history, selected_item_nr);
    GString* s_selected_item = g_string_new((gchar*)element->data);
    GtkTextBuffer* clipboard_buffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(clipboard_buffer, s_selected_item->str, -1);
    /* Create the dialog */
    GtkWidget* dialog = gtk_dialog_new_with_buttons(_("Editing Clipboard"), NULL,
                                                   (GTK_DIALOG_MODAL   +    GTK_DIALOG_NO_SEPARATOR),
                                                    GTK_STOCK_CANCEL,       GTK_RESPONSE_REJECT,
                                                    GTK_STOCK_OK,           GTK_RESPONSE_ACCEPT, NULL);
    
    gtk_window_set_default_size((GtkWindow*)dialog, 450, 300);
    gtk_window_set_icon((GtkWindow*)dialog, gtk_widget_render_icon(dialog, GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU, NULL));
    
    /* Build the scrolled window with the text view */
    GtkWidget* scrolled_window = gtk_scrolled_window_new((GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0),
                                                         (GtkAdjustment*) gtk_adjustment_new(0, 0, 0, 0, 0, 0));
    
    gtk_scrolled_window_set_policy((GtkScrolledWindow*)scrolled_window,
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), scrolled_window, TRUE, TRUE, 2);
    GtkWidget* text_view = gtk_text_view_new_with_buffer(clipboard_buffer);
    gtk_text_view_set_left_margin((GtkTextView*)text_view, 2);
    gtk_text_view_set_right_margin((GtkTextView*)text_view, 2);
    gtk_container_add((GtkContainer*)scrolled_window, text_view);
    
    /* Run the dialog */
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run((GtkDialog*)dialog) == GTK_RESPONSE_ACCEPT)
    {
      /* Save changes done to the clipboard */
      GtkTextIter start, end;
      gtk_text_buffer_get_start_iter(clipboard_buffer, &start);
      gtk_text_buffer_get_end_iter(clipboard_buffer, &end);

      /* Delete any duplicate */
      delete_duplicate(gtk_text_buffer_get_text(clipboard_buffer, &start, &end, TRUE));

      /* Insert new element before the old one */
      history = g_slist_insert_before(history, element->next,
                    g_strdup(gtk_text_buffer_get_text(clipboard_buffer, &start, &end, TRUE)));

      /* Remove old entry */
      history = g_slist_remove(history, element->data);

      if(selected_item_nr == 0)
      {
        GtkClipboard* clipboard;
        clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(clipboard, 
                               gtk_text_buffer_get_text(clipboard_buffer, &start, &end, TRUE),
                               -1);
      }
    }
    gtk_widget_destroy(dialog);
    g_string_free(s_selected_item, TRUE);
    search_history();
  }
}

/* Called when Remove is selected from Manage dialog */
static void remove_selected()
{
  GtkTreeIter sel_iter;
  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  /* This checks if there's anything selected */
  if (gtk_tree_selection_get_selected(search_selection, NULL, &sel_iter))
  {
    /* Get item to delete */
    gint selected_item_nr;
    gtk_tree_model_get((GtkTreeModel*)search_list, &sel_iter, 0, &selected_item_nr, -1);
    GSList* element = g_slist_nth(history, selected_item_nr);
    /* Remove entry */
    history = g_slist_remove(history, element->data);
    search_history();
  }
}

/* Called when Remove all is selected from Manage dialog */
static void remove_all_selected(gpointer user_data)
{
  /* Check for confirm clear option */
  if (prefs.confirm_clear)
  {
    GtkWidget* confirm_dialog = gtk_message_dialog_new(NULL,
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_OTHER,
                                                       GTK_BUTTONS_OK_CANCEL,
                                                       _("Clear the history?"));
    gtk_window_set_title((GtkWindow*)confirm_dialog, _("Clear history"));
    
    if (gtk_dialog_run((GtkDialog*)confirm_dialog) == GTK_RESPONSE_OK)
    {
      /* Clear history and free history-related variables */
      g_slist_free(history);
      history = NULL;
      save_history();
      clear_main_data();
      GtkTreeIter search_iter;
      while(gtk_tree_model_get_iter_first((GtkTreeModel*)search_list, &search_iter))
        gtk_list_store_remove(search_list, &search_iter);
    }
    gtk_widget_destroy(confirm_dialog);
  }
  else
  {
    /* Clear history and free history-related variables */
    g_slist_free(history);
    history = NULL;
    save_history();
    clear_main_data();
    GtkTreeIter search_iter;
    while(gtk_tree_model_get_iter_first((GtkTreeModel*)search_list, &search_iter))
      gtk_list_store_remove(search_list, &search_iter);
  }
}

static void search_doubleclick()
{
  GtkTreeIter sel_iter;
  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  /* Check if selected */
  if (gtk_tree_selection_get_selected(search_selection, NULL, &sel_iter))
  {
    gint selected_item_nr;
    gtk_tree_model_get((GtkTreeModel*)search_list, &sel_iter, 0, &selected_item_nr, -1);
    GSList *element = g_slist_nth(history, selected_item_nr);
    GtkClipboard* prim = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(prim, (gchar*)element->data, -1);
    gtk_clipboard_set_text(clip, (gchar*)element->data, -1);
  }
}

static gint search_click(GtkWidget *widget, GdkEventButton *event, GtkWidget *search_window)
{
  if(event->type==GDK_2BUTTON_PRESS || event->type==GDK_3BUTTON_PRESS)
  {
    search_doubleclick();
    gtk_widget_destroy(search_window);
  }
  return FALSE;
}

static gint search_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  /* Check if [Return] key was pressed */
  if ((event->keyval == 0xff0d) || (event->keyval == 0xff8d))
    search_history();
  return FALSE;
}

void search_window_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if(response_id < 0)
  {
    save_history();
    gtk_widget_destroy((GtkWidget*)dialog);
  }
}

/* Shows the search dialog */
gboolean show_search()
{
  /* Prevent multiple instances */
  if(gtk_grab_get_current())
    return FALSE;
  /* Declare some variables */
  GtkWidget *hbox;

  GtkTreeViewColumn *tree_column;
  
  /* Create the dialog */
  GtkWidget* search_dialog = gtk_dialog_new();

  gtk_window_set_icon((GtkWindow*)search_dialog, gtk_widget_render_icon(search_dialog, GTK_STOCK_FIND, GTK_ICON_SIZE_MENU, NULL));
  gtk_window_set_title((GtkWindow*)search_dialog, "Manage History");
  gtk_window_set_resizable((GtkWindow*)search_dialog, TRUE);
  gtk_window_set_position((GtkWindow*)search_dialog, GTK_WIN_POS_CENTER);

  GtkWidget* vbox_search = gtk_vbox_new(FALSE, 10);
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
  treeview_search = gtk_tree_view_new();
  gtk_tree_view_set_rules_hint((GtkTreeView*)treeview_search, TRUE);
  search_list = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
  gtk_tree_view_set_model((GtkTreeView*)treeview_search, (GtkTreeModel*)search_list);
  GtkCellRenderer* cell_renderer = gtk_cell_renderer_text_new();
  tree_column = gtk_tree_view_column_new_with_attributes("ID", cell_renderer, NULL);
  gtk_tree_view_column_set_visible(tree_column, FALSE);
  gtk_tree_view_append_column((GtkTreeView*)treeview_search, tree_column);
  cell_renderer = gtk_cell_renderer_text_new();
  g_object_set(cell_renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  g_object_set(cell_renderer, "single-paragraph-mode", TRUE, NULL);
  tree_column = gtk_tree_view_column_new_with_attributes("Results", cell_renderer, "text", 1, NULL);
  gtk_tree_view_append_column((GtkTreeView*)treeview_search, tree_column);
  gtk_container_add((GtkContainer*)scrolled_window_search, treeview_search);
  gtk_box_pack_start((GtkBox*)vbox_search, scrolled_window_search, TRUE, TRUE, 0);

  GtkWidget* edit_button = gtk_dialog_add_button((GtkDialog*)search_dialog, "Edit", 1);
  g_signal_connect((GObject*)edit_button, "clicked", (GCallback)edit_selected, NULL);
  GtkWidget* remove_button = gtk_dialog_add_button((GtkDialog*)search_dialog, "Remove", 1);
  g_signal_connect((GObject*)remove_button, "clicked", (GCallback)remove_selected, NULL);
  GtkWidget* remove_all_button = gtk_dialog_add_button((GtkDialog*)search_dialog, "Remove all", 1);
  g_signal_connect((GObject*)remove_all_button, "clicked", (GCallback)remove_all_selected, NULL);
  GtkWidget* close_button = gtk_dialog_add_button((GtkDialog*)search_dialog, "Close", GTK_RESPONSE_OK);
  g_signal_connect((GObject*)close_button, "clicked", (GCallback)search_history, NULL);

  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  gtk_tree_selection_set_mode(search_selection, GTK_SELECTION_BROWSE);
  g_signal_connect((GObject*)treeview_search, "button_press_event", (GCallback)search_click, search_dialog);

  g_signal_connect((GtkDialog*)search_dialog, "response", (GCallback)search_window_response, search_dialog);
  gtk_widget_show_all(vbox_search);

  /* show the dialog */
  gtk_widget_show_all((GtkWidget*)search_dialog);
  gtk_widget_set_sensitive((GtkWidget*)search_dialog, TRUE);
  gtk_grab_add((GtkWidget*)search_dialog);

  /* Populate the list */
  search_history();

  return FALSE;
}
