/*UNCRUSTIFY-OFF*/
/**
 * @file types.h
 * @author Manny Peterson <manny@heliosproj.org>
 * @brief Kernel source for enumerated, structured and data type definitions
 * @version 0.5.0
 * @date 2023-03-19
 * 
 * @copyright
 * HeliOS Embedded Operating System Copyright (C) 2020-2023 HeliOS Project <license@heliosproj.org>
 *  
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  
 * 
 */
/*UNCRUSTIFY-ON*/
#ifndef TYPES_H_
  #define TYPES_H_


  #include "posix.h"

  #include <stdint.h>


/*
 *     WARNING: MODIFYING THIS FILE MAY HAVE DISASTROUS CONSEQUENCES. YOU'VE
 * BEEN WARNED.
 */
  typedef enum TaskState_e {
    TaskStateSuspended,
    TaskStateRunning,
    TaskStateWaiting
  } TaskState_t;
  typedef enum SchedulerState_e {
    SchedulerStateSuspended,
    SchedulerStateRunning
  } SchedulerState_t;
  typedef enum Return_e {
    ReturnOK,
    ReturnError
  } Return_t;
  typedef enum TimerState_e {
    TimerStateSuspended,
    TimerStateRunning
  } TimerState_t;
  typedef enum DeviceState_e {
    DeviceStateSuspended,
    DeviceStateRunning
  } DeviceState_t;
  typedef enum DeviceMode_e {
    DeviceModeReadOnly,
    DeviceModeWriteOnly,
    DeviceModeReadWrite
  } DeviceMode_t;
  typedef enum ByteOrder_e {
    ByteOrderLittleEndian,
    ByteOrderBigEndian
  } ByteOrder_t;
  typedef VOID_TYPE TaskParm_t;
  typedef UINT8_TYPE Base_t;
  typedef UINT8_TYPE Byte_t;
  typedef VOID_TYPE Addr_t;
  typedef SIZE_TYPE Size_t;
  typedef UINT16_TYPE HalfWord_t;
  typedef UINT32_TYPE Word_t;
  typedef UINT32_TYPE Ticks_t;
  typedef struct Device_s {
    HalfWord_t uid;
    Byte_t name[CONFIG_DEVICE_NAME_BYTES];
    DeviceState_t state;
    DeviceMode_t mode;
    Word_t bytesWritten;
    Word_t bytesRead;
    Base_t available;



    Return_t (*init)(struct Device_s *device_);
    Return_t (*config)(struct Device_s *device_, Size_t *size_, Addr_t *config_);
    Return_t (*read)(struct Device_s *device_, Size_t *size_, Addr_t **data_);
    Return_t (*write)(struct Device_s *device_, Size_t *size_, Addr_t *data_);
    Return_t (*simple_read)(struct Device_s *device_, Byte_t *data_);
    Return_t (*simple_write)(struct Device_s *device_, Byte_t data_);



    struct Device_s *next;
  } Device_t;
  typedef struct MemoryEntry_s {
    Word_t magic;
    Byte_t free;
    HalfWord_t blocks;
    struct MemoryEntry_s *next;
  } MemoryEntry_t;
  typedef struct MemoryRegion_s {
    volatile Byte_t mem[MEMORY_REGION_SIZE_IN_BYTES];
    MemoryEntry_t *start;
    HalfWord_t entrySize;
    HalfWord_t allocations;
    HalfWord_t frees;
    Word_t minAvailableEver;
  } MemoryRegion_t;
  typedef struct TaskNotification_s {
    Base_t notificationBytes;
    Byte_t notificationValue[CONFIG_NOTIFICATION_VALUE_BYTES];
  } TaskNotification_t;
  typedef struct Task_s {
    Base_t id;
    Byte_t name[CONFIG_TASK_NAME_BYTES];
    TaskState_t state;
    TaskParm_t *taskParameter;



    void (*callback)(struct Task_s *task_, TaskParm_t *parm_);



    Base_t notificationBytes;
    Byte_t notificationValue[CONFIG_NOTIFICATION_VALUE_BYTES];
    Ticks_t lastRunTime;
    Ticks_t totalRunTime;
    Ticks_t timerPeriod;
    Ticks_t timerStartTime;



  #if defined(CONFIG_TASK_WD_TIMER_ENABLE)
      Ticks_t wdTimerPeriod;



  #endif /* if defined(CONFIG_TASK_WD_TIMER_ENABLE) */
    struct Task_s *next;
  } Task_t;
  typedef struct TaskRunTimeStats_s {
    Base_t id;
    Ticks_t lastRunTime;
    Ticks_t totalRunTime;
  } TaskRunTimeStats_t;
  typedef struct MemoryRegionStats_s {
    Word_t largestFreeEntryInBytes;
    Word_t smallestFreeEntryInBytes;
    Word_t numberOfFreeBlocks;
    Word_t availableSpaceInBytes;
    Word_t successfulAllocations;
    Word_t successfulFrees;
    Word_t minimumEverFreeBytesRemaining;
  } MemoryRegionStats_t;
  typedef struct TaskInfo_s {
    Base_t id;
    Byte_t name[CONFIG_TASK_NAME_BYTES];
    TaskState_t state;
    Ticks_t lastRunTime;
    Ticks_t totalRunTime;
  } TaskInfo_t;
  typedef struct TaskList_s {
    Base_t nextId;
    Base_t length;
    Task_t *head;
  } TaskList_t;
  typedef struct DeviceList_s {
    Base_t length;
    Device_t *head;
  } DeviceList_t;
  typedef struct Timer_s {
    TimerState_t state;
    Ticks_t timerPeriod;
    Ticks_t timerStartTime;
  } Timer_t;
  typedef struct TimerList_s {
    Base_t length;
    Timer_t *head;
  } TimerList_t;
  typedef struct Flags_s {
    Base_t running;
    Base_t overflow;
    Base_t memfault;
    Base_t littleend;
  } Flags_t;
  typedef struct QueueMessage_s {
    Base_t messageBytes;
    Byte_t messageValue[CONFIG_MESSAGE_VALUE_BYTES];
  } QueueMessage_t;
  typedef struct Message_s {
    Base_t messageBytes;
    Byte_t messageValue[CONFIG_MESSAGE_VALUE_BYTES];
    struct Message_s *next;
  } Message_t;
  typedef struct Queue_s {
    Base_t length;
    Base_t limit;
    Base_t locked;
    Message_t *head;
    Message_t *tail;
  } Queue_t;
  typedef struct SystemInfo_s {
    Byte_t productName[OS_PRODUCT_NAME_SIZE];
    Base_t majorVersion;
    Base_t minorVersion;
    Base_t patchVersion;
    Base_t numberOfTasks;
    Base_t littleEndian;
  } SystemInfo_t;
  typedef struct StreamBuffer_s {
    Byte_t buffer[CONFIG_STREAM_BUFFER_BYTES];
    HalfWord_t length;
  } StreamBuffer_t;
  typedef struct Volume_s {
    HalfWord_t blockDeviceUID;
    Word_t fatStartSector;
    Word_t dataStartSector;
    Word_t rootDirCluster;
    Byte_t sectorsPerCluster;
    HalfWord_t bytesPerSector;
    HalfWord_t reservedSectors;
    Byte_t numFATs;
    Word_t sectorsPerFAT;
    Base_t mounted;
  } Volume_t;
  typedef struct File_s {
    struct Volume_s *volume;
    Word_t firstCluster;
    Word_t currentCluster;
    Word_t fileSize;
    Word_t position;
    Byte_t mode;
    Base_t isOpen;
    Base_t isDirty;
  } File_t;
  typedef struct DirEntry_s {
    Byte_t name[256];
    Word_t size;
    Word_t firstCluster;
    Base_t isDirectory;
    Base_t isReadOnly;
    Base_t isHidden;
    Base_t isSystem;
  } DirEntry_t;
  typedef struct Dir_s {
    struct Volume_s *volume;
    Word_t currentCluster;
    HalfWord_t entryIndex;
    Base_t isOpen;
  } Dir_t;
  typedef struct VolumeInfo_s {
    Word_t totalClusters;
    Word_t freeClusters;
    Word_t totalBytes;
    Word_t freeBytes;
    HalfWord_t bytesPerSector;
    Byte_t sectorsPerCluster;
    Word_t bytesPerCluster;
  } VolumeInfo_t;


#endif /* ifndef TYPES_H_ */