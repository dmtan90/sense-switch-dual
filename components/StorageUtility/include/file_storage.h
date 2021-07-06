/*
  * file_storage
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/


#ifndef FILE_STORAGE_UTILITY_H
#define FILE_STORAGE_UTILITY_H

extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
}

struct sFile{
    char mFileName[10];
    uint32_t mFileSize;
    bool mRemoveOld;
    uint8_t* mData;
};

class file_storage
{
public:
    static void init();
    static void writeFile(const char* tableName, uint8_t* data, uint32_t size, bool removeOld);
    static void readFile(const char* tableName, uint8_t* data, uint32_t& size);
    static bool removeFile(const char* tableName);
    static void uninit();
    static bool isFileExist(const char* table);
    static bool factoryReset();
    static uint32_t getRunningMode();
    static void writeTask(void* data);
    static const uint32_t MAX_FILE_LENGTH = 32;

private:
    static char* getSystemPath(const char* table);

private:
    static void createAndTakeSemaphore();
    static void releaseSemaphore();
    static esp_vfs_fat_mount_config_t mMountConfig;
    static const char mBasePath[];
    static const char mTag[];
    static wl_handle_t mWearLevelHandle;
    static SemaphoreHandle_t s_Semaphore;
};

#endif