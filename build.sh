#!/bin/sh
# Cross-build the netmee-n9 launcher for Nokia N9 (Harmattan PR1.3, ARMv7 hard-float).
# TC: hdr/ (Ubuntu armhf cross headers), rt/ (Harmattan libs and Qt headers),
#     ld-arm (GNU ld wrapper). moc = Harmattan moc run under qemu-arm-static.
set -e
TC=${TC:-/tmp/claude-0/x/tc}
D=$(cd "$(dirname "$0")" && pwd)
VER=${VER:-0.1.3}
OUT=${OUT:-$D/build}
mkdir -p $OUT
H=$TC/hdr/usr/arm-linux-gnueabihf
Q=$TC/rt/usr/include/qt4
L=$TC/rt/usr/lib
QEMU_LD_PREFIX=$TC/rt qemu-arm-static -E LD_LIBRARY_PATH=/usr/lib:/lib $TC/rt/usr/bin/host-moc -I$Q $D/src/webitem.h -o $OUT/moc_webitem.cpp
T="--target=arm-linux-gnueabihf -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb"
CF="$T -O2 -fno-pic -nostdinc -isystem $(clang -print-resource-dir)/include -isystem $H/include/c++/13 -isystem $H/include/c++/13/arm-linux-gnueabihf -isystem $H/include -U_FORTIFY_SOURCE -D_FILE_OFFSET_BITS=32"
for f in $D/src/main.cpp $D/src/webitem.cpp $OUT/moc_webitem.cpp; do
  clang++ $CF -std=gnu++98 -fno-rtti -fno-exceptions -fno-threadsafe-statics -DQT_NO_DEBUG -DQT_SHARED -DNETMEE_VERSION=\"$VER\" \
    -I $Q -I $Q/QtCore -I $Q/QtGui -I $Q/QtNetwork -I $Q/QtDeclarative -I $Q/QtWebKit -I $D/src -c $f -o $OUT/$(basename $f .cpp).o
done
G=$TC/hdr/usr/lib/gcc-cross/arm-linux-gnueabihf/13
clang++ $T --ld-path=$TC/ld-arm -no-pie -nostdlib -Wl,--allow-shlib-undefined -Wl,--as-needed -Wl,--hash-style=both -Wl,-z,norelro -Wl,-dynamic-linker,/lib/ld-linux.so.3 \
  $H/lib/crt1.o $H/lib/crti.o $G/crtbegin.o $OUT/main.o $OUT/webitem.o $OUT/moc_webitem.o \
  $L/libQtWebKit.so.4 $L/libQtDeclarative.so.4 $L/libQtNetwork.so.4 $L/libQtGui.so.4 $L/libQtCore.so.4 \
  $L/libstdc++.so.6 $TC/rt/lib/libc.so.6 $TC/rt/lib/libgcc_s.so.1 \
  $G/crtend.o $H/lib/crtn.o -o $OUT/netmee-n9
llvm-strip $OUT/netmee-n9
file $OUT/netmee-n9
