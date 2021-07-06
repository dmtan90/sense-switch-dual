/*
  * ota_update
  *
  * Created: March 11, 2017
  * Author: Tan Do
  * Email: dmtan90@gmail.com
*/

#include "ota_update.h"


// Partition for the OTA update.
static const esp_partition_t *otaPartition;

// SPI flash address for next write operation.
static uint32_t sFlashCurrentAddress;

static esp_ota_handle_t otaHandle;

const char TAG[] = "OTA_Update";

ota_update *ota_update_instance  =  NULL;

ota_update::ota_update()
{
    ESP_LOGD(TAG, "ota_update");
};

ota_update* ota_update::getInstance()
{
    if(ota_update_instance == NULL)
    {
        ota_update_instance = new ota_update();
    }
    return ota_update_instance;
}

ota_update::~ota_update()
{
    ESP_LOGD(TAG, "~ota_update");
};

void ota_update::init()
{
    spi_flash_init();
}

int ota_update::inProgress()
{
    return otaPartition ? 1 : 0;
}

ota_update_result ota_update::begin()
{
    otaPartition = findNextBootPartition();
    if (!otaPartition) {
        return OTA_ERR_PARTITION_NOT_FOUND;
    }

    sFlashCurrentAddress = otaPartition->address;
    ESP_LOGD(TAG, "Set start address for flash writes to 0x%08x", sFlashCurrentAddress);

    // TODO
    // This operation would trigger the watchdog of the currently running task if we fed it with the full partition size.
    // To avoid the issue, we erase only a small part here and afterwards erase every page before writing to it.
    esp_err_t result = esp_ota_begin(otaPartition, OTA_SIZE_UNKNOWN, &otaHandle);
    ESP_LOGD(TAG, "Result from esp_ota_begin: %d %d", result, otaHandle);
    if (result == ESP_OK) {
        return OTA_OK;
    }

    return OTA_ERR_BEGIN_FAILED;
}

ota_update_result ota_update::end()
{
    if (!otaPartition) {
        return OTA_ERR_PARTITION_NOT_FOUND;
    }

    esp_err_t result = esp_ota_end(otaHandle);
    if (result != ESP_OK) {
        return OTA_ERR_END_FAILED;
    }

    result = esp_ota_set_boot_partition(otaPartition);
    if (result == ESP_OK) {
        ESP_LOGD(TAG, "Boot partition activated: %s", otaPartition->label);
        return OTA_OK;
    }

    ESP_LOGD(TAG, "Failed to activate boot partition %s, error %d", otaPartition->label, result);

    otaPartition = NULL;
    return OTA_ERR_PARTITION_NOT_ACTIVATED;
}

ota_update_result ota_update::writeHexData(const char *hexData, size_t len)
{
    /*uint8_t buf[len];

    for (int i = 0; i < 4096; i++) {
        buf[i] = (i < len) ? hexData[i] : 0xff;
    }*/

    // Erase flash pages at 4k boundaries.
    /*if (sFlashCurrentAddress % 0x1000 == 0) {
        int flashSectorToErase = sFlashCurrentAddress / 0x1000;
        ESP_LOGD(TAG, "Erasing flash sector %d", flashSectorToErase);
        spi_flash_erase_sector(flashSectorToErase);
    }*/

    // Write data into flash memory.
    ESP_LOGD(TAG, "Writing flash at 0x%08x...", sFlashCurrentAddress);
    // esp_err_t result = spi_flash_write(sFlashCurrentAddress, buf, 4096);
    esp_err_t result = esp_ota_write(otaHandle, hexData, len);
    if (result != ESP_OK) {
        ESP_LOGD(TAG, "Failed to write flash at address 0x%08x, error %d", sFlashCurrentAddress, result);
        return OTA_ERR_WRITE_FAILED;
    }

    sFlashCurrentAddress += len;
    return OTA_OK;
}

void ota_update::dumpInformation()
{
    uint8_t buf[4096], buf2[4096];
    esp_err_t result;

    ESP_LOGD(TAG, "dumpInformation");

    size_t chipSize = spi_flash_get_chip_size();
    ESP_LOGD(TAG, "flash chip size = %d", chipSize);

    ESP_LOGD(TAG, "Reading flash at 0x00000000....");
    result = spi_flash_read(0, buf, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0, buf);

    ESP_LOGD(TAG, "Reading flash at 0x00001000....");
    result = spi_flash_read(0x1000, buf, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0x1000, buf);

    ESP_LOGD(TAG, "Reading flash at 0x00004000....");
    result = spi_flash_read(0x4000, buf, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0x4000, buf);

    ESP_LOGD(TAG, "Reading flash at 0x0000D000....");
    result = spi_flash_read(0xD000, buf, 4096);
    dump128bytes(0xD000, buf);

    result = spi_flash_read(0xE000, buf, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0xE000, buf);

    ESP_LOGD(TAG, "Reading flash at 0x00010000....");
    result = spi_flash_read(0x10000, buf, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0x10000, buf);

    ESP_LOGD(TAG, "Reading flash at 0x00020000....");
    result = spi_flash_read(0x20000, buf2, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0x20000, buf2);

    ESP_LOGD(TAG, "Reading flash at 0x00110000....");
    result = spi_flash_read(0x110000, buf2, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0x110000, buf2);

    ESP_LOGD(TAG, "Reading flash at 0x00210000....");
    result = spi_flash_read(0x210000, buf2, 4096);
    ESP_LOGD(TAG, "Result = %d", result);
    dump128bytes(0x210000, buf2);
}


const esp_partition_t* ota_update::findNextBootPartition()
{
    // Factory -> OTA_0
    // OTA_0   -> OTA_1
    // OTA_1   -> OTA_0

    const esp_partition_t *currentBootPartition = esp_ota_get_boot_partition();
    const esp_partition_t *nextBootPartition = NULL;

    if (!strcmp("factory", currentBootPartition->label)) {
        nextBootPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_0");
    }

    if (!strcmp("ota_0", currentBootPartition->label)) {
        nextBootPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_1");
    }

    if (!strcmp("ota_1", currentBootPartition->label)) {
        nextBootPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_0");
    }

    if (nextBootPartition) {
        ESP_LOGD(TAG, "Found next boot partition: %02x %02x 0x%08x %s",
                 nextBootPartition->type, nextBootPartition->subtype, nextBootPartition->address, nextBootPartition->label);
    } else {
        ESP_LOGD(TAG, "Failed to determine next boot partition from current boot partition: %s",
                 currentBootPartition ? currentBootPartition->label : "NULL");
    }

    return nextBootPartition;
}

void ota_update::dump128bytes(uint32_t addr, uint8_t *p)
{
    for (int i = 0; i < 8; i++) {
        uint32_t addr2 = addr + 16 * i;
        dump16bytes(addr2, &p[16*i]);
    }
}

void ota_update::dump16bytes(uint32_t addr, uint8_t *p)
{
    ESP_LOGI("ota_update", "%08X : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             addr, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
}
