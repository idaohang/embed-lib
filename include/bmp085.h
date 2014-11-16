/*
 * Filename: bmp085.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for BMP085 device class
 */

#ifndef EMBED_BMP085_H
#define EMBED_BMP085_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <map>

#include "i2c.h"
#include "gpio.h"

namespace embed
{

class BMP085
{
 public:
    // Oversampling setting enum
    typedef enum OSSR_SETTING_ENUM
    {
      OSSR_LOW_POWER = 0,
      OSSR_STANDARD,
      OSSR_HIGH_RES,
      OSSR_ULTRA_HIGH_RES,
      OSSR_NUM
    } OSSR_SETTING;

    // Interrupt callback
    typedef void (*EOCIntHandler) (const int16_t _temp, const int32_t _pressure, void* _data);

    BMP085 (I2C* _bus, GPIO* _eoc = NULL, GPIO* _xclr = NULL);
    ~BMP085 ();

    // Initialize
    bool init (bool _async);
    void destroy();

    // Set and get functions for OSSR setting
    OSSR_SETTING getOSSR () {return m_ossr;}
    void setOSSR (OSSR_SETTING _ossr) {m_ossr = _ossr;}

    // Synchronous poll reads
    int16_t readRawTempSync ();
    int32_t readRawPressureSync ();

    // Register for asynchronous reads
    void registerListener (EOCIntHandler _handler, void* _data);
    void unregisterListener (EOCIntHandler);

    // Helper functions
    void calcTempPressure (const int16_t _rawTemp, const int32_t _rawPressure,
                           double* _tempC, double* _pressurehPa);
    void calcApproxAlt (double _pressurehPa, double* _absAltM);
 private:
    // Device parameters
    static const uint8_t ADDRESS        = 0x77;
    static const uint8_t REG_WIDTH      = 1;

    // Device registers
    static const uint8_t AC1_MSB_REG    = 0xAA;
    static const uint8_t AC1_LSB_REG    = 0xAB;
    static const uint8_t AC2_MSB_REG    = 0xAC;
    static const uint8_t AC2_LSB_REG    = 0xAD;
    static const uint8_t AC3_MSB_REG    = 0xAE;
    static const uint8_t AC3_LSB_REG    = 0xAF;
    static const uint8_t AC4_MSB_REG    = 0xB0;
    static const uint8_t AC4_LSB_REG    = 0xB1;
    static const uint8_t AC5_MSB_REG    = 0xB2;
    static const uint8_t AC5_LSB_REG    = 0xB3;
    static const uint8_t AC6_MSB_REG    = 0xB4;
    static const uint8_t AC6_LSB_REG    = 0xB5;
    static const uint8_t B1_MSB_REG     = 0xB6;
    static const uint8_t B1_LSB_REG     = 0xB7;
    static const uint8_t B2_MSB_REG     = 0xB8;
    static const uint8_t B2_LSB_REG     = 0xB9;
    static const uint8_t MB_MSB_REG     = 0xBA;
    static const uint8_t MB_LSB_REG     = 0xBB;
    static const uint8_t MC_MSB_REG     = 0xBC;
    static const uint8_t MC_LSB_REG     = 0xBD;
    static const uint8_t MD_MSB_REG     = 0xBE;
    static const uint8_t MD_LSB_REG     = 0xBF;

    static const uint8_t CTRL_REG       = 0xF4;
    static const uint8_t TEMPERATURE    = 0x2E;
    static const uint8_t PRESSURE_OSRS0 = 0x34;
    static const uint8_t PRESSURE_OSRS1 = 0x74;
    static const uint8_t PRESSURE_OSRS2 = 0xB4;
    static const uint8_t PRESSURE_OSRS3 = 0xF4;

    static const uint8_t VALUE_MSB_REG  = 0xF6;
    static const uint8_t VALUE_LSB_REG  = 0xF7;
    static const uint8_t VALUE_XLSB_REG = 0xF8;

    // Pressure at sea level
    static const double PRESSURE_SEA_LEVEL_HPA;

    // Array to convert oversampling setting to conversion time
    static const double  OSSR_CONVERSION_TIME[OSSR_NUM];

    typedef enum ASYNC_STATE_ENUM
    {
      WAIT_TEMP_CONVERSION = 0,
      WAIT_PRESSURE_CONVERSION,
      ASYNC_STATE_NUM
    } ASYNC_STATE;

    // Whether device parameters are initialized
    bool                            m_initialized;

    // Device parameters read from EEPROM
    int16_t                         m_AC1;
    int16_t                         m_AC2;
    int16_t                         m_AC3;
    uint16_t                        m_AC4;
    uint16_t                        m_AC5;
    uint16_t                        m_AC6;
    int16_t                         m_B1;
    int16_t                         m_B2;
    int16_t                         m_MB;
    int16_t                         m_MC;
    int16_t                         m_MD;

    // I2C bus pointer
    I2C*                            m_bus;

    // OSSR setting
    OSSR_SETTING                    m_ossr;

    // State for asynchronous state machine
    ASYNC_STATE                     m_state;

    // Whether we are in async mode
    bool                            m_async;

    // Saved temp value across interrupts for async
    int16_t                         m_rawTempAsync;

    // GPIOs
    GPIO*                           m_eoc;
    GPIO*                           m_xclr;

    // Async listeners
    std::map<EOCIntHandler,void*>   m_listeners;

    // Interrupt handler from GPIO
    static void eocIntHandler (void* _data);

    // Private helper functions
    uint8_t readReg (const uint8_t _reg);
    void writeReg (const uint8_t _reg, const uint8_t _val);
};

}

#endif
