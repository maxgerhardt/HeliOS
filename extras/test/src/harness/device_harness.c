/**
 * @file device_harness.c
 * @author Manny Peterson (mannymsp@gmail.com)
 * @brief
 * @version 0.3.5
 * @date 2022-09-02
 *
 * @copyright
 * HeliOS Embedded Operating System
 * Copyright (C) 2020-2022 Manny Peterson <mannymsp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "device_harness.h"


void device_harness(void) {

  unit_begin("xDeviceRegisterDevice()");


  unit_try(RETURN_SUCCESS == xDeviceRegisterDevice(loopback_self_register));

  unit_end();



  unit_begin("xDeviceWrite()");

  HWord_t bytes = 0x8u;

  Byte_t *data = NULL;

  data = (Byte_t *)xMemAlloc(bytes);

  memcpy(data, "1234567\0", bytes);

  unit_try(RETURN_SUCCESS == xDeviceWrite(0xFFu, &bytes, data));

  xMemFree(data);

  unit_end();



  unit_begin("xDeviceRead()");

  HWord_t bytes2 = 0x10u;

  Byte_t *data2 = NULL;

  data2 = (Byte_t *)xMemAlloc(bytes2);

  unit_try(RETURN_SUCCESS == xDeviceRead(0xFFu, &bytes2, data2));

  printf("[%s]\n", data2);

  xMemFree(data2);


  unit_end();


  return;
}

