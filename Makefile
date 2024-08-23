EXE=build/fs/eden.exe

all: build/disk.img

build/fs:
	mkdir -p build/fs

build/%.bin: boot/%.kc build/compiler
	build/compiler $< $@

build/fs/%.exe: boot/%.kc build/compiler
	echo not yet > $@

build/disk.img: build/fs build/mbr.bin build/vbr.bin $(EXE)
	dd if=/dev/zero of=$@ bs=1M count=64
	echo 'start=1, type=c, bootable' | sfdisk build/disk.img
	mkfs.vfat -F 32 --offset 1 -h 1 build/disk.img
	mcopy -s -i build/disk.img@@512 build/fs/* ::/
	dd if=build/mbr.bin of=build/disk.img conv=notrunc
	dd if=build/vbr.bin of=build/disk.img bs=1 seek=602 conv=notrunc

.PHONY: clean run build/compiler

clean:
	rm -rf build

run: build/disk.img
	qemu-system-i386 -hda $<

build/compiler:
	$(MAKE) -C compilerc