/*UNCRUSTIFY-OFF*/
/**
 * @file block_driver.h
 * @author Manny Peterson <manny@heliosproj.org>
 * @brief Generic block device driver for HeliOS
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
#ifndef BLOCK_DRIVER_H_
  #define BLOCK_DRIVER_H_

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
  #define DEVICE_NAME BLOCKDEV
  #define DEVICE_UID 0x1000u
  #define DEVICE_MODE DeviceModeReadWrite
  #define DEVICE_STATE DeviceStateRunning


  /* Storage device protocols - determines command sequences */
  #if defined(BLOCK_PROTOCOL_SD_CARD)
    #undef BLOCK_PROTOCOL_SD_CARD
  #endif /* if defined(BLOCK_PROTOCOL_SD_CARD) */
  #define BLOCK_PROTOCOL_SD_CARD 0x01u /* 1 */

  #if defined(BLOCK_PROTOCOL_MMC)
    #undef BLOCK_PROTOCOL_MMC
  #endif /* if defined(BLOCK_PROTOCOL_MMC) */
  #define BLOCK_PROTOCOL_MMC 0x02u /* 2 */

  #if defined(BLOCK_PROTOCOL_EMMC)
    #undef BLOCK_PROTOCOL_EMMC
  #endif /* if defined(BLOCK_PROTOCOL_EMMC) */
  #define BLOCK_PROTOCOL_EMMC 0x03u /* 3 */

  #if defined(BLOCK_PROTOCOL_RAW)
    #undef BLOCK_PROTOCOL_RAW
  #endif /* if defined(BLOCK_PROTOCOL_RAW) */
  #define BLOCK_PROTOCOL_RAW 0xFFu /* 255 - Direct I/O, no protocol */
  /* Block operation commands - internal use */
  #if defined(BLOCK_CMD_READ_SINGLE)
    #undef BLOCK_CMD_READ_SINGLE
  #endif /* if defined(BLOCK_CMD_READ_SINGLE) */
  #define BLOCK_CMD_READ_SINGLE 0x01u /* 1 */

  #if defined(BLOCK_CMD_READ_MULTIPLE)
    #undef BLOCK_CMD_READ_MULTIPLE
  #endif /* if defined(BLOCK_CMD_READ_MULTIPLE) */
  #define BLOCK_CMD_READ_MULTIPLE 0x02u /* 2 */

  #if defined(BLOCK_CMD_WRITE_SINGLE)
    #undef BLOCK_CMD_WRITE_SINGLE
  #endif /* if defined(BLOCK_CMD_WRITE_SINGLE) */
  #define BLOCK_CMD_WRITE_SINGLE 0x03u /* 3 */

  #if defined(BLOCK_CMD_WRITE_MULTIPLE)
    #undef BLOCK_CMD_WRITE_MULTIPLE
  #endif /* if defined(BLOCK_CMD_WRITE_MULTIPLE) */
  #define BLOCK_CMD_WRITE_MULTIPLE 0x04u /* 4 */


  /**
   * @brief Block device configuration structure
   *
   * Configures the block device with I/O driver details and storage protocol.
   * All hardware-specific details are handled by the I/O driver.
   */
  typedef struct BlockDeviceConfig_s {
    HalfWord_t ioDriverUID;          /* UID of I/O driver (SPI/I2C/etc.) */
    Byte_t protocol; /* BLOCK_PROTOCOL_* constant */
    HalfWord_t blockSize; /* Block/sector size (typically 512) */
    Word_t totalBlocks; /* Total capacity in blocks (0 = auto-detect) */
    Byte_t _padding[3]; /* Padding to distinguish from BlockDeviceCommand_t */
  } BlockDeviceConfig_t;


  /**
   * @brief Block device command structure
   *
   * Specifies block-level operations. Used to set addressing before read/write.
   */
  typedef struct BlockDeviceCommand_s {
    Byte_t command;                   /* BLOCK_CMD_* constant */
    Word_t blockNumber; /* Starting block/sector number */
    HalfWord_t blockCount; /* Number of blocks to read/write */
    Byte_t reserved; /* Padding */
  } BlockDeviceCommand_t;


  /**
   * @brief Block device information structure
   *
   * Returns device capabilities and statistics.
   */
  typedef struct BlockDeviceInfo_s {
    HalfWord_t blockSize;             /* Block/sector size in bytes */
    Word_t totalBlocks; /* Total capacity in blocks */
    Word_t totalBytes; /* Total capacity in bytes */
    Byte_t protocol; /* BLOCK_PROTOCOL_* constant */
    Base_t isWriteProtected; /* Write protection status */
    Base_t isInitialized; /* Initialization status */
  } BlockDeviceInfo_t;

  #ifdef __cplusplus
    extern "C" {
  #endif /* ifdef __cplusplus */
  /* Driver interface functions */
  Return_t BLOCKDEV_self_register(void);
  Return_t BLOCKDEV_init(Device_t *device_);
  Return_t BLOCKDEV_config(Device_t *device_, Size_t *size_, Addr_t *config_);
  Return_t BLOCKDEV_read(Device_t *device_, Size_t *size_, Addr_t **data_);
  Return_t BLOCKDEV_write(Device_t *device_, Size_t *size_, Addr_t *data_);
  Return_t BLOCKDEV_simple_read(Device_t *device_, Byte_t *data_);
  Return_t BLOCKDEV_simple_write(Device_t *device_, Byte_t data_);

  #if defined(POSIX_ARCH_OTHER)
    void __BlockDeviceStateClear__(void);
  #endif /* if defined(POSIX_ARCH_OTHER) */

  #ifdef __cplusplus
    }
  #endif /* ifdef __cplusplus */
#endif /* ifndef BLOCK_DRIVER_H_ */