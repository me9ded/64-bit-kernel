#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Information from the bootloader (GRUB, via the multiboot2 protocol):
 * the physical memory map, the bootloader's name and the kernel command line.
 */

/* Memory region types, as numbered by multiboot2 */
enum {
    MEMORY_USABLE = 1,
    MEMORY_RESERVED = 2,
    MEMORY_ACPI_RECLAIMABLE = 3,  /* usable once ACPI tables have been read */
    MEMORY_ACPI_NVS = 4,          /* must be preserved */
    MEMORY_BAD = 5,               /* defective RAM */
};

struct memory_region {
    uint64_t base;
    uint64_t length;
    uint32_t type;
};

#define BOOT_INFO_MAX_REGIONS 32

struct boot_info {
    uint64_t info_address;        /* where the multiboot2 block is (don't overwrite it) */
    uint32_t info_size;
    const char* bootloader_name;  /* "" if not provided */
    const char* command_line;     /* "" if not provided */
    size_t region_count;
    bool regions_truncated;       /* more than BOOT_INFO_MAX_REGIONS were provided */
    struct memory_region regions[BOOT_INFO_MAX_REGIONS];
};

/* Kernel image bounds, from targets/x86_64/linker.ld (page-aligned). */
extern char __kernel_start[];
extern char __kernel_end[];

/*
 * Parse a multiboot2 information block. Returns 0 on success, -1 if it's
 * malformed (bad sizes, missing end tag, ...). Strings point into the block.
 */
int bootinfo_parse(const void* info, struct boot_info* out);

/* Parse the block GRUB passed to kernel_main and keep it for boot_info(). */
int bootinfo_init(uint64_t multiboot_info_address);

/* The boot information from bootinfo_init(), or NULL if that failed. */
const struct boot_info* boot_info(void);

/* Total bytes in MEMORY_USABLE regions. */
uint64_t bootinfo_usable_bytes(const struct boot_info* info);

/* "usable", "reserved", "ACPI reclaimable", ... */
const char* memory_type_name(uint32_t type);
