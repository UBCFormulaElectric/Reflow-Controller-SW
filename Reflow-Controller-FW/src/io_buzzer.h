#pragma once
#include <stdbool.h>

enum buzz_duration {
    SHORT_BUZZ = 300,
    LONG_BUZZ = 1000
};

void io_buzzer_buzz(enum buzz_duration millisOn);
bool io_buzzer_is_on();
void io_buzzer_tick();
