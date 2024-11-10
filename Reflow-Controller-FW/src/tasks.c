#include "main.h"
#include "tasks.h"

#include <stdio.h>
#include "string.h";
#include "usbd_cdc_if.h";

#define THERM_WHOLE_SHIFT 4
#define THERM_FRAC_MASK 0xC
#define THERM_FRAC_SHIFT 2
#define IC_WHOLE_SHIFT 8
#define IC_FRAC_MASK 0xF0
#define IC_FRAC_SHIFT 4
#define THERM_FAULT_MASK 0x1
#define IC_FAULT_MASK 0x7

#define MIN_TEMP 10
#define MAX_TEMP 400

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

#define UPDATE_TIME 250

#define SHORTBUZZ 300
#define LONGBUZZ 1000

static double currentOvenTemp = 0;
static double currentBoardTemp = 0;
static uint8_t usbBuffer[64];
static int buzzOn = 0;
static int buzzLengthInMillis = 0;

static int readTemperature() {
    //Receive a 32-bit transfer from the thermocouple ADC chip
    char spi_buf[4];
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    const HAL_StatusTypeDef result = HAL_SPI_Receive(&hspi2, (uint8_t *) &spi_buf, 4, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    if (result != HAL_OK) {
        return -1;
    }

    //First 16 bits are for the thermocouple's temp, the last 16 are for the sensor's temp
    const int thermocouple_data = spi_buf[0] << 8 | spi_buf[1];
    const int ambient_data = spi_buf[2] << 8 | spi_buf[3];

    if (thermocouple_data & THERM_FAULT_MASK || ambient_data & IC_FAULT_MASK)
        return -1;

    const int ovenWhole = thermocouple_data >> THERM_WHOLE_SHIFT;
    const double ovenFrac = ((thermocouple_data & THERM_FRAC_MASK) >> THERM_FRAC_SHIFT) * 0.25;
    const int ambientWhole = ambient_data >> IC_WHOLE_SHIFT;
    const double ambientFrac = ((ambient_data & IC_FRAC_MASK) >> IC_FRAC_SHIFT) * 0.0625;

    const double newOvenTemp = ovenWhole + ovenFrac;
    const double newBoardTemp = ambientWhole + ambientFrac;
    if (newOvenTemp < MIN_TEMP || newOvenTemp > MAX_TEMP
        || newBoardTemp < MIN_TEMP || newBoardTemp > MAX_TEMP)
        return -1;

    currentOvenTemp = newOvenTemp;
    currentBoardTemp = newBoardTemp;
    return 0;
}

static void setStatusLED(const int r, const int g, const int b) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void buzz(const int millisOn) {
    buzzOn = 1;
    buzzLengthInMillis = millisOn;
}

static void onUsbReceive() {
    if (usbBuffer == NULL) {
        return;
    }

    //	char response[64] = RESPONSE_INVALID;
    if (!strcmp(usbBuffer, COMMAND_HEAT)) {
        setStatusLED(1, 0, 0);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
        //		strcpy(response, RESPONSE_HEAT);
    } else if (!strcmp(usbBuffer, COMMAND_IDLE)) {
        setStatusLED(0, 0, 1);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
        //		strcpy(response, RESPONSE_IDLE);
    } else if (!strcmp(usbBuffer, COMMAND_END)) {
        setStatusLED(0, 1, 0);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
        buzz(LONGBUZZ);
        //		strcpy(response, RESPONSE_END);
    } else if (!strcmp(usbBuffer, COMMAND_CONNECTED)) {
        setStatusLED(0, 1, 0);
        buzz(SHORTBUZZ);
        //		strcpy(response, RESPONSE_CONNECTED);
    } else if (!strcmp(usbBuffer, COMMAND_DISCONNECTED)) {
        setStatusLED(0, 0, 0);
        buzz(SHORTBUZZ);
        //		strcpy(response, RESPONSE_DISCONNECTED);
    }

    //	char confirmationMsg[64];
    //	sprintf(confirmationMsg, "Received: '%s'", usbBuffer);
    //	CDC_Transmit_FS((uint8_t *) confirmationMsg, strlen(confirmationMsg));
    //	CDC_Transmit_FS((uint8_t *) response, strlen(response));
}


// PUBLIC FUNCTIONS

void task_init() {
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	setStatusLED(0, 0, 0);
};

void task_superloop() {
        const int result = readTemperature();
        if (result == -1)
            return;

        char temperatureMsg[64];
        sprintf(temperatureMsg, "%s %.2f \n", TEMP_MSG, currentOvenTemp);
        CDC_Transmit_FS((uint8_t *) temperatureMsg, strlen(temperatureMsg));

        if (buzzOn) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
            HAL_Delay(buzzLengthInMillis);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
            buzzOn = 0;
        }

        HAL_Delay(UPDATE_TIME);
};