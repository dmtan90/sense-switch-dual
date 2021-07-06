#ifndef __cse7761_h__
#define __cse7761_h__

#include "config.h"

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>
}

#define ENERGY_MAX_PHASES      3

typedef struct {
  uint32_t frequency = 0;
  uint32_t voltage_rms = 0;
  uint32_t current_rms[2] = { 0 };
  uint32_t energy[2] = { 0 };
  uint32_t active_power[2] = { 0 };
  uint16_t coefficient[8] = { 0 };
  uint8_t energy_update = 0;
  uint8_t init = 4;
  uint8_t ready = 0;
} CSE7761Data;

typedef struct {
  float voltage[ENERGY_MAX_PHASES];             // 123.1 V
  float current[ENERGY_MAX_PHASES];             // 123.123 A
  float active_power[ENERGY_MAX_PHASES];        // 123.1 W
  float apparent_power[ENERGY_MAX_PHASES];      // 123.1 VA
  float reactive_power[ENERGY_MAX_PHASES];      // 123.1 VAr
  float power_factor[ENERGY_MAX_PHASES];        // 0.12
  float frequency[ENERGY_MAX_PHASES];           // 123.1 Hz
  float export_active[ENERGY_MAX_PHASES];       // 123.123 kWh

  float start_energy;                           // 12345.12345 kWh total previous
  float daily;                                  // 123.123 kWh
  float total;                                  // 12345.12345 kWh total energy

  unsigned long kWhtoday_delta;                 // 1212312345 Wh 10^-5 (deca micro Watt hours) - Overflows to Energy.kWhtoday (HLW and CSE only)
  unsigned long kWhtoday_offset;                // 12312312 Wh * 10^-2 (deca milli Watt hours) - 5764 = 0.05764 kWh = 0.058 kWh = Energy.daily
  unsigned long kWhtoday;                       // 12312312 Wh * 10^-2 (deca milli Watt hours) - 5764 = 0.05764 kWh = 0.058 kWh = Energy.daily
  unsigned long period;                         // 12312312 Wh * 10^-2 (deca milli Watt hours) - 5764 = 0.05764 kWh = 0.058 kWh = Energy.daily

  uint8_t fifth_second;
  uint8_t command_code;
  uint8_t data_valid[ENERGY_MAX_PHASES];

  uint8_t phase_count;                          // Number of phases active
  bool voltage_common;                          // Use single voltage
  bool frequency_common;                        // Use single frequency
  bool use_overtemp;                            // Use global temperature as overtemp trigger on internal energy monitor hardware
  bool kWhtoday_offset_init;

  bool voltage_available;                       // Enable if voltage is measured
  bool current_available;                       // Enable if current is measured

  bool type_dc;
  bool power_on;
} Energy;

typedef struct {
  unsigned long energy_power_calibration;    // 364
  unsigned long energy_voltage_calibration;  // 368
  unsigned long energy_current_calibration;  // 36C
  unsigned long energy_frequency_calibration;  // 7C8  Also used by HX711 to save last weight
} Settings;

#define ENERGY_UPDATE_INTERVAL_IN_MILISECONDS       200

class Cse7761
{
public:
    Cse7761();
    ~Cse7761();
    void init();
    bool Cse7761Write(const uint32_t reg, const uint32_t data, const bool response = false);
    bool Cse7761ReadOnce(const uint32_t reg, uint32_t size, uint32_t* value);
    uint32_t Cse7761Read(const uint32_t reg, uint32_t size);
    uint32_t Cse7761ReadFallback(uint32_t reg, uint32_t prev, uint32_t size);
    uint32_t Cse7761Ref(uint32_t unit);
    bool Cse7761ChipInit(void);
    void Cse7761GetData(void);
    void Cse7761Every200ms(void);
    void Cse7761EverySecond(void);
    static void updateEnergyTask(void *data);
    //bool Cse7761Command(void);
    static Cse7761* getInstance();
    Energy mEnergy;
    Settings mSettings;
    uint8_t mUpdateCounter;
protected:
    bool mBusy = false;
    static const char mTag[];
private:
    CSE7761Data mData;
    static Cse7761* mInstance;
};

#endif //__cse7761_h__