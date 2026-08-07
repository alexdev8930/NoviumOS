CC = gcc
CFLAGS = -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pie \
         -Iinclude \
         -Iarch/x86_32/include \
         -I.

all: build

build:
	mkdir -p build
	$(CC) $(CFLAGS) -c drivers/video/vga_console.c -o build/vga_console.o

clean:
	rm -rf build
