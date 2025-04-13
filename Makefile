all: build/disk.img

build/fs:
	mkdir -p build/fs

build/%.bin: boot/%.kc build/compiler
	build/compiler $< $@

build/fs/%.exe: boot/%.kc build/compiler
	build/compiler $< $@

doom/doom.elf: PureDOOM.c
	$(CC) -nostdlib -ffreestanding -static -fPIC -fPIE -static-pie -m32 -march=i386 -O2 $< -o $@

build/disk.img: build/fs build/mbr.bin build/vbr.bin build/fs/bstrap.exe doom/doom.elf
	dd if=/dev/zero of=$@ bs=1M count=64
	echo 'start=1, type=c, bootable' | sfdisk build/disk.img
	mkfs.vfat -F 32 --offset 1 -h 1 build/disk.img
	mcopy -s -i build/disk.img@@512 adam lib boot compiler doom scripts net sound ::/
	mcopy -i build/disk.img@@512 build/fs/bstrap.exe ::/eden.exe
	dd if=build/mbr.bin of=build/disk.img conv=notrunc
	dd if=build/vbr.bin of=build/disk.img bs=1 seek=602 conv=notrunc
	yes 1 | tr -d '\n' | qemu-system-i386 -hda $@ -nographic

.PHONY: clean run build/compiler

clean:
	rm -rf build doom/doom.elf

run: build/disk.img
	qemu-system-i386 -net user -net nic,model=rtl8139 -device sb16 -hda $<

build/compiler:
	$(MAKE) -C compilerc