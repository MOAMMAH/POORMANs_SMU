/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : DAC control via UART - receives commands and sets DAC values
  ******************************************************************************/
#include "main.h"
#include "mcp4728.h"
#include "ADS1115.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

I2C_HandleTypeDef hi2c1;  // For MCP4728 DAC
I2C_HandleTypeDef hi2c2;  // For ADS1115 ADC
UART_HandleTypeDef huart2;

// ADS1115 handle
ADS1115_Handle_t* adc_handle = NULL;

uint8_t tx_buffer[128];
uint8_t rx_buffer[64];
uint8_t rx_index = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART2_UART_Init(void);
void ProcessUARTCommand(void);
float ADS1115_ReadVoltage(uint8_t channel);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();
    MX_USART2_UART_Init();

    // Initialize MCP4728 on I2C1
    HAL_StatusTypeDef init_status = MCP4728_Init(&hi2c1);
    if (init_status != HAL_OK)
    {
        // If initialization failed, enter error state
        while (1)
        {
            HAL_Delay(1000);
        }
    }
    
    // Initialize all DAC channels to 0
    uint16_t dac_values[4] = {0, 0, 0, 0};
    MCP4728_SetAllChannels(&hi2c1, dac_values);
    
    // Initialize ADS1115 on I2C2
    // Configure for single-shot mode, ±6.144V range (supports 0-5V), 128 SPS
    ADS1115_Config_t adc_config = {
        .channel = ADS1115_MUX_AIN0_GND,      // Start with channel 0 vs GND
        .pgaConfig = ADS1115_PGA_6V144,        // ±6.144V range (supports 5V reference)
        .operatingMode = MODE_SINGLE_SHOT,      // Single-shot mode
        .dataRate = ADS1115_DR_128SPS,         // 128 samples per second
        .compareMode = ADS1115_COMP_MODE_TRAD,
        .polarityMode = ADS1115_POL_ACTIVE_LOW,
        .latchingMode = ADS1115_LAT_NON_LATCHING,
        .queueComparator = ADS1115_QUE_DISABLE
    };
    
    adc_handle = ADS1115_init(&hi2c2, ADS1115_ADDR_GND, adc_config);
    if (adc_handle == NULL)
    {
        // If initialization failed, enter error state
        while (1)
        {
            HAL_Delay(1000);
        }
    }
    
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

static void MX_I2C2_Init(void)
{
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 100000;
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c2) != HAL_OK) Error_Handler();
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
    
    // Handle "test_adc" command - test I2C communication with ADC
    if (strcmp((char*)rx_buffer, "test_adc") == 0)
    {
        if (adc_handle == NULL)
        {
            int len = sprintf((char*)tx_buffer, "ERROR:ADC_NOT_INIT\r\n");
            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
            return;
        }
        
        // Try to read config register to test I2C communication
        uint8_t reg_addr = 0x01; // Config register
        uint8_t config_bytes[2] = {0};
        
        HAL_StatusTypeDef status1 = HAL_I2C_Master_Transmit(&hi2c2, (ADS1115_ADDR_GND << 1), &reg_addr, 1, 100);
        HAL_StatusTypeDef status2 = HAL_I2C_Master_Receive(&hi2c2, (ADS1115_ADDR_GND << 1), config_bytes, 2, 100);
        
        if (status1 == HAL_OK && status2 == HAL_OK)
        {
            int len = sprintf((char*)tx_buffer, "OK:0x%02X%02X\r\n", config_bytes[0], config_bytes[1]);
            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
        }
        else
        {
            int len = sprintf((char*)tx_buffer, "ERROR:I2C_FAIL:%d,%d\r\n", status1, status2);
            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
        }
        return;
    }
    
    // Handle "read_adc_raw,channel" command - read raw ADC value for debugging
    if (strncmp((char*)rx_buffer, "read_adc_raw,", 13) == 0)
    {
        uint8_t channel = (uint8_t)atoi((char*)rx_buffer + 13);
        
        if (adc_handle == NULL || channel > 3)
        {
            int len = sprintf((char*)tx_buffer, "ERROR\r\n");
            HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
            return;
        }
        
        // Map channel to MUX setting
        ADS1115_MUX_t mux_settings[4] = {
            ADS1115_MUX_AIN0_GND, ADS1115_MUX_AIN1_GND,
            ADS1115_MUX_AIN2_GND, ADS1115_MUX_AIN3_GND
        };
        
        // Update config for selected channel
        ADS1115_Config_t config = adc_handle->config;
        config.channel = mux_settings[channel];
        adc_handle->config = config;
        
        // Read raw ADC value
        int16_t raw_adc = ADS1115_oneShotMeasure(adc_handle);
        
        // Send response: raw ADC value
        int len = sprintf((char*)tx_buffer, "%d\r\n", raw_adc);
        HAL_UART_Transmit(&huart2, tx_buffer, len, 100);
        return;
    }
    
    // Handle "read_adc,channel" command - read ADC voltage
    if (strncmp((char*)rx_buffer, "read_adc,", 9) == 0)
    {
        uint8_t channel = (uint8_t)atoi((char*)rx_buffer + 9);
        
        // Validate channel (0-3)
        if (channel > 3)
        {
            // Invalid channel, ignore
            return;
        }
        
        // Read voltage from ADC (this also reads the raw ADC value internally)
        float voltage = ADS1115_ReadVoltage(channel);
        
        // Send response: voltage as float string
        int len = sprintf((char*)tx_buffer, "%.4f\r\n", voltage);
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

/**
  * @brief  Read voltage from ADS1115 ADC channel
  * @param  channel: ADC channel (0-3)
  * @retval Voltage in Volts
  */
float ADS1115_ReadVoltage(uint8_t channel)
{
    if (adc_handle == NULL || channel > 3)
    {
        return 0.0f;
    }
    
    // Map channel to MUX setting (AINx vs GND)
    ADS1115_MUX_t mux_settings[4] = {
        ADS1115_MUX_AIN0_GND,  // Channel 0 -> AIN0 vs GND
        ADS1115_MUX_AIN1_GND,  // Channel 1 -> AIN1 vs GND
        ADS1115_MUX_AIN2_GND,  // Channel 2 -> AIN2 vs GND
        ADS1115_MUX_AIN3_GND   // Channel 3 -> AIN3 vs GND
    };
    
    // Update config for selected channel
    ADS1115_Config_t config = adc_handle->config;
    config.channel = mux_settings[channel];
    // Update the handle's config so oneShotMeasure uses the correct channel
    adc_handle->config = config;
    
    // Perform one-shot measurement (this function writes config and waits for conversion)
    int16_t adc_value = ADS1115_oneShotMeasure(adc_handle);
    
    // Check if we got a valid reading (0 could mean error or actual 0V)
    // For debugging, we'll check if I2C communication is working
    // If adc_value is 0, it might be an error, but it could also be actual 0V
    
    // Convert to voltage
    // ADS1115 with ±6.144V range: 
    // - Full scale: ±32768 counts for ±6.144V
    // - LSB = 6.144V / 32768 = 187.5 µV per count
    // - For 0-5V input, we expect values from 0 to ~26666 (5V / 0.0001875V)
    // Note: adc_value is signed, but for single-ended measurements (AINx vs GND),
    // it should be positive for 0-5V inputs
    float voltage = ((float)adc_value * 6.144f) / 32768.0f;
    
    // Clamp to 0-5V range for single-ended measurements
    if (voltage < 0.0f)
        voltage = 0.0f;
    if (voltage > 5.0f)
        voltage = 5.0f;
    
    return voltage;
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
 