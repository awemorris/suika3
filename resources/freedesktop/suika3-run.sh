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
    fi;
else
    if [ -f "$1" ]; then
        RUN_OK=1;
        WD=$(dirname "$1")
    fi;
    if [ -d "$1" ]; then
        RUN_OK=1;
        WD="$1"
    fi;
fi

if [ -z "$RUN_OK" ]; then
    FILE=$(zenity --file-selection \
		  --file-filter="NovelML files | *.novel" \
		  --file-filter="Ray scripts | *.ray" \
		  --file-filter="Asset packages | assets.arc" \
		  --file-filter="All files | *");
    if [ -z "$FILE" ]; then
        exit 1;
    fi;
    WD=$(dirname "$FILE")
fi

if cd "$WD"; then
    rm -f log.txt;
    suika3;
    if [ -f log.txt ]; then
        zenity --error \
               --title="Suika3 Engine" \
               --text="$(printf 'Error\n%s' "$(cat log.txt)")";
        exit 1;
    fi;
else
    zenity --error \
           --title="Suika3 Engine Runtime" \
           --text="Failed to access the directory:\n$WD\n\nPlease check your Flatpak permission settings."
    exit 1
fi
