#include <rtc.h>
#include <cpu.h>
#include <stdbool.h>

#define CMOS_INDEX 0x70
#define CMOS_DATA  0x71

#define REG_SECONDS  0x00
#define REG_MINUTES  0x02
#define REG_HOURS    0x04
#define REG_DAY      0x07
#define REG_MONTH    0x08
#define REG_YEAR     0x09
#define REG_STATUS_A 0x0A
#define REG_STATUS_B 0x0B

#define STATUS_A_UPDATING 0x80  /* the clock is updating: values may be inconsistent */
#define STATUS_B_24_HOUR  0x02
#define STATUS_B_BINARY   0x04  /* otherwise values are BCD: 0x59 means 59 */
#define HOUR_PM           0x80  /* in 12-hour mode */

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_INDEX, reg);
    return inb(CMOS_DATA);
}

static void read_raw(struct rtc_time* time) {
    /* an update takes ~2 ms; the bound avoids hanging if there's no RTC */
    for (int i = 0; i < 100000 && (cmos_read(REG_STATUS_A) & STATUS_A_UPDATING); i++) {
    }
    time->second = cmos_read(REG_SECONDS);
    time->minute = cmos_read(REG_MINUTES);
    time->hour = cmos_read(REG_HOURS);
    time->day = cmos_read(REG_DAY);
    time->month = cmos_read(REG_MONTH);
    time->year = cmos_read(REG_YEAR);
}

static bool same(const struct rtc_time* a, const struct rtc_time* b) {
    return a->second == b->second && a->minute == b->minute && a->hour == b->hour &&
           a->day == b->day && a->month == b->month && a->year == b->year;
}

static uint8_t from_bcd(uint8_t value) {
    return (uint8_t) ((value >> 4) * 10 + (value & 0x0F));
}

void rtc_read(struct rtc_time* time) {
    /* Read until two reads in a row agree, so we never mix values from before
       and after an update (e.g. 12:59:59 + 13:00:00 = 12:00:00). */
    struct rtc_time previous;
    read_raw(time);
    for (int attempt = 0; attempt < 5; attempt++) {
        previous = *time;
        read_raw(time);
        if (same(&previous, time)) {
            break;
        }
    }

    uint8_t status_b = cmos_read(REG_STATUS_B);
    bool pm = time->hour & HOUR_PM;
    time->hour &= (uint8_t) ~HOUR_PM;

    if (!(status_b & STATUS_B_BINARY)) {
        time->second = from_bcd(time->second);
        time->minute = from_bcd(time->minute);
        time->hour = from_bcd(time->hour);
        time->day = from_bcd(time->day);
        time->month = from_bcd(time->month);
        time->year = from_bcd((uint8_t) time->year);
    }

    if (!(status_b & STATUS_B_24_HOUR)) {
        /* 12-hour clock: 12 AM is 0:00, 12 PM is 12:00 */
        time->hour = (uint8_t) (time->hour % 12 + (pm ? 12 : 0));
    }

    time->year += 2000; /* the RTC stores only two digits */
}
