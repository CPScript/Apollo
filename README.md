# Apollo - x86_64 Operatin System made in C and asm

A  x86_64 operating system kernel with a cool architecture, improved input handling, modular design, text editor, a fully file system, and more. This implementation maintains visual compatibility with previous versions while featuring an entirely new codebase built from the ground up. 

> (The image below is from Apollo v1.0.0)

<img width="488" height="366" alt="491976349-fb7e5b1e-c90e-494c-afa4-c238eef3e86d" src="https://github.com/user-attachments/assets/f334aa19-5c7e-4ce7-b293-c8573310ba28" />


# Overview

### Core Design Principles
- **Modular Component Architecture**: Cleanly separated subsystems with well-defined interfaces
- **Enhanced Memory Management**: Heap allocator with block coalescing and alignment
- **Robust Input Handling**: PS/2 keyboard driver with proper interrupt management
- **Layered System Design**: Clear separation between hardware abstraction and kernel logic

### System Components
- **Terminal Subsystem**: Hardware-abstracted VGA text mode with advanced color management
- **Input Manager**: Buffered keyboard input with state tracking and scan code conversion
- **Command Processor**: Interactive shell with history management and command parsing
- **Memory Allocator**: Dynamic heap management with fragmentation prevention
- **Time Keeper**: Real-time clock interface with date/time functionality

## Features

### Updated Functionality
- **Improved Keyboard Input**: Fixed input handling with proper buffering and state management
- **Advanced Shell System**: Command history, tab completion, and enhanced user interaction
- **Better Memory Management**: Efficient allocation strategies with automatic defragmentation
- **Robust Error Handling**: Input validation and graceful error recovery
- **Hardware Abstraction**: Clean separation between hardware-specific and generic code

### Technical Improvements
- **64-bit Native Design**: Full x86_64 long mode implementation
- **Multiboot2 Compliance**: Enhanced bootloader compatibility
- **Modular Build System**: Organized source tree with logical component separation
- **Enhanced Documentation**: Inline documentation and API specifications

## Building the Kernel

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 grub-pc-bin grub-common xorriso

# Cross-compiler setup
wget https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/x86_64-elf-tools-linux.zip
unzip x86_64-elf-tools-linux.zip
export PATH=$PATH:$(pwd)/bin
```

### Compilation
```bash
# Build complete kernel
make all

# Run in QEMU emulator
make run

# Debug with GDB
make debug

# Clean build artifacts
make clean

# Show build information
make info

# Validate
make verify
```

## System Architecture

### Boot Sequence
1. **Multiboot2 Header**: GRUB bootloader integration
2. **32-bit Bootstrap**: Initial CPU setup and verification
3. **Long Mode Transition**: Switch to 64-bit execution
4. **Subsystem Initialization**: Component startup sequence
5. **Shell Activation**: Interactive user environment

### Memory Layout
- **Kernel Space**: 1MB+ physical address mapping
- **Heap Region**: 16MB dynamic allocation area
- **Stack Space**: 64KB kernel execution stack
- **Page Tables**: Identity-mapped 1GB address space

### Input/Output Architecture
- **VGA Text Mode**: 80x25 character display with full color support
- **PS/2 Keyboard**: Enhanced driver with proper interrupt handling
- **Serial Interface**: Debug output capability for development

## Commands Reference

| Command   | Description                    | Example Usage        |
|-----------|--------------------------------|----------------------|
| `help`    | Display available commands     | `help`               |
| `shell`   | Enter advanced shell mode      | `shell`              |
| `clear`   | Clear screen                   | `clear`              |
| `echo`    | Display text                   | `echo Hello World`   |
| `date`    | Show current date/time         | `date`               |
| `calc`    | Basic arithmetic calculator    | `calc 15 + 25`       |
| `meminfo` | Memory usage statistics        | `meminfo`            |
| `sysinfo` | System information             | `sysinfo`            |
| `history` | Show command history           | `history`            |
| `palette` | Color palette demonstration    | `palette`            |
| `uptime`  | System uptime                  | `uptime`             |
| `reboot`  | Restart system                 | `reboot`             |
| `shutdown`| Halt system                    | `shutdown`           |

### Example images 

> (The image below is from Apollo v1.0.0)

<img width="630" height="402" alt="491976426-57325cc4-dd78-47cf-8827-804cf53cb185" src="https://github.com/user-attachments/assets/2725aafa-2fe1-4201-8857-c048301d74d0" />

<img width="308" height="163" alt="491976413-0b86062a-a32c-4c5f-a6fc-20f09e9b77b4" src="https://github.com/user-attachments/assets/e0312457-23eb-4f6a-a022-7deeeb1fb674" />

<img width="474" height="359" alt="491976385-63fb8d22-9909-4921-8cd1-293cf9314de8" src="https://github.com/user-attachments/assets/e04c7c31-51d4-40f3-a218-8c3dcc20716b" />

<img width="487" height="401" alt="491976361-8c378fbe-461f-4580-b94d-247fefe88f2b" src="https://github.com/user-attachments/assets/c395609b-5be9-4acd-afb7-4f60c306e9cb" />


### Calculator Examples
```bash
apollo> calc 100 + 50
Result: 100 + 50 = 150

apollo> calc 256 / 8
Result: 256 / 8 = 32

apollo> calc 25 * 4
Result: 25 * 4 = 100
```

## Development and Debugging

### Running on Real Hardware
```bash
# Create bootable USB drive
sudo dd if=distribution/apollo.iso of=/dev/sdX bs=4M status=progress conv=fsync
# Replace /dev/sdX with your USB device
```

### Debug Information
```bash
# Boot with debugging symbols
qemu-system-x86_64 -cdrom apollo.iso -s -S -monitor stdio

# In another terminal, connect GDB
gdb distribution/apollo.bin
(gdb) target remote localhost:1234
(gdb) continue
```

## Technical Specifications

- **Architecture**: x86_64 (AMD64/Intel 64)
- **Boot Protocol**: Multiboot2
- **Memory Model**: 64-bit flat memory with paging
- **Graphics**: VGA text mode 80x25 with 16-color palette
- **Input**: PS/2 keyboard with full modifier support
- **Build System**: GNU Make with cross-compilation
- **Languages**: C99, x86_64 Assembly
- **Toolchain**: GCC cross-compiler, NASM assembler

---

**Apollo** - x86_64 Operating System Technology
