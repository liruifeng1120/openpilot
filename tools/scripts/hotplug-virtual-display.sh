#!/bin/bash
SRC="/data/carrot2-v8/tools/scripts/20-virtual-display.conf"
DST="/etc/X11/xorg.conf.d/20-virtual-display.conf"

for f in /sys/class/drm/card*-*/status; do
    grep -qix connected "$f" && exit 0
done

cp "$SRC" "$DST"
systemctl restart display-manager

for i in {1..15}; do
    [ -e /tmp/.X11-unix/X0 ] && sudo rm -f "$DST" && break
    sleep 1
done
[ -f "$DST" ] && rm -f "$DST"
