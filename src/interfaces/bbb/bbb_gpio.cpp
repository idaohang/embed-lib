/*
 * Filename: bbb_gpio.cpp
 * Date Created: 11/15/2014
 * Author: Michael McKeown
 * Description: Implementation file for GPIO BeagleBone Black implementation
 */

#include "bbb_gpio.h"

using namespace embed;

BBBGPIO::BBBGPIO(uint8_t _gpio) :
    m_initialized (false),
    m_gpio (_gpio),
    m_pinDir (INVALID)
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

    return true;
}

void BBBGPIO::destroy ()
{
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
}

void BBBGPIO::setMode (const GPIO::PIN_DIR _dir)
{
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
    if (m_pinDir != INPUT)
    {
        fprintf(stderr, "BBBGPIO::read called with pin direction not set to input\n");
        return 0;
    }

    int fd;
    char buf[MAX_BUF];
    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);
    if ((fd = open(buf, O_RDONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::read open error: %s\n", strerror(errno));
        return 0;
    }
    char val[1];
    if (read(fd, &val, 1) != 1)
    {
        fprintf(stderr, "BBBGPIO::read read error: %s\n", strerror(errno));
        return 0;
    }
    close(fd);

    return atoi(val);
}

void BBBGPIO::digitalWrite (const uint8_t _val)
{
    if (m_pinDir != OUTPUT)
    {
        fprintf(stderr, "BBBGPIO::write called with pin direction not set to output\n");
        return;
    }

    int fd;
    char buf[MAX_BUF];
    int len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", m_gpio);
    if ((fd = open(buf, O_WRONLY)) < 0)
    {
        fprintf(stderr, "BBBGPIO::write open error: %s\n", strerror(errno));
        return;
    }
    len = snprintf(buf, sizeof(buf), "%d", _val);
    if (write(fd, buf, len) != len)
    {
        fprintf(stderr, "BBBGPIO::write write error: %s\n", strerror(errno));
        return;
    }
    close(fd);
}
