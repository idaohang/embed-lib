/*
 * Filename: bbb_i2c.cpp
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Implementation file for I2C Beaglebone Black library
 */

#include "bbb_i2c.h"

using namespace embed;

BBBI2C::BBBI2C (const uint8_t _bus) :
    m_bus (_bus),
    m_handle (-1)
{
}

BBBI2C::~BBBI2C ()
{
}

bool BBBI2C::init ()
{
    char filename[16];
    sprintf(filename, "/dev/i2c-%d", m_bus);
    if ((m_handle = open(filename, O_RDWR)) < 0)
    {
        fprintf(stderr, "BBBI2C::init open error: %s\n", strerror(errno));
        destroy();
        return false;
    }

    return true;
}

uint8_t BBBI2C::readReg (const uint8_t _addr, const uint8_t _reg)
{
    if (m_handle == -1)
        return 0;

    if (ioctl(m_handle, I2C_SLAVE, _addr) < 0)
    {
        fprintf(stderr, "BBBI2C::readReg ioctl error: %s\n", strerror(errno));
        return 0;
    }

    uint8_t val = _reg;
    if (write(m_handle, &val, 1) != 1)
    {
        fprintf(stderr, "BBBI2C::readReg write error: %s\n", strerror(errno));
        return 0;
    }

    if (read(m_handle, &val, 1) != 1)
    {
        fprintf(stderr, "BBBI2C::readReg read error: %s\n", strerror(errno));
        return 0;
    }

    return val;
}

void BBBI2C::writeReg (const uint8_t _addr, const uint8_t _reg, const uint8_t _val)
{
    if (m_handle == -1)
        return;

    if (ioctl(m_handle, I2C_SLAVE, _addr) < 0)
    {
        fprintf(stderr, "BBBI2C::writeReg ioctl error: %s\n", strerror(errno));
        return;
    }

    uint8_t buf[2];
    buf[0] = _reg;
    buf[1] = _val;

    if (write(m_handle, &buf, 2) != 2)
        fprintf(stderr, "BBBI2C::writeReg write error: %s\n", strerror(errno));
}

void BBBI2C::destroy ()
{
    close(m_handle);
    m_handle = -1;
}
