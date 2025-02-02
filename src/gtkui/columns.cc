/*
 * columns.c
 * Copyright 2011 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <string.h>

#include <gtk/gtk.h>

#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/index.h>
#include <libaudcore/audstrings.h>
#include <libaudgui/libaudgui-gtk.h>
#include <libaudgui/list.h>

#include "ui_playlist_notebook.h"
#include "ui_playlist_widget.h"

const char * const pw_col_names[PW_COLS] = {N_("Entry number"), N_("Title"),
 N_("Artist"), N_("Year"), N_("Album"), N_("Track"), N_("Genre"),
 N_("Queue position"), N_("Length"), N_("File path"), N_("File name"),
 N_("Custom title"), N_("Bitrate")};

int pw_num_cols;
int pw_cols[PW_COLS];
int pw_col_widths[PW_COLS];

static const char * const pw_col_keys[PW_COLS] = {"number", "title", "artist",
 "year", "album", "track", "genre", "queued", "length", "path", "filename",
 "custom", "bitrate"};

static const int pw_default_widths[PW_COLS] = {10, 275, 175, 10, 175, 10, 100,
 10, 10, 275, 275, 275, 10};

void pw_col_init (void)
{
    pw_num_cols = 0;

    String columns = aud_get_str ("gtkui", "playlist_columns");
    Index<String> index = str_list_to_index (columns, " ");

    int count = index.len ();
    if (count > PW_COLS)
        count = PW_COLS;

    for (int c = 0; c < count; c ++)
    {
        const String & column = index[c];

        int i = 0;
        while (i < PW_COLS && strcmp (column, pw_col_keys[i]))
            i ++;

        if (i == PW_COLS)
            break;

        pw_cols[pw_num_cols ++] = i;
    }

    String widths = aud_get_str ("gtkui", "column_widths");
    if (! str_to_int_array (widths, pw_col_widths, PW_COLS))
        memcpy (pw_col_widths, pw_default_widths, sizeof pw_col_widths);
}

typedef struct {
    int column;
    bool_t selected;
} Column;

static GtkWidget * chosen_list = NULL, * avail_list = NULL;
static Index<Column *> chosen, avail;

static void apply_changes (void)
{
    int cols = chosen.len ();
    g_return_if_fail (cols <= PW_COLS);

    ui_playlist_notebook_empty ();

    for (pw_num_cols = 0; pw_num_cols < cols; pw_num_cols ++)
        pw_cols[pw_num_cols] = chosen[pw_num_cols]->column;

    ui_playlist_notebook_populate ();
}

static void get_value (void * user, int row, int column, GValue * value)
{
    auto & index = * (Index<Column *> *) user;
    g_return_if_fail (row >= 0 && row < index.len ());
    Column * c = index[row];
    g_value_set_string (value, _(pw_col_names[c->column]));
}

static bool_t get_selected (void * user, int row)
{
    auto & index = * (Index<Column *> *) user;
    g_return_val_if_fail (row >= 0 && row < index.len (), FALSE);
    return index[row]->selected;
}

static void set_selected (void * user, int row, bool_t selected)
{
    auto & index = * (Index<Column *> *) user;
    g_return_if_fail (row >= 0 && row < index.len ());
    index[row]->selected = selected;
}

static void select_all (void * user, bool_t selected)
{
    auto & index = * (Index<Column *> *) user;
    int rows = index.len ();
    for (int row = 0; row < rows; row ++)
        index[row]->selected = selected;
}

static void shift_rows (void * user, int row, int before)
{
    auto & index = * (Index<Column *> *) user;
    int rows = index.len ();
    g_return_if_fail (row >= 0 && row < rows);
    g_return_if_fail (before >= 0 && before <= rows);

    if (before == row)
        return;

    Index<Column *> move;
    Index<Column *> others;

    int begin, end;
    if (before < row)
    {
        begin = before;
        end = row + 1;
        while (end < rows && index[end]->selected)
            end ++;
    }
    else
    {
        begin = row;
        while (begin > 0 && index[begin - 1]->selected)
            begin --;
        end = before;
    }

    for (int i = begin; i < end; i ++)
    {
        if (index[i]->selected)
            move.append (index[i]);
        else
            others.append (index[i]);
    }

    if (before < row)
        move.move_from (others, 0, -1, -1, true, true);
    else
        move.move_from (others, 0, 0, -1, true, true);

    index.move_from (move, 0, begin, end - begin, false, true);

    GtkWidget * list = (& index == & chosen) ? chosen_list : avail_list;
    audgui_list_update_rows (list, begin, end - begin);
    audgui_list_update_selection (list, begin, end - begin);

    apply_changes ();
}

static const AudguiListCallbacks callbacks = {
    get_value,
    get_selected,
    set_selected,
    select_all,
    NULL,  // activate_row
    NULL,  // right_click
    shift_rows
};

static void transfer (Index<Column *> * source)
{
    Index<Column *> * dest;
    GtkWidget * source_list, * dest_list;
    if (source == & chosen)
    {
        dest = & avail;
        source_list = chosen_list;
        dest_list = avail_list;
    }
    else
    {
        dest = & chosen;
        source_list = avail_list;
        dest_list = chosen_list;
    }

    int source_rows = source->len ();
    int dest_rows = dest->len ();

    for (int row = 0; row < source_rows; )
    {
        Column * c = (* source)[row];
        if (! c->selected)
        {
            row ++;
            continue;
        }

        source->remove (row, 1);
        audgui_list_delete_rows (source_list, row, 1);
        source_rows --;
        dest->append (c);
        audgui_list_insert_rows (dest_list, dest_rows, 1);
        dest_rows ++;
    }

    apply_changes ();
}

static void destroy_cb (void)
{
    chosen_list = NULL;
    avail_list = NULL;

    for (Column * column : chosen)
        g_slice_free (Column, column);
    for (Column * column : avail)
        g_slice_free (Column, column);

    chosen.clear ();
    avail.clear ();
}

void * pw_col_create_chooser (void)
{
    bool_t added[PW_COLS];
    memset (added, 0, sizeof added);

    for (int i = 0; i < pw_num_cols; i ++)
    {
        if (added[pw_cols[i]])
            continue;
        added[pw_cols[i]] = TRUE;
        Column * column = g_slice_new (Column);
        column->column = pw_cols[i];
        column->selected = 0;
        chosen.append (column);
    }

    for (int i = 0; i < PW_COLS; i ++)
    {
        if (added[i])
            continue;
        Column * column = g_slice_new (Column);
        column->column = i;
        column->selected = 0;
        avail.append (column);
    }

    GtkWidget * hbox = gtk_hbox_new (FALSE, 6);
    gtk_widget_set_size_request (hbox, -1, 160);

    GtkWidget * scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scroll,
     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scroll,
     GTK_SHADOW_IN);
    gtk_box_pack_start ((GtkBox *) hbox, scroll, TRUE, TRUE, 0);

    avail_list = audgui_list_new (& callbacks, & avail, avail.len ());
    audgui_list_add_column (avail_list, _("Available columns"), 0, G_TYPE_STRING, -1);
    gtk_container_add ((GtkContainer *) scroll, avail_list);

    GtkWidget * vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start ((GtkBox *) hbox, vbox, FALSE, FALSE, 0);

    GtkWidget * button = gtk_button_new ();
    gtk_container_add ((GtkContainer *) button, gtk_image_new_from_icon_name
     ("go-next", GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start ((GtkBox *) vbox, button, TRUE, FALSE, 0);
    g_signal_connect_swapped (button, "clicked", (GCallback) transfer, & avail);

    button = gtk_button_new ();
    gtk_container_add ((GtkContainer *) button, gtk_image_new_from_icon_name
     ("go-previous", GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start ((GtkBox *) vbox, button, TRUE, FALSE, 0);
    g_signal_connect_swapped (button, "clicked", (GCallback) transfer, & chosen);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy ((GtkScrolledWindow *) scroll,
     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type ((GtkScrolledWindow *) scroll,
     GTK_SHADOW_IN);
    gtk_box_pack_start ((GtkBox *) hbox, scroll, TRUE, TRUE, 0);

    chosen_list = audgui_list_new (& callbacks, & chosen, chosen.len ());
    audgui_list_add_column (chosen_list, _("Displayed columns"), 0, G_TYPE_STRING, -1);
    gtk_container_add ((GtkContainer *) scroll, chosen_list);

    g_signal_connect (hbox, "destroy", (GCallback) destroy_cb, NULL);

    return hbox;
}

void pw_col_save (void)
{
    Index<String> index;
    for (int i = 0; i < pw_num_cols; i ++)
        index.append (String (pw_col_keys[pw_cols[i]]));

    save_column_widths ();

    aud_set_str ("gtkui", "playlist_columns", index_to_str_list (index, " "));
    aud_set_str ("gtkui", "column_widths", int_array_to_str (pw_col_widths, PW_COLS));
}
