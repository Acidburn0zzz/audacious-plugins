/*
 * menus.c
 * Copyright 2010-2014 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */

#include "menus.h"

#include <gdk/gdkkeysyms.h>

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/runtime.h>
#include <libaudgui/libaudgui.h>
#include <libaudgui/libaudgui-gtk.h>
#include <libaudgui/menu.h>

#include "actions-mainwin.h"
#include "actions-playlist.h"
#include "preset-browser.h"
#include "preset-list.h"
#include "view.h"

#define SHIFT GDK_SHIFT_MASK
#define CTRL GDK_CONTROL_MASK
#define ALT GDK_MOD1_MASK

#define SHIFT_CTRL (GdkModifierType) (SHIFT | CTRL)
#define CTRL_ALT   (GdkModifierType) (CTRL | ALT)
#define NO_MOD     (GdkModifierType) 0

#define NO_KEY     0, NO_MOD

static GtkWidget * menus[UI_MENUS];
static GtkAccelGroup * accel;

/* note: playback, playlist, and view menus must be created before main menu */
static GtkWidget * get_menu_playback (void) {return menus[UI_MENU_PLAYBACK]; }
static GtkWidget * get_menu_playlist (void) {return menus[UI_MENU_PLAYLIST]; }
static GtkWidget * get_menu_view (void) {return menus[UI_MENU_VIEW]; }

static GtkWidget * get_plugin_menu_main (void) {return audgui_get_plugin_menu (AUD_MENU_MAIN); }
static GtkWidget * get_plugin_menu_playlist (void) {return audgui_get_plugin_menu (AUD_MENU_PLAYLIST); }
static GtkWidget * get_plugin_menu_playlist_add (void) {return audgui_get_plugin_menu (AUD_MENU_PLAYLIST_ADD); }
static GtkWidget * get_plugin_menu_playlist_remove (void) {return audgui_get_plugin_menu (AUD_MENU_PLAYLIST_REMOVE); }

static const AudguiMenuItem main_items[] = {
    MenuCommand (N_("Open Files ..."), "document-open", 'l', NO_MOD, action_play_file),
    MenuCommand (N_("Open URL ..."), "folder-remote", 'l', CTRL, action_play_location),
    MenuSep (),
    MenuSub (N_("Playback"), NULL, get_menu_playback),
    MenuSub (N_("Playlist"), NULL, get_menu_playlist),
    MenuSub (N_("View"), NULL, get_menu_view),
    MenuSep (),
    MenuSub (N_("Services"), NULL, get_plugin_menu_main),
    MenuSep (),
    MenuCommand (N_("About ..."), "help-about", NO_KEY, audgui_show_about_window),
    MenuCommand (N_("Settings ..."), "preferences-system", 'p', CTRL, audgui_show_prefs_window),
    MenuCommand (N_("Quit"), "application-exit", 'q', CTRL, aud_quit)
};

static const AudguiMenuItem playback_items[] = {
    MenuCommand (N_("Song Info ..."), "dialog-information", 'i', NO_MOD, audgui_infowin_show_current),
    MenuSep (),
    MenuToggle (N_("Repeat"), NULL, 'r', NO_MOD, NULL, "repeat", NULL, "set repeat"),
    MenuToggle (N_("Shuffle"), NULL, 's', NO_MOD, NULL, "shuffle", NULL, "set shuffle"),
    MenuToggle (N_("No Playlist Advance"), NULL, 'n', CTRL, NULL, "no_playlist_advance", NULL, "set no_playlist_advance"),
    MenuToggle (N_("Stop After This Song"), NULL, 'm', CTRL, NULL, "stop_after_current_song", NULL, "set stop_after_current_song"),
    MenuSep (),
    MenuCommand (N_("Play"), "media-playback-start", 'x', NO_MOD, aud_drct_play),
    MenuCommand (N_("Pause"), "media-playback-pause", 'c', NO_MOD, aud_drct_pause),
    MenuCommand (N_("Stop"), "media-playback-stop", 'v', NO_MOD, aud_drct_stop),
    MenuCommand (N_("Previous"), "media-skip-backward", 'z', NO_MOD, aud_drct_pl_prev),
    MenuCommand (N_("Next"), "media-skip-forward", 'b', NO_MOD, aud_drct_pl_next),
    MenuSep (),
    MenuCommand (N_("Set A-B Repeat"), NULL, 'a', NO_MOD, action_ab_set),
    MenuCommand (N_("Clear A-B Repeat"), NULL, 'a', SHIFT, action_ab_clear),
    MenuSep (),
    MenuCommand (N_("Jump to Song ..."), "go-jump", 'j', NO_MOD, audgui_jump_to_track),
    MenuCommand (N_("Jump to Time ..."), "go-jump", 'j', CTRL, audgui_jump_to_time)
};

