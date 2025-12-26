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
  