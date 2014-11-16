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

BMP085::BMP085 (I2C* _bus, GPIO* _eoc, GPIO* _xclr) :
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
    m_eoc (_eoc),
    m_xclr (_xclr),
    m_listeners()
{
}

BMP085::~BMP085 ()
{
    m_listeners.clear();
}

bool BMP085::init (bool _async)
{
    if (m_initialized)
        return true;

    if (_async && m_eoc == NULL)
    {
        fprintf(stderr, "BMP085::init called specifying async without a valid EOC GPIO\n");
        return false;
    }

    // If XCLR pin is connected, need to set it to high (it is active low)
    if (m_xclr != NULL)
        m_xclr->digitalWrite(1);

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

    if (_async)
    {
        m_async = true;
        // Set the initial state
        m_state = WAIT_TEMP_CONVERSION;
        // Setup interrupt on EOC pin
        m_eoc->attachInterrupt(eocIntHandler, GPIO::RISING, this);
        // Kick off first temperature reading
        writeReg (CTRL_REG, TEMPERATURE);
    }

    m_initialized = true;

    return true;
}

void BMP085::destroy ()
{
    if (!m_initialized)
        return;

    // If async mode, need to detach interrupt
    if (m_async)
    {
        m_eoc->detachInterrupt(eocIntHandler);
        m_async = false;
    }

    if (m_xclr != NULL)
        m_xclr->digitalWrite(0);

    m_initialized = false;
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

void BMP085::registerListener (EOCIntHandler _handler, void* _data)
{
    m_listeners[_handler] = _data;
}

void BMP085::unregisterListener (EOCIntHandler _handler)
{
    std::map<EOCIntHandler,void*>::iterator it;
    for (it = m_listeners.begin(); it != m_listeners.end(); it++)
    {
        if (it->first == _handler)
            break;
    }
    if (it != m_listeners.end())
        m_listeners.erase(it);
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

void BMP085::eocIntHandler (void * _data)
{
    BMP085* _this = static_cast<BMP085*>(_data);

    switch (_this->m_state)
    {
        case WAIT_TEMP_CONVERSION:
        {
            // Read temperature
            _this->m_rawTempAsync = ((_this->readReg (VALUE_MSB_REG) << 8) | _this->readReg (VALUE_LSB_REG));

            // Start a pressure reading
            _this->writeReg (CTRL_REG, PRESSURE_OSRS0 | (_this->m_ossr << 6));

            // Transition to waiting for pressure conversion state
            _this->m_state = WAIT_PRESSURE_CONVERSION;
            break;
        }
        case WAIT_PRESSURE_CONVERSION:
        {
            // Read pressure
            int32_t pressure = (((_this->readReg (VALUE_MSB_REG) << 16) |
                                 (_this->readReg (VALUE_LSB_REG) << 8)  |
                                 _this->readReg (VALUE_XLSB_REG)) >>
                                (8 - _this->m_ossr));

            // Notify listeners
            std::map<EOCIntHandler,void*>::iterator it;
            for (it = _this->m_listeners.begin(); it != _this->m_listeners.end(); it++)
                it->first (_this->m_rawTempAsync, pressure, it->second);

            // start another temperature reading
            _this->writeReg (CTRL_REG, TEMPERATURE);

            // Transition back to waiting for temperature conversion
            _this->m_state = WAIT_TEMP_CONVERSION;
            break;
        }
        default:
        {
            fprintf (stderr, "BMP085::eocIntHandler invalid state, returning to safe state\n");
            // start a temperature reading
            _this->writeReg (CTRL_REG, TEMPERATURE);
            _this->m_state = WAIT_TEMP_CONVERSION;
            break;
        }
    }
}

uint8_t BMP085::readReg (const uint8_t _reg)
{
    return m_bus->readReg(ADDRESS, _reg);
}

void BMP085::writeReg (const uint8_t _reg, const uint8_t _val)
{
    m_bus->writeReg(ADDRESS, _reg, _val);
}
