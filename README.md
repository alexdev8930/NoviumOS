# Novium OS

Novium OS is a personal hobby operating system project. I’m building it to learn how kernels work and to see how far I can take a small, lightweight system on my own.

The project is inspired by the structure of the Linux kernel, but it is intentionally simple and focused on learning rather than trying to become a large production operating system.

## What this project is

This project is meant to be:

- small and lightweight
- practical for learning
- easy to follow and extend
- built step by step without overcomplicating things

## Current goals

Right now the focus is on getting the basics working:

- booting on x86
- getting basic output working
- handling early hardware input
- setting up memory and interrupts
- eventually building a simple desktop experience later on

## Project structure

The layout is inspired by the Linux kernel, but kept intentionally simple:

- arch/ - low-level boot and architecture-specific code
- drivers/ - basic hardware support
- kernel/ - core kernel code such as scheduling and interrupts
- init/ - startup and kernel entry
- include/ - shared headers and interfaces
- mm/ - memory management
- fs/ - filesystem work
- ipc/ - inter-process communication
- lib/ - utility code
- Documentation/ - notes and planning files

## Boot flow

The current boot path is very simple:

```text
boot.S -> setup.S -> bootstrap.S -> hw_init.c -> init/main.c
```

## Build and run

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt update && sudo apt install build-essential gcc-multilib g++-multilib qemu-system-x86 make
```

### Build

```bash
make
```

### Run

```bash
qemu-system-x86_64 -kernel novium_os.bin
```

## Roadmap

### Early steps
- [ ] set up the project layout
- [ ] define basic kernel types
- [ ] get basic output working
- [ ] finish the linker setup

### Kernel basics
- [x] switch into protected mode
- [ ] set up GDT/IDT basics
- [ ] add interrupt handling

### Later steps
- [ ] add memory management
- [ ] add basic scheduling
- [ ] build a simple filesystem
- [ ] explore lightweight desktop ideas later

## Notes

This is not meant to become a huge operating system. The goal is to make something small, fast, and useful for learning.