#include "test.h"
#include <rtc.h>
#include <timer.h>

TEST(rtc_values_are_in_range) {
    struct rtc_time now;
    rtc_read(&now);
    CHECK(now.year >= 2024 && now.year <= 2099);
    CHECK(now.month >= 1 && now.month <= 12);
    CHECK(now.day >= 1 && now.day <= 31);
    CHECK(now.hour < 24);
    CHECK(now.minute < 60);
    CHECK(now.second < 60);
}

TEST(rtc_clock_advances) {
    struct rtc_time before, after;
    rtc_read(&before);
    sleep_ms(1100);
    rtc_read(&after);
    uint32_t start = before.hour * 3600u + before.minute * 60u + before.second;
    uint32_t end = after.hour * 3600u + after.minute * 60u + after.second;
    uint32_t elapsed = (end + 86400 - start) % 86400; /* handles midnight */
    CHECK(elapsed >= 1);
    CHECK(elapsed <= 5);
}
