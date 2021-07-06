
#include "cse7761.h"
#include "sys_ctrl.h"
#include "uart_ctrl.h"

//#define Cse7761_SIMULATE                           // Enable simulation of Cse7761
#define Cse7761_FREQUENCY                          // Add support for frequency monitoring
//  #define Cse7761_ZEROCROSS                        // Add zero cross detection
//    #define Cse7761_ZEROCROSS_OFFSET  2200         // Zero cross offset due to chip calculation (microseconds)
//    #define Cse7761_RELAY_SWITCHTIME  3950         // Relay (Golden GI-1A-5LH 15ms max) switch power on time (microseconds)

#define Cse7761_UREF                  42563        // RmsUc
#define Cse7761_IREF                  52241        // RmsIAC
#define Cse7761_PREF                  44513        // PowerPAC
#define Cse7761_FREF                  3579545      // System clock (3.579545MHz) as used in frequency calculation

#define Cse7761_REG_SYSCON            0x00         // (2) System Control Register (0x0A04)
#define Cse7761_REG_EMUCON            0x01         // (2) Metering control register (0x0000)
#define Cse7761_REG_EMUCON2           0x13         // (2) Metering control register 2 (0x0001)
#define Cse7761_REG_PULSE1SEL         0x1D         // (2) Pin function output select register (0x3210)

#define Cse7761_REG_UFREQ             0x23         // (2) Voltage Frequency (0x0000)
#define Cse7761_REG_RMSIA             0x24         // (3) The effective value of channel A current (0x000000)
#define Cse7761_REG_RMSIB             0x25         // (3) The effective value of channel B current (0x000000)
#define Cse7761_REG_RMSU              0x26         // (3) Voltage RMS (0x000000)
#define Cse7761_REG_POWERFACTOR       0x27         // (3) Power factor register, select by command: channel A Power factor or channel B power factor (0x7FFFFF)
#define Cse7761_REG_POWERPA           0x2C         // (4) Channel A active power, update rate 27.2Hz (0x00000000)
#define Cse7761_REG_POWERPB           0x2D         // (4) Channel B active power, update rate 27.2Hz (0x00000000)
#define Cse7761_REG_SYSSTATUS         0x43         // (1) System status register

#define Cse7761_REG_COEFFOFFSET       0x6E         // (2) Coefficient checksum offset (0xFFFF)
#define Cse7761_REG_COEFFCHKSUM       0x6F         // (2) Coefficient checksum
#define Cse7761_REG_RMSIAC            0x70         // (2) Channel A effective current conversion coefficient
#define Cse7761_REG_RMSIBC            0x71         // (2) Channel B effective current conversion coefficient
#define Cse7761_REG_RMSUC             0x72         // (2) Effective voltage conversion coefficient
#define Cse7761_REG_POWERPAC          0x73         // (2) Channel A active power conversion coefficient
#define Cse7761_REG_POWERPBC          0x74         // (2) Channel B active power conversion coefficient
#define Cse7761_REG_POWERSC           0x75         // (2) Apparent power conversion coefficient
#define Cse7761_REG_ENERGYAC          0x76         // (2) Channel A energy conversion coefficient
#define Cse7761_REG_ENERGYBC          0x77         // (2) Channel B energy conversion coefficient

#define Cse7761_SPECIAL_COMMAND       0xEA         // Start special command
#define Cse7761_CMD_RESET             0x96         // Reset command, after receiving the command, the chip resets
#define Cse7761_CMD_CHAN_A_SELECT     0x5A         // Current channel A setting command, which specifies the current used to calculate apparent power,
                                                   //   Power factor, phase angle, instantaneous active power, instantaneous apparent power and
                                                   //   The channel indicated by the signal of power overload is channel A
#define Cse7761_CMD_CHAN_B_SELECT     0xA5         // Current channel B setting command, which specifies the current used to calculate apparent power,
                                                   //   Power factor, phase angle, instantaneous active power, instantaneous apparent power and
                                                   //   The channel indicated by the signal of power overload is channel B
#define Cse7761_CMD_CLOSE_WRITE       0xDC         // Close write operation
#define Cse7761_CMD_ENABLE_WRITE      0xE5         // Enable write operation

const uint32_t HLW_PREF_PULSE = 12530;      // was 4975us = 201Hz = 1000W
const uint32_t HLW_UREF_PULSE = 1950;       // was 1666us = 600Hz = 220V
const uint32_t HLW_IREF_PULSE = 3500;       // was 1666us = 600Hz = 4.545A

