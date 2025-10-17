/*UNCRUSTIFY-OFF*/
/**
 * @file fs.c
 * @author Manny Peterson <manny@heliosproj.org>
 * @brief Kernel source for FAT32 filesystem support
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
#include "fs.h"


/* Forward declaration of block device command structure */
typedef struct BlockDeviceCommand_s {
  Byte_t command;
  Word_t blockNumber;
  HalfWord_t blockCount;
  Byte_t reserved;
} BlockDeviceCommand_t;


/* FAT32 Boot Sector Structure (aligned for direct memory mapping) */
typedef struct __attribute__ ((packed)) FAT32BootSector_s {
  Byte_t jumpBoot[3];               /* 0x00: Jump instruction */
  Byte_t oemName[8]; /* 0x03: OEM name */
  Byte_t bytesPerSector[2]; /* 0x0B: Bytes per sector (little-endian) */
  Byte_t sectorsPerCluster; /* 0x0D: Sectors per cluster */
  Byte_t reservedSectors[2]; /* 0x0E: Reserved sectors */
  Byte_t numFATs; /* 0x10: Number of FATs */
  Byte_t rootEntryCount[2]; /* 0x11: Root entries (0 for FAT32) */
  Byte_t totalSectors16[2]; /* 0x13: Total sectors (0 for FAT32) */
  Byte_t mediaType; /* 0x15: Media descriptor */
  Byte_t FATSize16[2]; /* 0x16: FAT size (0 for FAT32) */
  Byte_t sectorsPerTrack[2]; /* 0x18: Sectors per track */
  Byte_t numHeads[2]; /* 0x1A: Number of heads */
  Byte_t hiddenSectors[4]; /* 0x1C: Hidden sectors */
  Byte_t totalSectors32[4]; /* 0x20: Total sectors */
  Byte_t FATSize32[4]; /* 0x24: FAT size */
  Byte_t extFlags[2]; /* 0x28: Extended flags */
  Byte_t fsVersion[2]; /* 0x2A: Filesystem version */
  Byte_t rootCluster[4]; /* 0x2C: Root directory cluster */
  Byte_t fsInfo[2]; /* 0x30: FSInfo sector */
  Byte_t backupBootSector[2]; /* 0x32: Backup boot sector */
  Byte_t reserved[12]; /* 0x34: Reserved */
  Byte_t driveNumber; /* 0x40: Drive number */
  Byte_t reserved1; /* 0x41: Reserved */
  Byte_t bootSignature; /* 0x42: Boot signature (0x29) */
  Byte_t volumeID[4]; /* 0x43: Volume ID */
  Byte_t volumeLabel[11]; /* 0x47: Volume label */
  Byte_t fsType[8]; /* 0x52: Filesystem type */
} FAT32BootSector_t;


/* FAT32 Directory Entry Structure (32 bytes) */
typedef struct __attribute__ ((packed)) FAT32DirEntry_s {
  Byte_t name[11];                  /* 0x00: 8.3 filename */
  Byte_t attr; /* 0x0B: File attributes */
  Byte_t ntReserved; /* 0x0C: Reserved for Windows NT */
  Byte_t createTimeTenth; /* 0x0D: Creation time (tenths of second) */
  Byte_t createTime[2]; /* 0x0E: Creation time */
  Byte_t createDate[2]; /* 0x10: Creation date */
  Byte_t lastAccessDate[2]; /* 0x12: Last access date */
  Byte_t firstClusterHigh[2]; /* 0x14: High word of first cluster */
  Byte_t writeTime[2]; /* 0x16: Last write time */
  Byte_t writeDate[2]; /* 0x18: Last write date */
  Byte_t firstClusterLow[2]; /* 0x1A: Low word of first cluster */
  Byte_t fileSize[4]; /* 0x1C: File size */
} FAT32DirEntry_t;


/* FAT32 File Attributes */
#define FAT_ATTR_READ_ONLY 0x01u
#define FAT_ATTR_HIDDEN 0x02u
#define FAT_ATTR_SYSTEM 0x04u
#define FAT_ATTR_VOLUME_ID 0x08u
#define FAT_ATTR_DIRECTORY 0x10u
#define FAT_ATTR_ARCHIVE 0x20u
#define FAT_ATTR_LONG_NAME 0x0Fu


/* FAT32 Cluster Markers */
#define FAT32_EOC_MIN 0x0FFFFFF8u /* End of cluster chain (minimum) */
#define FAT32_EOC_MAX 0x0FFFFFFFu /* End of cluster chain (maximum) */
#define FAT32_BAD_CLUSTER 0x0FFFFFF7u /* Bad cluster marker */
#define FAT32_FREE_CLUSTER 0x00000000u /* Free cluster */


/* Helper function to read 16-bit little-endian value */
static HalfWord_t __ReadLE16__(const Byte_t *data_) {
  return((HalfWord_t) data_[0] | ((HalfWord_t) data_[1] << 8));
}


/* Helper function to read 32-bit little-endian value */
static Word_t __ReadLE32__(const Byte_t *data_) {
  return((Word_t) data_[0] | ((Word_t) data_[1] << 8) | ((Word_t) data_[2] << 16) | ((Word_t) data_[3] << 24));
}


/* Helper function to write 16-bit little-endian value */
static void __WriteLE16__(Byte_t *data_, HalfWord_t value_) {
  data_[0] = (Byte_t) (value_ & 0xFFu);
  data_[1] = (Byte_t) ((value_ >> 8) & 0xFFu);
}


/* Helper function to write 32-bit little-endian value */
static void __WriteLE32__(Byte_t *data_, Word_t value_) {
  data_[0] = (Byte_t) (value_ & 0xFFu);
  data_[1] = (Byte_t) ((value_ >> 8) & 0xFFu);
  data_[2] = (Byte_t) ((value_ >> 16) & 0xFFu);
  data_[3] = (Byte_t) ((value_ >> 24) & 0xFFu);
}


