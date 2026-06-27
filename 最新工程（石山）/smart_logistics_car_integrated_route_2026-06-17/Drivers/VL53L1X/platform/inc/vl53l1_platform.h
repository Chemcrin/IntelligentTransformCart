/*
* This file is part of VL53L1 Platform
*
* Copyright (c) 2016, STMicroelectronics - All Rights Reserved
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#ifndef _VL53L1_PLATFORM_H_
#define _VL53L1_PLATFORM_H_

#include "vl53l1_ll_def.h"
#include "vl53l1_platform_log.h"

#define VL53L1_IPP_API
#include "vl53l1_platform_user_data.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Writes the supplied byte buffer to the device
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   pdata     Pointer to uint8_t buffer containing the data to be written
 * @param   count     Number of bytes in the supplied byte buffer
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WriteMulti(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata, uint32_t count);

/**
 * Reads the requested number of bytes from the device
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   pdata     Pointer to the uint8_t buffer to store read data
 * @param   count     Number of uint8_t's to read
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_ReadMulti(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata, uint32_t count);

/**
 * Write single byte register
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   data      8 bit register data
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WrByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t VL53L1_PRM_00005);

/**
 * Write word register
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   data      16 bit register data
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WrWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t VL53L1_PRM_00005);

/**
 * Write double word (4 byte) register
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   data      32 bit register data
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WrDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t VL53L1_PRM_00005);

/**
 * Read single byte register
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   data      pointer to 8 bit data
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_RdByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata);

/**
 * Read word (2byte) register
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   data      pointer to 16 bit data
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_RdWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t *pdata);

/**
 * Read dword (4byte) register
 * @param   pdev      Device Handle
 * @param   index     The register index
 * @param   data      pointer to 32 bit data
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_RdDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t *pdata);

/**
 * Delay function(us)
 * @param   pdev      Device Handle
 * @param   wait_us   a 32 bit data
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, int32_t wait_us);

/**
 * Delay function(ms)
 * @param   pdev      Device Handle
 * @param   wait_ms   a 32 bit data
 * @return  VL53L1_ERROR_NONE     Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, int32_t wait_ms);

/**
 * This parameter is used to obtain the frequency of a timer
 * @param   ptimer_freq_hz       Function argument, a pointer to type int32_t that stores the frequency value of the timer.
 * @return  VL53L1_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L1_Error
 */
VL53L1_Error VL53L1_GetTimerFrequency(int32_t *ptimer_freq_hz);

/**
 * To get the current system tick count
 * @param   ptime_ms             A function that takes a pointer to a uint32_t type variable as a parameter, where the function stores the current system tick count (in milliseconds) into the variable pointed to by the pointer.
 * @return  VL53L1_ERROR_NONE    Success
 * @return  "Other error code"   See ::VL53L1_Error
 */
VL53L1_Error VL53L1_GetTickCount(uint32_t *ptime_ms);

/**
 * Platform implementation of WaitValueMaskEx V2WReg script command
 * @param   pdev            Device Handle
 * @param   timeout_ms      Timeout
 * @param   index           The register index
 * @param   value           a 8 bit data
 * @param   mask            a 8 bit data
 * @param   poll_delay_ms   Polling time
 * @return  VL53L1_ERROR_NONE        Success
 * @return  "Other error code"    See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WaitValueMaskEx(VL53L1_Dev_t *pdev, uint32_t timeout_ms, uint16_t index, uint8_t value, uint8_t mask, uint32_t poll_delay_ms);

#ifdef __cplusplus
}
#endif

#endif
