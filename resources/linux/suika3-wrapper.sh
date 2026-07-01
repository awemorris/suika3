#!/bin/sh

if [ -z "$1" ]; then
    echo "Specify an option.";
fi

if [ "$1" = "suika3" ]; then
    if [ -n "`which suika3`" ]; then
        exec suika3;
    fi;
    if flatpak info "vn.suika3.engine" > /dev/null 2>&1; then
        flatpak override --user --filesystem=host vn.suika3.engine;
        exec flatpak run vn.suika3.engine .;
    else
        echo -n "Do you want to install Suika3 via Flathub? <y/N>";
        read s;
        if [ "$s" = "y" ]; then
            flatpak install --user -y SDK/linux/Suika3-$(uname -m).flatpak;
        else
            exit 1;
        fi;
        flatpak override --user --filesystem=host vn.suika3.engine;
        exec flatpak run vn.suika3.engine .;
    fi;
fi

if [ "$1" = "suika3-pack" ]; then
    if [ -n "`which suika3-pack`" ]; then
        exec suika3-pack $(ls -A | grep -vE '^(SDK|suika3.exe|.vscode|assets.arc)$');
    fi;
    if flatpak info "vn.suika3.engine" > /dev/null 2>&1; then
        exec flatpak run --command=suika3-pack vn.suika3.engine $(ls -A | grep -vE '^(SDK|suika3.exe|.vscode|assets.arc)$');
    else
        echo -n "Do you want to install Suika3 via Flathub? <y/N>";
        read s;
        if [ "$s" = "y" ]; then
            flatpak install --user -y SDK/linux/Suika3-$(uname -m).flatpak;
            flatpak override --user --filesystem=host vn.suika3.engine;
        else
            exit 1;
        fi;
        exec flatpak run --command=suika3-pack vn.suika3.engine $(ls -A | grep -vE '^(SDK|suika3.exe|.vscode|assets.arc)$');
    fi;
fi
