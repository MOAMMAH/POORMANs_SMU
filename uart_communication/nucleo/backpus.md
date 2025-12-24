******************************************************
AMIN
*******************************************************

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "mcp4728.h"
#include <stdio.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t tx_buffer[64];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* DAC configuration struct */
    dacChannelConfig dac_cfg;

    /* 12-bit DAC values for 5V VDD */
    dac_cfg.channel_Val[0] = (uint16_t)(4095 * 1.0f / 5.0f); // VA = 1V
    dac_cfg.channel_Val[1] = (uint16_t)(4095 * 2.0f / 5.0f); // VB = 2V
    dac_cfg.channel_Val[2] = (uint16_t)(4095 * 3.0f / 5.0f); // VC = 3V
    dac_cfg.channel_Val[3] = (uint16_t)(4095 * 5.0f / 5.0f); // VD = 4V

    dac_cfg.channelVref = MCP4728_VREF_VDD;
    dac_cfg.channel_Gain = MCP4728_GAIN_1;

    /* Initialize and write DAC */
    MCP4728_Init(&hi2c1, dac_cfg);
    MCP4728_Write_AllChannels_Diff(&hi2c1, dac_cfg);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  float vA = (dac_cfg.channel_Val[0] / 4095.0f) * 5.0f;

	            int len = sprintf((char*)tx_buffer,
	                              "DAC A = %.2f V\r\n", vA);

	            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
	            HAL_Delay(1000);

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
#ifdef USE_FULL_ASSERT
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









#############################################
main.h
#############################################



/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */





####################################
MCP.c
###################################


#include "mcp4728.h"

/**
 * @brief  Initialize the MCP4728 DAC
 */
void MCP4728_Init(I2C_HandleTypeDef *I2CHandler, dacChannelConfig output)
{
    // Wake up the device and reset the state machine
    MCP4728_Write_GeneralCall(I2CHandler, MCP4728_GENERAL_WAKEUP);
    HAL_Delay(5);

    // Set the initial values provided in the config struct
    MCP4728_Write_AllChannels_Diff(I2CHandler, output);
}

/**
 * @brief  Writes to all channels with different values (Sequential Write Mode)
 */
void MCP4728_Write_AllChannels_Diff(I2C_HandleTypeDef *I2CHandler, dacChannelConfig output)
{
    uint8_t data[8]; // 2 bytes per channel x 4 channels

    for (int i = 0; i < 4; i++) {
        // Sequential Write format:
        // Byte 0: 0 0 0 0 D11 D10 D9 D8
        // Byte 1: D7 D6 D5 D4 D3 D2 D1 D0
        data[i * 2] = (output.channel_Val[i] >> 8) & 0x0F;
        data[i * 2 + 1] = output.channel_Val[i] & 0xFF;
    }

    // Command 0x50 starts Sequential Write from Channel A
    HAL_I2C_Mem_Write(I2CHandler, MCP4728_BASEADDR, MCP4728_CMD_DACWRITE_SEQ, 1, data, 8, 100);
}

/**
 * @brief  Sends General Call commands (Reset, Wakeup, etc.)
 */
void MCP4728_Write_GeneralCall(I2C_HandleTypeDef *I2CHandler, uint8_t command)
{
    // General call address is 0x00
    HAL_I2C_Master_Transmit(I2CHandler, 0x00, &command, 1, 100);
}

// Add empty stubs for the other prototypes in your .h to satisfy the compiler
void MCP4728_Write_AllChannels_Same(I2C_HandleTypeDef *I2CHandler, uint16_t output) {}
void MCP4728_Write_SingleChannel(I2C_HandleTypeDef *I2CHandler, uint8_t channel, uint16_t output, dacChannelConfig channels) {}





####################################
MCP.h
###################################

/*
******************************************************************************
* @file           : mcp4728.h
* @brief          : Header for mcp4728.c file.
*                   This file contains the defines for the mcp4728 driver
******************************************************************************
*/
#ifndef __MCP4728_H
#define __MCP4728_H

#include "stm32f4xx_hal.h"

typedef struct
{
	/* 4 bits, 0 = VDD, 1 = interal 2.048V, 0000ABCD*/
	uint8_t		channelVref;
	/* 4 bits, 0 = unity gain, 1 = x2 gain, 0000ABCD*/
	uint8_t		channel_Gain;
	/* 4 12bit numbers specifying the desired initial output values */
	uint16_t	channel_Val[4];
}dacChannelConfig;

/*
*	Function Prototypes
*/
void MCP4728_Write_GeneralCall(I2C_HandleTypeDef *I2CHandler, uint8_t command);
void MCP4728_Write_AllChannels_Same(I2C_HandleTypeDef *I2CHandler, uint16_t output);
void MCP4728_Write_AllChannels_Diff(I2C_HandleTypeDef *I2CHandler, dacChannelConfig output);
void MCP4728_Write_SingleChannel(I2C_HandleTypeDef *I2CHandler, uint8_t channel, uint16_t output, dacChannelConfig channels);
void MCP4728_Init(I2C_HandleTypeDef *I2CHandler, dacChannelConfig output);

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

#define MCP4728_CMD_FASTWRITE		0x00
#define MCP4728_CMD_DACWRITE_MULTI	0x40
#define MCP4728_CMD_DACWRITE_SEQ	0x50
#define MCP4728_CMD_DACWRITE_SINGLE	0x58
#define MCP4728_CMD_ADDRWRITE		0x60
#define	MCP4728_CMD_VREFWRITE		0x80
#define MCP4728_CMD_GAINWRITE		0xC0
#define MCP4728_CMD_PWRDWNWRITE		0xA0

#define MCP4728_BASEADDR			0x60<<1

#define MCP4728_VREF_VDD				0x0
#define MCP4728_VREF_INTERNAL		0x1

#define MCP4728_GAIN_1				0x0
#define MCP4728_GAIN_2				0x1

#define MCP4728_CHANNEL_A			0x0
#define MCP4728_CHANNEL_B			0x1
#define MCP4728_CHANNEL_C			0x2
#define MCP4728_CHANNEL_D			0x3

#define MCP4728_PWRDWN_NORMAL		0x0
#define MCP4728_PWRDWN_1			0x1
#define MCP4728_PWRDWN_2			0x2
#define MCP4728_PWRDWN_3			0x3

#define MCP4728_UDAC_UPLOAD			0x1
#define MCP4728_UDAC_NOLOAD			0x0

#define MCP4728_GENERAL_RESET		0x06
#define MCP4728_GENERAL_WAKEUP		0x09
#define MCP4728_GENERAL_SWUPDATE	0x08
#define MCP4728_GENERAL_READADDR	0x0C

#endif
