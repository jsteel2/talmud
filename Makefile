EXE=build/fs/eden.exe

all: build/fs build/disk.img

build/fs:
	mkdir -p build/fs

build/%.bin: boot/%.kc build/compiler
	build/compiler $< $@

build/fs/%.exe: boot/%.kc build/compiler
	build/compiler $< $@

build/disk.img: build/mbr.bin build/vbr.bin $(EXE)
	dd if=/dev/zero of=$@ bs=1M count=64
	(echo n; echo; echo; echo; echo; echo a; echo t; echo c; echo p; echo w; echo q) | fdisk $@
	mkfs.vfat -F 32 --offset 1 -h 1 build/disk.img
	mcopy -s -i build/disk.img@@512 build/fs/* ::/

.PHONY: clean run

clean:
	rm -rf build

run: build/disk.img
	qemu-system-x86_64 -hda $<

build/compiler:
	$(MAKE) -C compilerc