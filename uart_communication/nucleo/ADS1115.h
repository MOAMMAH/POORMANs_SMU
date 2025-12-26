/**
  ******************************************************************************
  * @file    ADS1115.h
  * @brief   Driver header for ADS1115 16-bit ADC (I2C)
  * @author  Filip Rak (adapted)
  * @date    October 2025
  ******************************************************************************
  */

#ifndef INC_ADS1115_H_
#define INC_ADS1115_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdlib.h>

/* ADS1115 I2C Addresses */
#define ADS1115_ADDR_GND    0x48  // ADDR pin to GND
#define ADS1115_ADDR_VDD    0x49  // ADDR pin to VDD
#define ADS1115_ADDR_SDA    0x4A  // ADDR pin to SDA
#define ADS1115_ADDR_SCL    0x4B  // ADDR pin to SCL

/* Register addresses */
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01
#define ADS1115_REG_LO_THRESH   0x02
#define ADS1115_REG_HI_THRESH   0x03

/* Channel selection (MUX bits) - values 0-7 */
typedef enum {
    ADS1115_MUX_AIN0_AIN1 = 0,  // AIN0 vs AIN1 (default)
    ADS1115_MUX_AIN0_AIN3 = 1,  // AIN0 vs AIN3
    ADS1115_MUX_AIN1_AIN3 = 2,  // AIN1 vs AIN3
    ADS1115_MUX_AIN2_AIN3 = 3,  // AIN2 vs AIN3
    ADS1115_MUX_AIN0_GND  = 4,  // AIN0 vs GND
    ADS1115_MUX_AIN1_GND  = 5,  // AIN1 vs GND
    ADS1115_MUX_AIN2_GND  = 6,  // AIN2 vs GND
    ADS1115_MUX_AIN3_GND  = 7   // AIN3 vs GND
} ADS1115_MUX_t;

/* PGA (Programmable Gain Amplifier) settings - values 0-5 */
typedef enum {
    ADS1115_PGA_6V144 = 0,  // ±6.144V
    ADS1115_PGA_4V096 = 1,  // ±4.096V
    ADS1115_PGA_2V048 = 2,  // ±2.048V
    ADS1115_PGA_1V024 = 3,  // ±1.024V
    ADS1115_PGA_0V512 = 4,  // ±0.512V
    ADS1115_PGA_0V256 = 5   // ±0.256V
} ADS1115_PGA_t;

/* Operating mode - values 0-1 */
typedef enum {
    MODE_CONTINOUS = 0,
    MODE_SINGLE_SHOT = 1
} ADS1115_OperatingMode_t;

/* Data rate - values 0-7 */
typedef enum {
    ADS1115_DR_8SPS   = 0,
    ADS1115_DR_16SPS  = 1,
    ADS1115_DR_32SPS  = 2,
    ADS1115_DR_64SPS  = 3,
    ADS1115_DR_128SPS = 4,
    ADS1115_DR_250SPS = 5,
    ADS1115_DR_475SPS = 6,
    ADS1115_DR_860SPS = 7
} ADS1115_DataRate_t;

/* Comparator mode - values 0-1 */
typedef enum {
    ADS1115_COMP_MODE_TRAD = 0,  // Traditional
    ADS1115_COMP_MODE_WINDOW = 1  // Window
} ADS1115_CompareMode_t;

/* Polarity mode - values 0-1 */
typedef enum {
    ADS1115_POL_ACTIVE_LOW = 0,
    ADS1115_POL_ACTIVE_HIGH = 1
} ADS1115_PolarityMode_t;

/* Latching mode - values 0-1 */
typedef enum {
    ADS1115_LAT_NON_LATCHING = 0,
    ADS1115_LAT_LATCHING = 1
} ADS1115_LatchingMode_t;

/* Queue comparator - values 0-3 */
typedef enum {
    ADS1115_QUE_1_CONV = 0,
    ADS1115_QUE_2_CONV = 1,
    ADS1115_QUE_4_CONV = 2,
    ADS1115_QUE_DISABLE = 3
} ADS1115_QueueComparator_t;

/* Configuration structure */
typedef struct {
    ADS1115_MUX_t channel;           // Input channel selection
    ADS1115_PGA_t pgaConfig;         // PGA gain setting
    ADS1115_OperatingMode_t operatingMode;  // Single-shot or continuous
    ADS1115_DataRate_t dataRate;     // Data rate
    ADS1115_CompareMode_t compareMode;       // Comparator mode
    ADS1115_PolarityMode_t polarityMode;     // Polarity
    ADS1115_LatchingMode_t latchingMode;     // Latching
    ADS1115_QueueComparator_t queueComparator;  // Queue comparator
} ADS1115_Config_t;

/* Handle structure */
typedef struct ADS1115_Config_Tag {
    I2C_HandleTypeDef *hi2c;
    uint16_t address;
    ADS1115_Config_t config;
} ADS1115_Handle_t;

/* Function Prototypes */
ADS1115_Handle_t* ADS1115_init(I2C_HandleTypeDef *hi2c, uint16_t Addr, ADS1115_Config_t config);
void ADS1115_deinit(ADS1115_Handle_t* pConfig);
void ADS1115_updateConfig(ADS1115_Handle_t *pConfig, ADS1115_Config_t config);
void ADS1115_updateI2Chandler(ADS1115_Handle_t *pConfig, I2C_HandleTypeDef *hi2c);
void ADS1115_updateAddress(ADS1115_Handle_t *pConfig, uint16_t address);
int16_t ADS1115_oneShotMeasure(ADS1115_Handle_t *pConfig);
int16_t ADS1115_getData(ADS1115_Handle_t *pConfig);
void ADS1115_setThresholds(ADS1115_Handle_t *pConfig, int16_t lowValue, int16_t highValue);
void ADS1115_flushData(ADS1115_Handle_t* pConfig);
void ADS1115_setConversionReadyPin(ADS1115_Handle_t* pConfig);
void ADS1115_startContinousMode(ADS1115_Handle_t *pConfig);
void ADS1115_stopContinousMode(ADS1115_Handle_t *pConfig);

#ifdef __cplusplus
}
#endif

#endif /* INC_ADS1115_H_ */

