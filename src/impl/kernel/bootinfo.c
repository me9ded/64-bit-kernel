#include <bootinfo.h>
#include <string.h>

/* multiboot2 tag types we read (others are skipped) */
#define TAG_END              0
#define TAG_COMMAND_LINE     1
#define TAG_BOOTLOADER_NAME  2
#define TAG_MEMORY_MAP       6

#define HEADER_SIZE 8         /* total_size + reserved, both uint32 */
#define TAG_HEADER_SIZE 8     /* type + size, both uint32 */

struct mmap_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

static uint32_t read_u32(const uint8_t* p) {
    uint32_t value;
    memcpy(&value, p, sizeof value);
    return value;
}

/* A string tag is valid only if it ends with a NUL inside the tag. */
static const char* tag_string(const uint8_t* tag, uint32_t size) {
    const char* text = (const char*) (tag + TAG_HEADER_SIZE);
    for (uint32_t i = 0; i < size - TAG_HEADER_SIZE; i++) {
        if (text[i] == '\0') {
            return text;
        }
    }
    return NULL;
}

static int parse_memory_map(const uint8_t* tag, uint32_t size, struct boot_info* out) {
    /* tag header, then entry_size and entry_version (uint32 each), then entries */
    if (size < TAG_HEADER_SIZE + 8) {
        return -1;
    }
    uint32_t entry_size = read_u32(tag + 8);
    if (entry_size < sizeof(struct mmap_entry)) {
        return -1;
    }

    for (uint32_t offset = TAG_HEADER_SIZE + 8; offset + entry_size <= size; offset += entry_size) {
        if (out->region_count == BOOT_INFO_MAX_REGIONS) {
            out->regions_truncated = true;
            break;
        }
        struct mmap_entry entry;
        memcpy(&entry, tag + offset, sizeof entry);
        out->regions[out->region_count++] = (struct memory_region) {
            .base = entry.base,
            .length = entry.length,
            .type = entry.type,
        };
    }
    return 0;
}

int bootinfo_parse(const void* info, struct boot_info* out) {
    memset(out, 0, sizeof *out);
    out->bootloader_name = "";
    out->command_line = "";

    if (info == NULL || ((uintptr_t) info & 7) != 0) {
        return -1; /* multiboot2 guarantees 8-byte alignment */
    }

    const uint8_t* base = info;
    uint32_t total_size = read_u32(base);
    if (total_size < HEADER_SIZE + TAG_HEADER_SIZE) {
        return -1;
    }
    out->info_address = (uint64_t) (uintptr_t) info;
    out->info_size = total_size;

    uint32_t offset = HEADER_SIZE;
    while (offset + TAG_HEADER_SIZE <= total_size) {
        const uint8_t* tag = base + offset;
        uint32_t type = read_u32(tag);
        uint32_t size = read_u32(tag + 4);

        if (size < TAG_HEADER_SIZE || size > total_size - offset) {
            return -1; /* the tag would run past the end of the block */
        }

        switch (type) {
            case TAG_END:
                return 0;
            case TAG_COMMAND_LINE:
            case TAG_BOOTLOADER_NAME: {
                const char* text = tag_string(tag, size);
                if (text == NULL) {
                    return -1;
                }
                if (type == TAG_COMMAND_LINE) {
                    out->command_line = text;
                } else {
                    out->bootloader_name = text;
                }
                break;
            }
            case TAG_MEMORY_MAP:
                if (parse_memory_map(tag, size, out) != 0) {
                    return -1;
                }
                break;
            default:
                break; /* a tag we don't need */
        }

        offset += (size + 7) & ~7u; /* tags start on 8-byte boundaries */
    }

    return -1; /* no end tag */
}

static struct boot_info global_boot_info;
static bool global_boot_info_valid;

/* The boot page tables (boot/main.asm) map only the first 1 GiB. */
#define BOOT_MAPPED_LIMIT (1ull << 30)

int bootinfo_init(uint64_t multiboot_info_address) {
    global_boot_info_valid = false;

    /* Don't touch a pointer outside mapped memory: that would page-fault
       before the interrupt handlers exist. */
    if (multiboot_info_address == 0 || multiboot_info_address > BOOT_MAPPED_LIMIT - HEADER_SIZE) {
        return -1;
    }
    uint32_t total_size = read_u32((const uint8_t*) (uintptr_t) multiboot_info_address);
    if (total_size > BOOT_MAPPED_LIMIT - multiboot_info_address) {
        return -1;
    }

    int status = bootinfo_parse((const void*) (uintptr_t) multiboot_info_address, &global_boot_info);
    global_boot_info_valid = (status == 0);
    return status;
}

const struct boot_info* boot_info(void) {
    return global_boot_info_valid ? &global_boot_info : NULL;
}

uint64_t bootinfo_usable_bytes(const struct boot_info* info) {
    uint64_t total = 0;
    for (size_t i = 0; i < info->region_count; i++) {
        if (info->regions[i].type == MEMORY_USABLE) {
            total += info->regions[i].length;
        }
    }
    return total;
}

const char* memory_type_name(uint32_t type) {
    switch (type) {
        case MEMORY_USABLE: return "usable";
        case MEMORY_RESERVED: return "reserved";
        case MEMORY_ACPI_RECLAIMABLE: return "ACPI reclaimable";
        case MEMORY_ACPI_NVS: return "ACPI NVS";
        case MEMORY_BAD: return "bad RAM";
        default: return "unknown";
    }
}
