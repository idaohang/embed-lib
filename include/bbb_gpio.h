/*
 * Filename: bbb_gpio.h
 * Date Created: 11/15/2014
 * Author: Michael McKeown
 * Description: Header file for GPIO BeagleBone Black class
 */

#ifndef EMBED_BBB_GPIO_H
#define EMBED_BBB_GPIO_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "gpio.h"

namespace embed
{

class BBBGPIO : public GPIO
{
 public:
    BBBGPIO(uint8_t _gpio);
    ~BBBGPIO();

    bool init();
    void destroy();

    void setMode (const PIN_DIR _dir);
    uint8_t digitalRead ();
    void digitalWrite (const uint8_t _val);
 private:
    static const uint8_t MAX_BUF = 64;

    bool    m_initialized;
    uint8_t m_gpio;
    PIN_DIR m_pinDir;
};

}

#endif
