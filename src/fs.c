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


Return_t xFSMount(Volume_t **volume_, const HalfWord_t blockDeviceUID_) {
  FUNCTION_ENTER;

  Volume_t *vol = null;

  if(__PointerIsNotNull__(volume_)) {
    /* Allocate volume structure in heap memory */
    if(OK(xMemAlloc((volatile Addr_t **)&vol, sizeof(Volume_t)))) {
      /* Store block device UID for all I/O operations */
      vol->blockDeviceUID = blockDeviceUID_;
      vol->mounted = false;

      /* Read boot sector would go here - for now just return success */
      vol->bytesPerSector = 512;
      vol->sectorsPerCluster = 8;
      vol->reservedSectors = 32;
      vol->numFATs = 2;
      vol->sectorsPerFAT = 16;
      vol->rootDirCluster = 2;
      vol->fatStartSector = 32;
      vol->dataStartSector = 64;
      vol->mounted = true;

      *volume_ = vol;
      __ReturnOk__();
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

    /* Free volume structure */
    if(OK(xMemFree(volume_))) {
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
    /* Allocate info structure */
    if(OK(xMemAlloc((volatile Addr_t **)&info, sizeof(VolumeInfo_t)))) {
      info->bytesPerSector = volume_->bytesPerSector;
      info->sectorsPerCluster = volume_->sectorsPerCluster;
      info->bytesPerCluster = (Word_t)volume_->bytesPerSector * volume_->sectorsPerCluster;
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

  /* Format implementation would go here */

  FUNCTION_EXIT;
}


Return_t xFileOpen(File_t **file_, Volume_t *volume_, const Byte_t *path_, const Byte_t mode_) {
  FUNCTION_ENTER;

  File_t *file = null;

  if(__PointerIsNotNull__(file_) && __PointerIsNotNull__(volume_) && __PointerIsNotNull__(path_)) {
    /* Allocate file structure */
    if(OK(xMemAlloc((volatile Addr_t **)&file, sizeof(File_t)))) {
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

    /* Free file structure */
    if(OK(xMemFree(file_))) {
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

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xFileWrite(File_t *file_, const Size_t size_, const Byte_t *data_) {
  FUNCTION_ENTER;

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xFileSeek(File_t *file_, const Word_t offset_, const Byte_t origin_) {
  FUNCTION_ENTER;

  /* Stub implementation */

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

  /* Stub implementation */

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
    /* Allocate directory handle */
    if(OK(xMemAlloc((volatile Addr_t **)&dir, sizeof(Dir_t)))) {
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

    if(OK(xMemFree(dir_))) {
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

  /* Stub implementation */

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

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xDirRemove(Volume_t *volume_, const Byte_t *path_) {
  FUNCTION_ENTER;

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xFileExists(Volume_t *volume_, const Byte_t *path_, Base_t *exists_) {
  FUNCTION_ENTER;

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xFileUnlink(Volume_t *volume_, const Byte_t *path_) {
  FUNCTION_ENTER;

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xFileRename(Volume_t *volume_, const Byte_t *oldPath_, const Byte_t *newPath_) {
  FUNCTION_ENTER;

  /* Stub implementation */

  FUNCTION_EXIT;
}


Return_t xFileGetInfo(Volume_t *volume_, const Byte_t *path_, DirEntry_t **entry_) {
  FUNCTION_ENTER;

  /* Stub implementation */

  FUNCTION_EXIT;
}


#if defined(POSIX_ARCH_OTHER)


/* For unit testing only! */
void __FSStateClear__(void) {
  /* Clear any static state if needed */

  return;
}


#endif /* if defined(POSIX_ARCH_OTHER) */
