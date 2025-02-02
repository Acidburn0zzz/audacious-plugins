/*
 * Copyright (c) 2011 William Pitcock <nenolod@dereferenced.org>.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include <gtk/gtk.h>

#include <libaudcore/drct.h>
#include <libaudcore/hook.h>
#include <libaudcore/i18n.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudgui/libaudgui.h>
#include <libaudgui/libaudgui-gtk.h>

#define MAX_BANDS   (256)
#define VIS_DELAY 2 /* delay before falloff in frames */
#define VIS_FALLOFF 2 /* falloff in pixels per frame */

static GtkWidget * spect_widget = NULL;
static gfloat xscale[MAX_BANDS + 1];
static gint width, height, bands;
static gint bars[MAX_BANDS + 1];
static gint delay[MAX_BANDS + 1];

static void calculate_xscale (void)
{
    for (gint i = 0; i <= bands; i ++)
        xscale[i] = powf (256, (gfloat) i / bands) - 0.5f;
}

static void render_cb (gfloat * freq)
{
    g_return_if_fail (spect_widget);

    if (! bands)
        return;

    for (gint i = 0; i < bands; i ++)
    {
        gint a = ceilf (xscale[i]);
        gint b = floorf (xscale[i + 1]);
        gfloat n = 0;

        if (b < a)
            n += freq[b] * (xscale[i + 1] - xscale[i]);
        else
        {
            if (a > 0)
                n += freq[a - 1] * (a - xscale[i]);
            for (; a < b; a ++)
                n += freq[a];
            if (b < 256)
                n += freq[b] * (xscale[i + 1] - b);
        }

        /* fudge factor to make the graph have the same overall height as a
           12-band one no matter how many bands there are */
        n *= (gfloat) bands / 12;

        /* 40 dB range */
        gint x = 40 + 20 * log10f (n);
        x = CLAMP (x, 0, 40);

        bars[i] -= MAX (0, VIS_FALLOFF - delay[i]);

        if (delay[i])
            delay[i]--;

        if (x > bars[i])
        {
            bars[i] = x;
            delay[i] = VIS_DELAY;
        }
    }

    gtk_widget_queue_draw (spect_widget);
}

static void rgb_to_hsv (gfloat r, gfloat g, gfloat b, gfloat * h, gfloat * s, gfloat * v)
{
    gfloat max, min;

    max = r;
    if (g > max)
        max = g;
    if (b > max)
        max = b;

    min = r;
    if (g < min)
        min = g;
    if (b < min)
        min = b;

    * v = max;

    if (max == min)
    {
        * h = 0;
        * s = 0;
        return;
    }

    if (r == max)
        * h = 1 + (g - b) / (max - min);
    else if (g == max)
        * h = 3 + (b - r) / (max - min);
    else
        * h = 5 + (r - g) / (max - min);

    * s = (max - min) / max;
}

static void hsv_to_rgb (gfloat h, gfloat s, gfloat v, gfloat * r, gfloat * g, gfloat * b)
{
    for (; h >= 2; h -= 2)
    {
        gfloat * p = r;
        r = g;
        g = b;
        b = p;
    }

    if (h < 1)
    {
        * r = 1;
        * g = 0;
        * b = 1 - h;
    }
    else
    {
        * r = 1;
        * g = h - 1;
        * b = 0;
    }

    * r = v * (1 - s * (1 - * r));
    * g = v * (1 - s * (1 - * g));
    * b = v * (1 - s * (1 - * b));
}

static void get_color (GtkWidget * widget, int i, float * r, float * g, float * b)
{
    GdkColor * c = (gtk_widget_get_style (widget))->base + GTK_STATE_SELECTED;
    float h, s, v;

    rgb_to_hsv (c->red / 65535.0, c->green / 65535.0, c->blue / 65535.0, & h, & s, & v);

    if (s < 0.1) /* monochrome theme? use blue instead */
    {
        h = 5;
        s = 0.75;
    }

    s = 1 - 0.9 * i / (bands - 1);
    v = 0.75 + 0.25 * i / (bands - 1);

    hsv_to_rgb (h, s, v, r, g, b);
}

static void draw_background (GtkWidget * area, cairo_t * cr)
{
#if 0
    GdkColor * c = (gtk_widget_get_style (area))->bg;
#endif
    GtkAllocation alloc;
    gtk_widget_get_allocation (area, & alloc);

#if 0
    gdk_cairo_set_source_color(cr, c);
#endif
    cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
    cairo_fill (cr);
}

#if 0
static void draw_grid (GtkWidget * area, cairo_t * cr)
{
    GdkColor * c = (gtk_widget_get_style (area))->bg;
    GtkAllocation alloc;
    gtk_widget_get_allocation (area, & alloc);
    gint i;
    gfloat base_s = (height / 40);

    for (i = 1; i < 41; i++)
    {
        gdk_cairo_set_source_color(cr, c);
        cairo_move_to(cr, 0.0, i * base_s);
        cairo_line_to(cr, alloc.width, i * base_s);
        cairo_stroke(cr);
    }
}
#endif

static void draw_visualizer (GtkWidget *widget, cairo_t *cr)
{
    gfloat base_s = (height / 40);

    for (gint i = 0; i < bands; i++)
    {
        gint x = ((width / bands) * i) + 2;
        gfloat r, g, b;

        get_color (widget, i, & r, & g, & b);
        cairo_set_source_rgb (cr, r, g, b);
        cairo_rectangle (cr, x + 1, height - (bars[i] * base_s), (width / bands) - 1, (bars[i] * base_s));
        cairo_fill (cr);
    }
}

static gboolean configure_event (GtkWidget * widget, GdkEventConfigure * event)
{
    width = event->width;
    height = event->height;
    gtk_widget_queue_draw(widget);

    bands = width / 10;
    bands = CLAMP(bands, 12, MAX_BANDS);
    calculate_xscale ();

    return TRUE;
}

static gboolean draw_event (GtkWidget * widget)
{
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (widget));

    draw_background (widget, cr);
    draw_visualizer (widget, cr);
#if 0
    draw_grid (widget, cr);
#endif

    cairo_destroy (cr);
    return TRUE;
}

static gboolean destroy_event (void)
{
    aud_vis_func_remove ((VisFunc) render_cb);
    spect_widget = NULL;
    return TRUE;
}

static /* GtkWidget * */ gpointer get_widget(void)
{
    GtkWidget *area = gtk_drawing_area_new();
    spect_widget = area;

    g_signal_connect(area, "expose-event", (GCallback) draw_event, NULL);
    g_signal_connect(area, "configure-event", (GCallback) configure_event, NULL);
    g_signal_connect(area, "destroy", (GCallback) destroy_event, NULL);

    aud_vis_func_add (AUD_VIS_TYPE_FREQ, (VisFunc) render_cb);

    GtkWidget * frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type ((GtkFrame *) frame, GTK_SHADOW_IN);
    gtk_container_add ((GtkContainer *) frame, area);
    return frame;
}

#define AUD_PLUGIN_NAME        N_("Spectrum Analyzer")
#define AUD_VIS_GET_WIDGET     get_widget
#define AUD_VIS_CLEAR          NULL

#define AUD_DECLARE_VIS
#include <libaudcore/plugin-declare.h>
