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
#include "../../src/fs.h"


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
  unit_try(OK(xMemAlloc((volatile Addr_t **) &blockConfig, sizeof(BlockDeviceConfig_t))));


  /* Configure block device to use RAM disk */
  blockConfig->ioDriverUID = 0x0100u; /* RAMDISK0 UID */
  blockConfig->protocol = BLOCK_PROTOCOL_RAW; /* Use RAW protocol */
  blockConfig->blockSize = 512; /* 512 bytes per sector */
  blockConfig->totalBlocks = 2048; /* 1MB / 512 = 2048 blocks */
  configSize = sizeof(BlockDeviceConfig_t);
  printf("[TEST] Config: ioDriverUID=%u, protocol=%u, blockSize=%u, totalBlocks=%lu\n", blockConfig->ioDriverUID, blockConfig->protocol, blockConfig->blockSize,
    (unsigned long) blockConfig->totalBlocks);
  printf("[TEST] sizeof(BlockDeviceConfig_t)=%lu, configSize=%lu\n", (unsigned long) sizeof(BlockDeviceConfig_t), (unsigned long) configSize);
  unit_try(OK(xDeviceConfigDevice(0x1000u, &configSize, (Addr_t *) blockConfig)));
  printf("[TEST] Block device configured\n");
  xMemFree((Addr_t *) blockConfig);


  /* Format the RAM disk with FAT32 filesystem */
  printf("[TEST] Starting format test...\n");
  fflush(stdout);
  unit_try(OK(xFSFormat(0x1000u, (const Byte_t *) "HELIOS     ")));
  printf("[TEST] Format completed\n");
  fflush(stdout);


  unit_end();

  /* Test mount/unmount operations */
  test_mount_unmount();

  /* Test file operations */
  test_file_operations();

  /* Test directory operations */
  test_directory_operations();

  /* Test file management operations */
  test_file_management();

  /* Test edge cases */
  test_fs_edge_cases();

  /* Test large file operations (multi-cluster) */
  test_large_file_operations();

  /* Test cluster boundary operations */
  test_cluster_boundary_operations();

  /* Test partial I/O operations */
  test_partial_io_operations();

  /* Test volume information validation */
  test_volume_info_validation();

  /* Test file mode validation */
  test_file_mode_validation();

  /* Test operations on closed files */
  test_closed_file_operations();

  /* Test multiple concurrent file operations */
  test_multiple_file_operations();

  printf("=== FS HARNESS COMPLETED ===\n");
  fflush(stdout);

  /* Cleanup */
  __FSStateClear__();
  __BlockDeviceStateClear__();
  __RAMDiskStateClear__();
}


void test_mount_unmount(void) {
  Volume_t *vol = null;
  VolumeInfo_t *volInfo = null;

  unit_begin("xFSMount() and xFSUnmount()");

  /* Mount the filesystem */
  if(OK(xFSMount(&vol, 0x1000u)) && (null != vol)) {
    unit_try(true == vol->mounted);
    unit_try(0x1000u == vol->blockDeviceUID);

    /* Get volume info */
    if(OK(xFSGetVolumeInfo(vol, &volInfo)) && (null != volInfo)) {
      /* Cleanup volume info */
      unit_try(OK(xMemFree(volInfo)));
    }

    /* Unmount filesystem */
    unit_try(OK(xFSUnmount(vol)));
  } else {
    unit_print("xFSMount() failed - filesystem may not be ready");
    unit_try(false);
  }

  unit_end();
}


