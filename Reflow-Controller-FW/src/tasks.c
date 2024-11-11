#include "main.h"
#include "tasks.h"

#include <stdio.h>
#include "string.h"
#include "usbd_cdc_if.h"

// io
#include "io_buzzer.h"
#include "io_tempsns.h"
#include "io_status_led.h"

#define UPDATE_TIME 250

// PUBLIC FUNCTIONS

void task_init() {
	HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
	io_status_led_set(0, 0, 0);
}

void task_superloop() {
    const struct temp_read_res r = io_tempsns_read();
    if (r.status == READ_TEMP_OK) {
        char temperatureMsg[64];
        sprintf(temperatureMsg, "%s %.2f \n", "Temp", r.value);
        CDC_Transmit_FS((uint8_t*) temperatureMsg, strlen(temperatureMsg));
    }

    io_buzzer_tick();
    HAL_Delay(UPDATE_TIME);
}
