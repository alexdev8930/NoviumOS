VERSION = 0
PATCHLEVEL = 3
SUBLEVEL = 0
EXTRAVERSION = -dev
VERSION_TAG = $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)

DRIVE_FLAGS = -cdrom
BUILD_DIR = build
VPATH = init drivers/video drivers/input kernel lib mm

ARCH ?= x86_32

CC = gcc
LD = ld

GRUB_CFG = arch/$(ARCH)/boot/grub/grub.cfg
ISO_ROOT = $(BUILD_DIR)/iso/staging
ISO_IMAGE = $(BUILD_DIR)/iso/novium-v$(VERSION_TAG).iso

ifeq ($(ARCH),x86_32)
CFLAGS_ARCH  = -m32
LDFLAGS_ARCH = -m elf_i386
else
$(error unsupported ARCH '$(ARCH)')
endif

CFLAGS = $(CFLAGS_ARCH) \
         -ffreestanding \
         -fno-builtin \
         -fno-stack-protector \
         -fno-pie \
         -O2 \
         -g \
         -Wall \
         -Wextra \
         -Iinclude \
         -Iarch/$(ARCH)/include

LDFLAGS = $(LDFLAGS_ARCH) \
          -T arch/$(ARCH)/kernel/link.ld

KERNEL_OBJS = \
    $(BUILD_DIR)/multiboot.o \
    $(BUILD_DIR)/bootstrap.o \
    $(BUILD_DIR)/hw_init.o \
    $(BUILD_DIR)/irq.o \
    $(BUILD_DIR)/isr.o \
    $(BUILD_DIR)/debug.o \
    $(BUILD_DIR)/ps2.o \
    $(BUILD_DIR)/keyboard.o \
    $(BUILD_DIR)/pit.o \
    $(BUILD_DIR)/vga_console.o \
    $(BUILD_DIR)/string.o \
    $(BUILD_DIR)/stdio.o \
    $(BUILD_DIR)/main.o \
    $(BUILD_DIR)/sched.o \
    $(BUILD_DIR)/process.o \
    $(BUILD_DIR)/switch.o

.PHONY: all iso run clean

all: $(ISO_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/multiboot.o: arch/$(ARCH)/boot/multiboot.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bootstrap.o: arch/$(ARCH)/kernel/bootstrap.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/isr.o: arch/$(ARCH)/kernel/isr.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/switch.o: arch/$(ARCH)/kernel/switch.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hw_init.o: arch/$(ARCH)/boot/hw_init.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/irq.o: arch/$(ARCH)/kernel/irq.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pit.o: arch/$(ARCH)/kernel/pit.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/process.o: arch/$(ARCH)/kernel/process.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) arch/$(ARCH)/kernel/link.ld
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@

$(ISO_IMAGE): $(BUILD_DIR)/kernel.elf $(GRUB_CFG) | $(BUILD_DIR)
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(BUILD_DIR)/kernel.elf $(ISO_ROOT)/boot/novium
	cp $(GRUB_CFG) $(ISO_ROOT)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_ROOT)

iso: $(ISO_IMAGE)

run: $(ISO_IMAGE)
	qemu-system-i386 $(QEMU_FLAGS) $(DRIVE_FLAGS) $(ISO_IMAGE)

clean:
	rm -rf $(BUILD_DIR)