#include "test.h"
#include <bootinfo.h>

/* --- building fake multiboot2 blocks ------------------------------------ */

static uint64_t storage[256]; /* 2 KiB, 8-byte aligned like the real thing */

struct builder {
    uint8_t* data;
    uint32_t size;
};

struct fake_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
};

static void put_u32(uint8_t* p, uint32_t value) {
    memcpy(p, &value, sizeof value);
}

static void begin(struct builder* b) {
    memset(storage, 0, sizeof storage);
    b->data = (uint8_t*) storage;
    b->size = 8; /* total_size + reserved */
}

static uint8_t* add_tag(struct builder* b, uint32_t type, const void* payload, uint32_t payload_size) {
    uint8_t* tag = b->data + b->size;
    put_u32(tag, type);
    put_u32(tag + 4, 8 + payload_size);
    memcpy(tag + 8, payload, payload_size);
    b->size += (8 + payload_size + 7) & ~7u;
    return tag;
}

static void add_string(struct builder* b, uint32_t type, const char* text) {
    add_tag(b, type, text, (uint32_t) strlen(text) + 1);
}

static void add_memory_map(struct builder* b, const struct fake_entry* entries, uint32_t count) {
    uint8_t payload[8 + 40 * sizeof(struct fake_entry)];
    put_u32(payload, sizeof(struct fake_entry)); /* entry_size */
    put_u32(payload + 4, 0);                     /* entry_version */
    memcpy(payload + 8, entries, count * sizeof(struct fake_entry));
    add_tag(b, 6, payload, 8 + count * (uint32_t) sizeof(struct fake_entry));
}

static void finish(struct builder* b) {
    add_tag(b, 0, NULL, 0);    /* end tag */
    put_u32(b->data, b->size); /* total_size */
}

static const struct fake_entry three_regions[] = {
    { 0x0, 0x9FC00, MEMORY_USABLE, 0 },
    { 0xF0000, 0x10000, MEMORY_RESERVED, 0 },
    { 0x100000, 0x7EE0000, MEMORY_USABLE, 0 },
};

/* --- parser tests -------------------------------------------------------- */

TEST(bootinfo_parses_all_tags) {
    struct builder b;
    begin(&b);
    add_string(&b, 1, "quiet");
    add_tag(&b, 99, "12345", 5);           /* unknown tag, needs padding: skipped */
    add_string(&b, 2, "TestLoader");
    add_memory_map(&b, three_regions, 3);
    finish(&b);

    struct boot_info info;
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), 0);
    CHECK_EQ_STR(info.command_line, "quiet");
    CHECK_EQ_STR(info.bootloader_name, "TestLoader");
    CHECK_EQ_INT(info.info_size, b.size);
    CHECK_EQ_INT(info.region_count, 3);
    CHECK(!info.regions_truncated);
    CHECK_EQ_INT(info.regions[1].base, 0xF0000);
    CHECK_EQ_INT(info.regions[1].length, 0x10000);
    CHECK_EQ_INT(info.regions[1].type, MEMORY_RESERVED);
    CHECK_EQ_INT(bootinfo_usable_bytes(&info), 0x9FC00 + 0x7EE0000);
}

TEST(bootinfo_without_optional_tags) {
    struct builder b;
    begin(&b);
    finish(&b);

    struct boot_info info;
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), 0);
    CHECK_EQ_INT(info.region_count, 0);
    CHECK_EQ_STR(info.bootloader_name, "");
    CHECK_EQ_STR(info.command_line, "");
}

TEST(bootinfo_caps_region_count) {
    struct fake_entry many[40];
    for (int i = 0; i < 40; i++) {
        many[i] = (struct fake_entry) { (uint64_t) i << 20, 1 << 20, MEMORY_USABLE, 0 };
    }
    struct builder b;
    begin(&b);
    add_memory_map(&b, many, 40);
    finish(&b);

    struct boot_info info;
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), 0);
    CHECK_EQ_INT(info.region_count, BOOT_INFO_MAX_REGIONS);
    CHECK(info.regions_truncated);
}

TEST(bootinfo_rejects_bad_blocks) {
    struct boot_info info;
    struct builder b;

    CHECK_EQ_INT(bootinfo_parse(NULL, &info), -1);

    begin(&b);
    finish(&b);
    CHECK_EQ_INT(bootinfo_parse(b.data + 4, &info), -1);    /* not 8-byte aligned */

    put_u32(b.data, 4);                                    /* total_size too small */
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), -1);

    begin(&b);                                             /* no end tag */
    add_string(&b, 2, "TestLoader");
    put_u32(b.data, b.size);
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), -1);

    begin(&b);                                             /* tag runs past the end */
    uint8_t* tag = add_tag(&b, 99, "x", 1);
    finish(&b);
    put_u32(tag + 4, 4096);
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), -1);

    begin(&b);                                             /* string without NUL */
    add_tag(&b, 2, "abc", 3);
    finish(&b);
    CHECK_EQ_INT(bootinfo_parse(b.data, &info), -1);
}

/* --- the real boot information from GRUB --------------------------------- */

TEST(bootinfo_from_grub) {
    const struct boot_info* info = boot_info();
    if (info == NULL) {
        test_fail(__FILE__, __LINE__, "boot_info() is NULL: the multiboot pointer was lost");
        return;
    }
    uint64_t usable_mib = bootinfo_usable_bytes(info) >> 20;
    CHECK(usable_mib >= 120 && usable_mib <= 128); /* tests/run.sh passes -m 128M */
    CHECK(strstr(info->bootloader_name, "GRUB") != NULL);
    CHECK(info->region_count >= 2);
    CHECK(info->info_address != 0 && info->info_address < (1ull << 30));
}

TEST(bootinfo_init_rejects_unmapped_addresses) {
    if (boot_info() == NULL) {
        test_fail(__FILE__, __LINE__, "no real boot information to restore afterwards");
        return;
    }
    uint64_t real_address = boot_info()->info_address;

    CHECK_EQ_INT(bootinfo_init(0), -1);
    CHECK(boot_info() == NULL);
    CHECK_EQ_INT(bootinfo_init(1ull << 30), -1);     /* not mapped: must not be read */
    CHECK_EQ_INT(bootinfo_init(0xFFFFFFFFull), -1);

    CHECK_EQ_INT(bootinfo_init(real_address), 0);    /* restore for later tests */
    CHECK(boot_info() != NULL);
}

TEST(kernel_bounds_from_linker_script) {
    uintptr_t start = (uintptr_t) __kernel_start;
    uintptr_t end = (uintptr_t) __kernel_end;
    CHECK_EQ_INT(start, 0x100000);
    CHECK(end > start);
    CHECK_EQ_INT(end % 4096, 0);

    uintptr_t code = (uintptr_t) bootinfo_parse;
    uintptr_t data = (uintptr_t) storage;
    CHECK(code >= start && code < end);
    CHECK(data >= start && data < end);

    const struct boot_info* info = boot_info();
    if (info != NULL) {
        /* GRUB must not have put its information inside the kernel image */
        CHECK(info->info_address >= end || info->info_address + info->info_size <= start);
    }
}
