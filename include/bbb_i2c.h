/*
 * Filename: bbb_i2c.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for I2C Beaglebone Black library
 */

#ifndef EMBED_BBB_I2C_H
#define EMBED_BBB_I2C_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <string.h>

#include "i2c.h"

namespace embed
{

class BBBI2C : public I2C
{
 public:
    BBBI2C (const uint8_t _bus);
    ~BBBI2C ();

    bool init();
    void destroy();

    uint8_t readReg (const uint8_t _addr, const uint8_t _reg);
    void writeReg (const uint8_t _addr, const uint8_t _reg, const uint8_t _val);

 private:
    uint8_t         m_bus;
    int32_t         m_handle;
};

}

#endif
