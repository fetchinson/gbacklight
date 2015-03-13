Gbacklight is a graphical frontend to  [xbacklight](http://gitweb.freedesktop.org/?p=xorg/app/xbacklight.git) for adjusting the display brightness. It is a stand alone application with no dependencies other than GTK and X.

# Usage

    gbacklight options


where options are


    --help, -h   Print usage summary and exit

    --display, -d display    Use 'display' instead of the default one

# Installation

    ./configure
    make
    make install

You need to have the packages xrandr, xrender, x11 and gtk installed although these typically come with recent linux distros. 

# Feedback

Please send any feedback about gbacklight to fetchinson@gmail.com


# Credit

gbacklight uses xbacklight which is copyright by Keith Packard.
