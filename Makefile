CC = gcc
LD = ld

CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
         -O2 -Wall -Wextra \
         -Iinclude -Iarch/x86_32/include -I.

LDFLAGS = -m elf_i386 -T arch/x86_32/kernel/link.ld

KERNEL_OBJS = build/bootstrap.o build/hw_init.o build/vga_console.o build/main.o

all: build/novium.bin build/kernel.elf

# Standard C compilation rules (Logged perfectly by Bear)
build/%.o: init/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: drivers/video/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/hw_init.o: arch/x86_32/boot/hw_init.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o build/hw_init.o

build/bootstrap.o: arch/x86_32/kernel/bootstrap.S
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o build/bootstrap.o

# --- INDEPENDENT 16-BIT REAL-MODE IMAGE GENERATION ---

build/boot.bin: arch/x86_32/boot/boot.S
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o build/boot.o
	$(LD) -m elf_i386 -Ttext 0x7C00 build/boot.o -o build/boot.elf
	objcopy -O binary build/boot.elf build/boot.bin

build/setup.bin: arch/x86_32/boot/setup.S
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o build/setup.o
	$(LD) -m elf_i386 -Ttext 0x7E00 build/setup.o -o build/setup.elf
	objcopy -O binary build/setup.elf build/setup.bin
	@truncate -s 2K build/setup.bin

# --- LINK AND CONCATENATION TARGETS ---

build/kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o build/kernel.elf

build/kernel.bin: build/kernel.elf
	objcopy -O binary build/kernel.elf build/kernel.bin
	@truncate -s 16K build/kernel.bin

# Concatenate: Sector 1 (512b) + Sectors 2-5 (2048b) + Sector 6+ (16KB Kernel)
build/novium.bin: build/boot.bin build/setup.bin build/kernel.bin
	cat build/boot.bin build/setup.bin build/kernel.bin > build/novium.bin
	@truncate -s 1440K build/novium.bin

run: build/kernel.elf
	qemu-system-i386 -kernel build/kernel.elf

run-raw: build/novium.bin
	qemu-system-i386 -fda build/novium.bin -boot a

clean:
	rm -rf build compile_commands.json
