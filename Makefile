# AI Generated 

# =============================================================================
#  Cross-Compiler Makefile — AArch64 & x86-64 Bare-Metal
#
#  Directory structure:
#    src/kernel.c
#    src/aarch64/linker.ld
#    src/aarch64/limine/limine.conf
#    src/aarch64/limine/limine_requests.h
#    src/x86_64/linker.ld
#    src/x86_64/limine/limine.conf
#    src/x86_64/limine/limine_requests.h
#
#  Usage:
#    make ARCH=aarch64       → build AArch64 only
#    make ARCH=x86_64        → build x86-64 only
#    make both               → build both architectures
#    make run   ARCH=aarch64 → run in QEMU
#    make debug ARCH=aarch64 → run in QEMU + wait for GDB (port 1234)
#    make dump  ARCH=aarch64 → ELF disassembly
#    make info  ARCH=aarch64 → show active toolchain
#    make clean              → remove all build output
# =============================================================================

# -----------------------------------------------------------------------------
# Source files — add manually
# -----------------------------------------------------------------------------

# Compiled for both architectures
SRCS_COMMON := \
    src/kernel.c \
    src/drivers/framebuffer/framebuffer.c \
    src/drivers/framebuffer/fonts/font_8x16.c \
    src/utils/log.c

# AArch64-specific sources
SRCS_AARCH64 :=

# x86_64-specific sources
SRCS_X86_64 := \
    src/x86_64/interrupt/idt.c \
    src/x86_64/interrupt/exception.c \

# -----------------------------------------------------------------------------
# Architecture selection (default: x86_64)
# -----------------------------------------------------------------------------
ARCH ?= x86_64

# -----------------------------------------------------------------------------
# AArch64 toolchain
# -----------------------------------------------------------------------------
CC_AARCH64      := aarch64-linux-gnu-gcc
LD_AARCH64      := aarch64-linux-gnu-ld
OBJCOPY_AARCH64 := aarch64-linux-gnu-objcopy
OBJDUMP_AARCH64 := aarch64-linux-gnu-objdump

CFLAGS_AARCH64  := -ffreestanding \
                   -nostdlib \
                   -march=armv8-a \
                   -mgeneral-regs-only \
                   -Wall -Wextra \
                   -O2 -g

LDFLAGS_AARCH64 := -nostdlib \
                   -T src/aarch64/linker.ld \
                   --no-dynamic-linker

QEMU_AARCH64       := qemu-system-aarch64
QEMU_FLAGS_AARCH64 := \
                   -machine virt \
                   -cpu cortex-a53 \
                   -m 128M \
                   -bios /usr/share/edk2-ovmf/x64/OVMF.4m.fd \
                   -cdrom build/aarch64/os.iso \
                   -serial stdio \

# -----------------------------------------------------------------------------
# x86_64 toolchain (native — host is already x86_64)
# -----------------------------------------------------------------------------
CC_X86_64       := gcc
LD_X86_64       := ld
OBJCOPY_X86_64  := objcopy
OBJDUMP_X86_64  := objdump

CFLAGS_X86_64   := -ffreestanding \
                   -nostdlib \
                   -m64 \
                   -mno-red-zone \
                   -mno-mmx \
                   -mno-sse \
                   -mno-sse2 \
                   -Wall -Wextra \
                   -O2 -g \
                   -fno-stack-protector \
                   -mno-80387

LDFLAGS_X86_64  := -nostdlib \
                   -T src/x86_64/linker.ld \
                   --no-dynamic-linker \
                   -z max-page-size=0x1000

QEMU_X86_64        := qemu-system-x86_64
QEMU_FLAGS_X86_64  := \
                   -machine q35 \
                   -cpu qemu64 \
                   -m 128M \
                   -bios /usr/share/edk2-ovmf/x64/OVMF.4m.fd \
                   -cdrom build/x86_64/os.iso \
                   -serial stdio \
                   -d int,cpu_reset \
                   -no-reboot \
                   -no-shutdown

# -----------------------------------------------------------------------------
# Select variables based on ARCH
# -----------------------------------------------------------------------------
ifeq ($(ARCH),aarch64)
    CC          := $(CC_AARCH64)
    LD          := $(LD_AARCH64)
    OBJCOPY     := $(OBJCOPY_AARCH64)
    OBJDUMP     := $(OBJDUMP_AARCH64)
    CFLAGS      := $(CFLAGS_AARCH64)
    LDFLAGS     := $(LDFLAGS_AARCH64)
    SRCS        := $(SRCS_COMMON) $(SRCS_AARCH64)
    QEMU        := $(QEMU_AARCH64)
    QEMU_FLAGS  := $(QEMU_FLAGS_AARCH64)
    LIMINE_CONF := src/aarch64/limine/limine.conf
