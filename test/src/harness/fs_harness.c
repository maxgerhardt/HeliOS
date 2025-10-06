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

/* Driver registration function declarations */
extern Return_t RAMDISK0_self_register(void);
extern Return_t BLOCKDEV_self_register(void);

/* Cleanup function declarations */
extern void __FSStateClear__(void);
extern void __BlockDeviceStateClear__(void);
extern void __RAMDiskStateClear__(void);


void fs_harness(void) {
  Volume_t *vol = null;

  /* Driver registration tests */
  unit_begin("Driver Registration and FS Mount Tests");

  /* RAM disk driver registration */
  unit_try(OK(xDeviceRegisterDevice(RAMDISK0_self_register)));
  unit_try(OK(xDeviceInitDevice(0x0100u)));

  /* Block device driver registration */
  unit_try(OK(xDeviceRegisterDevice(BLOCKDEV_self_register)));
  unit_try(OK(xDeviceInitDevice(0x1000u)));

  /* Mount filesystem */
  unit_try(OK(xFSMount(&vol, 0x1000u)));
  unit_try(__PointerIsNotNull__(vol));
  unit_try(vol->mounted == true);

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));

  unit_end();

  /* Cleanup */
  __FSStateClear__();
  __BlockDeviceStateClear__();
  __RAMDiskStateClear__();
}
