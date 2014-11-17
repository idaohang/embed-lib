/*
 * Filename: gpio_int_test.cpp
 * Date Created: 11/16/2014
 * Author Michael McKeown
 * Description: A test program that prints the interrupt count on a GPIO
 */

#include "gpio.h"
#include "bbb_gpio.h"
#include "screen.h"

using namespace embed;

void intHandler (void* _data);

int main(int argc, char * argv[])
{
    // Parse cmd line args
    if (argc < 2)
    {
        printf("Usage: %s <gpio-pin>\n", argv[0]);
        return 1;
    }
    uint8_t gpioPin = atoi(argv[1]);

    // Setup TUI
    if (!Screen::Instance()->init())
        return 1;

    int32_t width, height;
    Screen::Instance()->size(width, height);

    // Initialize gpio
    GPIO*   gpio = NULL;
#ifdef BEAGLEBONEBLACK
    // Initialize interrupt thread
    BBBIntThread    intThread;
    gpio = new BBBGPIO(gpioPin, &intThread);
#endif

    if (gpio == NULL)
    {
        fprintf(stderr, "Error: No target device was specified when compiling this test\n");
        Screen::Instance()->destroy();
        return 1;
    }
    if (!gpio->init())
    {
        fprintf(stderr, "Error: Initializing GPIO\n");
        Screen::Instance()->destroy();
        return 1;
    }
    gpio->setMode(GPIO::INPUT);

    // Line content definitions
    typedef enum LINE_CONTENT_ENUM
    {
        BORDER_TOP_LINE = 0,
        TITLE_LINE,
        TITLE_SEP_LINE,
        BLANK0,
        GPIO_LINE,
        BLANK1,
        CONTROLS_LINE,
        BORDER_BOT_LINE
    } LINE_CONTENT;

    // Print border
    Screen::Instance()->printRectangle(0, BORDER_TOP_LINE, width - 1, BORDER_BOT_LINE);

    // Print static text to screen
    char buf[64];
#ifdef BEAGLEBONEBLACK
    Screen::Instance()->printTextCenter(0, width - 1, TITLE_LINE, "embed-lib: BeagleBone Black GPIO Interrupt Test");
#endif
    Screen::Instance()->printHLine (0, width - 1, TITLE_SEP_LINE);
    snprintf(buf, sizeof(buf), " GPIO_%d Interrupt Count: ", gpioPin);
    Screen::Instance()->printText (1, GPIO_LINE, buf);
    Screen::Instance()->printText(1, CONTROLS_LINE, " q - Quit");

    // Constants for updating GUI
    const int32_t VALUE_OFFSET = 27;

    // Setup interrupt (start at -1 because
    // for some reason in this test an interrupt
    // occurs whenever it starts)
    int32_t intCount = -1;
    gpio->attachInterrupt(intHandler, GPIO::RISING, &intCount);
#ifdef BEAGLEBONEBLACK
    intThread.start();
#endif

    // Main loop
    char c;
    while ((c = (Screen::Instance()->getCh())) != 'q')
    {
        snprintf(buf, sizeof(buf), "%3d", intCount);
        Screen::Instance()->printText(VALUE_OFFSET, GPIO_LINE, buf);
    }

#ifdef BEAGLEBONEBLACK
    intThread.end();
#endif
    gpio->detachInterrupt(intHandler);

    gpio->destroy();
    delete gpio;

    Screen::Instance()->destroy();
}

void intHandler (void* _data)
{
    int32_t * _intCount = static_cast<int32_t*>(_data);
    (*_intCount)++;
}
