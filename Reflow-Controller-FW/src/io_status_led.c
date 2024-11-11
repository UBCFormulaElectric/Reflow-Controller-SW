#include "io_status_led.h"
#include "main.h"

void io_status_led_set(const bool r, const bool g, const bool b) {
    HAL_GPIO_WritePin(STATUS_R_GPIO_Port, STATUS_R_Pin, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STATUS_B_GPIO_Port, STATUS_B_Pin, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STATUS_G_GPIO_Port, STATUS_G_Pin, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
