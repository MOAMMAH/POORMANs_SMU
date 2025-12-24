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

/**
  * @brief I2C MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hi2c->Instance==I2C1)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**I2C1 GPIO Configuration
        PB6     ------> I2C1_SCL
        PB7     ------> I2C1_SDA
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}

/**
  * @brief I2C MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hi2c: I2C handle pointer
  * @retval None
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    if(hi2c->Instance==I2C1)
    {
        __HAL_RCC_I2C1_CLK_DISABLE();

        /**I2C1 GPIO Configuration
        PB6     ------> I2C1_SCL
        PB7     ------> I2C1_SDA
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);
    }
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

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(huart->Instance==USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART2 GPIO Configuration
        PA2     ------> USART2_TX
        PA3     ------> USART2_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
    if(huart->Instance==USART2)
    {
        __HAL_RCC_USART2_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);
    }
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
 