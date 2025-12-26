/**
  ******************************************************************************
  * @file    ADS1115.c
  * @brief   Driver implementation for ADS1115 16-bit ADC (I2C)
  * @author  Filip Rak (adapted)
  * @date    October 2025
  ******************************************************************************
  */

#include "ADS1115.h"

static void prepareConfigFrame(uint8_t *pOutFrame, ADS1115_Config_t config);

/**
  * @brief  Initialize ADS1115 driver
  * @param  hi2c: Pointer to HAL I2C handle
  * @param  Addr: I2C address (0x48-0x4B)
  * @param  config: Configuration structure
  * @retval Pointer to handle structure
  */
ADS1115_Handle_t* ADS1115_init(I2C_HandleTypeDef *hi2c, uint16_t Addr, ADS1115_Config_t config)
{
    ADS1115_Handle_t *pConfig = malloc(sizeof(ADS1115_Handle_t));
    pConfig->hi2c = hi2c;
    pConfig->address = Addr;
    pConfig->config = config;
    return pConfig;
}

/**
  * @brief  Deinitialize ADS1115 driver
  * @param  pConfig: Pointer to handle structure
  * @retval None
  */
void ADS1115_deinit(ADS1115_Handle_t* pConfig)
{
    free(pConfig);
}

/**
  * @brief  Update ADS1115 configuration
  * @param  pConfig: Pointer to handle structure
  * @param  config: New configuration structure
  * @retval None
  */
void ADS1115_updateConfig(ADS1115_Handle_t *pConfig, ADS1115_Config_t config)
{
    pConfig->config = config;

    uint8_t bytes[3] = {0};
    prepareConfigFrame(bytes, pConfig->config);

    HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), bytes, 3, 100);
}

/**
  * @brief  Update I2C handler
  * @param  pConfig: Pointer to handle structure
  * @param  hi2c: Pointer to new HAL I2C handle
  * @retval None
  */
void ADS1115_updateI2Chandler(ADS1115_Handle_t *pConfig, I2C_HandleTypeDef *hi2c)
{
    pConfig->hi2c = hi2c;
}

/**
  * @brief  Update I2C address
  * @param  pConfig: Pointer to handle structure
  * @param  address: New I2C address
  * @retval None
  */
void ADS1115_updateAddress(ADS1115_Handle_t *pConfig, uint16_t address)
{
    pConfig->address = address;
}

/**
  * @brief  Perform one-shot measurement
  * @param  pConfig: Pointer to handle structure
  * @retval 16-bit signed ADC value
  */
int16_t ADS1115_oneShotMeasure(ADS1115_Handle_t *pConfig)
{
    uint8_t bytes[3] = {0};

    prepareConfigFrame(bytes, pConfig->config);

    bytes[1] |= (1 << 7); // OS one shot measure - start conversion

    // Write config register to start conversion
    if (HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), bytes, 3, 100) != HAL_OK)
    {
        return 0; // I2C error
    }

    // Wait for conversion to complete
    // At 128 SPS, conversion takes ~8ms. Wait longer to be safe (15ms should be enough)
    HAL_Delay(15);
    
    // Read the conversion data
    // Note: We don't poll the OS bit because:
    // 1. When OS=1 is written, the device immediately clears it to 0 and starts converting
    // 2. After waiting 15ms (well beyond the ~8ms conversion time), conversion should be complete
    // 3. Reading the conversion register will return the latest conversion result
    return ADS1115_getData(pConfig);
}

/**
  * @brief  Read conversion data from ADS1115
  * @param  pConfig: Pointer to handle structure
  * @retval 16-bit signed ADC value (returns 0 on error)
  */
int16_t ADS1115_getData(ADS1115_Handle_t *pConfig)
{
    uint8_t reg_addr = 0x00; // Conversion register address
    uint8_t bytes[2] = {0};
    
    // Write register address
    if (HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), &reg_addr, 1, 50) != HAL_OK)
    {
        return 0; // I2C transmit error
    }

    // Read 2 bytes from conversion register
    if (HAL_I2C_Master_Receive(pConfig->hi2c, (pConfig->address << 1), bytes, 2, 50) != HAL_OK)
    {
        return 0; // I2C receive error
    }

    // Combine bytes (MSB first)
    int16_t readValue = ((int16_t)(bytes[0] << 8) | bytes[1]);
    
    // Note: We keep negative values as they represent negative voltages
    // The caller should handle the sign appropriately
    return readValue;
}