enum Cse7761Reg { RmsIAC, RmsIBC, RmsUC, PowerPAC, PowerPBC, PowerSC, EnergyAC, EnergyBC };

enum EnergyCommands {
  CMND_POWERCAL, CMND_VOLTAGECAL, CMND_CURRENTCAL, CMND_FREQUENCYCAL,
  CMND_POWERSET, CMND_VOLTAGESET, CMND_CURRENTSET, CMND_FREQUENCYSET };

const char Cse7761::mTag[]          = "Cse7761";
Cse7761* Cse7761::mInstance = NULL;

Cse7761::Cse7761()
{
    ESP_LOGD(mTag, "Cse7761");
    this->init();
}

Cse7761* Cse7761::getInstance()
{
    if(NULL == mInstance)
    {
        mInstance = new Cse7761();
    }

    return mInstance;
}


Cse7761::~Cse7761()
{
    ESP_LOGI(mTag, "~Cse7761");
    mInstance = NULL;
}

void Cse7761::init()
{
    // Software serial init needs to be done here as earlier (serial) interrupts may lead to Exceptions
    ESP_LOGD(mTag, "init");
    uart_ctrl *uart_ctrl = uart_ctrl::getInstance();

#ifdef Cse7761_FREQUENCY
#ifdef Cse7761_ZEROCROSS
    ZeroCrossInit(Cse7761_ZEROCROSS_OFFSET + Cse7761_RELAY_SWITCHTIME);
#endif  // Cse7761_ZEROCROSS
#endif  // Cse7761_FREQUENCY

    mData.ready = 0;
    mData.init = 4;                              // Init setup steps
    
    mEnergy.phase_count = 2;                     // Handle two channels as two phases
    mEnergy.voltage_common = true;               // Use common voltage
    mEnergy.voltage[0] = 0;
    mEnergy.voltage[1] = 0;
    mEnergy.current[0] = 0;
    mEnergy.current[1] = 0;
    mEnergy.active_power[0] = 0;
    mEnergy.active_power[1] = 0;
    mEnergy.total = 0;
    mSettings.energy_power_calibration = HLW_PREF_PULSE;

#ifdef Cse7761_FREQUENCY
    mEnergy.frequency_common = true;             // Use common frequency
#endif
    mEnergy.use_overtemp = true;                 // Use global temperature for overtemp detection
    mUpdateCounter = 0;

    //init timer update task
    xTaskCreatePinnedToCore(updateEnergyTask, "updateEnergyTask", 9096, NULL, tskIDLE_PRIORITY, NULL, 1);
}

void Cse7761::updateEnergyTask(void *data)
{
    while(mInstance != NULL && mInstance->mBusy == false)
    {
        mInstance->mBusy = true;
        if(mInstance->mUpdateCounter == 0)
        {
            mInstance->Cse7761EverySecond();
        }
        mInstance->Cse7761Every200ms();

        mInstance->mUpdateCounter++;
        if(mInstance->mUpdateCounter == 5)
        {
            mInstance->mUpdateCounter = 0;
        }
        delay_ms(ENERGY_UPDATE_INTERVAL_IN_MILISECONDS);
        mInstance->mBusy = false;
    }

    sys_ctrl::rebootSystem(0);
    vTaskDelete(NULL);
}

/********************************************************************************************/

bool Cse7761::Cse7761Write(const uint32_t reg, const uint32_t data, const bool response) {
    ESP_LOGD(mTag, "Cse7761Write: reg=0x%02X", reg);
    uint8_t buffer[5];
    buffer[0] = 0xA5;
    buffer[1] = reg;
    uint32_t len = 2;
    if (data) 
    {
        if (data < 0xFF) 
        {
            buffer[2] = data & 0xFF;
            len = 3;
        } 
        else 
        {
            buffer[2] = (data >> 8) & 0xFF;
            buffer[3] = data & 0xFF;
            len = 4;
        }
        uint8_t crc = 0;
        for (uint32_t i = 0; i < len; i++) 
        {
            crc += buffer[i];
        }
        buffer[len] = ~crc;
        len++;
    }

    uart_ctrl *uart_ctrl = uart_ctrl::getInstance();
    return uart_ctrl->sendData(&buffer[0], len, response);
}

