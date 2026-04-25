#!/bin/sh

# Remove thirdparty libraries.
rm -rf external/PlayfieldEngine/external/StratoHAL/lib

# Remove samples of Playfield Engine.
rm -rf external/PlayfieldEngine/samples

# Remove asset files.
rm -rf external/PlayfieldEngine/resources/assets.arc
rm -rf resources/assets.arc

# Remove projects for proprietary OSes.
rm -rf resources/projects

# Remove fonts.
rm -rf game/system/font/*
