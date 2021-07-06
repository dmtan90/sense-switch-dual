/*
  * file_storage
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "file_storage.h"
#include "config.h"

wl_handle_t file_storage::mWearLevelHandle;
const char file_storage::mBasePath[] = "/fs";
const char file_storage::mTag[] = "FileStorage";
SemaphoreHandle_t file_storage::s_Semaphore = NULL;

esp_vfs_fat_mount_config_t file_storage::mMountConfig = {
    .format_if_mount_failed = true,
    .max_files = 4,
    .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
};

void file_storage::init()
{
    ESP_LOGI(mTag, "file_storage::init()");

    mWearLevelHandle = WL_INVALID_HANDLE;
    esp_err_t err = esp_vfs_fat_spiflash_mount(mBasePath, "storage", &mMountConfig, &mWearLevelHandle);
    if (ESP_OK != err) {
        ESP_LOGE(mTag, "Failed to mount FATFS (0x%x)", err);
        return;
    } 
}

void file_storage::uninit()
{
    ESP_LOGI(mTag, "file_storage::uninit()");
    ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(mBasePath, mWearLevelHandle));
}

void file_storage::writeFile(const char* table, uint8_t* data, uint32_t size, bool removeOld)
{
    sFile *fileObj = new sFile();
    strcpy(fileObj->mFileName, table);
    fileObj->mFileSize = size;
    fileObj->mRemoveOld = removeOld;
    fileObj->mData = new uint8_t[size+1];
    for(uint32_t i = 0; i < size; i++)
    {
        fileObj->mData[i] = data[i];
    }
    fileObj->mData[size] = '\0';

    ESP_LOGD(mTag, "writeFile file=%s removeOld=%d size=%d", 
        fileObj->mFileName, fileObj->mRemoveOld, fileObj->mFileSize);
    uint32_t mem = 1024*10 + (size / 1024)*1024;
    xTaskCreatePinnedToCore(writeTask, "writeTask", mem, (void*)fileObj, tskIDLE_PRIORITY + 1, NULL, 1);
}

void file_storage::readFile(const char* table, uint8_t* data, uint32_t& size)
{
    createAndTakeSemaphore();
    char* pathSystem = getSystemPath(table);
    ESP_LOGI(mTag, "Read at: %s", pathSystem);

    FILE *f = fopen(pathSystem, "rb");
    if (f == NULL) 
    {
        ESP_LOGE(mTag, "Failed to open file %s for reading: %s", pathSystem, strerror(errno));
        goto exit;
    }

    size = fread(data, 1, size, f);
    fclose(f);
exit:
    if(NULL != pathSystem)
    {
        delete[] pathSystem;
    }
    releaseSemaphore();
}

bool file_storage::removeFile(const char* table)
{
    bool rs = false;
    if(true == isFileExist(table))
    {
        char* pathSystem = getSystemPath(table);
        ESP_LOGI(mTag, "removeFile at: %s", pathSystem);
        if( remove( pathSystem ) != 0 )
        {
            ESP_LOGE(mTag, "Error deleting file %s" , pathSystem);
        }
        else
        {
            ESP_LOGD(mTag, "File successfully deleted" );
            rs = true;
        }
        delete[] pathSystem;
    }
    else
    {
        ESP_LOGD(mTag, "File is not existed %s", table);
    }
    return rs;
}

char* file_storage::getSystemPath(const char* table)
{
    char* mFileSystemPath = new char[MAX_FILE_LENGTH];
    sprintf(mFileSystemPath, "%s/%s.bin", mBasePath, table);
    return mFileSystemPath;
}

bool file_storage::isFileExist(const char* table)
{
    createAndTakeSemaphore();
    char* pathSystem = getSystemPath(table);
    ESP_LOGI(mTag, "isFileExist at: %s", pathSystem);
    bool fileExist = false;

    // Try to open file
    FILE * file = fopen(pathSystem, "r");
    if (NULL != file)
    {
        fclose(file);
        fileExist =  true;
    }
    
    delete[] pathSystem;
    releaseSemaphore();
    return fileExist;
}

uint32_t file_storage::getRunningMode()
{
    return 0;
}

bool file_storage::factoryReset()
{
    createAndTakeSemaphore();
    ESP_LOGD(mTag, "factoryReset");
    uint32_t MAX_FILE_IN_DIR = 1024;
    uint8_t MAX_FILE_NAME_SIZE = 10;
    char * files[MAX_FILE_IN_DIR];

    bool result = false;
    struct dirent *de = NULL;
    DIR *d = NULL;
    ESP_LOGD(mTag, "Open directory %s", mBasePath);
    d = opendir( mBasePath );
    if(d == NULL)
    {
        ESP_LOGD(mTag, "Can't open directory %s", mBasePath);
        releaseSemaphore();
        return result;
    }
    ESP_LOGD(mTag, "Read directory %s", mBasePath);
    de = readdir(d);
    int n = 0;
    // Loop while not NULL
    while(de != NULL)
    {
        if(MAX_FILE_IN_DIR > n && strstr(de->d_name, ".BIN") != NULL)
        {
            ESP_LOGD(mTag, "Read file %s len=%d", de->d_name, strlen(de->d_name));
            files[n] = strdup(de->d_name);
            n++;
        }

        de = readdir(d);
    }
    result = true;

    closedir(d);
    n = n - 1;
    ESP_LOGD(mTag, "Total file %d", n);
    //remove files
    for(int i = 0; i < n; i++)
    {
        ESP_LOGD(mTag, "Delete file %s", files[i]);
        //don't append *.bin
        char* mFileSystemPath = new char[MAX_FILE_LENGTH];
        sprintf(mFileSystemPath, "%s/%s", mBasePath, files[i]);
        ESP_LOGI(mTag, "removeFile at: %s", mFileSystemPath);
        if( remove( mFileSystemPath ) != 0 )
        {
            ESP_LOGE(mTag, "Error deleting file" );
        }
        else
        {
            ESP_LOGD(mTag, "File successfully deleted" );
        } 
        delete[] mFileSystemPath;
        delete[] files[i];
        delay_ms(100);  
    }

    releaseSemaphore();
    return result;
}

void file_storage::writeTask(void* data)
{
    createAndTakeSemaphore();
    sFile *fileObj = (sFile*)data;

    ESP_LOGD(mTag, "writeTask file=%s removeOld=%d size=%d", 
        fileObj->mFileName, fileObj->mRemoveOld, fileObj->mFileSize);
    char* pathSystem = getSystemPath(fileObj->mFileName);
    uint32_t writecount = 0;

    ESP_LOGI(mTag, "Write at: %s", pathSystem);
    FILE *f = NULL;
    if(true == fileObj->mRemoveOld)
    {
        f = fopen(pathSystem, "wb");
    }
    else
    {
        f = fopen(pathSystem, "ab");
    }

    if (NULL == f)
    {
        ESP_LOGE(mTag, "Failed to open file for writing: %s", strerror(errno));
        goto exit;
    }
    ESP_LOGI(mTag, "Begin writing");
    writecount = fwrite(fileObj->mData, 1, fileObj->mFileSize, f);
    ESP_LOGI(mTag, "End writing");
    if(fileObj->mFileSize != writecount)
    {
        ESP_LOGE(mTag, "Write error occur %s", strerror(errno));
    }

    fclose(f);
    ESP_LOGI(mTag, "Close file");
exit:
    delete fileObj->mData;
    delete fileObj;
    delete[] pathSystem;
    releaseSemaphore();
    vTaskDelete(NULL);
}

void file_storage::createAndTakeSemaphore()
{
    if(NULL == s_Semaphore)
    {
        s_Semaphore = xSemaphoreCreateMutex();
    }

    while( xSemaphoreTake( s_Semaphore, ( TickType_t ) 100 ) == 0 );
}

void file_storage::releaseSemaphore()
{
    xSemaphoreGive( s_Semaphore );
}