bool Cse7761::Cse7761ReadOnce(const uint32_t reg, uint32_t size, uint32_t* value) {
    Cse7761Write(reg, 0, true);

    uart_ctrl *uart_ctrl = uart_ctrl::getInstance();
    uint16_t len = 0;
    uint8_t* buffer = uart_ctrl->readData(len);

    if (!len)
    {
        ESP_LOGD(mTag, "C61: Rx none");
        return false;
    }

    /*ESP_LOGD(mTag, "C61: Rx ===========");
    for(uint16_t i = 0; i < len; i++)
    {
        ESP_LOGD(mTag, "0x%02X", buffer[i]);
    }
    ESP_LOGD(mTag, "===========");*/

    if (len > 5)
    {
        ESP_LOGD(mTag, "C61: Rx overflow");
        return false;
    }

    len--;
    uint32_t result = 0;
    uint8_t crc = 0xA5 + reg;
    for (uint32_t i = 0; i < len; i++) 
    {
        result = (result << 8) | buffer[i];
        crc += buffer[i];
    }
    crc = ~crc;
    if (crc != buffer[len]) 
    {
        ESP_LOGD(mTag, "C61: Len %d, Rx %02X, CRC error %02X", len + 1, buffer[len - 1], crc);
        return false;
    }

    *value = result;
    return true;
}

uint32_t Cse7761::Cse7761Read(const uint32_t reg, uint32_t size) {
    ESP_LOGD(mTag, "Cse7761Read: reg=0x%02X", reg);
    bool result = false;  // Start loop
    uint32_t retry = 3;   // Retry up to three times
    uint32_t value = 0;   // Default no value
    while (!result && retry) 
    {
        retry--;
        result = Cse7761ReadOnce(reg, size, &value);
    }
    return value;
}

uint32_t Cse7761::Cse7761ReadFallback(const uint32_t reg, uint32_t prev, uint32_t size) {
    uint32_t value = Cse7761Read(reg, size);
    if (!value) 
    {    
        // Error so use previous value read
        value = prev;
    }
    return value;
}

/********************************************************************************************/

uint32_t Cse7761::Cse7761Ref(uint32_t unit) {
    switch (unit) 
    {
        case RmsUC: return 0x400000 * 100 / mData.coefficient[RmsUC];
        case RmsIAC: return (0x800000 * 100 / mData.coefficient[RmsIAC]) * 10;  // Stay within 32 bits
        case PowerPAC: return 0x80000000 / mData.coefficient[PowerPAC];
    }
    return 0;
}

