/**
 * @file device_harness.h
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
#ifndef DEVICE_HARNESS_H_
#define DEVICE_HARNESS_H_


#include "HeliOS.h"
#include "unit.h"


typedef enum {
  DeviceStateError,
  DeviceStateSuspended,
  DeviceStateRunning
} DeviceState_t;

typedef enum {
  DeviceModeReadOnly,
  DeviceModeWriteOnly,
  DeviceModeReadWrite
} DeviceMode_t;

typedef struct Device_s {
  HWord_t uid;
  char name[CONFIG_DEVICE_NAME_BYTES];
  DeviceState_t state;
  DeviceMode_t mode;
  Word_t bytesWritten;
  Word_t bytesRead;
  Byte_t (*init)(struct Device_s *device_);
  Byte_t (*config)(struct Device_s *device_, void *config_);
  Byte_t (*read)(struct Device_s *device_, HWord_t *bytes_, void *data_);
  Byte_t (*write)(struct Device_s *device_, HWord_t *bytes_, void *data_);
} Device_t;


#ifdef __cplusplus
extern "C" {
#endif




Base_t __RegisterDevice__(HWord_t uid_,
                          const char *name_,
                          DeviceState_t state_,
                          DeviceMode_t mode_,
                          Base_t (*init_)(Device_t *device_),
                          Base_t (*config_)(Device_t *device_, void *config_),
                          Base_t (*read_)(Device_t *device_, HWord_t *bytes_, void *data_),
                          Base_t (*write_)(Device_t *device_, HWord_t *bytes_, void *data_));


xBase device_self_register(void);
Base_t device_init(Device_t *device_);
Base_t device_config(Device_t *device_, void *config_);
Base_t device_read(Device_t *device_, HWord_t *bytes_, void *data_);
Base_t device_write(Device_t *device_, HWord_t *bytes_, void *data_);
void device_harness(void);

#ifdef __cplusplus
}  // extern "C" {
#endif

#endif