static const AudguiMenuItem playlist_items[] = {
    MenuCommand (N_("Play This Playlist"), "media-playback-start", GDK_KEY_Return, SHIFT, action_playlist_play),
    MenuSep (),
    MenuCommand (N_("New Playlist"), "document-new", 'n', SHIFT, action_playlist_new),
    MenuCommand (N_("Rename Playlist ..."), "insert-text", GDK_KEY_F2, NO_MOD, action_playlist_rename),
    MenuCommand (N_("Remove Playlist"), "edit-delete", 'd', SHIFT, action_playlist_delete),
    MenuSep (),
    MenuCommand (N_("Previous Playlist"), "media-skip-backward", GDK_KEY_Tab, SHIFT, action_playlist_prev),
    MenuCommand (N_("Next Playlist"), "media-skip-forward", GDK_KEY_Tab, NO_MOD, action_playlist_next),
    MenuSep (),
    MenuCommand (N_("Import Playlist ..."), "document-open", 'o', NO_MOD, audgui_import_playlist),
    MenuCommand (N_("Export Playlist ..."), "document-save", 's', SHIFT, audgui_export_playlist),
    MenuSep (),
    MenuCommand (N_("Playlist Manager ..."), "audio-x-generic", 'p', NO_MOD, audgui_playlist_manager),
    MenuCommand (N_("Queue Manager ..."), NULL, 'u', CTRL, audgui_queue_manager_show),
    MenuSep (),
    MenuCommand (N_("Refresh Playlist"), "view-refresh", GDK_KEY_F5, NO_MOD, action_playlist_refresh_list)
};

static const AudguiMenuItem view_items[] = {
    MenuToggle (N_("Show Playlist Editor"), NULL, 'e', ALT, "skins", "playlist_visible", view_apply_show_playlist, "skins set playlist_visible"),
    MenuToggle (N_("Show Equalizer"), NULL, 'g', ALT, "skins", "equalizer_visible", view_apply_show_equalizer, "skins set equalizer_visible"),
    MenuSep (),
    MenuToggle (N_("Show Remaining Time"), NULL, 'r', CTRL, "skins", "show_remaining_time", view_apply_show_remaining, "skins set show_remaining_time"),
    MenuSep (),
    MenuToggle (N_("Always on Top"), NULL, 'o', CTRL, "skins", "always_on_top", view_apply_on_top, "skins set always_on_top"),
    MenuToggle (N_("On All Workspaces"), NULL, 's', CTRL, "skins", "sticky", view_apply_sticky, "skins set sticky"),
    MenuSep (),
    MenuToggle (N_("Roll Up Player"), NULL, 'w', CTRL, "skins", "player_shaded", view_apply_player_shaded, "skins set player_shaded"),
    MenuToggle (N_("Roll Up Playlist Editor"), NULL, 'w', SHIFT_CTRL, "skins", "playlist_shaded", view_apply_playlist_shaded, "skins set playlist_shaded"),
    MenuToggle (N_("Roll Up Equalizer"), NULL, 'w', CTRL_ALT, "skins", "equalizer_shaded", view_apply_equalizer_shaded, "skins set equalizer_shaded")
};

static const AudguiMenuItem playlist_add_items[] = {
    MenuSub (N_("Services"), NULL, get_plugin_menu_playlist_add),
    MenuSep (),
    MenuCommand (N_("Add URL ..."), "folder-remote", 'h', CTRL, action_playlist_add_url),
    MenuCommand (N_("Add Files ..."), "list-add", 'f', NO_MOD, action_playlist_add_files)
};

static const AudguiMenuItem dupe_items[] = {
    MenuCommand (N_("By Title"), NULL, NO_KEY, action_playlist_remove_dupes_by_title),
    MenuCommand (N_("By Filename"), NULL, NO_KEY, action_playlist_remove_dupes_by_filename),
    MenuCommand (N_("By File Path"), NULL, NO_KEY, action_playlist_remove_dupes_by_full_path)
};

