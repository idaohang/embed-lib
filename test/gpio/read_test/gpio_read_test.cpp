/*
 * Filename: gpio_read_test.cpp
 * Date Created: 11/15/2014
 * Author Michael McKeown
 * Description: A test program that prints the value of a GPIO
 */

#include "gpio.h"
#include "bbb_gpio.h"
#include "screen.h"

using namespace embed;

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
    gpio = new BBBGPIO(gpioPin);
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
    Screen::Instance()->printTextCenter(0, width - 1, TITLE_LINE, "embed-lib: BeagleBone Black GPIO Test");
#endif
    Screen::Instance()->printHLine (0, width - 1, TITLE_SEP_LINE);
    snprintf(buf, sizeof(buf), " GPIO_%d Value: ", gpioPin);
    Screen::Instance()->printText (1, GPIO_LINE, buf);
    Screen::Instance()->printText(1, CONTROLS_LINE, " q - Quit");

    // Constants for updating GUI
    const int32_t VALUE_OFFSET = 17;

    // Main loop
    char c;
    while ((c = (Screen::Instance()->getCh())) != 'q')
    {
        uint8_t val = gpio->digitalRead();
        snprintf(buf, sizeof(buf), "%d", val);
        Screen::Instance()->printText(VALUE_OFFSET, GPIO_LINE, buf);
    }

    gpio->destroy();
    delete gpio;

    Screen::Instance()->destroy();
}
