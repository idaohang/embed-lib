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
#include <map>

#include "gpio.h"
#include "bbb_int_thread.h"

namespace embed
{

class BBBGPIO : public GPIO
{
 public:
    BBBGPIO(uint8_t _gpio, BBBIntThread* _intThread = NULL);
    ~BBBGPIO();

    bool init();
    void destroy();

    void setMode (const PIN_DIR _dir);
    uint8_t digitalRead ();
    void digitalWrite (const uint8_t _val);
    void attachInterrupt (GPIOIntHandler _handler, INT_MODE _mode, void* _data = NULL);
    void detachInterrupt (GPIOIntHandler _handler);
 private:
    static const uint8_t MAX_BUF = 64;

    static void intHandler (void* _data);

    bool                                m_initialized;
    uint8_t                             m_gpio;
    PIN_DIR                             m_pinDir;
    BBBIntThread*                       m_intThread;
    std::map<GPIOIntHandler,void*>      m_listeners;
    pthread_mutex_t                     m_listenersMutex;
    INT_MODE                            m_mode;
    int32_t                             m_valueFd;
};

}

#endif
