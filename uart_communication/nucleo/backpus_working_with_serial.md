******************************************************
main 
*******************************************************
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DAC control via UART - receives commands and sets DAC values
  ******************************************************************************/
#include "main.h"
#include "mcp4728.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

uint8_t tx_buffer[128];
uint8_t rx_buffer[64];
uint8_t rx_index = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
void ProcessUARTCommand(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    // Initialize MCP4728
    HAL_StatusTypeDef init_status = MCP4728_Init(&hi2c1);
    if (init_status != HAL_OK)
    {
        // If initialization failed, enter error state
        while (1)
        {
            HAL_Delay(1000);
        }
    }

    // Initialize all channels to 0
    uint16_t dac_values[4] = {0, 0, 0, 0};
    MCP4728_SetAllChannels(&hi2c1, dac_values);

    // Clear receive buffer
    memset(rx_buffer, 0, sizeof(rx_buffer));
    rx_index = 0;

    // Main loop: receive and process UART commands
    while (1)
    {
        uint8_t byte;
        // Try to receive one byte (non-blocking)
        if (HAL_UART_Receive(&huart2, &byte, 1, 10) == HAL_OK)
        {
            // Check for newline or carriage return (end of command)
            if (byte == '\n' || byte == '\r')
            {
                if (rx_index > 0)
                {
                    rx_buffer[rx_index] = '\0';  // Null terminate
                    ProcessUARTCommand();
                    rx_index = 0;  // Reset for next command
                    memset(rx_buffer, 0, sizeof(rx_buffer));
                }
            }
            else if (rx_index < (sizeof(rx_buffer) - 1))
            {
                // Add byte to buffer
                rx_buffer[rx_index++] = byte;
            }
            else
            {
                // Buffer overflow, reset
                rx_index = 0;
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
        }
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
        Error_Handler();
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

void ProcessUARTCommand(void)
{
    // Handle COMM_OK command
    if (strcmp((char*)rx_buffer, "COMM_OK") == 0)
    {
        int len = sprintf((char*)tx_buffer, "COMM_OK\r\n");
        HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
        return;
    }

    // Handle "set_all,dac_value" command - set all channels to same value
    if (strncmp((char*)rx_buffer, "set_all,", 8) == 0)
    {
        uint16_t dac_value = (uint16_t)atoi((char*)rx_buffer + 8);

        // Validate DAC value (0-4095)
        if (dac_value > 4095)
        {
            // Invalid value, ignore
            return;
        }

        // Set all channels to the same value
        uint16_t dac_values[4] = {dac_value, dac_value, dac_value, dac_value};
        HAL_StatusTypeDef status = MCP4728_SetAllChannels(&hi2c1, dac_values);

        // Send response: 1 for success, 0 for failure
        if (status == HAL_OK)
        {
            int len = sprintf((char*)tx_buffer, "1\r\n");
            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
        }
        else
        {
            int len = sprintf((char*)tx_buffer, "0\r\n");
            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
        }
        return;
    }

    // Parse "channel,dac_value" format
    char* comma = strchr((char*)rx_buffer, ',');
    if (comma == NULL)
    {
        // Invalid format, ignore
        return;
    }

    // Split string at comma
    *comma = '\0';
    uint8_t channel = (uint8_t)atoi((char*)rx_buffer);
    uint16_t dac_value = (uint16_t)atoi(comma + 1);

    // Validate channel (0-3) and DAC value (0-4095)
    if (channel > 3 || dac_value > 4095)
    {
        // Invalid values, ignore
        return;
    }

    // Set DAC channel
    HAL_StatusTypeDef status = MCP4728_WriteChannel(&hi2c1, (MCP4728_Channel)channel, dac_value);

    // Send response: 1 for success, 0 for failure
    if (status == HAL_OK)
    {
        int len = sprintf((char*)tx_buffer, "1\r\n");
        HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
    }
    else
    {
        int len = sprintf((char*)tx_buffer, "0\r\n");
        HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */







##########
main.h
##########

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







####
mcp.c
#####

/**
  ******************************************************************************
  * @file    mcp4728.c
  * @brief   Driver implementation for MCP4728 4-Channel 12-bit DAC (I2C)
  * @author  Isai
  * @date    October 2025
  ******************************************************************************
  */

#include "mcp4728.h"

/**
  * @brief  Sends General Call commands (Reset, Wakeup, etc.)
  * @param  hi2c: Pointer to HAL I2C handle
  * @param  command: General call command (MCP4728_GENERAL_WAKEUP, etc.)
  * @retval HAL status
  */
HAL_StatusTypeDef MCP4728_Write_GeneralCall(I2C_HandleTypeDef *hi2c, uint8_t command)
{
    // General call address is 0x00
    return HAL_I2C_Master_Transmit(hi2c, 0x00, &command, 1, 100);
}

/**
  * @brief  Initialize the MCP4728 DAC (wake up device)
  * @param  hi2c: Pointer to HAL I2C handle
  * @retval HAL status
  */
HAL_StatusTypeDef MCP4728_Init(I2C_HandleTypeDef *hi2c)
{
    // Wake up the device and reset the state machine
    HAL_StatusTypeDef status = MCP4728_Write_GeneralCall(hi2c, MCP4728_GENERAL_WAKEUP);
    if (status != HAL_OK)
        return status;

    HAL_Delay(5);  // Small delay after wakeup

    return HAL_OK;
}

/**
  * @brief  Write a 12-bit value to a specific channel using Sequential Write mode
  * @param  hi2c: Pointer to HAL I2C handle
  * @param  ch: DAC channel (A–D)
  * @param  value: 12-bit DAC value (0–4095)
  * @retval HAL status
  */
HAL_StatusTypeDef MCP4728_WriteChannel(I2C_HandleTypeDef *hi2c, MCP4728_Channel ch, uint16_t value)
{
    // Read current values for other channels (or use 0)
    // For simplicity, we'll write all channels but only update the target one
    // This ensures VREF/GAIN settings are maintained

    uint8_t data[8];

    // Prepare data - set target channel, keep others at 0 (or read current values)
    for (int i = 0; i < 4; i++)
    {
        if (i == ch)
        {
            data[i * 2] = (value >> 8) & 0x0F;
            data[i * 2 + 1] = value & 0xFF;
        }
        else
        {
            // Keep other channels at 0 (or you could read current values)
            data[i * 2] = 0x00;
            data[i * 2 + 1] = 0x00;
        }
    }

    // Use Sequential Write (0x50) - this updates all channels and maintains settings
    return HAL_I2C_Mem_Write(hi2c, MCP4728_BASEADDR, MCP4728_CMD_DACWRITE_SEQ, 1, data, 8, HAL_MAX_DELAY);
}
/**
  * @brief  Set all four DAC channels using Sequential Write mode
  * @param  hi2c: Pointer to HAL I2C handle
  * @param  values: Array of four 12-bit values (A–D)
  * @retval HAL status
  */
HAL_StatusTypeDef MCP4728_SetAllChannels(I2C_HandleTypeDef *hi2c, uint16_t values[4])
{
    uint8_t data[8];  // 2 bytes per channel x 4 channels

    // Prepare data for all 4 channels
    for (int i = 0; i < 4; i++)
    {
        // Sequential Write format:
        // Byte 0: 0 0 0 0 D11 D10 D9 D8
        // Byte 1: D7 D6 D5 D4 D3 D2 D1 D0
        data[i * 2] = (values[i] >> 8) & 0x0F;
        data[i * 2 + 1] = values[i] & 0xFF;
    }

    // Command 0x50 starts Sequential Write from Channel A
    return HAL_I2C_Mem_Write(hi2c, MCP4728_BASEADDR, MCP4728_CMD_DACWRITE_SEQ, 1, data, 8, HAL_MAX_DELAY);
}











####
mcp.h
####


/**
  ******************************************************************************
  * @file    mcp4728.h
  * @brief   Driver header for MCP4728 4-Channel 12-bit DAC (I2C)
  * @author  Isai
  * @date    October 2025
  ******************************************************************************
  */

#ifndef INC_MCP4728_H_
#define INC_MCP4728_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

/* MCP4728 I2C Address (A0 = GND by default) */
#define MCP4728_BASEADDR        (0x60 << 1)
#define MCP4728_I2C_ADDR       MCP4728_BASEADDR

/* MCP4728 Channel IDs */
typedef enum {
    MCP4728_CHANNEL_A = 0,
    MCP4728_CHANNEL_B = 1,
    MCP4728_CHANNEL_C = 2,
    MCP4728_CHANNEL_D = 3
} MCP4728_Channel;

/* Command definitions */
#define MCP4728_CMD_FASTWRITE          0x00
#define MCP4728_CMD_DACWRITE_MULTI    0x40
#define MCP4728_CMD_DACWRITE_SEQ      0x50
#define MCP4728_CMD_DACWRITE_SINGLE   0x58

/* General Call commands */
#define MCP4728_GENERAL_RESET      0x06
#define MCP4728_GENERAL_WAKEUP     0x09
#define MCP4728_GENERAL_SWUPDATE   0x08
#define MCP4728_GENERAL_READADDR   0x0C

/* Function Prototypes */
HAL_StatusTypeDef MCP4728_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MCP4728_WriteChannel(I2C_HandleTypeDef *hi2c, MCP4728_Channel ch, uint16_t value);
HAL_StatusTypeDef MCP4728_SetAllChannels(I2C_HandleTypeDef *hi2c, uint16_t values[4]);
HAL_StatusTypeDef MCP4728_Write_GeneralCall(I2C_HandleTypeDef *hi2c, uint8_t command);

#ifdef __cplusplus
}
#endif

#endif /* INC_MCP4728_H_ */
