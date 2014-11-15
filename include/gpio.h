/*
 * Filename: gpio.h
 * Date Created: 11/15/2014
 * Author: Michael McKeown
 * Description: Header file for GPIO generic interface class
 */

#ifndef EMBED_GPIO_H
#define EMBED_GPIO_H

#include <stdint.h>

namespace embed
{

// GPIO Interface Class
class GPIO
{
 public:
    virtual ~GPIO() {};

    typedef enum PIN_DIR_ENUM
    {
        INVALID = 0,
        INPUT,
        OUTPUT
    } PIN_DIR;

    virtual bool init() = 0;
    virtual void destroy() = 0;

    virtual void setMode (const PIN_DIR _dir) = 0;
    virtual uint8_t digitalRead() = 0;
    virtual void digitalWrite (const uint8_t _val) = 0;
 private:
};

}

#endif
