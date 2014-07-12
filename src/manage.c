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

#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <X11/keysym.h>
#include "main.h"
#include "utils.h"
#include "history.h"
#include "keybinder.h"
#include "preferences.h"
#include "clipit-i18n.h"

GtkListStore *search_list;
GtkWidget *search_entry;
GtkWidget *treeview_search;

static void add_iter(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *piter, gpointer userdata)
{
  GArray *sel = (GArray*)userdata;
  GtkTreeIter iter = *piter;
  g_array_append_val(sel, iter);
}

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
      GList* element;
      gint element_number = 0;
      /* Go through each element and adding each */
      for (element = history; element != NULL; element = element->next)
      {
        history_item *elem_data = element->data;
        GString* string = g_string_new((gchar*)elem_data->content);
        gchar* strn_cmp_to = g_utf8_strdown(string->str, string->len);
        gchar* strn_find = g_utf8_strdown(search_input, search_len);
        char* result = strstr(strn_cmp_to, strn_find);
        if(result)
        {
          GtkTreeIter row_iter;
          gtk_list_store_append(search_list, &row_iter);
          string = ellipsize_string(string);
          string = remove_newlines_string(string);
          int row_num = g_list_position(history, element);
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
      GList *element;
      /* Go through each element and adding each */
      for (element = history; element != NULL; element = element->next)
      {
        history_item *elem_data = element->data;
        GString *string = g_string_new((gchar*)elem_data->content);
        GtkTreeIter row_iter;
        gtk_list_store_append(search_list, &row_iter);
        string = ellipsize_string(string);
        string = remove_newlines_string(string);
        int row_num = g_list_position(history, element);
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
  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  /* This checks if there's anything selected */
  gint selected_count = gtk_tree_selection_count_selected_rows(search_selection);
  if (selected_count > 0) {
    /* Create clipboard buffer and set its text */
    gint selected_item_nr;
    GArray *sel = g_array_new(FALSE, FALSE, sizeof(GtkTreeIter));
    gtk_tree_selection_selected_foreach(search_selection, add_iter, sel);
    gtk_tree_selection_unselect_all(search_selection);
    GtkTreeIter *iter = &g_array_index(sel, GtkTreeIter, 0);
    gtk_tree_model_get((GtkTreeModel*)search_list, iter, 0, &selected_item_nr, -1);
    g_array_free(sel, TRUE);
    GList *element = g_list_nth(history, selected_item_nr);
    history_item *elem_data = element->data;
    GList* elementafter = element->next;
    GtkTextBuffer* clipboard_buffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(clipboard_buffer, (gchar*)elem_data->content, -1);
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
    GtkWidget *static_check = gtk_check_button_new_with_mnemonic(_("_Static item"));
    gtk_toggle_button_set_active((GtkToggleButton*)static_check, elem_data->is_static);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), static_check, FALSE, FALSE, 2);
    
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

      /* Remove old entry */
      history = g_list_remove(history, elem_data);

      /* Insert new element where the old one was */
      history_item *hitem = g_new0(history_item, 1);
      hitem->content = g_strdup(gtk_text_buffer_get_text(clipboard_buffer, &start, &end, TRUE));
      hitem->is_static = gtk_toggle_button_get_active((GtkToggleButton*)static_check);
      history = g_list_insert_before(history, elementafter, hitem);

      if(selected_item_nr == 0)
      {
        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        history_item *hitem = history->data;
        gtk_clipboard_set_text(clipboard, hitem->content, -1);
      }
    }
    gtk_widget_destroy(dialog);
    search_history();
  }
}