bool Cse7761::Cse7761ChipInit(void) {
    uint16_t calc_chksum = 0xFFFF;
    for (uint32_t i = 0; i < 8; i++) 
    {
        mData.coefficient[i] = Cse7761Read(Cse7761_REG_RMSIAC + i, 2);
        calc_chksum += mData.coefficient[i];
    }
    calc_chksum = ~calc_chksum;
    //  uint16_t dummy = Cse7761Read(Cse7761_REG_COEFFOFFSET, 2);
    uint16_t coeff_chksum = Cse7761Read(Cse7761_REG_COEFFCHKSUM, 2);
    if ((calc_chksum != coeff_chksum) || (!calc_chksum)) 
    {
        ESP_LOGD(mTag, "C61: Default calibration");
        mData.coefficient[RmsIAC] = Cse7761_IREF;
        //    mData.coefficient[RmsIBC] = 0xCC05;
        mData.coefficient[RmsUC] = Cse7761_UREF;
        mData.coefficient[PowerPAC] = Cse7761_PREF;
        //    mData.coefficient[PowerPBC] = 0xADD7;
    }
    if (HLW_PREF_PULSE == mSettings.energy_power_calibration) 
    {
        mSettings.energy_frequency_calibration = Cse7761_FREF;
        mSettings.energy_voltage_calibration = Cse7761Ref(RmsUC);
        mSettings.energy_current_calibration = Cse7761Ref(RmsIAC);
        mSettings.energy_power_calibration = Cse7761Ref(PowerPAC);
    }
    // Just to fix intermediate users
    if (mSettings.energy_frequency_calibration < Cse7761_FREF / 2) 
    {
        mSettings.energy_frequency_calibration = Cse7761_FREF;
    }

    Cse7761Write(Cse7761_SPECIAL_COMMAND, Cse7761_CMD_ENABLE_WRITE);

    uint8_t sys_status = Cse7761Read(Cse7761_REG_SYSSTATUS, 1);
#ifdef Cse7761_SIMULATE
    sys_status = 0x11;
#endif
    if (sys_status & 0x10) 
    {       // Write enable to protected registers (WREN)
/*
    System Control Register (SYSCON)  Addr:0x00  Default value: 0x0A04
    Bit    name               Function description
    15-11  NC                 -, the default is 1
    10     ADC2ON
                              =1, means ADC current channel B is on (Sonoff Dual R3 Pow)
                              =0, means ADC current channel B is closed
    9      NC                 -, the default is 1.
    8-6    PGAIB[2:0]         Current channel B analog gain selection highest bit
                              =1XX, PGA of current channel B=16 (Sonoff Dual R3 Pow)
                              =011, PGA of current channel B=8
                              =010, PGA of current channel B=4
                              =001, PGA of current channel B=2
                              =000, PGA of current channel B=1
    5-3    PGAU[2:0]          Highest bit of voltage channel analog gain selection
                              =1XX, PGA of voltage U=16
                              =011, PGA of voltage U=8
                              =010, PGA of voltage U=4
                              =001, PGA of voltage U=2
                              =000, PGA of voltage U=1 (Sonoff Dual R3 Pow)
    2-0    PGAIA[2:0]         Current channel A analog gain selection highest bit
                              =1XX, PGA of current channel A=16 (Sonoff Dual R3 Pow)
                              =011, PGA of current channel A=8
                              =010, PGA of current channel A=4
                              =001, PGA of current channel A=2
                              =000, PGA of current channel A=1
*/
    Cse7761Write(Cse7761_REG_SYSCON | 0x80, 0xFF04);

/*
    Energy Measure Control Register (EMUCON)  Addr:0x01  Default value: 0x0000
    Bit    name               Function description
    15-14  Tsensor_Step[1:0]  Measurement steps of temperature sensor:
                              =2'b00 The first step of temperature sensor measurement, the Offset of OP1 and OP2 is +/+. (Sonoff Dual R3 Pow)
                              =2'b01 The second step of temperature sensor measurement, the Offset of OP1 and OP2 is +/-.
                              =2'b10 The third step of temperature sensor measurement, the Offset of OP1 and OP2 is -/+.
                              =2'b11 The fourth step of temperature sensor measurement, the Offset of OP1 and OP2 is -/-.
                              After measuring these four results and averaging, the AD value of the current measured temperature can be obtained.
    13     tensor_en          Temperature measurement module control
                              =0 when the temperature measurement module is closed; (Sonoff Dual R3 Pow)
                              =1 when the temperature measurement module is turned on;
    12     comp_off           Comparator module close signal:
                              =0 when the comparator module is in working state
                              =1 when the comparator module is off (Sonoff Dual R3 Pow)
    11-10  Pmode[1:0]         Selection of active energy calculation method:
                              Pmode =00, both positive and negative active energy participate in the accumulation,
                                the accumulation method is algebraic sum mode, the reverse REVQ symbol indicates to active power; (Sonoff Dual R3 Pow)
                              Pmode = 01, only accumulate positive active energy;
                              Pmode = 10, both positive and negative active energy participate in the accumulation,
                                and the accumulation method is absolute value method. No reverse active power indication;
                              Pmode =11, reserved, the mode is the same as Pmode =00
    9      NC                 -
    8      ZXD1               The initial value of ZX output is 0, and different waveforms are output according to the configuration of ZXD1 and ZXD0:
                              =0, it means that the ZX output changes only at the selected zero-crossing point (Sonoff Dual R3 Pow)
                              =1, indicating that the ZX output changes at both the positive and negative zero crossings
    7      ZXD0
                              =0, indicates that the positive zero-crossing point is selected as the zero-crossing detection signal (Sonoff Dual R3 Pow)
                              =1, indicating that the negative zero-crossing point is selected as the zero-crossing detection signal
    6      HPFIBOFF
                              =0, enable current channel B digital high-pass filter (Sonoff Dual R3 Pow)
                              =1, turn off the digital high-pass filter of current channel B
    5      HPFIAOFF
                              =0, enable current channel A digital high-pass filter (Sonoff Dual R3 Pow)
                              =1, turn off the digital high-pass filter of current channel A
    4      HPFUOFF
                              =0, enable U channel digital high pass filter (Sonoff Dual R3 Pow)
                              =1, turn off the U channel digital high-pass filter
    3-2    NC                 -
    1      PBRUN
                              =1, enable PFB pulse output and active energy register accumulation; (Sonoff Dual R3 Pow)
                              =0 (default), turn off PFB pulse output and active energy register accumulation.
    0      PARUN
                              =1, enable PFA pulse output and active energy register accumulation; (Sonoff Dual R3 Pow)
                              =0 (default), turn off PFA pulse output and active energy register accumulation.
*/
//    Cse7761Write(Cse7761_REG_EMUCON | 0x80, 0x1003);
    Cse7761Write(Cse7761_REG_EMUCON | 0x80, 0x1183);  // Tasmota enable zero cross detection on both positive and negative signal

/*
    Energy Measure Control Register (EMUCON2)  Addr: 0x13  Default value: 0x0001
    Bit    name               Function description
    15-13  NC                 -
    12     SDOCmos
                              =1, SDO pin CMOS open-drain output
                              =0, SDO pin CMOS output (Sonoff Dual R3 Pow)
    11     EPB_CB             Energy_PB clear signal control, the default is 0, and it needs to be configured to 1 in UART mode.
                                Clear after reading is not supported in UART mode
                              =1, Energy_PB will not be cleared after reading; (Sonoff Dual R3 Pow)
                              =0, Energy_PB is cleared after reading;
    10     EPA_CB             Energy_PA clear signal control, the default is 0, it needs to be configured to 1 in UART mode,
                                Clear after reading is not supported in UART mode
                              =1, Energy_PA will not be cleared after reading; (Sonoff Dual R3 Pow)
                              =0, Energy_PA is cleared after reading;
    9-8    DUPSEL[1:0]        Average register update frequency control
                              =00, Update frequency 3.4Hz
                              =01, Update frequency 6.8Hz
                              =10, Update frequency 13.65Hz
                              =11, Update frequency 27.3Hz (Sonoff Dual R3 Pow)
    7      CHS_IB             Current channel B measurement selection signal
                              =1, measure the current of channel B (Sonoff Dual R3 Pow)
                              =0, measure the internal temperature of the chip
    6      PfactorEN          Power factor function enable
                              =1, turn on the power factor output function (Sonoff Dual R3 Pow)
                              =0, turn off the power factor output function
    5      WaveEN             Waveform data, instantaneous data output enable signal
                              =1, turn on the waveform data output function (Tasmota add frequency)
                              =0, turn off the waveform data output function (Sonoff Dual R3 Pow)
    4      SAGEN              Voltage drop detection enable signal, WaveEN=1 must be configured first
                              =1, turn on the voltage drop detection function
                              =0, turn off the voltage drop detection function (Sonoff Dual R3 Pow)
    3      OverEN             Overvoltage, overcurrent, and overload detection enable signal, WaveEN=1 must be configured first
                              =1, turn on the overvoltage, overcurrent, and overload detection functions
                              =0, turn off the overvoltage, overcurrent, and overload detection functions (Sonoff Dual R3 Pow)
    2      ZxEN               Zero-crossing detection, phase angle, voltage frequency measurement enable signal
                              =1, turn on the zero-crossing detection, phase angle, and voltage frequency measurement functions (Tasmota add frequency)
                              =0, disable zero-crossing detection, phase angle, voltage frequency measurement functions (Sonoff Dual R3 Pow)
    1      PeakEN             Peak detect enable signal
                              =1, turn on the peak detection function
                              =0, turn off the peak detection function (Sonoff Dual R3 Pow)
    0      NC                 Default is 1
*/
#ifndef Cse7761_FREQUENCY
    Cse7761Write(Cse7761_REG_EMUCON2 | 0x80, 0x0FC1);  // Sonoff Dual R3 Pow
#else
    Cse7761Write(Cse7761_REG_EMUCON2 | 0x80, 0x0FE5);  // Tasmota add Frequency

#ifdef Cse7761_ZEROCROSS
/*
    Pin function output selection register (PULSE1SEL)  Addr: 0x1D  Default value: 0x3210
    Bit    name               Function description
    15-13  NC                 -
    12     SDOCmos
                              =1, SDO pin CMOS open-drain output

    15-12  NC                 NC, the default value is 4'b0011
    11-8   NC                 NC, the default value is 4'b0010
    7-4    P2Sel              Pulse2 Pin output function selection, see the table below
    3-0    P1Sel              Pulse1 Pin output function selection, see the table below

    Table Pulsex function output selection list
    Pxsel  Select description
    0000   Output of energy metering calibration pulse PFA
    0001   The output of the energy metering calibration pulse PFB
    0010   Comparator indication signal comp_sign
    0011   Interrupt signal IRQ output (the default is high level, if it is an interrupt, set to 0)
    0100   Signal indication of power overload: only PA or PB can be selected
    0101   Channel A negative power indicator signal
    0110   Channel B negative power indicator signal
    0111   Instantaneous value update interrupt output
    1000   Average update interrupt output
    1001   Voltage channel zero-crossing signal output (Tasmota add zero-cross detection)
    1010   Current channel A zero-crossing signal output
    1011   Current channel B zero crossing signal output
    1100   Voltage channel overvoltage indication signal output
    1101   Voltage channel undervoltage indication signal output
    1110   Current channel A overcurrent signal indication output
    1111   Current channel B overcurrent signal indication output
*/
    Cse7761Write(Cse7761_REG_PULSE1SEL | 0x80, 0x3290);
#endif  // Cse7761_ZEROCROSS
#endif  // Cse7761_FREQUENCY
    } 
    else 
    {
        ESP_LOGD(mTag, "C61: Write failed");
        return false;
    }
    return true;
}

