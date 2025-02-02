/*
 *  Blur Scope plugin for Audacious
 *  Copyright (C) 2010-2012 John Lindgren
 *
 *  Based on BMP - Cross-platform multimedia player:
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libaudcore/i18n.h>
#include <libaudcore/runtime.h>
#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>

#define D_WIDTH 64
#define D_HEIGHT 32

static gboolean bscope_init (void);
static void bscope_cleanup(void);
static void bscope_clear (void);
static void bscope_render (const gfloat * data);
static void /* GtkWidget */ * bscope_get_widget (void);
static void /* GtkWidget */ * bscope_get_color_chooser (void);

static const PreferencesWidget bscope_widgets[] = {
    WidgetLabel (N_("<b>Color</b>")),
    WidgetCustom (bscope_get_color_chooser)
};

static const PluginPreferences bscope_prefs = {
    bscope_widgets,
    ARRAY_LEN (bscope_widgets)
};

#define AUD_PLUGIN_NAME        N_("Blur Scope")
#define AUD_PLUGIN_PREFS       & bscope_prefs
#define AUD_PLUGIN_INIT        bscope_init
#define AUD_PLUGIN_CLEANUP     bscope_cleanup
#define AUD_VIS_CLEAR          bscope_clear
#define AUD_VIS_RENDER_MONO    bscope_render
#define AUD_VIS_GET_WIDGET     bscope_get_widget

#define AUD_DECLARE_VIS
#include <libaudcore/plugin-declare.h>

static GtkWidget * area = NULL;
static gint width, height, stride, image_size;
static guint32 * image = NULL, * corner = NULL;

static const gchar * const bscope_defaults[] = {
 "color", "16727935", /* 0xFF3F7F */
 NULL};

static gint color;

static gboolean bscope_init (void)
{
    aud_config_set_defaults ("BlurScope", bscope_defaults);
    color = aud_get_int ("BlurScope", "color");

    return TRUE;
}

static void bscope_cleanup (void)
{
    aud_set_int ("BlurScope", "color", color);

    g_free (image);
    image = NULL;
}

static void bscope_resize (gint w, gint h)
{
    width = w;
    height = h;
    stride = width + 2;
    image_size = (stride << 2) * (height + 2);
    image = (guint32 *) g_realloc (image, image_size);
    memset (image, 0, image_size);
    corner = image + stride + 1;
}

static void bscope_draw_to_cairo (cairo_t * cr)
{
    cairo_surface_t * surf = cairo_image_surface_create_for_data ((guchar *)
     image, CAIRO_FORMAT_RGB24, width, height, stride << 2);
    cairo_set_source_surface (cr, surf, 0, 0);
    cairo_paint (cr);
    cairo_surface_destroy (surf);
}

static void bscope_draw (void)
{
    if (! area || ! gtk_widget_get_window (area))
        return;

    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (area));
    bscope_draw_to_cairo (cr);
    cairo_destroy (cr);
}

static gboolean configure_event (GtkWidget * widget, GdkEventConfigure * event)
{
    bscope_resize (event->width, event->height);
    return TRUE;
}

static gboolean draw_cb (GtkWidget * widget)
{
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));
    bscope_draw_to_cairo (cr);
    cairo_destroy (cr);
    return TRUE;
}

static void /* GtkWidget */ * bscope_get_widget (void)
{
    area = gtk_drawing_area_new ();
    gtk_widget_set_size_request (area, D_WIDTH, D_HEIGHT);
    bscope_resize (D_WIDTH, D_HEIGHT);

    g_signal_connect (area, "expose-event", (GCallback) draw_cb, NULL);
    g_signal_connect (area, "configure-event", (GCallback) configure_event, NULL);
    g_signal_connect (area, "destroy", (GCallback) gtk_widget_destroyed, & area);

    GtkWidget * frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type ((GtkFrame *) frame, GTK_SHADOW_IN);
    gtk_container_add ((GtkContainer *) frame, area);
    return frame;
}

static void bscope_clear (void)
{
    g_return_if_fail (image != NULL);
    memset (image, 0, image_size);
    bscope_draw ();
}

static void bscope_blur (void)
{
    for (gint y = 0; y < height; y ++)
    {
        guint32 * p = corner + stride * y;
        guint32 * end = p + width;
        guint32 * plast = p - stride;
        guint32 * pnext = p + stride;

        /* We do a quick and dirty average of four color values, first masking
         * off the lowest two bits.  Over a large area, this masking has the net
         * effect of subtracting 1.5 from each value, which by a happy chance
         * is just right for a gradual fade effect. */
        for (; p < end; p ++)
            * p = ((* plast ++ & 0xFCFCFC) + (p[-1] & 0xFCFCFC) + (p[1] &
             0xFCFCFC) + (* pnext ++ & 0xFCFCFC)) >> 2;
    }
}

static inline void draw_vert_line (gint x, gint y1, gint y2)
{
    gint y, h;

    if (y1 < y2) {y = y1 + 1; h = y2 - y1;}
    else if (y2 < y1) {y = y2; h = y1 - y2;}
    else {y = y1; h = 1;}

    guint32 * p = corner + y * stride + x;

    for (; h --; p += stride)
        * p = color;
}

static void bscope_render (const gfloat * data)
{
    bscope_blur ();

    gint prev_y = (0.5 + data[0]) * height;
    prev_y = CLAMP (prev_y, 0, height - 1);

    for (gint i = 0; i < width; i ++)
    {
        gint y = (0.5 + data[i * 512 / width]) * height;
        y = CLAMP (y, 0, height - 1);
        draw_vert_line (i, prev_y, y);
        prev_y = y;
    }

    bscope_draw ();
}

static void color_set_cb (GtkWidget * chooser)
{
    GdkColor gdk_color;
    gtk_color_button_get_color ((GtkColorButton *) chooser, & gdk_color);
    color = ((gdk_color.red & 0xff00) << 8) | (gdk_color.green & 0xff00) | (gdk_color.blue >> 8);
}

static void /* GtkWidget */ * bscope_get_color_chooser (void)
{
    GdkColor gdk_color = {0, (guint16) ((color & 0xff0000) >> 8),
     (guint16) (color & 0xff00), (guint16) ((color & 0xff) << 8)};
    GtkWidget * chooser = gtk_color_button_new_with_color (& gdk_color);
    gtk_color_button_set_use_alpha ((GtkColorButton *) chooser, FALSE);

    g_signal_connect (chooser, "color-set", (GCallback) color_set_cb, NULL);

    return chooser;
}