else ifeq ($(ARCH),x86_64)
    CC          := $(CC_X86_64)
    LD          := $(LD_X86_64)
    OBJCOPY     := $(OBJCOPY_X86_64)
    OBJDUMP     := $(OBJDUMP_X86_64)
    CFLAGS      := $(CFLAGS_X86_64)
    LDFLAGS     := $(LDFLAGS_X86_64)
    SRCS        := $(SRCS_COMMON) $(SRCS_X86_64)
    QEMU        := $(QEMU_X86_64)
    QEMU_FLAGS  := $(QEMU_FLAGS_X86_64)
    LIMINE_CONF := src/x86_64/limine/limine.conf
else
    $(error Invalid ARCH: "$(ARCH)". Use: make ARCH=aarch64  or  make ARCH=x86_64)
endif

# -----------------------------------------------------------------------------
# Include paths
# -----------------------------------------------------------------------------
INCLUDES := \
    -I include \
    -I src \
    -I src/$(ARCH) \
    -I limine-protocol/include

# -----------------------------------------------------------------------------
# Output directories
# -----------------------------------------------------------------------------
BUILD_DIR  := build/$(ARCH)
OBJ_DIR    := $(BUILD_DIR)/obj
ISO_DIR    := $(BUILD_DIR)/iso

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
ISO        := $(BUILD_DIR)/os.iso

OBJS       := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------
.PHONY: all both clean run debug dump info

# Default target — respects ARCH parameter
all: $(ISO)

# Build both architectures
both:
	$(MAKE) -f $(MAKEFILE_LIST) ARCH=aarch64
	$(MAKE) -f $(MAKEFILE_LIST) ARCH=x86_64

# -----------------------------------------------------------------------------
# Build rules
# -----------------------------------------------------------------------------

# Link objects into kernel ELF, then wrap into bootable ISO
$(ISO): $(KERNEL_ELF)
	@mkdir -p $(ISO_DIR)/boot/limine
	@mkdir -p $(ISO_DIR)/EFI/BOOT
	@cp $(KERNEL_ELF)                 $(ISO_DIR)/boot/kernel.elf
	@cp $(LIMINE_CONF)                $(ISO_DIR)/boot/limine/limine.conf
	@cp limine/limine-uefi-cd.bin     $(ISO_DIR)/boot/limine/
	@cp limine/BOOTX64.EFI            $(ISO_DIR)/EFI/BOOT/
	@cp limine/BOOTAA64.EFI           $(ISO_DIR)/EFI/BOOT/
	xorriso -as mkisofs \
	    -b boot/limine/limine-uefi-cd.bin \
	    -no-emul-boot \
	    --efi-boot boot/limine/limine-uefi-cd.bin \
	    -efi-boot-part \
	    --efi-boot-image \
	    --protective-msdos-label \
	    $(ISO_DIR) -o $@
	@echo "[ISO] $@"

# Link .o files into kernel ELF
$(KERNEL_ELF): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $^ -o $@
	@echo "[LD]  $@"

# Compile .c to .o, preserving source directory structure under build/
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "[CC]  $<"

# -----------------------------------------------------------------------------
# Utility targets
# -----------------------------------------------------------------------------

# Run kernel in QEMU
run: $(ISO)
	$(QEMU) $(QEMU_FLAGS)

# Run kernel in QEMU and wait for GDB on port 1234
# Connect with: aarch64-linux-gnu-gdb build/aarch64/kernel.elf
debug: $(ISO)
	$(QEMU) $(QEMU_FLAGS) -s -S

# Disassemble kernel ELF
dump: $(KERNEL_ELF)
	$(OBJDUMP) -d $<

# Print active toolchain variables
info:
	@echo "ARCH        : $(ARCH)"
	@echo "CC          : $(CC)"
	@echo "LD          : $(LD)"
	@echo "CFLAGS      : $(CFLAGS)"
	@echo "LDFLAGS     : $(LDFLAGS)"
	@echo "SRCS        : $(SRCS)"
	@echo "OBJS        : $(OBJS)"
	@echo "LIMINE_CONF : $(LIMINE_CONF)"
	@echo "ISO         : $(ISO)"

clean:
	rm -rf build/
	@echo "[CLEAN] build/ removed"