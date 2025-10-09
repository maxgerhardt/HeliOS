/*UNCRUSTIFY-OFF*/
/**
 * @file fs_harness.c
 * @author Manny Peterson <manny@heliosproj.org>
 * @brief Unit test harness for filesystem
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
#include "fs_harness.h"
#include "../../src/HeliOS.h"

/* Block device structures - forward declarations to avoid duplicate includes */
#define BLOCK_PROTOCOL_RAW 0xFFu

typedef struct BlockDeviceConfig_s {
  HalfWord_t ioDriverUID;
  Byte_t protocol;
  HalfWord_t blockSize;
  Word_t totalBlocks;
  Byte_t _padding[3];
} BlockDeviceConfig_t;

/* Driver registration function declarations */
extern Return_t RAMDISK0_self_register(void);
extern Return_t BLOCKDEV_self_register(void);

/* Cleanup function declarations */
extern void __FSStateClear__(void);
extern void __BlockDeviceStateClear__(void);
extern void __RAMDiskStateClear__(void);


void fs_harness(void) {
  BlockDeviceConfig_t *blockConfig = null;
  Size_t configSize = 0;

  printf("=== FS HARNESS STARTED ===\n");
  fflush(stdout);

  /* Driver registration tests */
  unit_begin("Driver Registration and FS Mount Tests");

  printf("[TEST] Registering RAM disk driver...\n");
  /* RAM disk driver registration */
  unit_try(OK(xDeviceRegisterDevice(RAMDISK0_self_register)));
  printf("[TEST] Initializing RAM disk...\n");
  unit_try(OK(xDeviceInitDevice(0x0100u)));

  printf("[TEST] Registering block device driver...\n");
  /* Block device driver registration */
  unit_try(OK(xDeviceRegisterDevice(BLOCKDEV_self_register)));
  printf("[TEST] Initializing block device...\n");
  unit_try(OK(xDeviceInitDevice(0x1000u)));

  printf("[TEST] Configuring block device...\n");
  /* Allocate config structure from heap (required by xDeviceConfigDevice) */
  unit_try(OK(xMemAlloc((volatile Addr_t *)&blockConfig, sizeof(BlockDeviceConfig_t))));

  /* Configure block device to use RAM disk */
  blockConfig->ioDriverUID = 0x0100u;          /* RAMDISK0 UID */
  blockConfig->protocol = BLOCK_PROTOCOL_RAW;  /* Use RAW protocol */
  blockConfig->blockSize = 512;                /* 512 bytes per sector */
  blockConfig->totalBlocks = 2048;             /* 1MB / 512 = 2048 blocks */
  configSize = sizeof(BlockDeviceConfig_t);
  printf("[TEST] Config: ioDriverUID=%u, protocol=%u, blockSize=%u, totalBlocks=%lu\n",
         blockConfig->ioDriverUID, blockConfig->protocol, blockConfig->blockSize, (unsigned long)blockConfig->totalBlocks);
  printf("[TEST] sizeof(BlockDeviceConfig_t)=%lu, configSize=%lu\n",
         (unsigned long)sizeof(BlockDeviceConfig_t), (unsigned long)configSize);
  unit_try(OK(xDeviceConfigDevice(0x1000u, &configSize, (Addr_t *)blockConfig)));
  printf("[TEST] Block device configured\n");
  xMemFree((Addr_t *)blockConfig);

  /* Format the RAM disk with FAT32 filesystem */
  printf("[TEST] Starting format test...\n");
  fflush(stdout);
  unit_try(OK(xFSFormat(0x1000u, (const Byte_t *)"HELIOS     ")));
  printf("[TEST] Format completed\n");
  fflush(stdout);

  /* Mount filesystem */
  /* unit_try(OK(xFSMount(&vol, 0x1000u))); */
  /* unit_try(__PointerIsNotNull__(vol)); */
  /* unit_try(vol->mounted == true); */

  /* Unmount filesystem */
  /* unit_try(OK(xFSUnmount(vol))); */

  unit_end();

  /* Cleanup */
  __FSStateClear__();
  __BlockDeviceStateClear__();
  __RAMDiskStateClear__();
}