/**
  * @brief  Set comparator thresholds
  * @param  pConfig: Pointer to handle structure
  * @param  lowValue: Low threshold value
  * @param  highValue: High threshold value
  * @retval None
  */
void ADS1115_setThresholds(ADS1115_Handle_t *pConfig, int16_t lowValue, int16_t highValue)
{
    uint8_t ADSWrite[3] = { 0 };

    //hi threshold reg
    ADSWrite[0] = 0x03;
    ADSWrite[1] = (uint8_t)((highValue & 0xFF00) >> 8);
    ADSWrite[2] = (uint8_t)(highValue & 0x00FF);
    HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), ADSWrite, 3, 100);

    //lo threshold reg
    ADSWrite[0] = 0x02;
    ADSWrite[1] = (uint8_t)((lowValue & 0xFF00) >> 8);
    ADSWrite[2] = (uint8_t)(lowValue & 0x00FF);
    HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), ADSWrite, 3, 100);
}

/**
  * @brief  Flush data register
  * @param  pConfig: Pointer to handle structure
  * @retval None
  */
void ADS1115_flushData(ADS1115_Handle_t* pConfig)
{
    ADS1115_getData(pConfig);
}

/**
  * @brief  Set conversion ready pin mode
  * @param  pConfig: Pointer to handle structure
  * @retval None
  */
void ADS1115_setConversionReadyPin(ADS1115_Handle_t* pConfig)
{
    ADS1115_setThresholds(pConfig, 0x0000, 0xFFFF);
}

/**
  * @brief  Start continuous conversion mode
  * @param  pConfig: Pointer to handle structure
  * @retval None
  */
void ADS1115_startContinousMode(ADS1115_Handle_t *pConfig)
{
    uint8_t bytes[3] = {0};

    ADS1115_Config_t configReg = pConfig->config;
    configReg.operatingMode = MODE_CONTINOUS;
    prepareConfigFrame(bytes, configReg);

    HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), bytes, 3, 100);
}

/**
  * @brief  Stop continuous conversion mode
  * @param  pConfig: Pointer to handle structure
  * @retval None
  */
void ADS1115_stopContinousMode(ADS1115_Handle_t *pConfig)
{
    uint8_t bytes[3] = {0};
    ADS1115_Config_t configReg = pConfig->config;
    configReg.operatingMode = MODE_SINGLE_SHOT;
    prepareConfigFrame(bytes, configReg);

    HAL_I2C_Master_Transmit(pConfig->hi2c, (pConfig->address << 1), bytes, 3, 100);
}

/**
  * @brief  Prepare configuration frame for transmission
  * @param  pOutFrame: Output buffer (3 bytes)
  * @param  config: Configuration structure
  * @retval None
  */
static void prepareConfigFrame(uint8_t *pOutFrame, ADS1115_Config_t config)
{
    pOutFrame[0] = 0x01;  // Config register address
    
    // Byte 1: MUX (bits 14-12), PGA (bits 11-9), MODE (bit 8)
    // Original code: (config.channel << 4) | (config.pgaConfig << 1) | (config.operatingMode << 0)
    pOutFrame[1] = 0;
    pOutFrame[1] |= (config.channel << 4);        // MUX: bits 6-4
    pOutFrame[1] |= (config.pgaConfig << 1);      // PGA: bits 3-1
    pOutFrame[1] |= (config.operatingMode << 0);   // MODE: bit 0
    
    // Byte 2: DR (bits 7-5), COMP_MODE (bit 4), POL (bit 3), LAT (bit 2), QUE (bits 1-0)
    // Original code: (config.dataRate << 5) | (config.compareMode << 4) | (config.polarityMode << 3)
    //                | (config.latchingMode << 2) | (config.queueComparator << 0)
    pOutFrame[2] = 0;
    pOutFrame[2] |= (config.dataRate << 5);           // Data rate: bits 7-5
    pOutFrame[2] |= (config.compareMode << 4);        // Compare mode: bit 4
    pOutFrame[2] |= (config.polarityMode << 3);      // Polarity: bit 3
    pOutFrame[2] |= (config.latchingMode << 2);      // Latching: bit 2
    pOutFrame[2] |= (config.queueComparator << 0);   // Queue: bits 1-0
}

