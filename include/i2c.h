/*
 * Filename: i2c.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for I2C generic interface class
 */

#ifndef EMBED_I2C_H
#define EMBED_I2C_H

#include <stdint.h>

namespace embed
{

// I2C Interface Class
class I2C
{
 public:
    virtual ~I2C() {};

    virtual bool init() = 0;
    virtual void destroy() = 0;

    virtual uint8_t readReg(const uint8_t addr, const uint8_t _reg) = 0;
    virtual void writeReg (const uint8_t _addr, const uint8_t _reg, const uint8_t _val) = 0;
 private:
};

}

#endif
