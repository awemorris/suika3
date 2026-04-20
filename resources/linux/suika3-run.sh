#!/bin/sh

RUN_OK=""
WD=""

if [ -z "$1" ]; then
    if [ -f main.ray ]; then
        RUN_OK=1;
        WD=`pwd`;
    elif [ -f assets.arc ]; then
        RUN_OK=1;
        WD=`pwd`;
    fi
else
    if [ -f "$1" ]; then
        RUN_OK=1;
        WD=$(dirname "$1")
    fi
    if [ -d "$1" ]; then
        RUN_OK=1;
        WD="$1"
    fi
fi

if [ -z "$RUN_OK" ]; then
    zenity --error \
           --title="Suika3 Engine Runtime" \
           --text="No script file specified.\n\nPlease right-click a 'main.ray' file and select 'Open with Suika3'." \
           --width=400
    exit 1
fi

if cd "$WD"; then
    rm -f log.txt
    /app/bin/suika3
    if [ -f log.txt ]; then
        /usr/bin/xdg-open log.txt;
    fi
else
    zenity --error \
           --title="Suika3 Engine Runtime" \
           --text="Failed to access the directory:\n$WD\n\nPlease check your Flatpak permission settings."
    exit 1
fi
