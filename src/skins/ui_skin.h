/*  Audacious
 *  Copyright (C) 2005-2011  Audacious development team.
 *
 *  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses>.
 *
 *  The Audacious team does not consider modular code linking to
 *  Audacious or using our public API to be a derived work.
 */

#ifndef SKIN_H
#define SKIN_H

#include <gtk/gtk.h>

#define COLOR(r,g,b) (((guint32) (r) << 16) | ((guint32) (g) << 8) | (guint32) (b))
#define COLOR_R(c) ((gint) (((c) & 0xff0000) >> 16))
#define COLOR_G(c) ((gint) (((c) & 0xff00) >> 8))
#define COLOR_B(c) ((gint) ((c) & 0xff))

typedef enum {
    SKIN_MAIN = 0,
    SKIN_CBUTTONS,
    SKIN_TITLEBAR,
    SKIN_SHUFREP,
    SKIN_TEXT,
    SKIN_VOLUME,
    SKIN_BALANCE,
    SKIN_MONOSTEREO,
    SKIN_PLAYPAUSE,
    SKIN_NUMBERS,
    SKIN_POSBAR,
    SKIN_PLEDIT,
    SKIN_EQMAIN,
    SKIN_EQ_EX,
    SKIN_PIXMAP_COUNT
} SkinPixmapId;

typedef enum {
    SKIN_MASK_MAIN = 0,
    SKIN_MASK_MAIN_SHADE,
    SKIN_MASK_EQ,
    SKIN_MASK_EQ_SHADE,
    SKIN_MASK_COUNT
} SkinMaskId;

typedef enum {
    SKIN_PLEDIT_NORMAL = 0,
    SKIN_PLEDIT_CURRENT,
    SKIN_PLEDIT_NORMALBG,
    SKIN_PLEDIT_SELECTEDBG,
    SKIN_TEXTBG,
    SKIN_TEXTFG,
    SKIN_COLOR_COUNT
} SkinColorId;

struct SkinProperties {
    /* Vis properties */
    int mainwin_vis_x = 24;
    int mainwin_vis_y = 43;
    int mainwin_vis_visible = true;

    /* Text properties */
    int mainwin_text_x = 112;
    int mainwin_text_y = 27;
    int mainwin_text_width = 153;
    int mainwin_text_visible = true;

    /* Infobar properties */
    int mainwin_infobar_x = 112;
    int mainwin_infobar_y = 43;
    int mainwin_othertext_visible = false;

    int mainwin_number_0_x = 36;
    int mainwin_number_0_y = 26;

    int mainwin_number_1_x = 48;
    int mainwin_number_1_y = 26;

    int mainwin_number_2_x = 60;
    int mainwin_number_2_y = 26;

    int mainwin_number_3_x = 78;
    int mainwin_number_3_y = 26;

    int mainwin_number_4_x = 90;
    int mainwin_number_4_y = 26;

    int mainwin_playstatus_x = 24;
    int mainwin_playstatus_y = 28;

    int mainwin_volume_x = 107;
    int mainwin_volume_y = 57;

    int mainwin_balance_x = 177;
    int mainwin_balance_y = 57;

    int mainwin_position_x = 16;
    int mainwin_position_y = 72;

    int mainwin_previous_x = 16;
    int mainwin_previous_y = 88;

    int mainwin_play_x = 39;
    int mainwin_play_y = 88;

    int mainwin_pause_x = 62;
    int mainwin_pause_y = 88;

    int mainwin_stop_x = 85;
    int mainwin_stop_y = 88;

    int mainwin_next_x = 108;
    int mainwin_next_y = 88;

    int mainwin_eject_x = 136;
    int mainwin_eject_y = 89;

    int mainwin_eqbutton_x = 219;
    int mainwin_eqbutton_y = 58;

    int mainwin_plbutton_x = 242;
    int mainwin_plbutton_y = 58;

    int mainwin_shuffle_x = 164;
    int mainwin_shuffle_y = 89;

    int mainwin_repeat_x = 210;
    int mainwin_repeat_y = 89;

    int mainwin_about_x = 247;
    int mainwin_about_y = 83;

    int mainwin_minimize_x = 244;
    int mainwin_minimize_y = 3;

    int mainwin_shade_x = 254;
    int mainwin_shade_y = 3;

    int mainwin_close_x = 264;
    int mainwin_close_y = 3;

    int mainwin_width = 275;
    int mainwin_height = 116;

    int mainwin_menurow_visible = true;
    int mainwin_streaminfo_visible = true;
    int mainwin_othertext_is_status = false;

    int textbox_bitmap_font_width = 5;
    int textbox_bitmap_font_height = 6;
};

extern const SkinProperties skin_default_hints;

typedef struct {
    gchar *path;
    cairo_surface_t * pixmaps[SKIN_PIXMAP_COUNT];
    guint32 colors[SKIN_COLOR_COUNT];
    guint32 vis_colors[24];
    GdkBitmap * masks[SKIN_MASK_COUNT];
    SkinProperties properties;
} Skin;

extern Skin * active_skin;

gboolean init_skins(const gchar * path);
void cleanup_skins(void);

gboolean active_skin_load(const gchar * path);

void skin_draw_pixbuf (cairo_t * cr, SkinPixmapId id, gint xsrc, gint ysrc,
 gint xdest, gint ydest, gint width, gint height);

void skin_get_eq_spline_colors(Skin * skin, guint32 colors[19]);
void skin_install_skin(const gchar * path);

void skin_draw_playlistwin_shaded (cairo_t * cr, gint width, gboolean focus);
void skin_draw_playlistwin_frame (cairo_t * cr, gint width, gint height,
 gboolean focus);
void skin_draw_mainwin_titlebar (cairo_t * cr, gboolean shaded, gboolean focus);

/* ui_skin_load_ini.c */
void skin_load_hints (Skin * skin, const char * path);
void skin_load_pl_colors (Skin * skin, const char * path);
void skin_load_masks (Skin * skin, const char * path);

static inline void set_cairo_color (cairo_t * cr, guint32 c)
{
    cairo_set_source_rgb (cr, COLOR_R(c) / 255.0, COLOR_G(c) / 255.0, COLOR_B(c)
     / 255.0);
}

#endif