void Cse7761::Cse7761GetData(void) {
    // The effective value of current and voltage Rms is a 24-bit signed number, the highest bit is 0 for valid data,
    //   and when the highest bit is 1, the reading will be processed as zero
    // The active power parameter PowerA/B is in two’s complement format, 32-bit data, the highest bit is Sign bit.
    uint32_t value = Cse7761ReadFallback(Cse7761_REG_RMSU, mData.voltage_rms, 3);
#ifdef Cse7761_SIMULATE
    value = 2342160;  // 237.7V
#endif
    mData.voltage_rms = (value >= 0x800000) ? 0 : value;

#ifdef Cse7761_FREQUENCY
    value = Cse7761ReadFallback(Cse7761_REG_UFREQ, mData.frequency, 2);
#ifdef Cse7761_SIMULATE
    value = 8948;  // 49.99Hz
#endif
    mData.frequency = (value >= 0x8000) ? 0 : value;
#endif  // Cse7761_FREQUENCY

    value = Cse7761ReadFallback(Cse7761_REG_RMSIA, mData.current_rms[0], 3);
#ifdef Cse7761_SIMULATE
    value = 455;
#endif
    mData.current_rms[0] = ((value >= 0x800000) || (value < 1600)) ? 0 : value;  // No load threshold of 10mA
    value = Cse7761ReadFallback(Cse7761_REG_POWERPA, mData.active_power[0], 4);
#ifdef Cse7761_SIMULATE
    value = 217;
#endif
    mData.active_power[0] = (0 == mData.current_rms[0]) ? 0 : (value & 0x80000000) ? (~value) + 1 : value;

    value = Cse7761ReadFallback(Cse7761_REG_RMSIB, mData.current_rms[1], 3);
#ifdef Cse7761_SIMULATE
    value = 29760;  // 0.185A
#endif
    mData.current_rms[1] = ((value >= 0x800000) || (value < 1600)) ? 0 : value;  // No load threshold of 10mA
    value = Cse7761ReadFallback(Cse7761_REG_POWERPB, mData.active_power[1], 4);
#ifdef Cse7761_SIMULATE
    value = 2126641;  // 44.05W
#endif
    mData.active_power[1] = (0 == mData.current_rms[1]) ? 0 : (value & 0x80000000) ? (~value) + 1 : value;

    ESP_LOGD(mTag, "C61 before processed: F: %d, U: %d, I: %d/%d, P: %d/%d",
        mData.frequency, mData.voltage_rms,
        mData.current_rms[0], mData.current_rms[1],
        mData.active_power[0], mData.active_power[1]);

    // float voltage = (float)(((uint64_t)mData.voltage_rms * mData.coefficient[RmsUC] * 10) >> 22) / 1000;  // V
    // float frequency = (mData.frequency) ? ((float)mSettings.energy_frequency_calibration / 8 / mData.frequency) : 0;  // Hz
    // float current1 = (float)(((uint64_t)mData.current_rms[0] * mData.coefficient[RmsIAC + 0]) >> 23) / 1000;  // A
    // float current2 = (float)(((uint64_t)mData.current_rms[1] * mData.coefficient[RmsIAC + 1]) >> 23) / 1000;  // A
    // float active_power1 = (float)(((uint64_t)mData.active_power[0] * mData.coefficient[PowerPAC + 0] * 1000) >> 31) / 1000;  // W
    // float active_power2 = (float)(((uint64_t)mData.active_power[1] * mData.coefficient[PowerPAC + 1] * 1000) >> 31) / 1000;  // W
    // ESP_LOGD(mTag, "C61 after processed: F: %.0f, U: %.0f, I: %.0f/%.0f, P: %.0f/%.0f",
    //     frequency, voltage,
    //     current1, current2,
    //     active_power1, active_power2);

    if (mEnergy.power_on) 
    {   
        // Powered on
        // Voltage = RmsU * RmsUC * 10 / 0x400000
        //mEnergy.voltage[0] = (float)(((uint64_t)mData.voltage_rms * mData.coefficient[RmsUC] * 10) >> 22) / 1000;  // V
        mEnergy.voltage[0] = ((float)mData.voltage_rms / mSettings.energy_voltage_calibration);  // V
#ifdef Cse7761_FREQUENCY
        mEnergy.frequency[0] = (mData.frequency) ? ((float)mSettings.energy_frequency_calibration / 8 / mData.frequency) : 0;  // Hz
#endif

        for (uint32_t channel = 0; channel < 2; channel++) 
        {
            mEnergy.data_valid[channel] = 0;
            // Active power = PowerPA * PowerPAC * 1000 / 0x80000000
            //mEnergy.active_power[channel] = (float)(((uint64_t)mData.active_power[channel] * mData.coefficient[PowerPAC + channel] * 1000) >> 31) / 1000;  // W
            mEnergy.active_power[channel] = (float)mData.active_power[channel] / mSettings.energy_power_calibration;  // W
            if (0 == mEnergy.active_power[channel]) 
            {
                mEnergy.current[channel] = 0;
            } 
            else 
            {
                // Current = RmsIA * RmsIAC / 0x800000
                //mEnergy.current[channel] = (float)(((uint64_t)mData.current_rms[channel] * mData.coefficient[RmsIAC + channel]) >> 23) / 1000;  // A
                mEnergy.current[channel] = (float)mData.current_rms[channel] / mSettings.energy_current_calibration;  // A
                mData.energy[channel] += mEnergy.active_power[channel];
                mData.energy_update++;
                mEnergy.total += mEnergy.active_power[channel];
            }
        }


        ESP_LOGD(mTag, "C61 after processed: F: %.0f, U: %.0f, I: %.0f/%.0f, P: %d/%d",
            mEnergy.frequency[0], mEnergy.voltage[0],
            mEnergy.current[0], mEnergy.current[1],
            mData.energy[0], mData.energy[1]);
    }
}

