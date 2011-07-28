/*
 * gbacklight is a GTK frontend to xbacklight 
 *
 * Copyright © 2008 Daniel Fetchinson 
 * Copyright © 2007 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <gtk/gtk.h>

static void help( void )
{
    printf( "usage: gbacklight [options]\n\n" );
    printf( "    where options are:\n\n" );
    printf( "    --display <display> or -d <display>\n" );
    printf( "    --help or -h\n" );
    exit( 1 );
}

typedef struct
{
    Display *display;
    GtkAdjustment *adjustment;
    Atom backlight;
} callback_data;

static long backlight_get( Atom backlight, Display * dpy, RROutput output )
{
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop;
    Atom actual_type;
    int actual_format;
    long value;

    if( XRRGetOutputProperty( dpy, output, backlight,
			      0, 4, False, False, None, &actual_type, &actual_format, &nitems, &bytes_after, &prop ) != Success )
	return -1;
    if( actual_type != XA_INTEGER || nitems != 1 || actual_format != 32 )
	value = -1;
    else
	value = *( ( long * ) prop );
    XFree( prop );
    return value;
}

static void backlight_set( Atom backlight, Display * dpy, RROutput output, long value )
{
    XRRChangeOutputProperty( dpy, output, backlight, XA_INTEGER, 32, PropModeReplace, ( unsigned char * ) &value, 1 );
}

static void set_callback( GtkWidget * widget, callback_data * data )
{
    double value = ( data->adjustment )->value;
    Display *dpy = data->display;
    Atom backlight = data->backlight;

    int j, screen;
    double min, max, set;

    XRRScreenResources *resources;
    RROutput output;
    XRRPropertyInfo *info;

    for( screen = 0; screen < ScreenCount( dpy ); screen++ )
    {
	resources = XRRGetScreenResources( dpy, RootWindow( dpy, screen ) );

	if( !resources )
	    continue;

	for( j = 0; j < resources->noutput; j++ )
	{
	    output = resources->outputs[j];
	    if( backlight_get( backlight, dpy, output ) != -1 )
	    {
		info = XRRQueryOutputProperty( dpy, output, backlight );

		if( info )
		{
		    if( info->range && info->num_values == 2 )
		    {
			min = info->values[0];
			max = info->values[1];
			set = min + value * ( max - min ) / 100;
			if( set > max )
			    set = max;
			if( set < min )
			    set = min;
			backlight_set( backlight, dpy, output, ( long ) set );
			XFlush( dpy );
		    }
		    XFree( info );
		}
	    }
	}
	XRRFreeScreenResources( resources );
    }
}

static void destroy_callback( GtkWidget * widget, callback_data * data )
{
    /* XSync( data->display, False ); */
    gtk_main_quit(  );
}

int main( int argc, char **argv )
{
    int i, j, screen, major, minor;
    double min, max, current;
    long current_test;
    char *dpy_name = NULL;
    Display *dpy;
    XRRScreenResources *resources;
    RROutput output;
    XRRPropertyInfo *info;
    Atom backlight;

    GtkWidget *window;
    GtkWidget *scale;
    GtkObject *adjustment;
    GtkWidget *button;
    GtkWidget *box;

    callback_data data;

    /* parse command line arguments */
    for( i = 1; i < argc; i++ )
    {
	if( !strcmp( argv[i], "--display" ) || !strcmp( "-d", argv[i] ) )
	{
	    if( ++i >= argc )
		help(  );
	    dpy_name = argv[i];
	    continue;
	}

	if( !strcmp( argv[i], "--help" ) || !strcmp( "-h", argv[i] ) )
	{
	    help(  );
	}

	help(  );
    }

    /* set up display and check some X stuff */
    dpy = XOpenDisplay( dpy_name );
    if( !dpy )
    {
	fprintf( stderr, "Can't open display \"%s\"\n", XDisplayName( dpy_name ) );
	exit( 1 );
    }
    if( !XRRQueryVersion( dpy, &major, &minor ) )
    {
	fprintf( stderr, "RandR extension missing\n" );
	exit( 1 );
    }
    if( major < 1 || ( major == 1 && minor < 2 ) )
    {
	fprintf( stderr, "RandR version %d.%d too old\n", major, minor );
	exit( 1 );
    }

    backlight = XInternAtom( dpy, "BACKLIGHT", True );
    if( backlight == None )
    {
	fprintf( stderr, "No outputs have backlight property\n" );
	exit( 1 );
    }

    /* get current backlight value */
    for( screen = 0; screen < ScreenCount( dpy ); screen++ )
    {
	resources = XRRGetScreenResources( dpy, RootWindow( dpy, screen ) );

	if( !resources )
	    continue;

	for( j = 0; j < resources->noutput; j++ )
	{
	    output = resources->outputs[j];
	    current_test = backlight_get( backlight, dpy, output );
	    if( current_test != -1 )
	    {
		info = XRRQueryOutputProperty( dpy, output, backlight );
		if( info )
		{
		    if( info->range && info->num_values == 2 )
		    {
			min = info->values[0];
			max = info->values[1];
			current = ( double ) ( current_test * 100 / ( max - min ) );
			break;
		    }
		    XFree( info );
		}
	    }
	    XRRFreeScreenResources( resources );
	}
    }

    /* do the gtk stuff */
    gtk_init( &argc, &argv );

    /* adjustment  */
    adjustment = gtk_adjustment_new( current, 0.0, 100.001, 0.001, 0.001, 0.001 );

    /* data is the thing we pass around in callbacks */
    data.adjustment = GTK_ADJUSTMENT( adjustment );
    data.display = dpy;
    data.backlight = backlight;

    /* window */
    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_container_set_border_width( GTK_CONTAINER( window ), 50 );
    gtk_window_set_title( GTK_WINDOW( window ), "gbacklight" );
    g_signal_connect( G_OBJECT( window ), "destroy", G_CALLBACK( destroy_callback ), &data );
    g_signal_connect( G_OBJECT( window ), "delete_event", G_CALLBACK( destroy_callback ), &data );

    /* box to store the scale and OK button */
    box = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( window ), box );

    /* scale  */
    scale = gtk_hscale_new( GTK_ADJUSTMENT( adjustment ) );

    gtk_widget_set_size_request( GTK_WIDGET( scale ), 350, 40 );
    gtk_range_set_update_policy( GTK_RANGE( scale ), GTK_UPDATE_CONTINUOUS );
    gtk_scale_set_digits( GTK_SCALE( scale ), 2 );
    gtk_scale_set_draw_value( GTK_SCALE( scale ), TRUE );
    gtk_scale_set_value_pos( GTK_SCALE( scale ), GTK_POS_RIGHT );
    g_signal_connect( G_OBJECT( scale ), "value_changed", G_CALLBACK( set_callback ), &data );
    gtk_box_pack_start( GTK_BOX( box ), scale, TRUE, TRUE, 0 );
    
    /* button */
    button = gtk_button_new_from_stock( GTK_STOCK_OK );
    g_signal_connect( G_OBJECT( button ), "clicked", G_CALLBACK( destroy_callback ), &data );
    gtk_box_pack_start( GTK_BOX( box ), button, TRUE, FALSE, 10 );
    
    
    gtk_widget_show( window );
    gtk_widget_show( box );
    gtk_widget_show( scale );
    gtk_widget_show( button );
    
    gtk_main(  );

    return 0;
}
