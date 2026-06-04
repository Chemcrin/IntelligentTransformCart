/*
 * COPYRIGHT (C) STMicroelectronics 2014. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/**
 * @file   VL53L1_platform.h
 * @brief  Function prototype definitions for Ewok Platform layer.
 *
 */


#ifndef _VL53L1_I2C_PLATFORM_H_
#define _VL53L1_I2C_PLATFORM_H_

#include "vl53l1_def.h"

#ifdef __cplusplus
extern "C" {
#endif

// Include uint8_t, unit16_t  etc definitions

#include <stdint.h>
#include <stdarg.h>


/**
 *  @brief Typedef defining .\n
 * The developer shoud modify this to suit the platform being deployed.
 *
 */

// enum  {TRUE = true, FALSE = false};

/**
 * @brief Typedef defining 8 bit unsigned char type.\n
 * The developer shoud modify this to suit the platform being deployed.
 *
 */

#ifndef bool_t
typedef unsigned char bool_t;
#endif

/**
 * @brief  Reads a single byte from the device
 *
 * Uses SystemVerilog Read Byte task.
 *
 * @code
 *
 * Example:
 *
 * uint8_t device_status = 0;
 *
 * int status = VL53L1_read_data(STATUS, &device_status);
 *
 * @endcode
 *
 * @param  address - uint8_t device address value
 * @param  index  - uint8_t register index value
 * @param  pdata  - pointer to uint8_t data value
 *
 * @return status - SystemVerilog status 0 = ok, 1 = error
 *
 */
uint8_t VL53l1X_read_data(uint8_t dev_address, uint8_t address, uint8_t *data);

/**
 * @brief  Reads n bytes of data from the device
 *
 * Wrapper SystemVerilog Read Byte task.
 *
 * @code
 *
 * Example:
 *
 * uint8_t page_number = MAIN_SELECT_PAGE;
 *
 * int status = VL53L1_read_nbyte(PAGE_SELECT, page_number);
 *
 * @endcode
 *
 * @param  address - uint8_t device address value
 * @param  index - uint8_t register index value
 * @param  data  - uint8_t data value to write
 *
 * @return status - SystemVerilog status 0 = ok, 1 = error
 *
 */
void VL53l1X_read_nbyte(uint8_t dev_address, uint8_t address, uint8_t len, uint8_t *rbuf);

/**
 * @brief  Reads a single byte from the device
 *
 * Uses SystemVerilog Read Byte task.
 *
 * @code
 *
 * Example:
 *
 * uint8_t device_status = 0;
 *
 * int status = VL53L1_read_byte(STATUS, &device_status);
 *
 * @endcode
 *
 * @param  address - uint8_t device address value
 * @param  index  - uint8_t register index value
 * @param  pdata  - pointer to uint8_t data value
 *
 * @return status - SystemVerilog status 0 = ok, 1 = error
 *
 */
void VL53l1X_read_byte(uint8_t dev_address, uint16_t data, uint8_t len, uint8_t *rbuf);

/**
 * @brief  Writes a data to the device
 *
 * Wrapper for SystemVerilog Write Byte task
 *
 * @code
 *
 * Example:
 *
 * uint8_t page_number = MAIN_SELECT_PAGE;
 *
 * int status = VL53L1_write_data(PAGE_SELECT, page_number);
 *
 * @endcode
 *
 * @param  address - uint8_t device address value
 * @param  index - uint8_t register index value
 * @param  data  - uint8_t data value to write
 *
 * @return status - SystemVerilog status 0 = ok, 1 = error
 *
 */
void VL53l1X_write_data(uint8_t dev_address, uint8_t address, uint8_t data);

/**
 * @brief  Writes a single byte to the device
 *
 * Wrapper for SystemVerilog Write Byte task
 *
 * @code
 *
 * Example:
 *
 * uint8_t page_number = MAIN_SELECT_PAGE;
 *
 * int status = VL53L1_write_byte(PAGE_SELECT, page_number);
 *
 * @endcode
 *
 * @param  address - uint8_t device address value
 * @param  index - uint8_t register index value
 * @param  data  - uint8_t data value to write
 *
 * @return status - SystemVerilog status 0 = ok, 1 = error
 *
 */
void VL53l1X_write_byte(uint8_t dev_address, uint8_t address, uint8_t len, uint8_t *wbuf);

/**
 * @brief  Writes n bytes of data to the device
 *
 * Wrapper for SystemVerilog Write Byte task
 *
 * @code
 *
 * Example:
 *
 * uint8_t page_number = MAIN_SELECT_PAGE;
 *
 * int status = VL53L1_write_nbyte(PAGE_SELECT, page_number);
 *
 * @endcode
 *
 * @param  address - uint8_t device address value
 * @param  index - uint8_t register index value
 * @param  data  - uint8_t data value to write
 *
 * @return status - SystemVerilog status 0 = ok, 1 = error
 *
 */
void VL53l1X_write_nbyte(uint8_t dev_address, uint16_t address, uint8_t len, uint8_t *wbuf);

#ifdef __cplusplus
}
#endif

#endif //_VL53L1_I2C_PLATFORM_H_

