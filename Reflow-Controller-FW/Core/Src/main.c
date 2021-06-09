/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "usbd_cdc_if.h";
#include "string.h";

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
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
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */
double currentOvenTemp = 0;
double currentBoardTemp = 0;
uint8_t usbBuffer[64];
int buzzOn = 0;
int buzzLengthInMillis = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */
int readTemperature(void);
void onUsbReceive(void);
void setStatusLED(int r, int g, int b);
void buzz(int);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	setStatusLED(0, 0, 0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		int result = readTemperature();
		if(result == -1) {
			continue;
		}

		char temperatureMsg[64];
		sprintf(temperatureMsg, "%s %.2f \n", TEMP_MSG, currentOvenTemp);
		CDC_Transmit_FS((uint8_t *) temperatureMsg, strlen(temperatureMsg));

		if(buzzOn) {
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
			HAL_Delay(buzzLengthInMillis);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
			buzzOn = 0;
		}

		HAL_Delay(UPDATE_TIME);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_MSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA0 PA1 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB11 PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
int readTemperature() {
	//Receive a 32-bit transfer from the thermocouple ADC chip
	char spi_buf[4];
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
	HAL_StatusTypeDef result = HAL_SPI_Receive(&hspi2, (uint8_t *)&spi_buf, 4, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

	if(result != HAL_OK) {
		return -1;
	}

	//First 16 bits are for the thermocouple's temp, the last 16 are for the sensor's temp
	int thermocouple_data = spi_buf[0] << 8 | spi_buf[1];
	int ambient_data = spi_buf[2] << 8 | spi_buf[3];

	if(thermocouple_data & THERM_FAULT_MASK || ambient_data & IC_FAULT_MASK)
		return -1;

	int ovenWhole = thermocouple_data >> THERM_WHOLE_SHIFT;
	double ovenFrac = ((thermocouple_data & THERM_FRAC_MASK) >> THERM_FRAC_SHIFT) * 0.25;
	int ambientWhole = ambient_data >> IC_WHOLE_SHIFT;
	double ambientFrac = ((ambient_data & IC_FRAC_MASK) >> IC_FRAC_SHIFT) * 0.0625;

	double newOvenTemp = ovenWhole + ovenFrac;
	double newBoardTemp = ambientWhole + ambientFrac;
	if(newOvenTemp < MIN_TEMP || newOvenTemp > MAX_TEMP
			|| newBoardTemp < MIN_TEMP || newBoardTemp > MAX_TEMP)
		return -1;

	currentOvenTemp = newOvenTemp;
	currentBoardTemp = newBoardTemp;
	return 0;
}

void onUsbReceive() {
	if(usbBuffer == NULL) {
		return;
	}

//	char response[64] = RESPONSE_INVALID;
	if(!strcmp(usbBuffer, COMMAND_HEAT)) {
		setStatusLED(1, 0, 0);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
//		strcpy(response, RESPONSE_HEAT);
	}
	else if(!strcmp(usbBuffer, COMMAND_IDLE)) {
		setStatusLED(0, 0, 1);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
//		strcpy(response, RESPONSE_IDLE);
	}
	else if(!strcmp(usbBuffer, COMMAND_END)) {
		setStatusLED(0, 1, 0);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
		buzz(LONGBUZZ);
//		strcpy(response, RESPONSE_END);
	}
	else if(!strcmp(usbBuffer, COMMAND_CONNECTED)) {
		setStatusLED(0, 1, 0);
		buzz(SHORTBUZZ);
//		strcpy(response, RESPONSE_CONNECTED);
	}
	else if(!strcmp(usbBuffer, COMMAND_DISCONNECTED)) {
		setStatusLED(0, 0, 0);
		buzz(SHORTBUZZ);
//		strcpy(response, RESPONSE_DISCONNECTED);
	}

//	char confirmationMsg[64];
//	sprintf(confirmationMsg, "Received: '%s'", usbBuffer);
//	CDC_Transmit_FS((uint8_t *) confirmationMsg, strlen(confirmationMsg));
//	CDC_Transmit_FS((uint8_t *) response, strlen(response));
}

void setStatusLED(int r, int g, int b) {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, r ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, g ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void buzz(int millisOn) {
	buzzOn = 1;
	buzzLengthInMillis = millisOn;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
