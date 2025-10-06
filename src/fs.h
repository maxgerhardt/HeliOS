/*UNCRUSTIFY-OFF*/
/**
 * @file fs.h
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
#ifndef FS_H_
  #define FS_H_

  #include "config.h"
  #include "defines.h"
  #include "types.h"
  #include "port.h"
  #include "device.h"
  #include "mem.h"
  #include "queue.h"
  #include "stream.h"
  #include "sys.h"
  #include "task.h"
  #include "timer.h"

  /* File open modes */
  #if defined(FS_MODE_READ)
    #undef FS_MODE_READ
  #endif /* if defined(FS_MODE_READ) */
  #define FS_MODE_READ 0x01u /* 1 */

  #if defined(FS_MODE_WRITE)
    #undef FS_MODE_WRITE
  #endif /* if defined(FS_MODE_WRITE) */
  #define FS_MODE_WRITE 0x02u /* 2 */

  #if defined(FS_MODE_APPEND)
    #undef FS_MODE_APPEND
  #endif /* if defined(FS_MODE_APPEND) */
  #define FS_MODE_APPEND 0x04u /* 4 */

  #if defined(FS_MODE_CREATE)
    #undef FS_MODE_CREATE
  #endif /* if defined(FS_MODE_CREATE) */
  #define FS_MODE_CREATE 0x08u /* 8 */

  /* File seek origins */
  #if defined(FS_SEEK_SET)
    #undef FS_SEEK_SET
  #endif /* if defined(FS_SEEK_SET) */
  #define FS_SEEK_SET 0x00u /* 0 */

  #if defined(FS_SEEK_CUR)
    #undef FS_SEEK_CUR
  #endif /* if defined(FS_SEEK_CUR) */
  #define FS_SEEK_CUR 0x01u /* 1 */

  #if defined(FS_SEEK_END)
    #undef FS_SEEK_END
  #endif /* if defined(FS_SEEK_END) */
  #define FS_SEEK_END 0x02u /* 2 */

  /* Maximum path length - configurable */
  #if !defined(CONFIG_FS_MAX_PATH_LENGTH)
    #define CONFIG_FS_MAX_PATH_LENGTH 256u /* 256 */
  #endif /* if !defined(CONFIG_FS_MAX_PATH_LENGTH) */

  #ifdef __cplusplus
    extern "C" {
  #endif /* ifdef __cplusplus */

  /* Volume Management */
  Return_t xFSMount(Volume_t **volume_, const HalfWord_t blockDeviceUID_);
  Return_t xFSUnmount(Volume_t *volume_);
  Return_t xFSGetVolumeInfo(const Volume_t *volume_, VolumeInfo_t **info_);
  Return_t xFSFormat(const HalfWord_t blockDeviceUID_, const Byte_t *volumeLabel_);

  /* File Operations */
  Return_t xFileOpen(File_t **file_, Volume_t *volume_, const Byte_t *path_, const Byte_t mode_);
  Return_t xFileClose(File_t *file_);
  Return_t xFileRead(File_t *file_, const Size_t size_, Byte_t **data_);
  Return_t xFileWrite(File_t *file_, const Size_t size_, const Byte_t *data_);
  Return_t xFileSeek(File_t *file_, const Word_t offset_, const Byte_t origin_);
  Return_t xFileTell(const File_t *file_, Word_t *position_);
  Return_t xFileGetSize(const File_t *file_, Word_t *size_);
  Return_t xFileSync(File_t *file_);
  Return_t xFileTruncate(File_t *file_, const Word_t size_);
  Return_t xFileEOF(const File_t *file_, Base_t *eof_);

  /* Directory Operations */
  Return_t xDirOpen(Dir_t **dir_, Volume_t *volume_, const Byte_t *path_);
  Return_t xDirClose(Dir_t *dir_);
  Return_t xDirRead(Dir_t *dir_, DirEntry_t **entry_);
  Return_t xDirRewind(Dir_t *dir_);
  Return_t xDirMake(Volume_t *volume_, const Byte_t *path_);
  Return_t xDirRemove(Volume_t *volume_, const Byte_t *path_);

  /* File/Directory Management */
  Return_t xFileExists(Volume_t *volume_, const Byte_t *path_, Base_t *exists_);
  Return_t xFileUnlink(Volume_t *volume_, const Byte_t *path_);
  Return_t xFileRename(Volume_t *volume_, const Byte_t *oldPath_, const Byte_t *newPath_);
  Return_t xFileGetInfo(Volume_t *volume_, const Byte_t *path_, DirEntry_t **entry_);

  #if defined(POSIX_ARCH_OTHER)
    void __FSStateClear__(void);
  #endif /* if defined(POSIX_ARCH_OTHER) */

  #ifdef __cplusplus
    }
  #endif /* ifdef __cplusplus */
#endif /* ifndef FS_H_ */
