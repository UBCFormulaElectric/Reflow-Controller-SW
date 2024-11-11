#include "hw_usb.h"
#include <string.h>

#include "main.h"
#include "usbd_def.h"
#include "usbd_cdc.h"

#include "io_status_led.h"
#include "io_buzzer.h"

#define COMMAND_HEAT "Heat\n"
#define COMMAND_IDLE "Idle\n"
#define COMMAND_END "End\n"
#define COMMAND_CONNECTED "Connected\n"
#define COMMAND_DISCONNECTED "Disconnected\n"
#define RESPONSE_HEAT "Heat received\n"
#define RESPONSE_IDLE "Idle received\n"
#define RESPONSE_END "End received\n"
#define RESPONSE_CONNECTED "Connected received\n"
#define RESPONSE_DISCONNECTED "Disconnected command\n"
#define RESPONSE_INVALID "Invalid command\n"
#define TEMP_MSG "Temp"

static char usbBuffer[64];

// hitting a raw extern here bc usb_device.c and usb_device.h are stupid
// also this is how it is done in usbd_cdc-if.c
extern USBD_HandleTypeDef hUsbDeviceFS;

int8_t onUsbReceive(uint8_t* Buf, const uint32_t *Len) {
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);

    memset(usbBuffer, '\0', 64);
    memcpy(usbBuffer, Buf, *Len);
    memset(Buf, '\0', *Len);

    //	char response[64] = RESPONSE_INVALID;
    if (!strcmp(usbBuffer, COMMAND_HEAT)) {
        io_status_led_set(1, 0, 0);
        HAL_GPIO_WritePin(SSR_CLOSED_GPIO_Port, SSR_CLOSED_Pin, GPIO_PIN_SET);
        //		strcpy(response, RESPONSE_HEAT);
    } else if (!strcmp(usbBuffer, COMMAND_IDLE)) {
        io_status_led_set(0, 0, 1);
        HAL_GPIO_WritePin(SSR_CLOSED_GPIO_Port, SSR_CLOSED_Pin, GPIO_PIN_RESET);
        //		strcpy(response, RESPONSE_IDLE);
    } else if (!strcmp(usbBuffer, COMMAND_END)) {
        io_status_led_set(0, 1, 0);
        HAL_GPIO_WritePin(SSR_CLOSED_GPIO_Port, SSR_CLOSED_Pin, GPIO_PIN_RESET);
        io_buzzer_buzz(LONG_BUZZ);
        //		strcpy(response, RESPONSE_END);
    } else if (!strcmp(usbBuffer, COMMAND_CONNECTED)) {
        io_status_led_set(0, 1, 0);
        io_buzzer_buzz(SHORT_BUZZ);
        //		strcpy(response, RESPONSE_CONNECTED);
    } else if (!strcmp(usbBuffer, COMMAND_DISCONNECTED)) {
        io_status_led_set(0, 0, 0);
        io_buzzer_buzz(SHORT_BUZZ);
        //		strcpy(response, RESPONSE_DISCONNECTED);
    }

    //	char confirmationMsg[64];
    //	sprintf(confirmationMsg, "Received: '%s'", usbBuffer);
    //	CDC_Transmit_FS((uint8_t *) confirmationMsg, strlen(confirmationMsg));
    //	CDC_Transmit_FS((uint8_t *) response, strlen(response));
    return USBD_OK;
}
