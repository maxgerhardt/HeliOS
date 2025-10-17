/*UNCRUSTIFY-OFF*/
/**
 * @file ramdisk_driver.h
 * @author Manny Peterson <manny@heliosproj.org>
 * @brief RAM disk driver for testing and simulation
 * @version 0.5.0
 * @date 2023-03-19
 *
 * @copyright
 * HeliOS Embedded Operating System Copyright (C) 2020-2023 HeliOS Project <license@heliosproj.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *
 */
/*UNCRUSTIFY-ON*/
#ifndef RAMDISK_DRIVER_H_
  #define RAMDISK_DRIVER_H_

  #include "config.h"
  #include "defines.h"
  #include "types.h"
  #include "port.h"
  #include "device.h"
  #include "mem.h"
  #include "queue.h"
  #include "streams.h"
  #include "sys.h"
  #include "task.h"
  #include "timer.h"


  /* Device name must be exactly CONFIG_DEVICE_NAME_BYTES (8 bytes) */
  #define DEVICE_NAME RAMDISK0
  #define DEVICE_UID 0x0100u
  #define DEVICE_MODE DeviceModeReadWrite
  #define DEVICE_STATE DeviceStateRunning


  /* RAM disk size - 1MB for testing */
  #if defined(RAMDISK_SIZE_BYTES)
    #undef RAMDISK_SIZE_BYTES
  #endif /* if defined(RAMDISK_SIZE_BYTES) */
  #define RAMDISK_SIZE_BYTES 0x100000u /* 1048576 (1MB) */
  /* Configuration commands - used with xDeviceConfigDevice() */
  #if defined(RAMDISK_CMD_SET_POSITION)
    #undef RAMDISK_CMD_SET_POSITION
  #endif /* if defined(RAMDISK_CMD_SET_POSITION) */
  #define RAMDISK_CMD_SET_POSITION 0x01u /* 1 */

  #if defined(RAMDISK_CMD_CLEAR_DISK)
    #undef RAMDISK_CMD_CLEAR_DISK
  #endif /* if defined(RAMDISK_CMD_CLEAR_DISK) */
  #define RAMDISK_CMD_CLEAR_DISK 0x02u /* 2 */

  #if defined(RAMDISK_CMD_GET_STATS)
    #undef RAMDISK_CMD_GET_STATS
  #endif /* if defined(RAMDISK_CMD_GET_STATS) */
  #define RAMDISK_CMD_GET_STATS 0x03u /* 3 */


  /**
   * @brief RAM disk position configuration
   *
   * Used with xDeviceConfigDevice() to set read/write position.
   */
  typedef struct RAMDiskPositionConfig_s {
    Byte_t command;            /* RAMDISK_CMD_SET_POSITION */
    Word_t position; /* Byte offset to set */
  } RAMDiskPositionConfig_t;


  /**
   * @brief RAM disk clear configuration
   *
   * Used with xDeviceConfigDevice() to clear disk contents.
   */
  typedef struct RAMDiskClearConfig_s {
    Byte_t command;            /* RAMDISK_CMD_CLEAR_DISK */
    Byte_t fillPattern; /* Pattern to fill with */
  } RAMDiskClearConfig_t;


  /**
   * @brief RAM disk statistics structure
   *
   * Used with xDeviceConfigDevice() to get statistics. Call with command =
   * RAMDISK_CMD_GET_STATS, then read back.
   */
  typedef struct RAMDiskStats_s {
    Byte_t command;            /* RAMDISK_CMD_GET_STATS */
    Word_t totalSize; /* Total size in bytes */
    Word_t currentPosition; /* Current read/write position */
    Word_t bytesRead; /* Total bytes read since init */
    Word_t bytesWritten; /* Total bytes written since init */
    Word_t readOperations; /* Number of read operations */
    Word_t writeOperations; /* Number of write operations */
  } RAMDiskStats_t;

  #ifdef __cplusplus
    extern "C" {
  #endif /* ifdef __cplusplus */
  /* Driver interface functions - DO NOT CALL DIRECTLY */
  /* Use xDevice* syscalls instead */
  Return_t RAMDISK0_self_register(void);
  Return_t RAMDISK0_init(Device_t *device_);
  Return_t RAMDISK0_config(Device_t *device_, Size_t *size_, Addr_t *config_);
  Return_t RAMDISK0_read(Device_t *device_, Size_t *size_, Addr_t **data_);
  Return_t RAMDISK0_write(Device_t *device_, Size_t *size_, Addr_t *data_);
  Return_t RAMDISK0_simple_read(Device_t *device_, Byte_t *data_);
  Return_t RAMDISK0_simple_write(Device_t *device_, Byte_t data_);

  #if defined(POSIX_ARCH_OTHER)
    void __RAMDiskStateClear__(void);
  #endif /* if defined(POSIX_ARCH_OTHER) */

  #ifdef __cplusplus
    }
  #endif /* ifdef __cplusplus */
#endif /* ifndef RAMDISK_DRIVER_H_ */