# Toolchain (override from the environment, e.g. the Docker image sets KCC=gcc KLD=ld).
# Named KCC/KLD because make predefines CC/LD, which would make `?=` a no-op.
KCC ?= x86_64-elf-gcc
KLD ?= x86_64-elf-ld
ASM ?= nasm

# Extra flags from the command line, e.g. `make EXTRA_CFLAGS=-Werror` (tests/run.sh does this).
EXTRA_CFLAGS ?=

CFLAGS := -I src/intf -ffreestanding -nostdlib -fno-pic -fno-pie -mno-red-zone -mgeneral-regs-only \
          -fno-stack-protector -fno-asynchronous-unwind-tables -fno-tree-loop-distribute-patterns \
          -Wall -Wextra -O2 -MMD -MP $(EXTRA_CFLAGS)

kernel_source_files := $(sort $(shell find src/impl/kernel -name '*.c'))
kernel_object_files := $(patsubst src/impl/kernel/%.c, build/kernel/%.o, $(kernel_source_files))

x86_64_c_source_files := $(sort $(shell find src/impl/x86_64 -name '*.c'))
x86_64_c_object_files := $(patsubst src/impl/x86_64/%.c, build/x86_64/%.o, $(x86_64_c_source_files))

x86_64_asm_source_files := $(sort $(shell find src/impl/x86_64 -name '*.asm'))
x86_64_asm_object_files := $(patsubst src/impl/x86_64/%.asm, build/x86_64/%.o, $(x86_64_asm_source_files))

x86_64_object_files := $(x86_64_c_object_files) $(x86_64_asm_object_files)

# Test kernel: same objects as the real kernel, but tests/kernel/*.c replace main.c.
test_source_files := $(sort $(shell find tests/kernel -name '*.c'))
test_object_files := $(patsubst tests/kernel/%.c, build/tests/%.o, $(test_source_files))
kernel_objects_without_main := $(filter-out build/kernel/main.o, $(kernel_object_files))

deps := $(kernel_object_files:.o=.d) $(x86_64_c_object_files:.o=.d) $(test_object_files:.o=.d)

.PHONY: build-x86_64 test-iso test clean run
.DEFAULT_GOAL := build-x86_64

$(kernel_object_files): build/kernel/%.o : src/impl/kernel/%.c
	mkdir -p $(dir $@)
	$(KCC) -c $(CFLAGS) $< -o $@

$(x86_64_c_object_files): build/x86_64/%.o : src/impl/x86_64/%.c
	mkdir -p $(dir $@)
	$(KCC) -c $(CFLAGS) $< -o $@

$(x86_64_asm_object_files): build/x86_64/%.o : src/impl/x86_64/%.asm
	mkdir -p $(dir $@)
	$(ASM) -f elf64 $< -o $@

$(test_object_files): build/tests/%.o : tests/kernel/%.c
	mkdir -p $(dir $@)
	$(KCC) -c $(CFLAGS) $< -o $@

build-x86_64: $(kernel_object_files) $(x86_64_object_files)
	mkdir -p dist/x86_64
	$(KLD) -n -z noexecstack -o dist/x86_64/kernel.bin -T targets/x86_64/linker.ld $(kernel_object_files) $(x86_64_object_files)
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64/kernel.iso targets/x86_64/iso

# Bootable test kernel (dist/x86_64/kernel-test.iso). Built inside the Docker image.
test-iso: $(kernel_objects_without_main) $(x86_64_object_files) $(test_object_files)
	mkdir -p dist/x86_64 build/iso-test/boot/grub
	$(KLD) -n -z noexecstack -o dist/x86_64/kernel-test.bin -T targets/x86_64/linker.ld $^
	cp dist/x86_64/kernel-test.bin build/iso-test/boot/kernel.bin
	cp targets/x86_64/iso/boot/grub/grub.cfg build/iso-test/boot/grub/grub.cfg
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64/kernel-test.iso build/iso-test

# Build everything in Docker, then boot-test and unit-test the kernel in QEMU.
# Run this on your machine (not inside the container): it needs Docker and QEMU.
test:
	./tests/run.sh

run:
	qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso -serial stdio

clean:
	rm -rf build dist targets/x86_64/iso/boot/kernel.bin

-include $(deps)
