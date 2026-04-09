#!/bin/sh

export DEBFULLNAME="Awe Morris"
export DEBEMAIL="awe@noctvm.io"

# Make chroots.
if [ ! -d /var/lib/chroot/sid-amd64 ]; then
  sudo sbuild-createchroot sid /var/lib/chroot/sid-amd64 http://deb.debian.org/debian/
fi
if [ ! -d /var/lib/chroot/noble-amd64 ]; then
  sudo sbuild-createchroot noble /var/lib/chroot/noble-amd64 http://archive.ubuntu.com/ubuntu/
fi

# Fetch upstream.
git checkout master
gbp import-orig --pristine-tar --uscan

# Build for Debian.
gbp dch --release --auto --distribution=sid --spawn-editor=never
git add debian/changelog
git commit -m "Prepare release"
gbp buildpackage --git-debian-branch=master --git-builder=sbuild --git-dist=sid --git-tag

# Build for Ubuntu.
git checkout ubuntu/noble
git merge master
gbp dch --release --distribution=noble --debian-branch=ubuntu/noble --local=~ubuntu24.04 --spawn-editor=never
git add debian/changelog
git commit -m "Prepare release"
gbp buildpackage --git-debian-branch=ubuntu/noble --git-builder=sbuild --git-dist=noble --git-tag
git checkout master
