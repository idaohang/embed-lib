/*
 * Filename: bbb_gpio.cpp
 * Date Created: 11/15/2014
 * Author: Michael McKeown
 * Description: Implementation file for GPIO BeagleBone Black implementation
 */

#include "bbb_gpio.h"

using namespace embed;

BBBGPIO::BBBGPIO(uint8_t _gpio, BBBIntThread* _intThread) :
    m_initialized (false),
    m_gpio (_gpio),
    m_pinDir (INVALID),
    m_intThread (_intThread),
    m_mode (RISING),
    m_valueFd (-1)
{
}

BBBGPIO::~BBBGPIO()
{
}

bool BBBGPIO::init ()
{
    // Export GPIO
    int fd;
    if ((fd = open("/sys/class/gpio/export", O_WRONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::init open error: %s\n", strerror(errno));
        return false;
    }
    char buf[MAX_BUF];
    int len = snprintf(buf, sizeof(buf), "%d", m_gpio);
    if (write(fd, buf, len) != len)
    {
        fprintf(stderr, "BBBGPIO::init write error: %s\n", strerror(errno));
        close(fd);
        return false;
    }
    close(fd);

    // Read the pin direction
    len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", m_gpio);
    if ((fd = open(buf, O_RDONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::init open error: %s\n", strerror(errno));
        destroy();
        return false;
    }
    if ((len = read(fd, buf, sizeof(buf))) < 2)
    {
        fprintf(stderr, "BBBGPIO::init read error: %s\n", strerror(errno));
        close(fd);
        destroy();
        return false;
    }
    close(fd);

    if (strncmp(buf, "in", len))
        m_pinDir = INPUT;
    else if (strncmp(buf, "out", len))
        m_pinDir = OUTPUT;
    else
    {
        fprintf(stderr, "BBBGPIO::init pin direction string comparison error\n");
        m_pinDir = INVALID;
        return false;
    }

    m_initialized = true;

    return true;
}

void BBBGPIO::destroy ()
{
    // If there are any listeners, get rid of them
    m_listeners.clear();

    // Close fd if open
    if (m_valueFd >= 0)
        close(m_valueFd);

    // Unexport GPIO
    int fd;
    if ((fd = open("/sys/class/gpio/unexport", O_WRONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::destroy open error: %s\n", strerror(errno));
        return;
    }
    char buf[MAX_BUF];
    int len = snprintf(buf, sizeof(buf), "%d", m_gpio);
    if (write(fd, buf, len) != len)
        fprintf(stderr, "BBBGPIO::destroy write error: %s\n", strerror(errno));
    close(fd);

    m_initialized = false;
}

void BBBGPIO::setMode (const GPIO::PIN_DIR _dir)
{
    if (!m_initialized)
        return;

    if (m_valueFd >= 0)
    {
        fprintf(stderr, "BBBGPIO::setMode called when in interrupt mode\n");
        return;
    }

    int fd;
    char buf[MAX_BUF];
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", m_gpio);
    if ((fd = open(buf, O_WRONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::setMode open error: %s\n", strerror(errno));
        return;
    }
    switch (_dir)
    {
        case INVALID:
            fprintf(stderr, "BBBGPIO::setMode called with INVALID PIN_MODE\n");
            close(fd);
            return;
        case INPUT:
            if (write(fd , "in", 3) != 3)
            {
                fprintf(stderr, "BBBGPIO::setMode write error: %s\n", strerror(errno));
                close (fd);
                return;
            }
            break;
        case OUTPUT:
            if (write(fd, "out", 4) != 4)
            {
                fprintf(stderr, "BBBGPIO::setMode write error: %s\n", strerror(errno));
                close (fd);
                return;
            }
            break;
        default:
            fprintf(stderr, "BBBGPIO::setMode called with unknown PIN_MODE\n");
            close (fd);
            return;
    }
    m_pinDir = _dir;
    close (fd);
}

uint8_t BBBGPIO::digitalRead ()
{
    if (!m_initialized)
        return 0;

    if (m_pinDir != INPUT)
    {
        fprintf(stderr, "BBBGPIO::digitalRead called with pin direction not set to input\n");
        return 0;
    }

    if (m_valueFd >= 0)
    {
        fprintf(stderr, "BBBGPIO::digitalRead called when in interrupt mode\n");
        return 0;
    }

    int fd;
    char buf[MAX_BUF];
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);
    if ((fd = open(buf, O_RDONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::digitalRead open error: %s\n", strerror(errno));
        return 0;
    }
    char val[1];
    if (read(fd, &val, 1) != 1)
    {
        fprintf(stderr, "BBBGPIO::digitalRead read error: %s\n", strerror(errno));
        return 0;
    }
    close(fd);

    return atoi(val);
}

void BBBGPIO::digitalWrite (const uint8_t _val)
{
    if (!m_initialized)
        return;

    if (m_pinDir != OUTPUT)
    {
        fprintf(stderr, "BBBGPIO::digitalWrite called with pin direction not set to output\n");
        return;
    }

    if (m_valueFd >= 0)
    {
        fprintf(stderr, "BBBGPIO::digitalWrite called when in interrupt mode\n");
        return;
    }

    int fd;
    char buf[MAX_BUF];
    int len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);
    if ((fd = open(buf, O_WRONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::digitalWrite open error: %s\n", strerror(errno));
        return;
    }
    len = snprintf(buf, sizeof(buf), "%d", _val);
    if (write(fd, buf, len) != len)
    {
        fprintf(stderr, "BBBGPIO::digitalWrite write error: %s\n", strerror(errno));
        return;
    }
    close(fd);
}

void BBBGPIO::attachInterrupt (GPIOIntHandler _handler, INT_MODE _mode, void* _data)
{
    if (!m_initialized)
        return;

    if (m_pinDir != INPUT)
    {
        fprintf(stderr, "BBBGPIO::attachInterrupt called without pin direction set to input\n");
        return;
    }

    if (m_intThread == NULL)
    {
        fprintf(stderr, "BBBGPIO::attachInterrupt called without a interrupt thread specified\n");
        return;
    }

    // Check if this is the first listener to register, if so
    // need to register with interrupt thread to get callbacks
    if (m_listeners.size() == 0)
    {
        // Set the mode
        int32_t fd;
        char buf[MAX_BUF];
        snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", m_gpio);
        if ((fd = open(buf, O_WRONLY)) < 0)
        {
            fprintf(stderr, "BBBGPIO::attachInterrupt open error: %s\n", strerror(errno));
            return;
        }
        switch (_mode)
        {
            case RISING:
                if (write(fd, "rising", 6) != 6)
                {
                    fprintf(stderr, "BBBGPIO::attachInterrupt write error: %s\n", strerror(errno));
                    return;
                }
                break;
            case FALLING:
                if (write(fd, "falling", 7) != 7)
                {
                    fprintf(stderr, "BBBGPIO::attachInterrupt write error: %s\n", strerror(errno));
                    return;
                }
                break;
            case CHANGE:
                if (write(fd, "both", 4) != 4)
                {
                    fprintf(stderr, "BBBGPIO::attachInterrupt write error: %s\n", strerror(errno));
                    return;
                }
                break;
            default:
                fprintf(stderr, "BBBGPIO::attachInterrupt called with invalid mode\n");
                return;
                break;
        }
        m_mode = _mode;

        // Open the value fd
        snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);
        if ((m_valueFd = open(buf, O_RDONLY | O_NONBLOCK)) < 0)
        {
            fprintf(stderr, "BBBGPIO::attachInterrupt open error: %s\n", strerror(errno));
            return;
        }
        m_intThread->registerListener (m_valueFd, intHandler, this);
    }
    else if (_mode != m_mode)
    {
        fprintf(stderr, "BBBGPIO::attachInterrupt called with conflicting interrupt mode\n");
        return;
    }

    // Add to listeners
    m_listeners[_handler] = _data;
}

void BBBGPIO::detachInterrupt (GPIOIntHandler _handler)
{
    if (!m_initialized)
        return;

    if (m_pinDir != INPUT)
    {
        fprintf(stderr, "BBBGPIO::detachInterrupt called without pin direction set to input\n");
        return;
    }

    if (m_intThread == NULL)
    {
        fprintf(stderr, "BBBGPIO::detachInterrupt called without a interrupt thread specified\n");
        return;
    }

    // Remove from listeners
    m_listeners.erase(_handler);

    // Check to see if this was the last listener, will unregister
    // with interrupt thread and close fd if so
    if (m_listeners.size() == 0)
    {
        m_intThread->unregisterListener (m_valueFd, intHandler);
        close(m_valueFd);
        m_valueFd = -1;
    }
}

void BBBGPIO::intHandler (void* _data)
{
    BBBGPIO* _this = static_cast<BBBGPIO*>(_data);

    // Notify listeners
    std::map<GPIOIntHandler,void*>::iterator it;
    for (it = _this->m_listeners.begin(); it != _this->m_listeners.end(); it++)
        it->first (it->second);
}
