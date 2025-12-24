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
#define MCP4728_I2C_ADDR     (0x60 << 1)

/* MCP4728 Channel IDs */
typedef enum {
    MCP4728_CHANNEL_A = 0,
    MCP4728_CHANNEL_B = 1,
    MCP4728_CHANNEL_C = 2,
    MCP4728_CHANNEL_D = 3
} MCP4728_Channel;

/* Function Prototypes */
HAL_StatusTypeDef MCP4728_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef MCP4728_WriteChannel(I2C_HandleTypeDef *hi2c, MCP4728_Channel ch, uint16_t value);
HAL_StatusTypeDef MCP4728_SetAllChannels(I2C_HandleTypeDef *hi2c, uint16_t values[4]);

#ifdef __cplusplus
}
#endif

#endif /* INC_MCP4728_H_ */
