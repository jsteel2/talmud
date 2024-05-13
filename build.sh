#!/bin/sh -ex

cd `dirname $0`
rm -rf build
mkdir -p build/fs/BIN
./compilerpy/main.py build/mbr.bin boot/mbr.kc
[ `stat -c %s build/mbr.bin` -gt 440 ] && echo "MBR too large" && exit 1
./compilerpy/main.py build/vbr.bin boot/vbr.kc
[ `stat -c %s build/vbr.bin` -gt 448 ] && echo "VBR too large" && exit 1

echo command.kc bin/*.kc | tr ' ' '\n' | xargs -I{} -P `nproc` sh -c './compilerpy/main.py build/fs/`echo {} | sed "s/kc/exe/" | tr a-z A-Z` "{}"'
find command.kc bin lib boot -type d -exec mkdir -p build/fs/SRC/{} \;
find command.kc bin lib boot -type f -exec sh -c 'fold -w 79 -s "{}" > "build/fs/SRC/{}"' \; -exec unix2dos build/fs/SRC/{} \;
cp -r stuff build/fs/stuff

dd if=/dev/zero of=build/disk.img bs=1M count=32
echo 'start=1, type=6, bootable' | sfdisk build/disk.img
dd if=build/mbr.bin of=build/disk.img conv=notrunc
mkfs.vfat -F 16 --offset 1 -h 1 build/disk.img
mcopy -s -i build/disk.img@@512 build/fs/* ::/
dd if=build/vbr.bin of=build/disk.img bs=1 seek=574 conv=notrunc

[ "$1" = "run" ] && qemu-system-i386 -hda build/disk.img -m 1M -cpu 486
exit 0
