#include "test.h"
#include <serial.h>

TEST(serial_init_finds_com1) {
    CHECK_EQ_INT(serial_init(), 0);
}
