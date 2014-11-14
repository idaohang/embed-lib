/*
 * Filename: bmp085.cpp
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Implementation file for BMP085 device class
 */

#include "bmp085.h"

using namespace embed;

const double BMP085::OSSR_CONVERSION_TIME[OSSR_NUM] = {4.5, 7.5, 13.5, 25.5};
const double BMP085::PRESSURE_SEA_LEVEL_HPA = 1013.25;

BMP085::BMP085 (I2C* _bus) :
    m_initialized (false),
    m_AC1 (0),
    m_AC2 (0),
    m_AC3 (0),
    m_AC4 (0),
    m_AC5 (0),
    m_AC6 (0),
    m_B1 (0),
    m_B2 (0),
    m_MB (0),
    m_MC (0),
    m_MD (0),
    m_bus (_bus),
    m_ossr (OSSR_STANDARD),
    m_state (WAIT_TEMP_CONVERSION),
    m_async (false),
    m_rawTempAsync (0),
    m_tempCB (NULL),
    m_pressureCB (NULL),
    m_altitudeCB (NULL)
{
}

BMP085::~BMP085 ()
{
}

void BMP085::registerTemperatureCallback (TemperatureCallback _cb)
{
    m_tempCB = _cb;
}

void BMP085::registerPressureCallback (PressureCallback _cb)
{
    m_pressureCB = _cb;
}

void BMP085::registerAltitudeCallback (AltitudeCallback _cb)
{
    m_altitudeCB = _cb;
}

void BMP085::init ()
{
    if (m_initialized)
        return;

    // Read device params from EEPROM
    m_AC1 = ((readReg (AC1_MSB_REG) << 8) | readReg (AC1_LSB_REG));
    m_AC2 = ((readReg (AC2_MSB_REG) << 8) | readReg (AC2_LSB_REG));
    m_AC3 = ((readReg (AC3_MSB_REG) << 8) | readReg (AC3_LSB_REG));
    m_AC4 = ((readReg (AC4_MSB_REG) << 8) | readReg (AC4_LSB_REG));
    m_AC5 = ((readReg (AC5_MSB_REG) << 8) | readReg (AC5_LSB_REG));
    m_AC6 = ((readReg (AC6_MSB_REG) << 8) | readReg (AC6_LSB_REG));
    m_B1 = ((readReg (B1_MSB_REG) << 8) | readReg (B1_LSB_REG));
    m_B2 = ((readReg (B2_MSB_REG) << 8) | readReg (B2_LSB_REG));
    m_B1 = ((readReg (B1_MSB_REG) << 8) | readReg (B1_LSB_REG));
    m_MB = ((readReg (MB_MSB_REG) << 8) | readReg (MB_LSB_REG));
    m_MC = ((readReg (MC_MSB_REG) << 8) | readReg (MC_LSB_REG));
    m_MD = ((readReg (MD_MSB_REG) << 8) | readReg (MD_LSB_REG));

    m_initialized = true;
}

// _eocISR should just call BMP085::eocISR
void BMP085::initAsync (int _eocPin, ISRFunc _eocIsr)
{
    m_async = true;

    // Perform regular initialization
    init ();

    //TODO: Configure interrupt
    //pinMode (_eocPin, INPUT);
    //attachInterrupt (_eocPin, _eocIsr, RISING);

    // Set the initial state
    m_state = WAIT_TEMP_CONVERSION;

    // Initialize temperature reading
    writeReg (CTRL_REG, TEMPERATURE);
}

