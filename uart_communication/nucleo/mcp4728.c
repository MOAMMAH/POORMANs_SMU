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
    * @brief  Initialize the MCP4728 DAC (verify communication)
    * @param  hi2c: Pointer to HAL I2C handle
    * @retval HAL status
    */
  HAL_StatusTypeDef MCP4728_Init(I2C_HandleTypeDef *hi2c)
  {
      uint8_t dummyData = 0x00;
      /* Simple check: transmit nothing to test ACK */
      return HAL_I2C_Master_Transmit(hi2c, MCP4728_I2C_ADDR, &dummyData, 0, 100);
  }
  
  /**
    * @brief  Write a 12-bit value to a specific channel using Fast Write mode
    * @param  hi2c: Pointer to HAL I2C handle
    * @param  ch: DAC channel (A–D)
    * @param  value: 12-bit DAC value (0–4095)
    * @retval HAL status
    */
  HAL_StatusTypeDef MCP4728_WriteChannel(I2C_HandleTypeDef *hi2c, MCP4728_Channel ch, uint16_t value)
  {
      uint8_t cmd = 0x40 | (ch << 1);     // Fast write command, select channel
      uint8_t data[3];
  
      data[0] = cmd;
      data[1] = (value >> 8) & 0x0F;      // D11–D8
      data[2] = value & 0xFF;             // D7–D0
  
      return HAL_I2C_Master_Transmit(hi2c, MCP4728_I2C_ADDR, data, 3, HAL_MAX_DELAY);
  }
  
  /**
    * @brief  Set all four DAC channels sequentially
    * @param  hi2c: Pointer to HAL I2C handle
    * @param  values: Array of four 12-bit values (A–D)
    * @retval HAL status
    */
  HAL_StatusTypeDef MCP4728_SetAllChannels(I2C_HandleTypeDef *hi2c, uint16_t values[4])
  {
      HAL_StatusTypeDef ret;
      for (uint8_t ch = 0; ch < 4; ch++)
      {
          ret = MCP4728_WriteChannel(hi2c, (MCP4728_Channel)ch, values[ch]);
          if (ret != HAL_OK)
              return ret;
          HAL_Delay(5);  // small delay between writes for stability
      }
      return HAL_OK;
  }
  