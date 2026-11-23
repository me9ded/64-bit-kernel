#include "test.h"
#include <cpu.h>
#include <timer.h>

TEST(timer_runs_at_100_hz) {
    CHECK_EQ_INT(timer_hz(), 100);
}

TEST(timer_ticks_advance) {
    uint64_t start = timer_ticks();
    /* only the timer IRQ is unmasked, so each hlt wakes up on a tick */
    for (int i = 0; i < 3; i++) {
        hlt();
    }
    CHECK(timer_ticks() > start);
}

TEST(sleep_ms_waits_long_enough) {
    uint64_t start = timer_ticks();
    sleep_ms(50);
    uint64_t elapsed = timer_ticks() - start;
    CHECK(elapsed >= 5);   /* 50 ms at 100 Hz */
    CHECK(elapsed <= 50);  /* generous: emulators can be slow */
}

TEST(uptime_matches_ticks) {
    uint64_t ticks = timer_ticks();
    uint64_t uptime = timer_uptime_ms();
    /* 10 ms per tick; allow for a tick arriving between the two reads */
    CHECK(uptime >= ticks * 10);
    CHECK(uptime <= (ticks + 2) * 10);
}