/* Called when Remove is selected from Manage dialog */
static void remove_selected()
{
  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  gint selected_count = gtk_tree_selection_count_selected_rows(search_selection);
  if (selected_count > 0) {
    gint i;
    GtkTreeModel *model = gtk_tree_view_get_model((GtkTreeView*)treeview_search);
    GtkListStore *store = GTK_LIST_STORE(model);
    GArray *sel = g_array_new(FALSE, FALSE, sizeof(GtkTreeIter));
    gtk_tree_selection_selected_foreach(search_selection, add_iter, sel);
    gtk_tree_selection_unselect_all(search_selection);
    for (i = 0; i < sel->len; i++) {
        gint remove_item;
        GtkTreeIter *iter = &g_array_index(sel, GtkTreeIter, i);
        gtk_tree_model_get((GtkTreeModel*)search_list, iter, 0, &remove_item, -1);
        history = g_list_remove(history, g_list_nth_data(history, remove_item));
        if (!gtk_list_store_remove(store, iter))
          continue;

        do {
          gint item;
          gtk_tree_model_get(model, iter, 0, &item, -1);
          gtk_list_store_set(store, iter, 0, item - 1, -1);
        } while (gtk_tree_model_iter_next(model, iter));
    }
    g_array_free(sel, TRUE);
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
      g_list_free(history);
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
    g_list_free(history);
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
  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  /* Check if selected */
  gint selected_count = gtk_tree_selection_count_selected_rows(search_selection);
  if (selected_count == 1) {
    gint selected_item_nr;
    GArray *sel = g_array_new(FALSE, FALSE, sizeof(GtkTreeIter));
    gtk_tree_selection_selected_foreach(search_selection, add_iter, sel);
    gtk_tree_selection_unselect_all(search_selection);
    GtkTreeIter *iter = &g_array_index(sel, GtkTreeIter, 0);
    gtk_tree_model_get((GtkTreeModel*)search_list, iter, 0, &selected_item_nr, -1);
    g_array_free(sel, TRUE);
    GList *element = g_list_nth(history, selected_item_nr);
    history_item *elem_data = element->data;
    GtkClipboard* prim = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(prim, (gchar*)elem_data->content, -1);
    gtk_clipboard_set_text(clip, (gchar*)elem_data->content, -1);
  }
}

static gboolean search_click(GtkWidget *widget, GdkEventButton *event, GtkWidget *search_window)
{
  if(event->type==GDK_2BUTTON_PRESS || event->type==GDK_3BUTTON_PRESS) {
    search_doubleclick();
    gtk_widget_destroy(search_window);
  }
  return FALSE;
}

static gboolean search_key_released(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  search_history();
  return FALSE;
}

static gboolean treeview_key_pressed(GtkWidget *widget, GdkEventKey *event, GtkWidget *search_window)
{
  switch (event->keyval) {
    case XK_Escape:
    case XK_Home:
    case XK_Up:
    case XK_Down:
    case XK_Page_Up:
    case XK_Page_Down:
    case XK_End:
    case XK_Shift_L:
    case XK_Shift_R:
    case XK_Control_L:
    case XK_Control_R:
    case XK_Tab:      // allow to switch focus by the Tab key
      return FALSE;
    case XK_Return:
      search_doubleclick();
      gtk_widget_destroy(search_window);
      break;
    case XK_Delete:
      remove_selected();
      break;
  }
  return TRUE;
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
  if(gtk_grab_get_current()) {
    /* A window is already open, so we present it to the user */
    GtkWidget *toplevel = gtk_widget_get_toplevel(gtk_grab_get_current());
    gtk_window_present((GtkWindow*)toplevel);
    return FALSE;
  }
  /* Declare some variables */
  GtkWidget *hbox;

  GtkTreeViewColumn *tree_column;
  
  /* Create the dialog */
  GtkWidget* search_dialog = gtk_dialog_new();

  gtk_window_set_icon((GtkWindow*)search_dialog, gtk_widget_render_icon(search_dialog, GTK_STOCK_FIND, GTK_ICON_SIZE_MENU, NULL));
  gchar *orig_title = _("Manage History");
  gchar *title = 0;
  if (prefs.offline_mode)
    title = g_strconcat(orig_title, _(" (Offline mode)"), NULL);
  else
    title = g_strdup(orig_title);
  gtk_window_set_title((GtkWindow*)search_dialog, title);
  g_free(title);
  gtk_window_set_resizable((GtkWindow*)search_dialog, TRUE);
  gtk_window_set_position((GtkWindow*)search_dialog, GTK_WIN_POS_CENTER);

  GdkScreen* screen = gtk_window_get_screen(GTK_WINDOW(search_dialog));
  gint screen_height = gdk_screen_get_height(screen)-120;
  gint win_height = 600;
  if (win_height > screen_height)
    win_height = screen_height;

  GtkWidget* vbox_search = gtk_vbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(search_dialog))), vbox_search, TRUE, TRUE, 2);
  gtk_widget_set_size_request((GtkWidget*)vbox_search, 400, win_height);

  hbox = gtk_hbox_new(TRUE, 4);
  gtk_box_pack_start((GtkBox*)vbox_search, hbox, FALSE, FALSE, 0);
  search_entry = gtk_entry_new();
  gtk_box_pack_end((GtkBox*)hbox, search_entry, TRUE, TRUE, 0);
  g_signal_connect((GObject*)search_entry, "key-release-event", (GCallback)search_key_released, NULL);

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
  tree_column = gtk_tree_view_column_new_with_attributes(_("Results"), cell_renderer, "text", 1, NULL);
  gtk_tree_view_append_column((GtkTreeView*)treeview_search, tree_column);
  gtk_container_add((GtkContainer*)scrolled_window_search, treeview_search);
  gtk_box_pack_start((GtkBox*)vbox_search, scrolled_window_search, TRUE, TRUE, 0);

  GtkWidget* edit_button = gtk_dialog_add_button((GtkDialog*)search_dialog, _("Edit"), 1);
  g_signal_connect((GObject*)edit_button, "clicked", (GCallback)edit_selected, NULL);
  GtkWidget* remove_button = gtk_dialog_add_button((GtkDialog*)search_dialog, _("Remove"), 1);
  g_signal_connect((GObject*)remove_button, "clicked", (GCallback)remove_selected, NULL);
  GtkWidget* remove_all_button = gtk_dialog_add_button((GtkDialog*)search_dialog, _("Remove all"), 1);
  g_signal_connect((GObject*)remove_all_button, "clicked", (GCallback)remove_all_selected, NULL);
  GtkWidget* close_button = gtk_dialog_add_button((GtkDialog*)search_dialog, _("Close"), GTK_RESPONSE_OK);
  g_signal_connect((GObject*)close_button, "clicked", (GCallback)search_history, NULL);

  GtkTreeSelection* search_selection = gtk_tree_view_get_selection((GtkTreeView*)treeview_search);
  gtk_tree_selection_set_mode(search_selection, GTK_SELECTION_MULTIPLE);
  g_signal_connect((GObject*)treeview_search, "button_press_event", (GCallback)search_click, search_dialog);
  g_signal_connect((GObject*)treeview_search, "key-press-event", (GCallback)treeview_key_pressed, search_dialog);

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