/* Forward declarations for helper functions */
static Return_t __ReadSector__(const Volume_t *vol_, Word_t sector_, Byte_t **data_);
static Return_t __WriteSector__(const Volume_t *vol_, Word_t sector_, const Byte_t *data_);
static Return_t __ReadCluster__(const Volume_t *vol_, Word_t cluster_, Byte_t **data_);
static Return_t __GetFATEntry__(const Volume_t *vol_, Word_t cluster_, Word_t *nextCluster_);
static Return_t __SetFATEntry__(const Volume_t *vol_, Word_t cluster_, Word_t value_);
static Word_t __ClusterToSector__(const Volume_t *vol_, Word_t cluster_);
static Return_t __FindFreeCluster__(const Volume_t *vol_, Word_t startHint_, Word_t *freeCluster_);


Return_t xFSMount(Volume_t **volume_, const HalfWord_t blockDeviceUID_) {
  FUNCTION_ENTER;


  Volume_t *vol = null;
  Byte_t *bootSectorData = null;
  FAT32BootSector_t *bs = null;


  if(__PointerIsNotNull__(volume_)) {
    /* Allocate volume structure in kernel heap memory */
    if(OK(__KernelAllocateMemory__((volatile Addr_t **) &vol, sizeof(Volume_t)))) {
      /* Store block device UID for all I/O operations */
      vol->blockDeviceUID = blockDeviceUID_;
      vol->mounted = false;


      /* Read boot sector (sector 0) */
      if(OK(__ReadSector__(vol, 0, &bootSectorData))) {
        bs = (FAT32BootSector_t *) bootSectorData;


        /* Parse boot sector parameters */
        vol->bytesPerSector = __ReadLE16__(bs->bytesPerSector);
        vol->sectorsPerCluster = bs->sectorsPerCluster;
        vol->reservedSectors = __ReadLE16__(bs->reservedSectors);
        vol->numFATs = bs->numFATs;
        vol->sectorsPerFAT = __ReadLE32__(bs->FATSize32);
        vol->rootDirCluster = __ReadLE32__(bs->rootCluster);


        /* Calculate FAT and data region start sectors */
        vol->fatStartSector = vol->reservedSectors;
        vol->dataStartSector = vol->reservedSectors + (vol->numFATs * vol->sectorsPerFAT);


        /* Validate FAT32 filesystem */
        if((vol->bytesPerSector >= 512) && (vol->sectorsPerCluster > 0) && (vol->rootDirCluster >= 2)) {
          vol->mounted = true;


          /* Free boot sector buffer */
          if(OK(xMemFree((Addr_t *) bootSectorData))) {
            *volume_ = vol;
            __ReturnOk__();
          } else {
            __AssertOnElse__();
          }
        } else {
          /* Invalid FAT32 parameters */
          xMemFree((Addr_t *) bootSectorData);
          __KernelFreeMemory__(vol);
          __AssertOnElse__();
        }
      } else {
        /* Failed to read boot sector */
        __KernelFreeMemory__(vol);
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


Return_t xFSUnmount(Volume_t *volume_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(volume_)) {
    volume_->mounted = false;


    /* Free volume structure from kernel heap */
    if(OK(__KernelFreeMemory__(volume_))) {
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFSGetVolumeInfo(const Volume_t *volume_, VolumeInfo_t **info_) {
  FUNCTION_ENTER;


  VolumeInfo_t *info = null;


  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(info_)) {
    /* Allocate info structure in kernel heap */
    if(OK(__KernelAllocateMemory__((volatile Addr_t **) &info, sizeof(VolumeInfo_t)))) {
      info->bytesPerSector = volume_->bytesPerSector;
      info->sectorsPerCluster = volume_->sectorsPerCluster;
      info->bytesPerCluster = (Word_t) volume_->bytesPerSector * volume_->sectorsPerCluster;
      info->totalClusters = 0;
      info->freeClusters = 0;
      info->totalBytes = 0;
      info->freeBytes = 0;
      *info_ = info;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFSFormat(const HalfWord_t blockDeviceUID_, const Byte_t *volumeLabel_) {
  FUNCTION_ENTER;


  Volume_t tempVol;
  Byte_t *bootSector = null;
  FAT32BootSector_t *bs = null;
  HalfWord_t bytesPerSector = 512;
  Byte_t sectorsPerCluster = 8;
  HalfWord_t reservedSectors = 32;
  Byte_t numFATs = 2;
  Word_t sectorsPerFAT = 256;  /* 256 sectors for FAT (128KB per FAT) */
  Word_t rootDirCluster = 2;
  Word_t fatStart = reservedSectors;
  Word_t dataStart = reservedSectors + (numFATs * sectorsPerFAT);


  /* Temporary volume structure for formatting */
  tempVol.blockDeviceUID = blockDeviceUID_;
  tempVol.bytesPerSector = bytesPerSector;
  tempVol.sectorsPerCluster = sectorsPerCluster;
  tempVol.reservedSectors = reservedSectors;
  tempVol.numFATs = numFATs;
  tempVol.sectorsPerFAT = sectorsPerFAT;
  tempVol.rootDirCluster = rootDirCluster;
  tempVol.fatStartSector = fatStart;
  tempVol.dataStartSector = dataStart;
  tempVol.mounted = false;


  /* Allocate and zero boot sector from user heap (required by xDeviceWrite) */
  if(OK(xMemAlloc((volatile Addr_t **) &bootSector, bytesPerSector))) {
    __memset__(bootSector, 0x00u, bytesPerSector);
    bs = (FAT32BootSector_t *) bootSector;


    /* Fill in boot sector */
    bs->jumpBoot[0] = 0xEBu; /* Jump instruction */
    bs->jumpBoot[1] = 0x58u;
    bs->jumpBoot[2] = 0x90u;
    __memcpy__(bs->oemName, "HELIOS  ", 8);
    __WriteLE16__(bs->bytesPerSector, bytesPerSector);
    bs->sectorsPerCluster = sectorsPerCluster;
    __WriteLE16__(bs->reservedSectors, reservedSectors);
    bs->numFATs = numFATs;
    __WriteLE16__(bs->rootEntryCount, 0);  /* 0 for FAT32 */
    __WriteLE16__(bs->totalSectors16, 0); /* 0 for FAT32 */
    bs->mediaType = 0xF8u; /* Fixed disk */
    __WriteLE16__(bs->FATSize16, 0); /* 0 for FAT32 */
    __WriteLE16__(bs->sectorsPerTrack, 63);
    __WriteLE16__(bs->numHeads, 16);
    __WriteLE32__(bs->hiddenSectors, 0);
    __WriteLE32__(bs->totalSectors32, 2048);  /* 1MB / 512 bytes */
    __WriteLE32__(bs->FATSize32, sectorsPerFAT);
    __WriteLE16__(bs->extFlags, 0);
    __WriteLE16__(bs->fsVersion, 0);
    __WriteLE32__(bs->rootCluster, rootDirCluster);
    __WriteLE16__(bs->fsInfo, 1);
    __WriteLE16__(bs->backupBootSector, 6);
    bs->driveNumber = 0x80u;
    bs->bootSignature = 0x29u;
    __WriteLE32__(bs->volumeID, 0x12345678u);
    __memcpy__(bs->volumeLabel, volumeLabel_, 11);
    __memcpy__(bs->fsType, "FAT32   ", 8);


    /* Boot sector signature */
    bootSector[510] = 0x55u;
    bootSector[511] = 0xAAu;


    /* Write boot sector */
    if(OK(__WriteSector__(&tempVol, 0, bootSector))) {
      Byte_t *fatSector = null;
      Word_t sector = 0;
      Word_t fat = 0;
      Base_t fatInitSuccess = true;

      /* Free boot sector as we're done with it */
      xMemFree((Addr_t *) bootSector);

      /* Initialize FAT tables - allocate a zero-filled sector buffer */
      if(OK(xMemAlloc((volatile Addr_t **) &fatSector, bytesPerSector))) {
        __memset__(fatSector, 0x00u, bytesPerSector);

        /* Write zeros to all sectors of all FAT copies */
        for(fat = 0; fat < numFATs && fatInitSuccess; fat++) {
          Word_t fatStartSector = fatStart + (fat * sectorsPerFAT);

          for(sector = 0; sector < sectorsPerFAT && fatInitSuccess; sector++) {
            if(ERROR(__WriteSector__(&tempVol, fatStartSector + sector, fatSector))) {
              fatInitSuccess = false;
            }
          }
        }

        if(fatInitSuccess) {
          /* Now set special FAT entries:
           * - Cluster 0: Media descriptor (0x0FFFFFF8)
           * - Cluster 1: Clean/dirty flag (0x0FFFFFFF)
           * - Cluster 2: Root directory (EOC marker 0x0FFFFFFF) */
          if(OK(__SetFATEntry__(&tempVol, 0, 0x0FFFFFF8u)) && OK(__SetFATEntry__(&tempVol, 1, 0x0FFFFFFFu)) && OK(__SetFATEntry__(&tempVol, 2, FAT32_EOC_MAX))) {
            /* Initialize root directory cluster to zeros */
            Word_t rootFirstSector = __ClusterToSector__(&tempVol, rootDirCluster);

            for(sector = 0; sector < sectorsPerCluster && fatInitSuccess; sector++) {
              if(ERROR(__WriteSector__(&tempVol, rootFirstSector + sector, fatSector))) {
                fatInitSuccess = false;
              }
            }

            xMemFree((Addr_t *) fatSector);

            if(fatInitSuccess) {
              __ReturnOk__();
            } else {
              __AssertOnElse__();
            }
          } else {
            xMemFree((Addr_t *) fatSector);
            __AssertOnElse__();
          }
        } else {
          xMemFree((Addr_t *) fatSector);
          __AssertOnElse__();
        }
      } else {
        __AssertOnElse__();
      }
    } else {
      xMemFree((Addr_t *) bootSector);
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileOpen(File_t **file_, Volume_t *volume_, const Byte_t *path_, const Byte_t mode_) {
  FUNCTION_ENTER;


  File_t *file = null;


  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_)) {
    /* Allocate file structure in kernel heap */
    if(OK(__KernelAllocateMemory__((volatile Addr_t **) &file, sizeof(File_t)))) {
      /* Store reference to parent volume */
      file->volume = volume_;
      file->mode = mode_;
      file->position = 0;
      file->isOpen = true;
      file->isDirty = false;
      file->firstCluster = 0;
      file->currentCluster = 0;
      file->fileSize = 0;
      *file_ = file;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileClose(File_t *file_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(file_)) {
    file_->isOpen = false;


    /* Free file structure from kernel heap */
    if(OK(__KernelFreeMemory__(file_))) {
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileRead(File_t *file_, const Size_t size_, Byte_t **data_) {
  FUNCTION_ENTER;


  Byte_t *buffer = null;
  Byte_t *clusterData = null;
  Size_t bytesToRead = size_;
  Size_t bytesRead = 0;
  Word_t clusterSize = 0;
  Word_t offsetInCluster = 0;
  Word_t bytesFromCluster = 0;
  Word_t nextCluster = 0;


  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(data_) && file_->isOpen && (nil < size_)) {
    clusterSize = (Word_t) file_->volume->bytesPerSector * file_->volume->sectorsPerCluster;


    /* Don't read past EOF */
    if((file_->position + bytesToRead) > file_->fileSize) {
      bytesToRead = file_->fileSize - file_->position;
    }

    if(nil == bytesToRead) {
      /* Already at EOF */
      __AssertOnElse__();
      FUNCTION_EXIT;
    }


    /* Allocate buffer for read data */
    if(OK(__KernelAllocateMemory__((volatile Addr_t **) &buffer, bytesToRead))) {
      /* If not at start of file, navigate to correct cluster */
      if(file_->currentCluster == 0) {
        file_->currentCluster = file_->firstCluster;
      }


      /* Read data cluster by cluster */
      while(bytesRead < bytesToRead) {
        /* Read current cluster */
        if(OK(__ReadCluster__(file_->volume, file_->currentCluster, &clusterData))) {
          /* Calculate offset within cluster */
          offsetInCluster = file_->position % clusterSize;


          /* Calculate how many bytes to copy from this cluster */
          bytesFromCluster = clusterSize - offsetInCluster;

          if(bytesFromCluster > (bytesToRead - bytesRead)) {
            bytesFromCluster = bytesToRead - bytesRead;
          }


          /* Copy data from cluster to buffer */
          __memcpy__(buffer + bytesRead, clusterData + offsetInCluster, bytesFromCluster);
          bytesRead += bytesFromCluster;
          file_->position += bytesFromCluster;
          __KernelFreeMemory__(clusterData);


          /* Move to next cluster if needed */
          if(bytesRead < bytesToRead) {
            if(OK(__GetFATEntry__(file_->volume, file_->currentCluster, &nextCluster))) {
              if(nextCluster >= FAT32_EOC_MIN) {
                /* Unexpected EOF */
                break;
              }

              file_->currentCluster = nextCluster;
            } else {
              __KernelFreeMemory__(buffer);
              __AssertOnElse__();
              FUNCTION_EXIT;
            }
          }
        } else {
          __KernelFreeMemory__(buffer);
          __AssertOnElse__();
          FUNCTION_EXIT;
        }
      }

      *data_ = buffer;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileWrite(File_t *file_, const Size_t size_, const Byte_t *data_) {
  FUNCTION_ENTER;


  Byte_t *clusterData = null;
  Size_t bytesToWrite = size_;
  Size_t bytesWritten = 0;
  Word_t clusterSize = 0;
  Word_t offsetInCluster = 0;
  Word_t bytesToCluster = 0;
  Word_t nextCluster = 0;
  Word_t i = 0;
  Word_t firstSector = 0;


  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(data_) && file_->isOpen && (nil < size_)) {
    clusterSize = (Word_t) file_->volume->bytesPerSector * file_->volume->sectorsPerCluster;


    /* Check write mode */
    if(((file_->mode & FS_MODE_WRITE) == 0) && ((file_->mode & FS_MODE_APPEND) == 0)) {
      __AssertOnElse__();
      FUNCTION_EXIT;
    }


    /* If append mode, seek to end */
    if((file_->mode & FS_MODE_APPEND) != 0) {
      file_->position = file_->fileSize;
    }


    /* If at start and no clusters allocated, allocate first cluster */
    if(file_->firstCluster == 0) {
      /* Find a free cluster starting from cluster 3 */
      Word_t freeCluster = 0;

      if(OK(__FindFreeCluster__(file_->volume, 3u, &freeCluster))) {
        file_->firstCluster = freeCluster;
        file_->currentCluster = freeCluster;
        __SetFATEntry__(file_->volume, freeCluster, FAT32_EOC_MAX);
        file_->isDirty = true;
      } else {
        /* No free clusters available */
        __AssertOnElse__();
        FUNCTION_EXIT;
      }
    }


    /* Navigate to correct cluster if needed */
    if(file_->currentCluster == 0) {
      file_->currentCluster = file_->firstCluster;


      /* TODO: Seek to correct cluster based on position */
    }


    /* Write data cluster by cluster */
    while(bytesWritten < bytesToWrite) {
      /* Read-modify-write current cluster */
      if(OK(__ReadCluster__(file_->volume, file_->currentCluster, &clusterData))) {
        offsetInCluster = file_->position % clusterSize;
        bytesToCluster = clusterSize - offsetInCluster;

        if(bytesToCluster > (bytesToWrite - bytesWritten)) {
          bytesToCluster = bytesToWrite - bytesWritten;
        }


        /* Modify cluster data */
        __memcpy__(clusterData + offsetInCluster, data_ + bytesWritten, bytesToCluster);


        /* Write cluster back */
        firstSector = __ClusterToSector__(file_->volume, file_->currentCluster);

        for(i = 0; i < file_->volume->sectorsPerCluster; i++) {
          __WriteSector__(file_->volume, firstSector + i, clusterData + (i * file_->volume->bytesPerSector));
        }

        bytesWritten += bytesToCluster;
        file_->position += bytesToCluster;

        if(file_->position > file_->fileSize) {
          file_->fileSize = file_->position;
          file_->isDirty = true;
        }

        __KernelFreeMemory__(clusterData);


        /* Allocate next cluster if needed */
        if(bytesWritten < bytesToWrite) {
          if(OK(__GetFATEntry__(file_->volume, file_->currentCluster, &nextCluster))) {
            if(nextCluster >= FAT32_EOC_MIN) {
              /* Need to allocate new cluster - find a free one */
              Word_t newCluster = 0;

              /* Start searching from current cluster + 1 for better locality */
              if(OK(__FindFreeCluster__(file_->volume, file_->currentCluster + 1u, &newCluster))) {
                /* Link current cluster to new cluster */
                __SetFATEntry__(file_->volume, file_->currentCluster, newCluster);
                /* Mark new cluster as end of chain */
                __SetFATEntry__(file_->volume, newCluster, FAT32_EOC_MAX);
                nextCluster = newCluster;
              } else {
                /* No free clusters available */
                __KernelFreeMemory__(clusterData);
                __AssertOnElse__();
                FUNCTION_EXIT;
              }
            }

            file_->currentCluster = nextCluster;
          } else {
            __KernelFreeMemory__(clusterData);
            __AssertOnElse__();
            FUNCTION_EXIT;
          }
        }
      } else {
        __AssertOnElse__();
        FUNCTION_EXIT;
      }
    }

    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileSeek(File_t *file_, const Word_t offset_, const Byte_t origin_) {
  FUNCTION_ENTER;


  Word_t newPosition = 0;
  Word_t clusterSize = 0;
  Word_t clustersToSkip = 0;
  Word_t i = 0;
  Word_t nextCluster = 0;


  if(__PointerIsNotNull__(file_) && file_->isOpen) {
    clusterSize = (Word_t) file_->volume->bytesPerSector * file_->volume->sectorsPerCluster;


    /* Calculate new position based on origin */
    switch(origin_) {
    case FS_SEEK_SET: newPosition = offset_;
      break;
    case FS_SEEK_CUR: newPosition = file_->position + offset_;
      break;
    case FS_SEEK_END: newPosition = file_->fileSize + offset_;
      break;
    default: __AssertOnElse__();
      FUNCTION_EXIT;
    }


    /* Don't seek past EOF for reads */
    if(newPosition > file_->fileSize) {
      newPosition = file_->fileSize;
    }

    file_->position = newPosition;


    /* Update current cluster */
    clustersToSkip = newPosition / clusterSize;
    file_->currentCluster = file_->firstCluster;

    for(i = 0; i < clustersToSkip; i++) {
      if(OK(__GetFATEntry__(file_->volume, file_->currentCluster, &nextCluster))) {
        if(nextCluster >= FAT32_EOC_MIN) {
          break;
        }

        file_->currentCluster = nextCluster;
      } else {
        __AssertOnElse__();
        FUNCTION_EXIT;
      }
    }

    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileTell(const File_t *file_, Word_t *position_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(position_)) {
    *position_ = file_->position;
    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileGetSize(const File_t *file_, Word_t *size_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(size_)) {
    *size_ = file_->fileSize;
    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileSync(File_t *file_) {
  FUNCTION_ENTER;


  /* Stub implementation */
  __ReturnOk__();
  FUNCTION_EXIT;
}


Return_t xFileTruncate(File_t *file_, const Word_t size_) {
  FUNCTION_ENTER;


  Word_t clusterSize = 0;
  Word_t clustersNeeded = 0;
  Word_t currentCluster = 0;
  Word_t nextCluster = 0;
  Word_t i = 0;


  if(__PointerIsNotNull__(file_) && file_->isOpen) {
    clusterSize = (Word_t) file_->volume->bytesPerSector * file_->volume->sectorsPerCluster;


    /* If truncating to larger size, file will be extended on write */
    if(size_ >= file_->fileSize) {
      file_->fileSize = size_;
      file_->isDirty = true;
      __ReturnOk__();
      FUNCTION_EXIT;
    }


    /* Truncating to smaller size - free excess clusters */
    clustersNeeded = (size_ + clusterSize - 1) / clusterSize;
    currentCluster = file_->firstCluster;


    /* Navigate to last needed cluster */
    for(i = 1; i < clustersNeeded && currentCluster != 0; i++) {
      if(OK(__GetFATEntry__(file_->volume, currentCluster, &nextCluster))) {
        if(nextCluster >= FAT32_EOC_MIN) {
          break;
        }

        currentCluster = nextCluster;
      } else {
        __AssertOnElse__();
        FUNCTION_EXIT;
      }
    }


    /* Mark this cluster as end of chain and free remaining */
    if(OK(__GetFATEntry__(file_->volume, currentCluster, &nextCluster))) {
      __SetFATEntry__(file_->volume, currentCluster, FAT32_EOC_MAX);


      /* Free remaining clusters in chain */
      while(nextCluster < FAT32_EOC_MIN) {
        Word_t clusterToFree = nextCluster;


        __GetFATEntry__(file_->volume, nextCluster, &nextCluster);
        __SetFATEntry__(file_->volume, clusterToFree, FAT32_FREE_CLUSTER);
      }
    }

    file_->fileSize = size_;
    file_->isDirty = true;
    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileEOF(const File_t *file_, Base_t *eof_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(eof_)) {
    *eof_ = (file_->position >= file_->fileSize) ? true : false;
    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDirOpen(Dir_t **dir_, Volume_t *volume_, const Byte_t *path_) {
  FUNCTION_ENTER;


  Dir_t *dir = null;


  if(__PointerIsNotNull__(dir_) && __PointerIsNotNull__(volume_)) {
    /* Allocate directory handle in kernel heap */
    if(OK(__KernelAllocateMemory__((volatile Addr_t **) &dir, sizeof(Dir_t)))) {
      dir->volume = volume_;
      dir->entryIndex = 0;
      dir->isOpen = true;
      dir->currentCluster = volume_->rootDirCluster;
      *dir_ = dir;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDirClose(Dir_t *dir_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(dir_)) {
    dir_->isOpen = false;


    /* Free directory handle from kernel heap */
    if(OK(__KernelFreeMemory__(dir_))) {
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDirRead(Dir_t *dir_, DirEntry_t **entry_) {
  FUNCTION_ENTER;


  Byte_t *clusterData = null;
  FAT32DirEntry_t *fatEntry = null;
  DirEntry_t *dirEntry = null;
  Word_t entriesPerCluster = 0;
  Word_t entryOffsetInCluster = 0;
  Word_t nextCluster = 0;


  if(__PointerIsNotNull__(dir_) && __PointerIsNotNull__(entry_) && dir_->isOpen) {
    entriesPerCluster = ((Word_t) dir_->volume->bytesPerSector * dir_->volume->sectorsPerCluster) / sizeof(FAT32DirEntry_t);


    /* Read current cluster */
    if(OK(__ReadCluster__(dir_->volume, dir_->currentCluster, &clusterData))) {
      /* Calculate entry offset within cluster */
      entryOffsetInCluster = dir_->entryIndex % entriesPerCluster;
      fatEntry = (FAT32DirEntry_t *) (clusterData + (entryOffsetInCluster * sizeof(FAT32DirEntry_t)));


      /* Skip deleted entries (first byte = 0xE5) and end marker (first byte =
       * 0x00) */
      while(fatEntry->name[0] == 0xE5u || fatEntry->name[0] == 0x00u) {
        if(fatEntry->name[0] == 0x00u) {
          /* End of directory */
          __KernelFreeMemory__(clusterData);
          __AssertOnElse__();
          FUNCTION_EXIT;
        }

        dir_->entryIndex++;
        entryOffsetInCluster = dir_->entryIndex % entriesPerCluster;


        /* Check if we need to read next cluster */
        if(entryOffsetInCluster == 0) {
          if(OK(__GetFATEntry__(dir_->volume, dir_->currentCluster, &nextCluster))) {
            if(nextCluster >= FAT32_EOC_MIN) {
              /* End of directory chain */
              __KernelFreeMemory__(clusterData);
              __AssertOnElse__();
              FUNCTION_EXIT;
            }

            __KernelFreeMemory__(clusterData);
            dir_->currentCluster = nextCluster;

            if(ERROR(__ReadCluster__(dir_->volume, dir_->currentCluster, &clusterData))) {
              __AssertOnElse__();
              FUNCTION_EXIT;
            }
          } else {
            __KernelFreeMemory__(clusterData);
            __AssertOnElse__();
            FUNCTION_EXIT;
          }
        }

        fatEntry = (FAT32DirEntry_t *) (clusterData + (entryOffsetInCluster * sizeof(FAT32DirEntry_t)));
      }


      /* Skip long filename entries */
      if((fatEntry->attr & FAT_ATTR_LONG_NAME) == FAT_ATTR_LONG_NAME) {
        /* Skip LFN entry - just increment and try again */
        dir_->entryIndex++;
        __KernelFreeMemory__(clusterData);
        __AssertOnElse__();
        FUNCTION_EXIT;
      }


      /* Allocate and fill directory entry */
      if(OK(__KernelAllocateMemory__((volatile Addr_t **) &dirEntry, sizeof(DirEntry_t)))) {
        /* Convert 8.3 filename to null-terminated string */
        Word_t i = 0;
        Word_t j = 0;


        /* Copy name part (8 chars) */
        for(i = 0; i < 8 && fatEntry->name[i] != ' '; i++) {
          dirEntry->name[j++] = fatEntry->name[i];
        }


        /* Add extension if present */
        if(fatEntry->name[8] != ' ') {
          dirEntry->name[j++] = '.';

          for(i = 8; i < 11 && fatEntry->name[i] != ' '; i++) {
            dirEntry->name[j++] = fatEntry->name[i];
          }
        }

        dirEntry->name[j] = '\0';


        /* Fill in file attributes */
        dirEntry->size = __ReadLE32__(fatEntry->fileSize);
        dirEntry->firstCluster = ((Word_t) __ReadLE16__(fatEntry->firstClusterHigh) << 16) | __ReadLE16__(fatEntry->firstClusterLow);
        dirEntry->isDirectory = (fatEntry->attr & FAT_ATTR_DIRECTORY) ? true : false;
        dirEntry->isReadOnly = (fatEntry->attr & FAT_ATTR_READ_ONLY) ? true : false;
        dirEntry->isHidden = (fatEntry->attr & FAT_ATTR_HIDDEN) ? true : false;
        dirEntry->isSystem = (fatEntry->attr & FAT_ATTR_SYSTEM) ? true : false;
        dir_->entryIndex++;
        __KernelFreeMemory__(clusterData);
        *entry_ = dirEntry;
        __ReturnOk__();
      } else {
        __KernelFreeMemory__(clusterData);
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


Return_t xDirRewind(Dir_t *dir_) {
  FUNCTION_ENTER;

  if(__PointerIsNotNull__(dir_)) {
    dir_->entryIndex = 0;
    __ReturnOk__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDirMake(Volume_t *volume_, const Byte_t *path_) {
  FUNCTION_ENTER;


  /* Simplified implementation - directory creation requires:
   * 1. Finding parent directory 2. Allocating cluster for new directory 3.
   * Creating . and .. entries 4. Adding entry to parent directory This is
   * complex and deferred for basic implementation */
  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_)) {
    /* TODO: Implement directory creation */
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xDirRemove(Volume_t *volume_, const Byte_t *path_) {
  FUNCTION_ENTER;


  /* Simplified implementation - directory removal requires:
   * 1. Verifying directory is empty 2. Finding directory entry in parent 3.
   * Marking entry as deleted 4. Freeing directory clusters This is complex and
   * deferred for basic implementation */
  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_)) {
    /* TODO: Implement directory removal */
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileExists(Volume_t *volume_, const Byte_t *path_, Base_t *exists_) {
  FUNCTION_ENTER;


  /* Simplified implementation - file lookup requires:
   * 1. Path parsing (splitting by /) 2. Traversing directory hierarchy 3.
   * Searching each directory for next component This is complex and deferred
   * for basic implementation */
  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_) && __PointerIsNotNull__(exists_)) {
    /* TODO: Implement file lookup */
    *exists_ = false;
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileUnlink(Volume_t *volume_, const Byte_t *path_) {
  FUNCTION_ENTER;


  /* Simplified implementation - file deletion requires:
   * 1. Finding file entry in directory 2. Marking entry as deleted (first byte
   * = 0xE5) 3. Freeing file's cluster chain This is deferred for basic
   * implementation */
  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_)) {
    /* TODO: Implement file deletion */
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileRename(Volume_t *volume_, const Byte_t *oldPath_, const Byte_t *newPath_) {
  FUNCTION_ENTER;


  /* Simplified implementation - file rename/move requires:
   * 1. Finding source file entry 2. Creating new entry in destination directory
   * 3. Marking old entry as deleted This is complex and deferred for basic
   * implementation */
  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(oldPath_) && __PointerIsNotNull__(newPath_)) {
    /* TODO: Implement file rename */
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


Return_t xFileGetInfo(Volume_t *volume_, const Byte_t *path_, DirEntry_t **entry_) {
  FUNCTION_ENTER;


  /* Simplified implementation - getting file info requires:
   * 1. Finding file entry in directory 2. Allocating and filling DirEntry_t
   * structure This is deferred for basic implementation */
  if(__PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_) && __PointerIsNotNull__(entry_)) {
    /* TODO: Implement file info lookup */
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


/* ============================================================================
* Internal Helper Functions
* ========================================================================== */


/**
 * @brief Read a single sector from the block device
 * @param  vol_    Pointer to mounted volume
 * @param  sector_ Sector number to read
 * @param  data_   Pointer to receive allocated buffer with sector data
 * @return         ReturnOK on success, ReturnError on failure
 */
static Return_t __ReadSector__(const Volume_t *vol_, Word_t sector_, Byte_t **data_) {
  FUNCTION_ENTER;


  Size_t blockSize = sizeof(BlockDeviceCommand_t);
  BlockDeviceCommand_t *cmd = null;
  Size_t readSize = 0;


  if(__PointerIsNotNull__(vol_) && __PointerIsNotNull__(data_)) {
    /* Allocate command structure from user heap (required by
     * xDeviceConfigDevice) */
    if(OK(xMemAlloc((volatile Addr_t **) &cmd, blockSize))) {
      /* Set up block device command to read single sector */
      cmd->command = 0x01u; /* BLOCK_CMD_READ_SINGLE */
      cmd->blockNumber = sector_;
      cmd->blockCount = 1;
      cmd->reserved = 0;


      /* Configure block device to address this sector */
      if(OK(xDeviceConfigDevice(vol_->blockDeviceUID, &blockSize, (Addr_t *) cmd))) {
        /* Read the sector data */
        readSize = (Size_t) vol_->bytesPerSector;

        if(OK(xDeviceRead(vol_->blockDeviceUID, &readSize, (Addr_t **) data_))) {
          xMemFree((Addr_t *) cmd);
          __ReturnOk__();
        } else {
          xMemFree((Addr_t *) cmd);
          __AssertOnElse__();
        }
      } else {
        xMemFree((Addr_t *) cmd);
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
 * @brief Write a single sector to the block device
 * @param  vol_    Pointer to mounted volume
 * @param  sector_ Sector number to write
 * @param  data_   Buffer containing sector data to write
 * @return         ReturnOK on success, ReturnError on failure
 */
static Return_t __WriteSector__(const Volume_t *vol_, Word_t sector_, const Byte_t *data_) {
  FUNCTION_ENTER;


  Size_t blockSize = sizeof(BlockDeviceCommand_t);
  BlockDeviceCommand_t *cmd = null;
  Size_t writeSize = 0;


  if(__PointerIsNotNull__(vol_) && __PointerIsNotNull__(data_)) {
    /* Allocate command structure from user heap (required by
     * xDeviceConfigDevice) */
    if(OK(xMemAlloc((volatile Addr_t **) &cmd, blockSize))) {
      /* Set up block device command to write single sector */
      cmd->command = 0x03u; /* BLOCK_CMD_WRITE_SINGLE */
      cmd->blockNumber = sector_;
      cmd->blockCount = 1;
      cmd->reserved = 0;


      /* Configure block device to address this sector */
      if(OK(xDeviceConfigDevice(vol_->blockDeviceUID, &blockSize, (Addr_t *) cmd))) {
        /* Write the sector data */
        writeSize = (Size_t) vol_->bytesPerSector;

        if(OK(xDeviceWrite(vol_->blockDeviceUID, &writeSize, (Addr_t *) data_))) {
          xMemFree((Addr_t *) cmd);
          __ReturnOk__();
        } else {
          xMemFree((Addr_t *) cmd);
          __AssertOnElse__();
        }
      } else {
        xMemFree((Addr_t *) cmd);
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
 * @brief Convert cluster number to first sector number
 * @param  vol_     Pointer to mounted volume
 * @param  cluster_ Cluster number
 * @return          First sector number of the cluster
 */
static Word_t __ClusterToSector__(const Volume_t *vol_, Word_t cluster_) {
  /* First cluster is cluster 2 in FAT32 */
  return(vol_->dataStartSector + ((cluster_ - 2u) * vol_->sectorsPerCluster));
}


/**
 * @brief Read entire cluster from volume
 * @param  vol_     Pointer to mounted volume
 * @param  cluster_ Cluster number to read
 * @param  data_    Pointer to receive allocated buffer with cluster data
 * @return          ReturnOK on success, ReturnError on failure
 */
static Return_t __ReadCluster__(const Volume_t *vol_, Word_t cluster_, Byte_t **data_) {
  FUNCTION_ENTER;


  Word_t firstSector = 0;
  Word_t clusterSize = 0;
  Byte_t *buffer = null;
  Byte_t *sectorData = null;
  Word_t i = 0;


  if(__PointerIsNotNull__(vol_) && __PointerIsNotNull__(data_)) {
    /* Calculate cluster size and first sector */
    clusterSize = (Word_t) vol_->bytesPerSector * vol_->sectorsPerCluster;
    firstSector = __ClusterToSector__(vol_, cluster_);


    /* Allocate buffer for entire cluster */
    if(OK(__KernelAllocateMemory__((volatile Addr_t **) &buffer, clusterSize))) {
      /* Read all sectors in cluster */
      for(i = 0; i < vol_->sectorsPerCluster; i++) {
        if(OK(__ReadSector__(vol_, firstSector + i, &sectorData))) {
          /* Copy sector data to cluster buffer */
          __memcpy__(buffer + (i * vol_->bytesPerSector), sectorData, vol_->bytesPerSector);


          /* Free sector buffer */
          xMemFree((Addr_t *) sectorData);
        } else {
          /* Failed to read sector - clean up and fail */
          __KernelFreeMemory__(buffer);
          __AssertOnElse__();
          FUNCTION_EXIT;
        }
      }

      *data_ = buffer;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


/**
 * @brief Get FAT entry for a cluster (find next cluster in chain)
 * @param  vol_         Pointer to mounted volume
 * @param  cluster_     Cluster number to look up
 * @param  nextCluster_ Pointer to receive next cluster number
 * @return              ReturnOK on success, ReturnError on failure
 */
static Return_t __GetFATEntry__(const Volume_t *vol_, Word_t cluster_, Word_t *nextCluster_) {
  FUNCTION_ENTER;


  Word_t fatOffset = 0;
  Word_t fatSector = 0;
  Word_t entryOffset = 0;
  Byte_t *sectorData = null;
  Word_t fatEntry = 0;


  if(__PointerIsNotNull__(vol_) && __PointerIsNotNull__(nextCluster_)) {
    /* Calculate FAT offset (each entry is 4 bytes in FAT32) */
    fatOffset = cluster_ * 4u;
    fatSector = vol_->fatStartSector + (fatOffset / vol_->bytesPerSector);
    entryOffset = fatOffset % vol_->bytesPerSector;


    /* Read FAT sector */
    if(OK(__ReadSector__(vol_, fatSector, &sectorData))) {
      /* Read 32-bit FAT entry (mask upper 4 bits per FAT32 spec) */
      fatEntry = __ReadLE32__(sectorData + entryOffset) & 0x0FFFFFFFu;


      /* Free sector buffer */
      xMemFree((Addr_t *) sectorData);
      *nextCluster_ = fatEntry;
      __ReturnOk__();
    } else {
      __AssertOnElse__();
    }
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


/**
 * @brief Set FAT entry for a cluster (update cluster chain)
 * @param  vol_     Pointer to mounted volume
 * @param  cluster_ Cluster number to update
 * @param  value_   Value to write (next cluster or marker)
 * @return          ReturnOK on success, ReturnError on failure
 */
static Return_t __SetFATEntry__(const Volume_t *vol_, Word_t cluster_, Word_t value_) {
  FUNCTION_ENTER;


  Word_t fatOffset = 0;
  Word_t fatSector = 0;
  Word_t entryOffset = 0;
  Byte_t *sectorData = null;
  Byte_t i = 0;


  if(__PointerIsNotNull__(vol_)) {
    /* Calculate FAT offset (each entry is 4 bytes in FAT32) */
    fatOffset = cluster_ * 4u;
    fatSector = vol_->fatStartSector + (fatOffset / vol_->bytesPerSector);
    entryOffset = fatOffset % vol_->bytesPerSector;


    /* Read FAT sector */
    if(OK(__ReadSector__(vol_, fatSector, &sectorData))) {
      /* Write 32-bit FAT entry (preserve upper 4 bits per FAT32 spec) */
      __WriteLE32__(sectorData + entryOffset, (value_ & 0x0FFFFFFFu) | (__ReadLE32__(sectorData + entryOffset) & 0xF0000000u));


      /* Write sector back */
      if(OK(__WriteSector__(vol_, fatSector, sectorData))) {
        /* Update all FAT copies */
        for(i = 1; i < vol_->numFATs; i++) {
          __WriteSector__(vol_, fatSector + (i * vol_->sectorsPerFAT), sectorData);
        }

        xMemFree((Addr_t *) sectorData);
        __ReturnOk__();
      } else {
        xMemFree((Addr_t *) sectorData);
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
 * @brief Find a free cluster in the FAT
 * @param  vol_          Pointer to mounted volume
 * @param  startHint_    Cluster number to start searching from (hint for efficiency)
 * @param  freeCluster_  Pointer to receive free cluster number
 * @return               ReturnOK on success, ReturnError if no free cluster found
 */
static Return_t __FindFreeCluster__(const Volume_t *vol_, Word_t startHint_, Word_t *freeCluster_) {
  FUNCTION_ENTER;


  Word_t cluster = 0;
  Word_t maxCluster = 0;
  Word_t fatEntry = 0;
  Word_t searchStart = 0;


  if(__PointerIsNotNull__(vol_) && __PointerIsNotNull__(freeCluster_)) {
    /* Calculate maximum cluster number based on FAT size */
    /* Each FAT entry is 4 bytes, so total clusters = (sectorsPerFAT * bytesPerSector) / 4 */
    maxCluster = (vol_->sectorsPerFAT * vol_->bytesPerSector) / 4u;

    /* Limit to reasonable maximum to avoid excessive searching */
    if(maxCluster > 0x10000u) {
      maxCluster = 0x10000u; /* Limit to 64K clusters for now */
    }

    /* Start from hint, but ensure we start from at least cluster 3 */
    /* Clusters 0 and 1 are reserved, cluster 2 is root directory */
    searchStart = (startHint_ >= 3u) ? startHint_ : 3u;

    /* Search for free cluster starting from hint */
    for(cluster = searchStart; cluster < maxCluster; cluster++) {
      if(OK(__GetFATEntry__(vol_, cluster, &fatEntry))) {
        if(fatEntry == FAT32_FREE_CLUSTER) {
          /* Found a free cluster */
          *freeCluster_ = cluster;
          __ReturnOk__();
        }
      } else {
        /* Error reading FAT entry */
        __AssertOnElse__();
        FUNCTION_EXIT;
      }
    }

    /* If we didn't find anything from the hint to end, search from cluster 3 to hint */
    if(searchStart > 3u) {
      for(cluster = 3u; cluster < searchStart; cluster++) {
        if(OK(__GetFATEntry__(vol_, cluster, &fatEntry))) {
          if(fatEntry == FAT32_FREE_CLUSTER) {
            /* Found a free cluster */
            *freeCluster_ = cluster;
            __ReturnOk__();
          }
        } else {
          /* Error reading FAT entry */
          __AssertOnElse__();
          FUNCTION_EXIT;
        }
      }
    }

    /* No free cluster found */
    __AssertOnElse__();
  } else {
    __AssertOnElse__();
  }

  FUNCTION_EXIT;
}


#if defined(POSIX_ARCH_OTHER)


/* For unit testing only! */
  void __FSStateClear__(void) {
    /* Clear any static state if needed */
    return;
  }


#endif /* if defined(POSIX_ARCH_OTHER) */