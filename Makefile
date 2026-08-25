OS_NAME = NoviumOS
KERNEL_NAME = neox
VERSION = 0.0.3

# Select target architecture (Linux-style: make ARCH=...).
# Only x86_32 is implemented; add a flag block + dirs for more.
ARCH ?= x86_32

CC = gcc
LD = ld
OBJCOPY = objcopy
TRUNCATE = truncate

# Per-architecture compile/link flags. Add a block here for each new ARCH.
ifeq ($(ARCH),x86_32)
CFLAGS_ARCH  = -m32
LDFLAGS_ARCH = -m elf_i386
else
$(error unsupported ARCH '$(ARCH)')
endif

CFLAGS = $(CFLAGS_ARCH) -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
         -O2 -Wall -Wextra \
         -Iinclude -Iarch/$(ARCH)/include -I.

LDFLAGS = $(LDFLAGS_ARCH) -T arch/$(ARCH)/kernel/link.ld

KERNEL_OBJS = build/bootstrap.o build/hw_init.o build/irq.o build/isr.o build/debug.o build/ps2.o build/keyboard.o build/pit.o build/vga_console.o build/main.o


.PHONY: all run run-raw clean

all: build/novium.bin build/kernel.elf

# ensure build directory always exists before compiling
build:
	mkdir -p build

# generic pattern rules for building files
build/%.o: init/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: drivers/video/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/hw_init.o: arch/$(ARCH)/boot/hw_init.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/bootstrap.o: arch/$(ARCH)/kernel/bootstrap.S | build
	$(CC) $(CFLAGS) -c $< -o $@

build/isr.o: arch/$(ARCH)/kernel/isr.S | build
	$(CC) $(CFLAGS) -c $< -o $@

build/irq.o: arch/$(ARCH)/kernel/irq.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/pit.o: arch/$(ARCH)/kernel/pit.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: drivers/input/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@


# build raw boot sector
build/boot.bin: arch/$(ARCH)/boot/boot.S | build
	$(CC) $(CFLAGS) -c $< -o build/boot.o
	$(LD) $(LDFLAGS_ARCH) -Ttext 0x7C00 build/boot.o -o build/boot.elf
	$(OBJCOPY) -O binary build/boot.elf build/boot.bin

# build setup code 
build/setup.bin: arch/$(ARCH)/boot/setup.S | build
	$(CC) $(CFLAGS) -c $< -o build/setup.o
	$(LD) $(LDFLAGS_ARCH) -Ttext 0x7E00 build/setup.o -o build/setup.elf
	$(OBJCOPY) -O binary build/setup.elf build/setup.bin
	$(TRUNCATE) -s 2K build/setup.bin

build/kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o build/kernel.elf

build/kernel.bin: build/kernel.elf
	$(OBJCOPY) -O binary build/kernel.elf build/kernel.bin
	$(TRUNCATE) -s 16384 build/kernel.bin

# glue everything into a single floppy image
build/novium.bin: build/boot.bin build/setup.bin build/kernel.bin
	cat build/boot.bin build/setup.bin build/kernel.bin > build/novium.bin
	$(TRUNCATE) -s 1440K build/novium.bin

# qemu targets
QEMU_FLAGS ?=

run: build/kernel.elf
	qemu-system-i386 $(QEMU_FLAGS) -kernel build/kernel.elf

run-raw: build/novium.bin
	qemu-system-i386 $(QEMU_FLAGS) -drive format=raw,file=build/novium.bin,index=0,if=floppy -boot a

clean:
	rm -rf build

# generate compile_commands.json for clangd/IDE support
compile_commands:
	bear -- make
