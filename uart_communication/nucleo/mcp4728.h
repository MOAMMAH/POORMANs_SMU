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
