#include "io_buzzer.h"
#include <stdint.h>
#include "main.h"

static bool is_buzz_on = false;
static uint32_t buzz_expiry_time = 0;

void io_buzzer_buzz(const enum buzz_duration millisOn) {
    is_buzz_on = true;
    // set the buzz_expiry_time
    buzz_expiry_time = HAL_GetTick() + millisOn;
}

bool io_buzzer_is_on() {
    return is_buzz_on;
}

void io_buzzer_tick() {
    // if buzz_expiry_time is in the past, stop the buzzer
    if(buzz_expiry_time < HAL_GetTick())
        is_buzz_on = false;
    HAL_GPIO_WritePin(BUZZ_GPIO_Port, BUZZ_Pin, is_buzz_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}