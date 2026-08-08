CC = gcc
AS = nasm
LD = ld

# Include paths for your global directories, arch directories, and root folder
CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
         -Iinclude \
         -Iarch/x86_32/include \
         -I.

LDFLAGS = -m elf_i386 -T arch/x86_32/kernel/link.ld

all: build

build:
	mkdir -p build
	
	# 1. Assemble NASM bootloader stages
	$(AS) -f elf32 arch/x86_32/boot/boot.asm -o build/boot.o
	$(AS) -f elf32 arch/x86_32/boot/setup.asm -o build/setup.o
	
	# 2. Assemble GNU S-file bridge
	$(CC) $(CFLAGS) -c arch/x86_32/kernel/bootstrap.S -o build/bootstrap.o
	
	# 3. Compile C Initialization and Driver Engine
	$(CC) $(CFLAGS) -c arch/x86_32/boot/hw_init.c -o build/hw_init.o
	$(CC) $(CFLAGS) -c drivers/video/vga_console.c -o build/vga_console.o
	$(CC) $(CFLAGS) -c init/main.c -o build/main.o
	
	# 4. Link ALL pieces into the final monolithic OS binary
	$(LD) $(LDFLAGS) \
		build/boot.o \
		build/setup.o \
		build/bootstrap.o \
		build/hw_init.o \
		build/vga_console.o \
		build/main.o \
		-o build/novium.bin

# Added shortcut to automatically compile and load right into QEMU
run: build
	qemu-system-i386 -kernel build/novium.bin

clean:
	rm -rf build
