/*UNCRUSTIFY-OFF*/
/**
 * @file device.c
 * @author Manny Peterson <manny@heliosproj.org>
 * @brief Kernel source for device I/O
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
#include "device.h"

static DeviceList_t *dlist = null;
static Return_t __DeviceListFind__(const HalfWord_t uid_, Device_t **device_);


#define __DeviceUidNonZero__() (nil < uid_)


Return_t xDeviceRegisterDevice(Return_t (*device_self_register_)()) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(device_self_register_)) {
    /* Call the device driver's DEVICENAME_self_register() function which will
     * in turn call __RegisterDevice__() in this file. */
    if(OK((*device_self_register_)())) {
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t __RegisterDevice__(const HalfWord_t uid_, const Byte_t *name_, const DeviceState_t state_, const DeviceMode_t mode_, Return_t (*init_)(Device_t *
  device_), Return_t (*config_)(Device_t *device_, Size_t *size_, Addr_t *config_), Return_t (*read_)(Device_t *device_, Size_t *size_, Addr_t **data_),
  Return_t (*write_)(Device_t *device_, Size_t *size_, Addr_t *data_), Return_t (*simple_read_)(Device_t *device_, Byte_t *data_), Return_t (*simple_write_)(
  Device_t *device_, Byte_t data_)) {
  FUNCTION_ENTER;


  Device_t *device = null;
  Device_t *cursor = null;


  /* NOTE: There is a __KernelAllocateMemory__() syscall buried in this if()
   * statement. */
  if((__DeviceUidNonZero__() && __PointerIsNotNull__(name_) && __PointerIsNotNull__(init_) && __PointerIsNotNull__(config_) && __PointerIsNotNull__(read_) &&
    __PointerIsNotNull__(write_) && __PointerIsNotNull__(simple_read_) && __PointerIsNotNull__(simple_write_) && __PointerIsNotNull__(dlist)) || (
    __DeviceUidNonZero__() && __PointerIsNotNull__(name_) && __PointerIsNotNull__(init_) && __PointerIsNotNull__(config_) && __PointerIsNotNull__(read_) &&
    __PointerIsNotNull__(write_) && __PointerIsNotNull__(simple_read_) && __PointerIsNotNull__(simple_write_) && __PointerIsNull__(dlist) && OK(
    __KernelAllocateMemory__((volatile Addr_t **) &dlist, sizeof(DeviceList_t))))) {
    if(__PointerIsNotNull__(dlist)) {
      /* We are expecting *NOT* to find the device unique identifier in the
       * device list. This is to confirm there isn't already a device with the
       * same unique identifier already registered. */
      if(!OK(__DeviceListFind__(uid_, &device))) {
        /* Likewise this should be null since we are expecting
         * __DeviceListFind__() will *NOT* find a device by that unique
         * identifier. */
        if(__PointerIsNull__(device)) {
          /* Allocate kernel memory for the device structure; then, if all goes
           * well, populate the structure with all of the device details. */
          if(OK(__KernelAllocateMemory__((volatile Addr_t **) &device, sizeof(Device_t)))) {
            if(__PointerIsNotNull__(device)) {
              if(OK(__memcpy__(device->name, name_, CONFIG_DEVICE_NAME_BYTES))) {
                device->uid = uid_;
                device->state = state_;
                device->mode = mode_;
                device->bytesWritten = nil;
                device->bytesRead = nil;
                device->available = nil;
                device->init = init_;
                device->config = config_;
                device->read = read_;
                device->write = write_;
                device->simple_read = simple_read_;
                device->simple_write = simple_write_;
                cursor = dlist->head;

                /* If this is the first device added to the device list, then go
                 * ahead and set the device list head to the device. Otherwise
                 * we need to traverse the list until we reach the end. */
                if(__PointerIsNotNull__(dlist->head)) {
                  while(__PointerIsNotNull__(cursor->next)) {
                    cursor = cursor->next;
                  }

                  cursor->next = device;
                } else {
                  dlist->head = device;
                }

                dlist->length++;
                __ReturnOk__();
              } else {
                __AssertOnElse__();


                /* Because the __memcpy__() failed, we need to free the kernel
                 * memory occupied by the device. */
                __KernelFreeMemory__(device);
              }
            } else {
              __AssertOnElse__();
            }
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceIsAvailable(const HalfWord_t uid_, Base_t *res_) {
  FUNCTION_ENTER;


  Device_t *device = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(res_) && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier in the device list.
     */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Set the result paramater of xDeviceIsAvailable() to the value of the
         * device structure available member.
         *
         * NOTE: There is *NO* particular meaning to the device's available
         * value, this is defined by the device driver's author. */
        *res_ = device->available;
        __ReturnOk__();
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceSimpleWrite(const HalfWord_t uid_, Byte_t data_) {
  FUNCTION_ENTER;


  Device_t *device = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier in the device list.
     */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Check to make sure the device is running *AND*
         * writable. */
        if(((DeviceModeReadWrite == device->mode) || (DeviceModeWriteOnly == device->mode)) && (DeviceStateRunning == device->state)) {
          if(OK((*device->simple_write)(device, data_))) {
            device->bytesWritten += sizeof(Byte_t);
            __ReturnOk__();
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceWrite(const HalfWord_t uid_, Size_t *size_, Addr_t *data_) {
  FUNCTION_ENTER;


  Device_t *device = null;
  Byte_t *data = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(size_) && (nil < *size_) && __PointerIsNotNull__(data_) && __PointerIsNotNull__(dlist)) {
    /* Confirm the data to be written to the device is waiting for us in heap
     * memory. */
    if(OK(__MemoryRegionCheckHeap__(data_, MEMORY_REGION_CHECK_OPTION_W_ADDR))) {
      /* Look-up the device by its unique identifier in the device list.
       */
      if(OK(__DeviceListFind__(uid_, &device))) {
        if(__PointerIsNotNull__(device)) {
          /* Check to make sure the device is running *AND*
           * writable. */
          if(((DeviceModeReadWrite == device->mode) || (DeviceModeWriteOnly == device->mode)) && (DeviceStateRunning == device->state)) {
            /* Allocate some kernel memory we will copy the data to be written
             * to the device from the heap into. */
            if(OK(__KernelAllocateMemory__((volatile Addr_t **) &data, *size_))) {
              if(__PointerIsNotNull__(data)) {
                /* Copy the data to be written to the device from the heap into
                 * the kernel memory then call the device driver's
                 * DEVICENAME_write() function. */
                if(OK(__memcpy__(data, data_, *size_))) {
                  if(OK((*device->write)(device, size_, data))) {
                    /* Free the kernel memory now that we are done. It is up to
                     * the end-user to free the heap memory the data occupies.
                     */
                    if(OK(__KernelFreeMemory__(data))) {
                      device->bytesWritten += *size_;
                      __ReturnOk__();
                    } else {
                      __AssertOnElse__();
                    }
                  } else {
                    __AssertOnElse__();


                    /* Because DEVICENAME_write() returned an error, we need to
                     * free the kernel memory. */
                    __KernelFreeMemory__(data);
                  }
                } else {
                  __AssertOnElse__();


                  /* Because __memcpy__() returned an error, we need to free the
                   * kernel memory. */
                  __KernelFreeMemory__(data);
                }
              } else {
                __AssertOnElse__();
              }
            } else {
              __AssertOnElse__();
            }
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


/**
 * @brief Internal kernel-level device write (no heap memory checks/copies)
 * @param  uid_  Device UID
 * @param  size_ Pointer to size of data in kernel memory
 * @param  data_ Pointer to data in kernel memory
 * @return       ReturnOK on success, ReturnError on failure
 */
Return_t __DeviceWrite__(const HalfWord_t uid_, Size_t *size_, Addr_t *data_) {
  FUNCTION_ENTER;


  Device_t *device = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(size_) && (nil < *size_) && __PointerIsNotNull__(data_) && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Check device is running and writable */
        if(((DeviceModeReadWrite == device->mode) || (DeviceModeWriteOnly == device->mode)) && (DeviceStateRunning == device->state)) {
          /* Call driver write directly with kernel memory (no copy needed) */
          if(OK((*device->write)(device, size_, data_))) {
            device->bytesWritten += *size_;
            __ReturnOk__();
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceSimpleRead(const HalfWord_t uid_, Byte_t *data_) {
  FUNCTION_ENTER;


  Device_t *device = null;
  Byte_t data = nil;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(data_) && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier in the device list.
     */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Check to make sure the device is running *AND*
         * readable. */
        if(((DeviceModeReadWrite == device->mode) || (DeviceModeReadOnly == device->mode)) && (DeviceStateRunning == device->state)) {
          /* Call the device driver's DEVICENAME_simple_read() function and
           * check that the data returned by the device driver is waiting for us
           * in kernel memory. */
          if(OK((*device->simple_read)(device, &data))) {
            *data_ = data;
            device->bytesWritten += sizeof(Byte_t);
            __ReturnOk__();
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceRead(const HalfWord_t uid_, Size_t *size_, Addr_t **data_) {
  FUNCTION_ENTER;


  Device_t *device = null;
  Addr_t *data = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(size_) && __PointerIsNotNull__(data_) && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier in the device list.
     */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Check to make sure the device is running *AND*
         * readable. */
        if(((DeviceModeReadWrite == device->mode) || (DeviceModeReadOnly == device->mode)) && (DeviceStateRunning == device->state)) {
          /* Call the device driver's DEVICENAME_read() function and check that
           * the data returned by the device driver is waiting for us in kernel
           * memory. */
          if(OK((*device->read)(device, size_, &data))) {
            if((nil < *size_) && __PointerIsNotNull__(data)) {
              if(OK(__MemoryRegionCheckKernel__(data, MEMORY_REGION_CHECK_OPTION_W_ADDR))) {
                /* Allocate "size_" of heap memory to copy the data read from
                 * the device in kernel memory into. */
                if(OK(__HeapAllocateMemory__((volatile Addr_t **) data_, *size_))) {
                  if(__PointerIsNotNull__(*data_)) {
                    /* Perform the copy from kernel memory to heap memory. */
                    if(OK(__memcpy__(*data_, data, *size_))) {
                      /* Free the kernel memory now that we are done. It is up
                       * to the end-user to free the heap memory the data
                       * occupies.
                       */
                      if(OK(__KernelFreeMemory__(data))) {
                        device->bytesRead += *size_;
                        __ReturnOk__();
                      }
                    } else {
                      __AssertOnElse__();


                      /* Because __memcpy__() returned an error, we need to free
                       * the kernel memory. */
                      __KernelFreeMemory__(data);


                      /* Because __memcpy__() returned an error, we also need to
                       * free the heap memory. */
                      __HeapFreeMemory__(*data_);
                    }
                  } else {
                    __AssertOnElse__();


                    /* Because __HeapAllocateMemory__() returned a null pointer,
                     * we need to free the kernel memory. */
                    __KernelFreeMemory__(data);
                  }
                } else {
                  __AssertOnElse__();


                  /* Because __HeapAllocateMemory__() returned an error, we need
                   * to free the kernel memory. */
                  __KernelFreeMemory__(data);
                }
              } else {
                __AssertOnElse__();
              }
            } else {
              __AssertOnElse__();
            }
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


/**
 * @brief Internal kernel-level device read (no heap memory allocation/copies)
 * @param  uid_  Device UID
 * @param  size_ Pointer to receive size of data read
 * @param  data_ Pointer to receive kernel memory buffer (caller must free)
 * @return       ReturnOK on success, ReturnError on failure
 */
Return_t __DeviceRead__(const HalfWord_t uid_, Size_t *size_, Addr_t **data_) {
  FUNCTION_ENTER;


  Device_t *device = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(size_) && __PointerIsNotNull__(data_) && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Check device is running and readable */
        if(((DeviceModeReadWrite == device->mode) || (DeviceModeReadOnly == device->mode)) && (DeviceStateRunning == device->state)) {
          /* Call driver read directly - returns kernel memory */
          if(OK((*device->read)(device, size_, data_))) {
            if((nil < *size_) && __PointerIsNotNull__(*data_)) {
              /* Verify data is in kernel memory */
              if(OK(__MemoryRegionCheckKernel__(*data_, MEMORY_REGION_CHECK_OPTION_W_ADDR))) {
                device->bytesRead += *size_;


                /* Note: Caller must free the kernel memory returned by driver
                 */
                __ReturnOk__();
              } else {
                __AssertOnElse__();
              }
            } else {
              __AssertOnElse__();
            }
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


static Return_t __DeviceListFind__(const HalfWord_t uid_, Device_t **device_) {
  FUNCTION_ENTER;


  Device_t *cursor = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(device_) && __PointerIsNotNull__(dlist)) {
    cursor = dlist->head;

    /* Traverse the device list while the cursor is not null and the unique
     * identifier passed to __DeviceListFind__() doesn't match the device
     * pointed to by the cursor. */
    while(__PointerIsNotNull__(cursor) && (cursor->uid != uid_)) {
      cursor = cursor->next;
    }

    if(__PointerIsNotNull__(cursor)) {
      *device_ = cursor;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceInitDevice(const HalfWord_t uid_) {
  FUNCTION_ENTER;


  Device_t *device = null;


  if(__DeviceUidNonZero__() && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier in the device list.
     */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Call the device drivers DEVICENAME_init() function to initialize the
         * device.
         *
         * NOTE: the behavior of the init function is defined by the device
         * driver's author. */
        if(OK((*device->init)(device))) {
          __ReturnOk__();
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDeviceConfigDevice(const HalfWord_t uid_, Size_t *size_, Addr_t *config_) {
  FUNCTION_ENTER;


  Device_t *device = null;
  Addr_t *config = null;


  if(__DeviceUidNonZero__() && (nil < *size_) && __PointerIsNotNull__(config_) && __PointerIsNotNull__(dlist)) {
    /* Confirm the data to be written to the device is waiting for us in heap
     * memory. */
    if(OK(__MemoryRegionCheckHeap__(config_, MEMORY_REGION_CHECK_OPTION_W_ADDR))) {
      /* Look-up the device by its unique identifier in the device list.
       */
      if(OK(__DeviceListFind__(uid_, &device))) {
        if(__PointerIsNotNull__(device)) {
          /* Allocate some kernel memory we will copy the configuration data to
           * be written to the device from the heap into. */
          if(OK(__KernelAllocateMemory__((volatile Addr_t **) &config, *size_))) {
            if(__PointerIsNotNull__(config)) {
              /* Copy the configuration data to be written to the device from
               * the heap into the kernel memory then call the device driver's
               * DEVICENAME_config() function.
               *
               * NOTE: DEVICENAME_config() is bi-direction, the configuration
               * data is read into and read out of the device so there are two
               * calls to __memcpy__(). */
              if(OK(__memcpy__(config, config_, *size_))) {
                if(OK((*device->config)(device, size_, config))) {
                  /* Copy the configuration data read back from the device from
                   * the kernel back into heap memory. */
                  if(OK(__memcpy__(config_, config, *size_))) {
                    /* Free the kernel memory now that we are done. It is up to
                     * the end-user to free the heap memory the data occupies.
                     */
                    if(OK(__KernelFreeMemory__(config))) {
                      __ReturnOk__();
                    } else {
                      __AssertOnElse__();
                    }
                  } else {
                    __AssertOnElse__();


                    /* Because __memcpy__() returned an error, we need to free
                     * the kernel memory. */
                    __KernelFreeMemory__(config);
                  }
                } else {
                  __AssertOnElse__();


                  /* Because DEVICENAME_config() returned an error, we need to
                   * free the kernel memory. */
                  __KernelFreeMemory__(config);
                }
              } else {
                __AssertOnElse__();


                /* Because __memcpy__() returned an error, we need to free the
                 * kernel memory. */
                __KernelFreeMemory__(config);
              }
            } else {
              __AssertOnElse__();
            }
          } else {
            __AssertOnElse__();
          }
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


/**
 * @brief Internal kernel-level device config (no heap memory checks/copies)
 * @param  uid_    Device UID
 * @param  size_   Pointer to size of config data in kernel memory
 * @param  config_ Pointer to config data in kernel memory (bidirectional)
 * @return         ReturnOK on success, ReturnError on failure
 */
Return_t __DeviceConfigDevice__(const HalfWord_t uid_, Size_t *size_, Addr_t *config_) {
  FUNCTION_ENTER;


  Device_t *device = null;


  if(__DeviceUidNonZero__() && (nil < *size_) && __PointerIsNotNull__(config_) && __PointerIsNotNull__(dlist)) {
    /* Look-up the device by its unique identifier */
    if(OK(__DeviceListFind__(uid_, &device))) {
      if(__PointerIsNotNull__(device)) {
        /* Call driver config directly with kernel memory (no copy needed) */
        /* Note: config is bidirectional - driver may modify it */
        if(OK((*device->config)(device, size_, config_))) {
          __ReturnOk__();
        } else {
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


#if defined(POSIX_ARCH_OTHER)


  /* For unit testing only! */
  void __DeviceStateClear__(void) {
    dlist = null;

    return;
  }


#endif /* if defined(POSIX_ARCH_OTHER) */