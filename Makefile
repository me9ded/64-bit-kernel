# Toolchain (override from the environment, e.g. the Docker image sets KCC=gcc KLD=ld).
# Named KCC/KLD because make predefines CC/LD, which would make `?=` a no-op.
KCC ?= x86_64-elf-gcc
KLD ?= x86_64-elf-ld
ASM ?= nasm

CFLAGS := -I src/intf -ffreestanding -nostdlib -fno-pic -fno-pie -mno-red-zone -mgeneral-regs-only \
          -fno-stack-protector -fno-asynchronous-unwind-tables \
          -Wall -Wextra -O2 -MMD -MP

kernel_source_files := $(shell find src/impl/kernel -name '*.c')
kernel_object_files := $(patsubst src/impl/kernel/%.c, build/kernel/%.o, $(kernel_source_files))

x86_64_c_source_files := $(shell find src/impl/x86_64 -name '*.c')
x86_64_c_object_files := $(patsubst src/impl/x86_64/%.c, build/x86_64/%.o, $(x86_64_c_source_files))

x86_64_asm_source_files := $(shell find src/impl/x86_64 -name '*.asm')
x86_64_asm_object_files := $(patsubst src/impl/x86_64/%.asm, build/x86_64/%.o, $(x86_64_asm_source_files))

x86_64_object_files := $(x86_64_c_object_files) $(x86_64_asm_object_files)
deps := $(kernel_object_files:.o=.d) $(x86_64_c_object_files:.o=.d)

.PHONY: build-x86_64 clean run
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

build-x86_64: $(kernel_object_files) $(x86_64_object_files)
	mkdir -p dist/x86_64
	$(KLD) -n -z noexecstack -o dist/x86_64/kernel.bin -T targets/x86_64/linker.ld $(kernel_object_files) $(x86_64_object_files)
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin
	grub-mkrescue /usr/lib/grub/i386-pc -o dist/x86_64/kernel.iso targets/x86_64/iso

run:
	qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso

clean:
	rm -rf build dist targets/x86_64/iso/boot/kernel.bin

-include $(deps)