static const AudguiMenuItem playlist_remove_items[] = {
    MenuSub (N_("Services"), NULL, get_plugin_menu_playlist_remove),
    MenuSep (),
    MenuCommand (N_("Remove All"), "edit-delete", NO_KEY, action_playlist_remove_all),
    MenuCommand (N_("Clear Queue"), "edit-clear", 'q', SHIFT, action_playlist_clear_queue),
    MenuSep (),
    MenuCommand (N_("Remove Unavailable Files"), "dialog-warning", NO_KEY, action_playlist_remove_unavailable),
    MenuSub (N_("Remove Duplicates"), "edit-copy", dupe_items, ARRAY_LEN (dupe_items)),
    MenuSep (),
    MenuCommand (N_("Remove Unselected"), "list-remove", NO_KEY, action_playlist_remove_unselected),
    MenuCommand (N_("Remove Selected"), "list-remove", GDK_KEY_Delete, NO_MOD, action_playlist_remove_selected)
};

static const AudguiMenuItem playlist_select_items[] = {
    MenuCommand (N_("Search and Select"), "edit-find", 'f', CTRL, action_playlist_search_and_select),
    MenuSep (),
    MenuCommand (N_("Invert Selection"), NULL, NO_KEY, action_playlist_invert_selection),
    MenuCommand (N_("Select None"), NULL, 'a', SHIFT_CTRL, action_playlist_select_none),
    MenuCommand (N_("Select All"), "edit-select-all", 'a', CTRL, action_playlist_select_all),
};

static const AudguiMenuItem sort_items[] = {
    MenuCommand (N_("By Title"), NULL, NO_KEY, action_playlist_sort_by_title),
    MenuCommand (N_("By Album"), NULL, NO_KEY, action_playlist_sort_by_album),
    MenuCommand (N_("By Artist"), NULL, NO_KEY, action_playlist_sort_by_artist),
    MenuCommand (N_("By Filename"), NULL, NO_KEY, action_playlist_sort_by_filename),
    MenuCommand (N_("By File Path"), NULL, NO_KEY, action_playlist_sort_by_full_path),
    MenuCommand (N_("By Release Date"), NULL, NO_KEY, action_playlist_sort_by_date),
    MenuCommand (N_("By Track Number"), NULL, NO_KEY, action_playlist_sort_by_track_number)
};

static const AudguiMenuItem sort_selected_items[] = {
    MenuCommand (N_("By Title"), NULL, NO_KEY, action_playlist_sort_selected_by_title),
    MenuCommand (N_("By Album"), NULL, NO_KEY, action_playlist_sort_selected_by_album),
    MenuCommand (N_("By Artist"), NULL, NO_KEY, action_playlist_sort_selected_by_artist),
    MenuCommand (N_("By Filename"), NULL, NO_KEY, action_playlist_sort_selected_by_filename),
    MenuCommand (N_("By File Path"), NULL, NO_KEY, action_playlist_sort_selected_by_full_path),
    MenuCommand (N_("By Release Date"), NULL, NO_KEY, action_playlist_sort_selected_by_date),
    MenuCommand (N_("By Track Number"), NULL, NO_KEY, action_playlist_sort_selected_by_track_number)
};

static const AudguiMenuItem playlist_sort_items[] = {
    MenuCommand (N_("Randomize List"), NULL, 'r', SHIFT_CTRL, action_playlist_randomize_list),
    MenuCommand (N_("Reverse List"), "view-sort-descending", NO_KEY, action_playlist_reverse_list),
    MenuSep (),
    MenuSub (N_("Sort Selected"), "view-sort-ascending", sort_selected_items, ARRAY_LEN (sort_selected_items)),
    MenuSub (N_("Sort List"), "view-sort-ascending", sort_items, ARRAY_LEN (sort_items))
};

static const AudguiMenuItem playlist_context_items[] = {
    MenuCommand (N_("Song Info ..."), "dialog-information", 'i', ALT, action_playlist_track_info),
    MenuSep (),
    MenuCommand (N_("Cut"), "edit-cut", 'x', CTRL, action_playlist_cut),
    MenuCommand (N_("Copy"), "edit-copy", 'c', CTRL, action_playlist_copy),
    MenuCommand (N_("Paste"), "edit-paste", 'v', CTRL, action_playlist_paste),
    MenuSep (),
    MenuCommand (N_("Queue/Unqueue"), NULL, 'q', NO_MOD, action_queue_toggle),
    MenuSep (),
    MenuSub (N_("Services"), NULL, get_plugin_menu_playlist)
};

