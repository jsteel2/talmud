#!/bin/sh -ex

cd `dirname $0`
mkdir -p build
./compilerpy/main.py build/mbr.bin boot/mbr.kc
[ `stat -c %s build/mbr.bin` -gt 440 ] && echo "MBR too large" && exit 1
./compilerpy/main.py build/vbr.bin boot/vbr.kc
[ `stat -c %s build/vbr.bin` -gt 448 ] && echo "VBR too large" && exit 1

./compilerpy/main.py build/command.exe command/main.kc

./compilerpy/main.py build/echo.exe bin/echo.kc
./compilerpy/main.py build/dir.exe bin/dir.kc
./compilerpy/main.py build/edit.exe bin/edit.kc

dd if=/dev/zero of=build/disk.img bs=1M count=32
echo 'start=1, type=6, bootable' | sfdisk build/disk.img
dd if=build/mbr.bin of=build/disk.img conv=notrunc
LOOP=`sudo losetup -f -P build/disk.img --show`
sudo mkfs.vfat -F 16 "$LOOP"p1
sudo mount "$LOOP"p1 /mnt
sudo cp build/command.exe /mnt/COMMAND.EXE
sudo mkdir /mnt/BIN
sudo mkdir /mnt/SRC
sudo find command bin lib boot -type d -exec mkdir -p /mnt/SRC/{} \;
sudo find command bin lib boot -type f -exec sh -c 'fold -s "{}" > "/mnt/SRC/{}"' \; -exec unix2dos /mnt/SRC/{} \;
sudo cp build/echo.exe /mnt/BIN/ECHO.EXE
sudo cp build/dir.exe /mnt/BIN/DIR.EXE
sudo cp build/edit.exe /mnt/BIN/EDIT.EXE
sudo umount /mnt
sudo losetup -d "$LOOP"
dd if=build/vbr.bin of=build/disk.img bs=1 seek=574 conv=notrunc

[ "$1" = "run" ] && qemu-system-i386 -hda build/disk.img -m 1M -cpu 486
exit 0
