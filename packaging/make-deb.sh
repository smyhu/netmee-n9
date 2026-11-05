#!/bin/sh
# make-deb.sh - builds netmee-n9_<VER>_armel.deb from build/netmee-n9 + qml/ + packaging/
set -e
D=$(cd "$(dirname "$0")/.." && pwd)
VER=${VER:-0.1.3}
OUTDIR=${OUTDIR:-$D/dist}
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
P=$T/data; mkdir -p $P/opt/netmee-n9/bin $P/opt/netmee-n9/qml $P/usr/share/applications $P/usr/share/icons/hicolor/80x80/apps $P/usr/share/doc/netmee-n9 $T/ctl
install -m 755 $D/build/netmee-n9 $P/opt/netmee-n9/bin/
install -m 644 $D/qml/* $P/opt/netmee-n9/qml/
install -m 644 $D/packaging/netmee-n9.desktop $P/usr/share/applications/
install -m 644 $D/qml/netmee-n9.png $P/usr/share/icons/hicolor/80x80/apps/
install -m 644 $D/blocklist.txt $P/opt/netmee-n9/
install -m 644 $D/LICENSE $P/usr/share/doc/netmee-n9/copyright
SIZE=$(du -sk $P | cut -f1)
cat > $T/ctl/control <<CTL
Package: netmee-n9
Version: $VER
Architecture: armel
Maintainer: smyhu <smyhu@users.noreply.github.com>
Installed-Size: $SIZE
Depends: libc6 (>= 2.10), libqtcore4 (>= 4.7.4~), libqtgui4 (>= 4.7.4~), libqt4-declarative (>= 4.7.4~), libqtwebkit4, qt-components (>= 1.0)
Section: user/network
Priority: optional
Homepage: https://github.com/smyhu/netmee-n9
Description: Light web browser with script and ad filters
 A small browser in the N9 style: address bar, back, home, bookmarks with
 search, reader mode through FrogFind. Every request is checked before it
 leaves the phone: ads and trackers are blocked, scripts from other sites
 and oversized scripts can be refused, and a script that runs too long is
 stopped so the app never hangs. Languages: en, pl, de, es, it, fr.
 Designed and written by Claude (Anthropic) together with smyhu for the Nokia N9.
Maemo-Display-Name: NetMee
Maemo-Flags: visible
CTL
printf 'Maemo-Icon-26: %s\n' "$(base64 -w0 $D/qml/netmee-n9.png)" >> $T/ctl/control
(cd $P && find . -type f | sed 's#^\./##' | sort | xargs md5sum) > $T/ctl/md5sums
entry() { printf 'S 15 com.nokia.maemo H 40 %s R %d %s\n' "$1" "${#2}" "$2"; }
(cd $P && find . -type f | sed 's#^\./##' | grep -v '^usr/share/' | sort | while read -r f; do entry "$(sha1sum "$f" | cut -d' ' -f1)" "$f"; done) > $T/ctl/digsigsums
(cd $T/ctl && tar czf ../control.tar.gz --owner=0 --group=0 --numeric-owner --format=gnu ./control ./md5sums ./digsigsums)
(cd $P && tar czf ../data.tar.gz --owner=0 --group=0 --numeric-owner --format=gnu .)
echo 2.0 > $T/debian-binary
mkdir -p $OUTDIR; DEB=$OUTDIR/netmee-n9_${VER}_armel.deb; rm -f $DEB
(cd $T && ar rc $DEB debian-binary control.tar.gz data.tar.gz)
md5sum $DEB