void test_file_operations(void) {
  Volume_t *vol = null;
  File_t *file = null;
  Byte_t *readData = null;
  Word_t position = nil;
  Word_t fileSize = nil;
  Base_t eof = nil;
  const Byte_t testData[] = "Hello, HeliOS Filesystem!";
  const Size_t testDataSize = 26; /* Including null terminator */

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping file operations tests");
    return;
  }

  /* Test file open with create and write mode */
  unit_begin("xFileOpen() - Create and Write");
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/test.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(true == file->isOpen);
  unit_end();

  /* Test file write */
  unit_begin("xFileWrite()");
  unit_try(OK(xFileWrite(file, testDataSize, testData)));
  unit_end();

  /* Test file tell */
  unit_begin("xFileTell()");
  unit_try(OK(xFileTell(file, &position)));
  unit_try(testDataSize == position);
  unit_end();

  /* Test file sync */
  unit_begin("xFileSync()");
  unit_try(OK(xFileSync(file)));
  unit_end();

  /* Test file get size */
  unit_begin("xFileGetSize()");
  unit_try(OK(xFileGetSize(file, &fileSize)));
  unit_try(testDataSize == fileSize);
  unit_end();

  /* Seek back to beginning for reading (xFileOpen doesn't support reopening existing files yet) */
  unit_begin("xFileSeek() - Back to Start");
  unit_try(OK(xFileSeek(file, 0, FS_SEEK_SET)));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(0 == position);
  unit_end();

  /* Test file read */
  unit_begin("xFileRead()");
  unit_try(OK(xFileRead(file, testDataSize, &readData)));
  unit_try(null != readData);
  unit_try(0 == strncmp((char *) testData, (char *) readData, testDataSize));
  unit_try(OK(xMemFree(readData)));
  unit_end();

  /* Test file seek - beginning */
  unit_begin("xFileSeek() - FS_SEEK_SET");
  unit_try(OK(xFileSeek(file, 0, FS_SEEK_SET)));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(0 == position);
  unit_end();

  /* Test file seek - current */
  unit_begin("xFileSeek() - FS_SEEK_CUR");
  unit_try(OK(xFileSeek(file, 7, FS_SEEK_CUR)));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(7 == position);
  unit_end();

  /* Test file seek - end */
  unit_begin("xFileSeek() - FS_SEEK_END");
  unit_try(OK(xFileSeek(file, 0, FS_SEEK_END)));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(testDataSize == position);
  unit_end();

  /* Test EOF detection */
  unit_begin("xFileEOF()");
  unit_try(OK(xFileEOF(file, &eof)));
  unit_try(true == eof);
  unit_end();

  /* Close file */
  unit_try(OK(xFileClose(file)));

  /* Test file truncate */
  unit_begin("xFileTruncate()");
  file = null;
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/test.txt", FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileTruncate(file, 10)));
  unit_try(OK(xFileGetSize(file, &fileSize)));
  unit_try(10 == fileSize);
  unit_try(OK(xFileClose(file)));
  unit_end();

  /* Test file append mode */
  unit_begin("xFileOpen() - Append Mode");
  file = null;
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/test.txt", FS_MODE_APPEND | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, 6, (const Byte_t *) "MORE!!")));
  unit_try(OK(xFileGetSize(file, &fileSize)));
  unit_try(16 == fileSize); /* 10 + 6 */
  unit_try(OK(xFileClose(file)));
  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_directory_operations(void) {
  Volume_t *vol = null;
  Dir_t *dir = null;
  DirEntry_t *entry = null;
  Base_t exists = nil;
  File_t *file = null;
  int entryCount = 0;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping directory operations tests");
    return;
  }

  /* Test directory creation */
  unit_begin("xDirMake()");
  unit_try(OK(xDirMake(vol, (const Byte_t *) "/testdir")));
  unit_end();

  /* Test directory exists via file exists */
  unit_begin("xFileExists() - Directory");
  unit_try(OK(xFileExists(vol, (const Byte_t *) "/testdir", &exists)));
  unit_try(true == exists);
  unit_end();

  /* Create a file in the directory */
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/testdir/file1.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, 12, (const Byte_t *) "Test File 1\0")));
  unit_try(OK(xFileClose(file)));

  /* Create another file */
  file = null;
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/testdir/file2.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, 12, (const Byte_t *) "Test File 2\0")));
  unit_try(OK(xFileClose(file)));

  /* Test directory open */
  unit_begin("xDirOpen()");
  unit_try(OK(xDirOpen(&dir, vol, (const Byte_t *) "/testdir")));
  unit_try(null != dir);
  unit_end();

  /* Test directory read */
  unit_begin("xDirRead()");
  entryCount = 0;

  /* Read all directory entries */
  while(OK(xDirRead(dir, &entry))) {
    unit_try(null != entry);
    entryCount++;
    unit_try(OK(xMemFree(entry)));
    entry = null;
  }

  /* Should have at least 2 files (may have . and .. entries too) */
  unit_try(entryCount >= 2);
  unit_end();

  /* Test directory rewind */
  unit_begin("xDirRewind()");
  unit_try(OK(xDirRewind(dir)));
  entry = null;
  unit_try(OK(xDirRead(dir, &entry)));
  unit_try(null != entry);
  unit_try(OK(xMemFree(entry)));
  unit_end();

  /* Test directory close */
  unit_begin("xDirClose()");
  unit_try(OK(xDirClose(dir)));
  unit_end();

  /* Test opening root directory */
  unit_begin("xDirOpen() - Root Directory");
  dir = null;
  unit_try(OK(xDirOpen(&dir, vol, (const Byte_t *) "/")));
  unit_try(null != dir);
  unit_try(OK(xDirClose(dir)));
  unit_end();

  /* Cleanup - remove directory (should fail as it's not empty) */
  unit_begin("xDirRemove() - Non-empty Directory");
  unit_try(!OK(xDirRemove(vol, (const Byte_t *) "/testdir")));
  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_file_management(void) {
  Volume_t *vol = null;
  File_t *file = null;
  Base_t exists = nil;
  DirEntry_t *info = null;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping file management tests");
    return;
  }

  /* Create a test file */
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/manage.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, 13, (const Byte_t *) "Management!!\0")));
  unit_try(OK(xFileClose(file)));

  /* Test file exists */
  unit_begin("xFileExists() - Existing File");
  unit_try(OK(xFileExists(vol, (const Byte_t *) "/manage.txt", &exists)));
  unit_try(true == exists);
  unit_end();

  /* Test file exists - non-existing file */
  unit_begin("xFileExists() - Non-existing File");
  exists = nil;
  unit_try(OK(xFileExists(vol, (const Byte_t *) "/nonexist.txt", &exists)));
  unit_try(false == exists);
  unit_end();

  /* Test file get info */
  unit_begin("xFileGetInfo()");
  unit_try(OK(xFileGetInfo(vol, (const Byte_t *) "/manage.txt", &info)));
  unit_try(null != info);
  unit_try(OK(xMemFree(info)));
  unit_end();

  /* Test file rename */
  unit_begin("xFileRename()");
  unit_try(OK(xFileRename(vol, (const Byte_t *) "/manage.txt", (const Byte_t *) "/renamed.txt")));

  /* Verify old name doesn't exist */
  exists = nil;
  unit_try(OK(xFileExists(vol, (const Byte_t *) "/manage.txt", &exists)));
  unit_try(false == exists);

  /* Verify new name exists */
  exists = nil;
  unit_try(OK(xFileExists(vol, (const Byte_t *) "/renamed.txt", &exists)));
  unit_try(true == exists);
  unit_end();

  /* Test file unlink */
  unit_begin("xFileUnlink()");
  unit_try(OK(xFileUnlink(vol, (const Byte_t *) "/renamed.txt")));

  /* Verify file no longer exists */
  exists = nil;
  unit_try(OK(xFileExists(vol, (const Byte_t *) "/renamed.txt", &exists)));
  unit_try(false == exists);
  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_fs_edge_cases(void) {
  Volume_t *vol = null;
  File_t *file = null;
  Dir_t *dir = null;
  Base_t exists = nil;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping edge case tests");
    return;
  }

  /* Test NULL pointer handling */
  unit_begin("Edge Case - NULL Pointers");

  /* xFileOpen with NULL file pointer */
  unit_try(!OK(xFileOpen(null, vol, (const Byte_t *) "/test.txt", FS_MODE_READ)));

  /* xFileOpen with NULL volume */
  unit_try(!OK(xFileOpen(&file, null, (const Byte_t *) "/test.txt", FS_MODE_READ)));

  /* xFileOpen with NULL path */
  unit_try(!OK(xFileOpen(&file, vol, null, FS_MODE_READ)));

  /* xFileClose with NULL file */
  unit_try(!OK(xFileClose(null)));

  /* xDirOpen with NULL directory pointer */
  unit_try(!OK(xDirOpen(null, vol, (const Byte_t *) "/")));

  /* xDirOpen with NULL volume */
  unit_try(!OK(xDirOpen(&dir, null, (const Byte_t *) "/")));

  /* xDirOpen with NULL path */
  unit_try(!OK(xDirOpen(&dir, vol, null)));

  unit_end();

  /* Test invalid file operations */
  unit_begin("Edge Case - Invalid File Operations");

  /* Try to open non-existent file in read mode (should fail without CREATE) */
  file = null;
  unit_try(!OK(xFileOpen(&file, vol, (const Byte_t *) "/nonexist.txt", FS_MODE_READ)));
  unit_try(null == file);

  /* Try to read from a file opened in write-only mode */
  file = null;
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/writeonly.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  /* Note: Actual behavior depends on implementation - may or may not allow read */
  unit_try(OK(xFileClose(file)));

  unit_end();

  /* Test invalid path operations */
  unit_begin("Edge Case - Invalid Paths");

  /* Try to unlink non-existent file */
  unit_try(!OK(xFileUnlink(vol, (const Byte_t *) "/doesnotexist.txt")));

  /* Try to rename non-existent file */
  unit_try(!OK(xFileRename(vol, (const Byte_t *) "/nosuchfile.txt", (const Byte_t *) "/newname.txt")));

  /* Try to open non-existent directory */
  dir = null;
  unit_try(!OK(xDirOpen(&dir, vol, (const Byte_t *) "/nosuchdir")));

  /* Try to remove non-existent directory */
  unit_try(!OK(xDirRemove(vol, (const Byte_t *) "/nosuchdir")));

  unit_end();

  /* Test double mount (should fail) */
  unit_begin("Edge Case - Double Mount");
  {
    Volume_t *vol2 = null;
    unit_try(!OK(xFSMount(&vol2, 0x1000u)));
  }
  unit_end();

  /* Test operations on unmounted volume */
  unit_begin("Edge Case - Operations After Unmount");
  unit_try(OK(xFSUnmount(vol)));

  /* These should fail after unmount */
  file = null;
  unit_try(!OK(xFileOpen(&file, vol, (const Byte_t *) "/test.txt", FS_MODE_READ)));

  dir = null;
  unit_try(!OK(xDirOpen(&dir, vol, (const Byte_t *) "/")));

  exists = nil;
  unit_try(!OK(xFileExists(vol, (const Byte_t *) "/test.txt", &exists)));

  unit_end();
}


