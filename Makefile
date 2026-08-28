OS_NAME = NoviumOS
KERNEL_NAME = neox
VERSION = 0.1.0

BUILD_DIR = build
VPATH = init drivers/video drivers/input kernel lib

ARCH ?= x86_32

CC = gcc
LD = ld
OBJCOPY = objcopy
TRUNCATE = truncate

# per-arch compile/link flags. add a block here for each new arch.
ifeq ($(ARCH),x86_32)
CFLAGS_ARCH  = -m32 
LDFLAGS_ARCH = -m elf_i386
else
$(error unsupported ARCH '$(ARCH)')
endif

CFLAGS = $(CFLAGS_ARCH) -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
         -O2 -g -Wall -Wextra \
         -Iinclude -Iarch/$(ARCH)/include

LDFLAGS = $(LDFLAGS_ARCH) -T arch/$(ARCH)/kernel/link.ld

KERNEL_OBJS = $(BUILD_DIR)/bootstrap.o $(BUILD_DIR)/hw_init.o $(BUILD_DIR)/irq.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/ps2.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/pit.o $(BUILD_DIR)/vga_console.o $(BUILD_DIR)/string.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/main.o $(BUILD_DIR)/sched.o

.PHONY: all run run-raw iso clean

all: $(BUILD_DIR)/novium.bin $(BUILD_DIR)/kernel.elf

# ensure build directory always exists before compiling
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# generic pattern rule for compiling C sources (sources found via VPATH)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bootstrap.o: arch/$(ARCH)/kernel/bootstrap.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/isr.o: arch/$(ARCH)/kernel/isr.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hw_init.o: arch/$(ARCH)/boot/hw_init.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/irq.o: arch/$(ARCH)/kernel/irq.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pit.o: arch/$(ARCH)/kernel/pit.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


# build raw boot sector
$(BUILD_DIR)/boot.bin: arch/$(ARCH)/boot/boot.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/boot.o
	$(LD) $(LDFLAGS_ARCH) -Ttext 0x7C00 $(BUILD_DIR)/boot.o -o $(BUILD_DIR)/boot.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/boot.elf $(BUILD_DIR)/boot.bin

# build setup code 
$(BUILD_DIR)/setup.bin: arch/$(ARCH)/boot/setup.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/setup.o
	$(LD) $(LDFLAGS_ARCH) -Ttext 0x7E00 $(BUILD_DIR)/setup.o -o $(BUILD_DIR)/setup.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/setup.elf $(BUILD_DIR)/setup.bin
	$(TRUNCATE) -s 2K $(BUILD_DIR)/setup.bin

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $(BUILD_DIR)/kernel.elf

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.bin
	$(TRUNCATE) -s 16384 $(BUILD_DIR)/kernel.bin

# glue everything into a single floppy image
$(BUILD_DIR)/novium.bin: $(BUILD_DIR)/boot.bin $(BUILD_DIR)/setup.bin $(BUILD_DIR)/kernel.bin
	cat $(BUILD_DIR)/boot.bin $(BUILD_DIR)/setup.bin $(BUILD_DIR)/kernel.bin > $(BUILD_DIR)/novium.bin
	$(TRUNCATE) -s 1440K $(BUILD_DIR)/novium.bin

# build iso release
$(BUILD_DIR)/iso:
	mkdir -p $(BUILD_DIR)/iso/staging


# Optional: to build the iso you need xorriso, commands to install:

# Ubuntu / Debian / Mint / WSL2: sudo apt install xorriso
# Fedora: sudo dnf install xorriso
# Arch: sudo pacman -S libisoburn

iso: $(BUILD_DIR)/novium.bin | $(BUILD_DIR)/iso
	cp $(BUILD_DIR)/novium.bin $(BUILD_DIR)/iso/staging/novium.bin
	xorriso -as mkisofs -quiet -V "$(OS_NAME)" \
		-o $(BUILD_DIR)/iso/novium.iso \
		-b novium.bin -c boot.cat \
		$(BUILD_DIR)/iso/staging

# qemu targets
QEMU_FLAGS ?=

run: $(BUILD_DIR)/kernel.elf
	qemu-system-i386 $(QEMU_FLAGS) -kernel $(BUILD_DIR)/kernel.elf

run-raw: $(BUILD_DIR)/novium.bin
	qemu-system-i386 $(QEMU_FLAGS) -drive format=raw,file=$(BUILD_DIR)/novium.bin,index=0,if=floppy -boot a

clean:
	rm -rf $(BUILD_DIR)