void BMP085::eocISR ()
{
    switch (m_state)
    {
        case WAIT_TEMP_CONVERSION:
        {
            // Read temperature
            m_rawTempAsync = ((readReg (VALUE_MSB_REG) << 8) | readReg (VALUE_LSB_REG));

            // Start a pressure reading
            writeReg (CTRL_REG, PRESSURE_OSRS0 | (m_ossr << 6));

            // Transition to waiting for pressure conversion state
            m_state = WAIT_PRESSURE_CONVERSION;
            break;
        }
        case WAIT_PRESSURE_CONVERSION:
        {
            // Read pressure
            int32_t pressure = (((readReg (VALUE_MSB_REG) << 16) | (readReg (VALUE_LSB_REG) << 8) | readReg (VALUE_XLSB_REG)) >> (8 - m_ossr));

            // Calculate true temperature
            int32_t X1 = (((int32_t) m_rawTempAsync - (int32_t) m_AC6) * (int32_t) m_AC5) >> 15;
            int32_t X2 = ((int32_t) m_MC << 11) / (X1 + m_MD);
            int32_t B5 = X1 + X2;
            int32_t T = (B5 + 8) >> 4;
            double tempC = T * 0.1;
            double tempF = (tempC * 9 / 5) + 32;

            // Calculate true pressure
            int32_t B6 = B5 - 4000;
            X1 = (m_B2 * (B6 * B6 >> 12)) >> 11;
            X2 = (m_AC2 * B6) >> 11;
            int32_t X3 = X1 + X2;
            int32_t B3 = (((((int32_t) m_AC1) * 4 + X3) << m_ossr) + 2) >> 2;
            X1 = (m_AC3 * B6) >> 13;
            X2 = (m_B1 * ((B6 * B6) >> 12)) >> 16;
            X3 = ((X1 + X2) + 2) >> 2;
            uint32_t B4 = (m_AC4 * (uint32_t)(X3 + 32768)) >> 15;
            uint32_t B7 = ((uint32_t)(pressure - B3) * (50000 >> m_ossr));
            int32_t p;
            if (B7 < 0x80000000)
                p = (B7 << 1) / B4;
            else
                p = (B7 / B4) << 1;
            X1 = (p >> 8) * (p >> 8);
            X1 = (X1 * 3038) >> 16;
            X2 = (-7357 * p) >> 16;
            p = p + ((X1 + X2 + 3791) >> 4);

            // Convert from Pa to hPa
            double pressurehPa = ((double) p) / 100.0;

            // Calculate altitude
            double altitudeM = 44330.0 * (1.0 - pow (pressurehPa / PRESSURE_SEA_LEVEL_HPA, 1 / 5.255));
            double altitudeF = altitudeM * 3.2808;

            // Make callbacks
            if (m_tempCB)
                m_tempCB (m_rawTempAsync, tempC, tempF);
            if (m_pressureCB)
                m_pressureCB (pressure, pressurehPa);
            if (m_altitudeCB)
                m_altitudeCB (altitudeM, altitudeF);

            // start another temperature reading
            writeReg (CTRL_REG, TEMPERATURE);

            // Transition back to waiting for temperature conversion
            m_state = WAIT_TEMP_CONVERSION;
            break;
        }
        default:
        {
            fprintf (stderr, "BMP085::eocISR invalid state, returning to safe state\n");

            // start a temperature reading
            writeReg (CTRL_REG, TEMPERATURE);

            m_state = WAIT_TEMP_CONVERSION;
            break;
        }
    }
}

int16_t BMP085::readRawTempSync ()
{
    if (m_async)
        return 0;

    writeReg (CTRL_REG, TEMPERATURE);

    usleep (4500);

    return ((readReg (VALUE_MSB_REG) << 8) | readReg (VALUE_LSB_REG));
}

int32_t BMP085::readRawPressureSync ()
{
    if (m_async)
        return 0;

    writeReg (CTRL_REG, PRESSURE_OSRS0 | (m_ossr << 6));

    usleep (OSSR_CONVERSION_TIME[m_ossr] * 1000.0);

    return (((readReg (VALUE_MSB_REG) << 16) | (readReg (VALUE_LSB_REG) << 8) | readReg (VALUE_XLSB_REG)) >> (8 - m_ossr));
}

void BMP085::calcTempPressure (const int16_t _rawTemp, const int32_t _rawPressure,
                               double* _tempC, double* _pressurehPa)
{
    int32_t X1 = (((int32_t) _rawTemp - (int32_t) m_AC6) * (int32_t) m_AC5) >> 15;
    int32_t X2 = ((int32_t) m_MC << 11) / (X1 + m_MD);
    int32_t B5 = X1 + X2;
    int32_t T = (B5 + 8) >> 4;
    (*_tempC) = T * 0.1;

    int32_t B6 = B5 - 4000;
    X1 = (m_B2 * (B6 * B6 >> 12)) >> 11;
    X2 = (m_AC2 * B6) >> 11;
    int32_t X3 = X1 + X2;
    int32_t B3 = (((((int32_t) m_AC1) * 4 + X3) << m_ossr) + 2) >> 2;
    X1 = (m_AC3 * B6) >> 13;
    X2 = (m_B1 * ((B6 * B6) >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    uint32_t B4 = (m_AC4 * (uint32_t)(X3 + 32768)) >> 15;
    uint32_t B7 = ((uint32_t)(_rawPressure - B3) * (50000 >> m_ossr));
    int32_t p;
    if (B7 < 0x80000000)
        p = (B7 << 1) / B4;
    else
        p = (B7 / B4) << 1;
    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    p = p + ((X1 + X2 + 3791) >> 4);

    // Convert from Pa to hPa
    (*_pressurehPa) = ((double) p) / 100.0;
}

void BMP085::calcApproxAlt (double _pressurehPa, double* _absAltM)
{
    (*_absAltM) = 44330.0 * (1.0 - pow (_pressurehPa / PRESSURE_SEA_LEVEL_HPA, 1 / 5.255));
}

uint8_t BMP085::readReg (const uint8_t _reg)
{
    return m_bus->readReg(ADDRESS, _reg);
}

void BMP085::writeReg (const uint8_t _reg, const uint8_t _val)
{
    m_bus->writeReg(ADDRESS, _reg, _val);
}