void Cse7761::Cse7761Every200ms(void)
{
    if (2 == mData.ready) 
    {
        Cse7761GetData();
    }
}

void Cse7761::Cse7761EverySecond(void) 
{
    if (mData.init) 
    {
        if (3 == mData.init) 
        {
            Cse7761Write(Cse7761_SPECIAL_COMMAND, Cse7761_CMD_RESET);
        }
        else if (2 == mData.init)
        {
            uint16_t syscon = Cse7761Read(0x00, 2);      // Default 0x0A04
#ifdef Cse7761_SIMULATE
            syscon = 0x0A04;
#endif
            if ((0x0A04 == syscon) && Cse7761ChipInit()) 
            {
                mData.ready = 1;
            }
        }
        else if (1 == mData.init) 
        {
            if (1 == mData.ready) 
            {
                Cse7761Write(Cse7761_SPECIAL_COMMAND, Cse7761_CMD_CLOSE_WRITE);
                ESP_LOGD(mTag, "C61: Cse7761 found");
                mData.ready = 2;
            }
        }
        mData.init--;
    }
    else 
    {
        if (2 == mData.ready) 
        {
            if (mData.energy_update) 
            {
                uint32_t energy_sum = ((mData.energy[0] + mData.energy[1]) * 1000) / mData.energy_update;
                if (energy_sum) 
                {
                    mEnergy.kWhtoday_delta += energy_sum / 36;
                    //EnergyUpdateToday();
                }
            }
            mData.energy[0] = 0;
            mData.energy[1] = 0;
            mData.energy_update = 0;
        }
    }
}
