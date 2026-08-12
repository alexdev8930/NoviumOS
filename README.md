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

The layout is inspired by the Linux kernel, but kept pretty simple:

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


## Build and Run

### Prerequisites (Ubuntu / Debian)

The primary development environment for Novium OS is Linux (Ubuntu/Debian) you can also use Windows with WSL.

```bash
sudo apt update && sudo apt install build-essential gcc-multilib g++-multilib qemu-system-x86 make bear mtools
```

### prerequisites (windows via WSL2)

You can build Novium OS on Windows using the Windows Subsystem for Linux (WSL2) and a native Windows emulator.

1. Install WSL2 (Ubuntu) by running this in PowerShell as Administrator:
   ```powershell
   wsl --install
   ```

2. Download and install [QEMU for Windows](https://qemu.org) to its default path (`C:\Program Files\qemu`).

3. Open your WSL2 Ubuntu terminal and install the compiler tools:
   ```bash
   sudo apt update && sudo apt install build-essential gcc-multilib make bear
   ```

4. Link WSL2 to your Windows QEMU by running these lines in your WSL terminal:
   ```bash
   echo "alias qemu-system-i386='\"/mnt/c/Program Files/qemu/qemu-system-i386.exe\"'" >> ~/.bashrc
   echo "alias qemu-system-x86_64='\"/mnt/c/Program Files/qemu/qemu-system-x86_64.exe\"'" >> ~/.bashrc
   source ~/.bashrc
   ```

### Compile

Once your prerequisites are installed (either on native Linux or inside WSL2), build the kernel:

```bash
make clean
bear -- make
```

### Execution

launch the kernel directly through QEMU:

```bash
make run
```

or run the raw floppy disk image to include the bootloader:

```bash
make run-raw
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