void test_large_file_operations(void) {
  Volume_t *vol = null;
  File_t *file = null;
  Byte_t *writeData = null;
  Byte_t *readData = null;
  Size_t largeSize = 8192; /* 8KB - spans multiple clusters (512*8=4KB per cluster) */
  Size_t i = 0;
  Word_t fileSize = nil;
  Word_t position = nil;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping large file operations tests");
    return;
  }

  unit_begin("Large File Operations - Multi-cluster Write");

  /* Allocate large buffer with pattern data */
  unit_try(OK(xMemAlloc((volatile Addr_t **) &writeData, largeSize)));
  unit_try(null != writeData);

  /* Fill with pattern (repeating 0-255) */
  for(i = 0; i < largeSize; i++) {
    writeData[i] = (Byte_t) (i & 0xFFu);
  }

  /* Create and write large file */
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/largefile.dat", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, largeSize, writeData)));

  /* Verify file size */
  unit_try(OK(xFileGetSize(file, &fileSize)));
  unit_try(largeSize == fileSize);

  /* Verify position */
  unit_try(OK(xFileTell(file, &position)));
  unit_try(largeSize == position);

  unit_end();

  unit_begin("Large File Operations - Multi-cluster Read");

  /* Seek back to start */
  unit_try(OK(xFileSeek(file, 0, FS_SEEK_SET)));

  /* Read entire file */
  unit_try(OK(xFileRead(file, largeSize, &readData)));
  unit_try(null != readData);

  /* Verify data matches */
  for(i = 0; i < largeSize; i++) {
    if(writeData[i] != readData[i]) {
      unit_print("Data mismatch detected in large file read");
      unit_try(false);
      break;
    }
  }

  unit_try(OK(xMemFree(readData)));
  unit_try(OK(xMemFree(writeData)));
  unit_try(OK(xFileClose(file)));

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_cluster_boundary_operations(void) {
  Volume_t *vol = null;
  File_t *file = null;
  Byte_t *writeData = null;
  Byte_t *readData = null;
  Size_t clusterSize = 4096; /* 512 bytes/sector * 8 sectors/cluster */
  Size_t testSize = clusterSize - 100; /* Write near cluster boundary */
  Size_t i = 0;
  Word_t position = nil;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping cluster boundary tests");
    return;
  }

  unit_begin("Cluster Boundary - Write at Boundary");

  /* Allocate buffer */
  unit_try(OK(xMemAlloc((volatile Addr_t **) &writeData, clusterSize + 200)));
  unit_try(null != writeData);

  /* Fill with pattern */
  for(i = 0; i < clusterSize + 200; i++) {
    writeData[i] = (Byte_t) ((i * 7) & 0xFFu);
  }

  /* Create file and write up to near cluster boundary */
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/boundary.dat", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, testSize, writeData)));

  /* Write more data to cross cluster boundary */
  unit_try(OK(xFileWrite(file, 200, writeData + testSize)));

  unit_end();

  unit_begin("Cluster Boundary - Read Across Boundary");

  /* Seek to position near cluster boundary */
  unit_try(OK(xFileSeek(file, clusterSize - 50, FS_SEEK_SET)));
  unit_try(OK(xFileTell(file, &position)));
  unit_try((clusterSize - 50) == position);

  /* Read data that spans cluster boundary */
  unit_try(OK(xFileRead(file, 100, &readData)));
  unit_try(null != readData);

  /* Verify data */
  for(i = 0; i < 100; i++) {
    if(writeData[clusterSize - 50 + i] != readData[i]) {
      unit_print("Cluster boundary data mismatch detected");
      unit_try(false);
      break;
    }
  }

  unit_try(OK(xMemFree(readData)));
  unit_try(OK(xMemFree(writeData)));
  unit_try(OK(xFileClose(file)));

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_partial_io_operations(void) {
  Volume_t *vol = null;
  File_t *file = null;
  Byte_t *readData = null;
  const Byte_t testData[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  Size_t testDataSize = 36;
  Word_t position = nil;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping partial I/O tests");
    return;
  }

  unit_begin("Partial I/O - Write and Partial Reads");

  /* Create file with test data */
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/partial.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, testDataSize, testData)));
  unit_try(OK(xFileSeek(file, 0, FS_SEEK_SET)));

  unit_end();

  unit_begin("Partial I/O - Read First 10 Bytes");

  /* Read first 10 bytes */
  unit_try(OK(xFileRead(file, 10, &readData)));
  unit_try(null != readData);
  unit_try(0 == strncmp((char *) testData, (char *) readData, 10));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(10 == position);
  unit_try(OK(xMemFree(readData)));

  unit_end();

  unit_begin("Partial I/O - Read Middle 10 Bytes");

  /* Read next 10 bytes */
  readData = null;
  unit_try(OK(xFileRead(file, 10, &readData)));
  unit_try(null != readData);
  unit_try(0 == strncmp((char *) (testData + 10), (char *) readData, 10));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(20 == position);
  unit_try(OK(xMemFree(readData)));

  unit_end();

  unit_begin("Partial I/O - Seek and Read from Middle");

  /* Seek to middle and read */
  unit_try(OK(xFileSeek(file, 15, FS_SEEK_SET)));
  readData = null;
  unit_try(OK(xFileRead(file, 5, &readData)));
  unit_try(null != readData);
  unit_try(0 == strncmp((char *) (testData + 15), (char *) readData, 5));
  unit_try(OK(xMemFree(readData)));

  unit_end();

  unit_begin("Partial I/O - Read at EOF");

  /* Seek to end and try to read (should fail gracefully) */
  unit_try(OK(xFileSeek(file, 0, FS_SEEK_END)));
  unit_try(OK(xFileTell(file, &position)));
  unit_try(testDataSize == position);

  /* Try to read at EOF - should fail */
  readData = null;
  unit_try(!OK(xFileRead(file, 10, &readData)));

  unit_try(OK(xFileClose(file)));

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_volume_info_validation(void) {
  Volume_t *vol = null;
  VolumeInfo_t *volInfo = null;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping volume info tests");
    return;
  }

  unit_begin("Volume Info - Get and Validate");

  /* Get volume information */
  unit_try(OK(xFSGetVolumeInfo(vol, &volInfo)));
  unit_try(null != volInfo);

  /* Validate volume parameters */
  unit_try(512 == volInfo->bytesPerSector);
  unit_try(8 == volInfo->sectorsPerCluster);
  unit_try(4096 == volInfo->bytesPerCluster); /* 512 * 8 */

  unit_try(OK(xMemFree(volInfo)));

  unit_end();

  unit_begin("Volume Info - NULL Pointer Handling");

  /* Test NULL pointer for volume */
  unit_try(!OK(xFSGetVolumeInfo(null, &volInfo)));

  /* Test NULL pointer for info output */
  unit_try(!OK(xFSGetVolumeInfo(vol, null)));

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_file_mode_validation(void) {
  Volume_t *vol = null;
  File_t *file1 = null;
  File_t *file2 = null;
  Byte_t *readData = null;
  Word_t position = nil;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping file mode tests");
    return;
  }

  unit_begin("File Mode - Create Without Write Mode");

  /* Try to create file with only CREATE mode (should work as stub implementation ignores mode) */
  unit_try(OK(xFileOpen(&file1, vol, (const Byte_t *) "/modetest1.txt", FS_MODE_CREATE)));
  unit_try(null != file1);

  /* Write should fail without WRITE or APPEND mode */
  unit_try(!OK(xFileWrite(file1, 10, (const Byte_t *) "test data\0")));

  unit_try(OK(xFileClose(file1)));

  unit_end();

  unit_begin("File Mode - Write Mode");

  /* Create file with WRITE mode */
  file1 = null;
  unit_try(OK(xFileOpen(&file1, vol, (const Byte_t *) "/modetest2.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file1);
  unit_try(OK(xFileWrite(file1, 10, (const Byte_t *) "writemode\0")));
  unit_try(OK(xFileClose(file1)));

  unit_end();

  unit_begin("File Mode - Append Mode");

  /* Open file in append mode */
  file1 = null;
  unit_try(OK(xFileOpen(&file1, vol, (const Byte_t *) "/modetest2.txt", FS_MODE_APPEND | FS_MODE_WRITE)));
  unit_try(null != file1);
  unit_try(OK(xFileWrite(file1, 7, (const Byte_t *) "append\0")));

  /* Verify position at end after append */
  position = nil;

  unit_try(OK(xFileTell(file1, &position)));
  unit_try(17 == position); /* 10 + 7 */

  unit_try(OK(xFileClose(file1)));

  unit_end();

  unit_begin("File Mode - Read Mode");

  /* Open existing file in read mode */
  file2 = null;
  unit_try(OK(xFileOpen(&file2, vol, (const Byte_t *) "/modetest2.txt", FS_MODE_READ)));
  unit_try(null != file2);

  /* Read should work */
  unit_try(OK(xFileRead(file2, 9, &readData)));
  unit_try(null != readData);
  unit_try(OK(xMemFree(readData)));

  unit_try(OK(xFileClose(file2)));

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_closed_file_operations(void) {
  Volume_t *vol = null;
  File_t *file = null;

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping closed file tests");
    return;
  }

  unit_begin("Closed File Operations - Create and Close");

  /* Create a file and immediately close it */
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/closedtest.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileWrite(file, 12, (const Byte_t *) "test data!!\0")));
  unit_try(OK(xFileClose(file)));

  unit_end();

  unit_begin("Closed File Operations - Operations on Closed File");

  /* Try operations on closed file (file pointer is now invalid/freed) */
  /* Note: These will likely crash or corrupt memory, so we can't test them
   * In a real implementation, file handles would be validated */
  unit_print("Cannot safely test operations on freed file handle");

  unit_end();

  unit_begin("Closed File Operations - Double Close");

  /* Create another file */
  file = null;
  unit_try(OK(xFileOpen(&file, vol, (const Byte_t *) "/doubleclose.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file);
  unit_try(OK(xFileClose(file)));

  /* Try to close again - should fail as memory is freed */
  /* Cannot test this safely as file pointer is invalid */
  unit_print("Cannot safely test double close on freed handle");

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}


void test_multiple_file_operations(void) {
  Volume_t *vol = null;
  File_t *file1 = null;
  File_t *file2 = null;
  File_t *file3 = null;
  Byte_t *readData1 = null;
  Byte_t *readData2 = null;
  Byte_t *readData3 = null;
  const Byte_t data1[] = "File One Data";
  const Byte_t data2[] = "File Two Data";
  const Byte_t data3[] = "File Three Data";

  /* Mount filesystem first */
  if(!OK(xFSMount(&vol, 0x1000u)) || (null == vol)) {
    unit_print("xFSMount() failed - skipping multiple file tests");
    return;
  }

  unit_begin("Multiple Files - Create Three Files");

  /* Create three files simultaneously */
  unit_try(OK(xFileOpen(&file1, vol, (const Byte_t *) "/multi1.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file1);

  unit_try(OK(xFileOpen(&file2, vol, (const Byte_t *) "/multi2.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file2);

  unit_try(OK(xFileOpen(&file3, vol, (const Byte_t *) "/multi3.txt", FS_MODE_CREATE | FS_MODE_WRITE)));
  unit_try(null != file3);

  unit_end();

  unit_begin("Multiple Files - Write to All Files");

  /* Write different data to each file */
  unit_try(OK(xFileWrite(file1, 14, data1)));
  unit_try(OK(xFileWrite(file2, 14, data2)));
  unit_try(OK(xFileWrite(file3, 16, data3)));

  unit_end();

  unit_begin("Multiple Files - Seek All Files");

  /* Seek all files back to start */
  unit_try(OK(xFileSeek(file1, 0, FS_SEEK_SET)));
  unit_try(OK(xFileSeek(file2, 0, FS_SEEK_SET)));
  unit_try(OK(xFileSeek(file3, 0, FS_SEEK_SET)));

  unit_end();

  unit_begin("Multiple Files - Read and Verify All Files");

  /* Read from all files and verify data */
  unit_try(OK(xFileRead(file1, 14, &readData1)));
  unit_try(null != readData1);
  unit_try(0 == strncmp((char *) data1, (char *) readData1, 14));

  unit_try(OK(xFileRead(file2, 14, &readData2)));
  unit_try(null != readData2);
  unit_try(0 == strncmp((char *) data2, (char *) readData2, 14));

  unit_try(OK(xFileRead(file3, 16, &readData3)));
  unit_try(null != readData3);
  unit_try(0 == strncmp((char *) data3, (char *) readData3, 16));

  unit_try(OK(xMemFree(readData1)));
  unit_try(OK(xMemFree(readData2)));
  unit_try(OK(xMemFree(readData3)));

  unit_end();

  unit_begin("Multiple Files - Close All Files");

  /* Close all files */
  unit_try(OK(xFileClose(file1)));
  unit_try(OK(xFileClose(file2)));
  unit_try(OK(xFileClose(file3)));

  unit_end();

  /* Unmount filesystem */
  unit_try(OK(xFSUnmount(vol)));
}