#!/bin/sh

set -eu

LANG=C
LANGUAGE=C
LC_ALL=C

# Get the version of Suika2.
echo 'Enter a version:'
read VERSION

# Get the minor version.
echo 'Enter a minor version:'
read MINOR

# Update the changelog
echo "suika3 ($VERSION-$MINOR) UNRELEASED; urgency=medium" > meta/debian/changelog;
echo '' >> meta/debian/changelog;
echo '  * Sync upstream' >> meta/debian/changelog;
echo '' >> meta/debian/changelog;
echo " -- Awe Morris <awe@noctvm.io>  `date '+%a, %d %b %Y %T %z'`" >> meta/debian/changelog;

# Make a directory and enter it.
rm -rf work
mkdir work
cd work

# Create a source tarball.
SAVE_DIR=`pwd`
cd ../../../../
ls -l
ls -l resources/
ls -l resources/packages
ls -l resources/packages/deb
git archive HEAD --output=resources/packages/deb/work/suika3_$VERSION.orig.tar.gz
cd "$SAVE_DIR"

# Make a sub-directory with version number, and enter it.
rm -rf suika3-$VERSION
mkdir suika3-$VERSION
cp -R ../meta/debian suika3-$VERSION/
cd suika3-$VERSION
tar xzf ../suika3_$VERSION.orig.tar.gz

# Build a source package.
debuild
cd ../../
cp work/*.deb ./

# Cleanup.
rm -rf work