static const AudguiMenuItem eq_preset_items[] = {
    MenuCommand (N_("Load Preset ..."), "document-open", NO_KEY, eq_preset_load),
    MenuCommand (N_("Load Auto Preset ..."), NULL, NO_KEY, eq_preset_load_auto),
    MenuCommand (N_("Load Default"), NULL, NO_KEY, eq_preset_load_default),
    MenuCommand (N_("Load Preset File ..."), NULL, NO_KEY, eq_preset_load_file),
    MenuCommand (N_("Load EQF File ..."), NULL, NO_KEY, eq_preset_load_eqf),
    MenuSep (),
    MenuCommand (N_("Save Preset ..."), "document-save", NO_KEY, eq_preset_save),
    MenuCommand (N_("Save Auto Preset ..."), NULL, NO_KEY, eq_preset_save_auto),
    MenuCommand (N_("Save Default"), NULL, NO_KEY, eq_preset_save_default),
    MenuCommand (N_("Save Preset File ..."), NULL, NO_KEY, eq_preset_save_file),
    MenuCommand (N_("Save EQF File ..."), NULL, NO_KEY, eq_preset_save_eqf),
    MenuSep (),
    MenuCommand (N_("Delete Preset ..."), "edit-delete", NO_KEY, eq_preset_delete),
    MenuCommand (N_("Delete Auto Preset ..."), NULL, NO_KEY, eq_preset_delete_auto),
    MenuSep (),
    MenuCommand (N_("Import Winamp Presets ..."), "document-open", NO_KEY, eq_preset_import_winamp),
    MenuSep (),
    MenuCommand (N_("Reset to Zero"), "edit-clear", NO_KEY, eq_preset_set_zero)
};

void menu_init (void)
{
    static const struct {
        const AudguiMenuItem * items;
        int n_items;
    } table[] = {
        {main_items, ARRAY_LEN (main_items)},
        {playback_items, ARRAY_LEN (playback_items)},
        {playlist_items, ARRAY_LEN (playlist_items)},
        {view_items, ARRAY_LEN (view_items)},
        {playlist_add_items, ARRAY_LEN (playlist_add_items)},
        {playlist_remove_items, ARRAY_LEN (playlist_remove_items)},
        {playlist_select_items, ARRAY_LEN (playlist_select_items)},
        {playlist_sort_items, ARRAY_LEN (playlist_sort_items)},
        {playlist_context_items, ARRAY_LEN (playlist_context_items)},
        {eq_preset_items, ARRAY_LEN (eq_preset_items)}
    };

    accel = gtk_accel_group_new ();

    for (int i = UI_MENUS; i --; )
    {
        menus[i] = gtk_menu_new ();
        audgui_menu_init (menus[i], table[i].items, table[i].n_items, accel);
        g_signal_connect (menus[i], "destroy", (GCallback) gtk_widget_destroyed, & menus[i]);
    }
}

void menu_cleanup (void)
{
    for (int i = 0; i < UI_MENUS; i ++)
    {
        if (menus[i])
            gtk_widget_destroy (menus[i]);
    }

    g_object_unref (accel);
    accel = NULL;
}

GtkAccelGroup * menu_get_accel_group (void)
{
    return accel;
}

static void get_monitor_geometry (GdkScreen * screen, gint x, gint y, GdkRectangle * geom)
{
    int monitors = gdk_screen_get_n_monitors (screen);

    for (int i = 0; i < monitors; i ++)
    {
        gdk_screen_get_monitor_geometry (screen, i, geom);

        if (x >= geom->x && x < geom->x + geom->width && y >= geom->y && y < geom->y + geom->height)
            return;
    }

    /* fall back to entire screen */
    geom->x = 0;
    geom->y = 0;
    geom->width = gdk_screen_get_width (screen);
    geom->height = gdk_screen_get_height (screen);
}

typedef struct {
    int x, y;
    bool_t leftward, upward;
} MenuPosition;

static void position_menu (GtkMenu * menu, int * x, int * y, bool_t * push_in, void * data)
{
    const MenuPosition * pos = (MenuPosition *) data;

    GdkRectangle geom;
    get_monitor_geometry (gtk_widget_get_screen ((GtkWidget *) menu), pos->x, pos->y, & geom);

    GtkRequisition request;
    gtk_widget_size_request ((GtkWidget *) menu, & request);

    if (pos->leftward)
        * x = MAX (pos->x - request.width, geom.x);
    else
        * x = MIN (pos->x, geom.x + geom.width - request.width);

    if (pos->upward)
        * y = MAX (pos->y - request.height, geom.y);
    else
        * y = MIN (pos->y, geom.y + geom.height - request.height);
}

void menu_popup (int id, int x, int y, bool_t leftward, bool_t upward,
 int button, int time)
{
    const MenuPosition pos = {x, y, leftward, upward};
    gtk_menu_popup ((GtkMenu *) menus[id], NULL, NULL, position_menu, (void *) & pos, button, time);
}
