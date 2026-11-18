VERSION = 0
PATCHLEVEL = 10
SUBLEVEL = 0
EXTRAVERSION = -rc1
VERSION_TAG = $(VERSION).$(PATCHLEVEL)$(if $(filter-out 0,$(SUBLEVEL)),.$(SUBLEVEL))$(EXTRAVERSION)

CC = gcc
LD = ld
QEMU ?= qemu-system-i386
GRUB_MKRESCUE ?= grub-mkrescue

ARCH ?= x86_32

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
DEPS_DIR  = $(BUILD_DIR)/_deps
VPATH = init drivers/video drivers/input kernel lib mm ipc fs arch/$(ARCH)/boot arch/$(ARCH)/kernel

DRIVE_FLAGS ?= -hda
QEMU_FLAGS ?= -m 64M -serial stdio

GRUB_CFG = arch/$(ARCH)/boot/grub/grub.cfg
ISO_ROOT = $(BUILD_DIR)/iso/staging
ISO_IMAGE = $(BUILD_DIR)/iso/novium-v$(VERSION_TAG).iso

ROOTFS_DIR = rootfs
ROOTFS_FILES = $(shell find $(ROOTFS_DIR) -type f 2>/dev/null)
INITRD_IMG = $(BUILD_DIR)/initrd.img

ifeq ($(ARCH),x86_32)
CFLAGS_ARCH  = -m32
LDFLAGS_ARCH = -m elf_i386
else
$(error unsupported ARCH '$(ARCH)')
endif

CPPFLAGS = -I. -Iinclude -Iarch/$(ARCH)/include

CFLAGS = $(CFLAGS_ARCH) \
         -ffreestanding \
         -fno-builtin \
         -fno-stack-protector \
         -fno-pie \
         -mno-sse \
         -mno-sse2 \
         -O2 \
         -g \
         -Wall \
         -Wextra \
         -MMD \
         -MP 

LDFLAGS = $(LDFLAGS_ARCH) \
          -T arch/$(ARCH)/kernel/link.ld

OBJS = \
    multiboot.o \
    bootstrap.o \
    hw_init.o \
    irq.o \
    isr.o \
    debug.o \
    ps2.o \
    keyboard.o \
    pit.o \
    vga_console.o \
    string.o \
    stdio.o \
    page_alloc.o \
    paging.o \
    main.o \
    sched.o \
    process.o \
    switch.o \
    heap.o \
    ipc.o \
    message.o \
    sync.o \
    vfs.o \
    initrd.o \
    shell.o \
    shell_cmds.o 

KERNEL_OBJS = $(addprefix $(OBJ_DIR)/,$(OBJS))
KERNEL_DEPS = $(addprefix $(DEPS_DIR)/,$(OBJS:.o=.d))

.PHONY: all iso run clean

all: $(ISO_IMAGE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(DEPS_DIR):
	mkdir -p $(DEPS_DIR)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR) $(DEPS_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MF $(DEPS_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: %.S | $(OBJ_DIR) $(DEPS_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MF $(DEPS_DIR)/$*.d -c $< -o $@

# Pull in the header dependencies.  A missing .d is fine on a fresh tree, and
# -MP keeps a deleted header from turning into a "no rule to make target" stop.
-include $(KERNEL_DEPS)

$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) arch/$(ARCH)/kernel/link.ld
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@

$(INITRD_IMG): $(ROOTFS_FILES) | $(BUILD_DIR)
	tar --format=ustar --exclude='.gitkeep' -cf $@ -C $(ROOTFS_DIR) .

$(ISO_IMAGE): $(BUILD_DIR)/kernel.elf $(INITRD_IMG) $(GRUB_CFG) | $(BUILD_DIR)
	mkdir -p $(ISO_ROOT)/boot/grub
	cp $(BUILD_DIR)/kernel.elf $(ISO_ROOT)/boot/novium
	cp $(INITRD_IMG) $(ISO_ROOT)/boot/initrd.img
	cp $(GRUB_CFG) $(ISO_ROOT)/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)


iso: $(ISO_IMAGE)

run: $(ISO_IMAGE)
	$(QEMU) $(QEMU_FLAGS) $(DRIVE_FLAGS) $(ISO_IMAGE)

clean:
	rm -rf $(BUILD_DIR)