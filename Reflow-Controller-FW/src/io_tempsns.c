#include "io_tempsns.h"

#include "main.h"

#define THERM_WHOLE_SHIFT 4
#define THERM_FRAC_MASK 0xC
#define THERM_FRAC_SHIFT 2
#define IC_WHOLE_SHIFT 8
#define IC_FRAC_MASK 0xF0
#define IC_FRAC_SHIFT 4
#define THERM_FAULT_MASK 0x1
#define IC_FAULT_MASK 0x7

static double currentOvenTemp = 0;
static double currentBoardTemp = 0;

#define MIN_TEMP 10
#define MAX_TEMP 400

struct temp_read_res io_tempsns_read() {
    //Receive a 32-bit transfer from the thermocouple ADC chip
    char spi_buf[4];
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    const HAL_StatusTypeDef result = HAL_SPI_Receive(&hspi2, (uint8_t *) &spi_buf, 4, 100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    if (result != HAL_OK) {
        return (struct temp_read_res){READ_TEMP_FAIL, -1};
    }

    //First 16 bits are for the thermocouple's temp, the last 16 are for the sensor's temp
    const int thermocouple_data = spi_buf[0] << 8 | spi_buf[1];
    const int ambient_data = spi_buf[2] << 8 | spi_buf[3];

    if (thermocouple_data & THERM_FAULT_MASK || ambient_data & IC_FAULT_MASK)
        return (struct temp_read_res){READ_TEMP_FAIL, -1};

    const int ovenWhole = thermocouple_data >> THERM_WHOLE_SHIFT;
    const double ovenFrac = ((thermocouple_data & THERM_FRAC_MASK) >> THERM_FRAC_SHIFT) * 0.25;
    const int ambientWhole = ambient_data >> IC_WHOLE_SHIFT;
    const double ambientFrac = ((ambient_data & IC_FRAC_MASK) >> IC_FRAC_SHIFT) * 0.0625;

    const double newOvenTemp = ovenWhole + ovenFrac;
    const double newBoardTemp = ambientWhole + ambientFrac;
    if (newOvenTemp < MIN_TEMP || newOvenTemp > MAX_TEMP
        || newBoardTemp < MIN_TEMP || newBoardTemp > MAX_TEMP)
        return (struct temp_read_res){READ_TEMP_FAIL, -1};

    currentOvenTemp = newOvenTemp;
    currentBoardTemp = newBoardTemp;
    return (struct temp_read_res){READ_TEMP_OK, currentOvenTemp};
}