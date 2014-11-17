/*
 * Filename: bmp085_async_test.cpp
 * Date Created: 11/17/2014
 * Author: Michael McKeown
 * Description: A test program that prints values received from the BMP085
 *              pressure and temperature sensor.
 */

#include <sys/time.h>
#include <map>

#include "bmp085.h"
#include "gpio.h"
#include "i2c.h"
#include "bbb_gpio.h"
#include "bbb_i2c.h"
#include "avg_filter.h"
#include "simple_avg_filter.h"
#include "exp_avg_filter.h"
#include "screen.h"

using namespace embed;

struct IntData
{
    pthread_mutex_t     m_dataMutex;
    bool                m_val;
    int16_t             m_temp;
    int32_t             m_pressure;
    int32_t             m_samplePeriodus;
    int32_t             m_lastSampleTimeus;
};

void eocIntHandler (const int16_t _temp, const int32_t pressure, void* _data);

int main (int argc, char *argv[])
{
    // Initialize XCLR GPIO
    GPIO*      xclrGPIO = NULL;
#ifdef BEAGLEBONEBLACK
    xclrGPIO = new BBBGPIO(115);
#endif

    if (xclrGPIO == NULL)
    {
        fprintf(stderr, "Error: No target device was specified when compiling this test\n");
        return 1;
    }

    if (!xclrGPIO->init())
    {
        fprintf(stderr, "Error: Initializing XCLR GPIO\n");
        delete xclrGPIO;
        return 1;
    }
    xclrGPIO->setMode(GPIO::OUTPUT);

    // Initialize the EOC GPIO
    GPIO*       eocGPIO = NULL;
#ifdef BEAGLEBONEBLACK
    BBBIntThread intThread;
    eocGPIO = new BBBGPIO (7, &intThread);
#endif

    if (eocGPIO == NULL)
    {
        fprintf(stderr, "Error: No target device was specified when compiling this test\n");
        xclrGPIO->destroy();
        delete xclrGPIO;
        return 1;
    }

    if (!eocGPIO->init())
    {
        fprintf(stderr, "Error: Initializing EOC GPIO\n");
        delete eocGPIO;
        xclrGPIO->destroy();
        delete xclrGPIO;
        return 1;
    }
    eocGPIO->setMode(GPIO::INPUT);

    // Initialize I2C bus
    I2C*       devBus = NULL;
#ifdef BEAGLEBONEBLACK
    devBus = new BBBI2C(1);
#endif

    if (devBus == NULL)
    {
        fprintf(stderr, "Error: No target device was specified when compiling this test\n");
        eocGPIO->destroy();
        delete eocGPIO;
        xclrGPIO->destroy();
        delete xclrGPIO;
        return 1;
    }

    if (!devBus->init())
    {
        fprintf(stderr, "Error: Initializing I2C bus\n");
        delete devBus;
        eocGPIO->destroy();
        delete eocGPIO;
        xclrGPIO->destroy();
        delete xclrGPIO;
        return 1;
    }

    // Initialize BMP device, passing it the I2C bus and XCLR GPIO
    BMP085  device(devBus, eocGPIO, xclrGPIO);
    device.setOSSR(BMP085::OSSR_ULTRA_HIGH_RES);
#ifdef BEAGLEBONEBLACK
    // For BBB, need to start thread because init kicks
    // off the first reading and depends on catching the
    // first interrupt
    intThread.start();
#endif
    if (!device.init(true))
    {
        fprintf(stderr, "Error: Initializing BMP085 device\n");
        devBus->destroy();
        delete devBus;
        eocGPIO->destroy();
        delete eocGPIO;
        xclrGPIO->destroy();
        delete xclrGPIO;
        return 1;
    }

    // Setup TUI
    if (!Screen::Instance()->init())
    {
        fprintf(stderr, "Error: Initializing Screen\n");
        device.destroy();
        devBus->destroy();
        delete devBus;
        eocGPIO->destroy();
        delete eocGPIO;
        xclrGPIO->destroy();
        delete xclrGPIO;
        return 1;
    }

    int32_t width, height;
    Screen::Instance()->size(width, height);

    // Average filters
    AvgFilter<double>* pressFilter = NULL;
    AvgFilter<double>* absAltFilter = NULL;
    AvgFilter<double>* relAltFilter = NULL;
    AvgFilter<double>* sampleRateFilter = NULL;

    // Filter parametes
    const int32_t pressFilterSize = 18;
    const int32_t absAltFilterSize = 18;
    const int32_t relAltFilterSize = 18;
    const int32_t sampleRateFilterSize = 18;
    const int32_t pressFilterStdDevInterval = 18;
    const int32_t absAltFilterStdDevInterval = 18;
    const int32_t relAltFilterStdDevInterval = 18;
    const int32_t sampleRateFilterStdDevInterval = 18;
    const double pressFilterAlpha = 0.5;
    const double absAltFilterAlpha = 0.5;
    const double relAltFilterAlpha = 0.5;
    const double sampleRateFilterAlpha = 0.5;

    // Filter type multiplexing
    typedef enum FILTER_TYPES_ENUM
    {
        SIMPLE_AVG_FILTER = 0,
        EXP_AVG_FILTER,
        NUM_FILTER_TYPES
    } FILTER_TYPES;
    std::map<int32_t, std::string> FILTER_STRING_MAP;
    FILTER_STRING_MAP[SIMPLE_AVG_FILTER] = "Simple Average";
    FILTER_STRING_MAP[EXP_AVG_FILTER] = "Exponential Average";

    FILTER_TYPES currentFilterType = SIMPLE_AVG_FILTER;

    // Average filter guards
    bool usePressFilter = false;
    bool useAbsAltFilter = false;
    bool useRelAltFilter = false;
    bool useSampleRateFilter = false;

    // Relative altitude variables
    double referenceAltitude = 0.0;

    // Line content defintions
    typedef enum LINE_CONTENT_ENUM
    {
        BORDER_TOP_LINE = 0,
        TITLE_LINE,
        TITLE_SEP_LINE,
        OPTIONS_LINE,
        BLANK0,
        TEMP_LINE,
        PRESS_LINE,
        ABS_ALT_LINE,
        REL_ALT_LINE,
        MEAS_RATE_LINE,
        BLANK1,
        CONTROLS_LINE,
        BORDER_BOT_LINE
    } LINE_CONTENT;

    // Print border
    Screen::Instance()->printRectangle(0, BORDER_TOP_LINE, width - 1, BORDER_BOT_LINE);

    // Print static text to screen
    char buf[width];
#ifdef BEAGLEBONEBLACK
    Screen::Instance()->printTextCenter(0, width - 1, TITLE_LINE, "embed-lib: BeagleBone Black BMP085 Asynchronous Test");
#endif
    Screen::Instance()->printHLine (0, width - 1, TITLE_SEP_LINE);
    sprintf(buf,                                     " Filter: %8s    Filter Type: %20s", "Disabled", "N/A");
    Screen::Instance()->printText(1, OPTIONS_LINE, buf);
    Screen::Instance()->printText(1, TEMP_LINE,      " Temperature              : ");
    Screen::Instance()->printText(1, PRESS_LINE,     " Pressure                 : ");
    Screen::Instance()->printText(1, ABS_ALT_LINE,   " Approx. Abs. Altitude    : ");
    Screen::Instance()->printText(1, REL_ALT_LINE,   " Approx. Rel. Altitude    : ");
    Screen::Instance()->printText(1, MEAS_RATE_LINE, " Measurement Rate         : ");
    Screen::Instance()->printText(1, CONTROLS_LINE,
                   " f - Toggle Filter   t - Toggle Filter Type    r - Reset Reference Altitude    q - Quit");

    // Offsets for updating GUI
    const int32_t VALUE_OFFSET = 29;
    const int32_t FILTER_OFFSET = 10;
    const int32_t FILTER_TYPE_OFFSET = 35;

    // Register interrupt with device
    struct IntData eocData;
    struct timeval ctime;
    pthread_mutex_init(&eocData.m_dataMutex, NULL);
    gettimeofday(&ctime, NULL);
    eocData.m_val = false;
    eocData.m_lastSampleTimeus = ctime.tv_usec;
    device.registerListener (eocIntHandler, &eocData);

    // Main loop
    bool firstMeas = true;
    char c;
    while((c = (Screen::Instance()->getCh())) != 'q')
    {
        // Process user input
        bool updateRefAlt = false;
        switch (c)
        {
            case 'f' :
                if (usePressFilter && useAbsAltFilter && useRelAltFilter && useSampleRateFilter)
                {
                    usePressFilter      = false;
                    useAbsAltFilter     = false;
                    useRelAltFilter     = false;
                    useSampleRateFilter = false;
                    memset (buf, '\0', width);
                    sprintf(buf, "%8s", "Disabled");
                    Screen::Instance()->printText(FILTER_OFFSET, OPTIONS_LINE, buf);
                    memset (buf, '\0', width);
                    sprintf(buf, "%20s", "N/A");
                    Screen::Instance()->printText(FILTER_TYPE_OFFSET, OPTIONS_LINE, buf);
                    // Blank out values
                    for (int32_t i = TEMP_LINE; i <= MEAS_RATE_LINE; i++)
                        Screen::Instance()->printText(VALUE_OFFSET, i,
                                                      "                                        ");
                }
                else
                {
                    // Initialize/reset filters
                    if (pressFilter == NULL && absAltFilter == NULL &&
                        relAltFilter == NULL && sampleRateFilter == NULL)
                    {
                        switch (currentFilterType)
                        {
                            case SIMPLE_AVG_FILTER :
                                pressFilter = new SimpleAvgFilter<double>(pressFilterSize);
                                absAltFilter = new SimpleAvgFilter<double>(absAltFilterSize);
                                relAltFilter = new SimpleAvgFilter<double>(relAltFilterSize);
                                sampleRateFilter = new SimpleAvgFilter<double>(sampleRateFilterSize);
                                break;
                            case EXP_AVG_FILTER :
                                pressFilter = new ExpAvgFilter<double>(pressFilterAlpha,
                                                                       pressFilterStdDevInterval);
                                absAltFilter = new ExpAvgFilter<double>(absAltFilterAlpha,
                                                                        absAltFilterStdDevInterval);
                                relAltFilter = new ExpAvgFilter<double>(relAltFilterAlpha,
                                                                        relAltFilterStdDevInterval);
                                sampleRateFilter = new ExpAvgFilter<double>(sampleRateFilterAlpha,
                                                                            sampleRateFilterStdDevInterval);
                                break;
                            default:
                                devBus->destroy();
                                Screen::Instance()->destroy();
                                fprintf(stderr, "Error: Invalid filter type\n");
                                return 1;
                                break;
                        }
                    }
                    else
                    {
                        pressFilter->reset();
                        absAltFilter->reset();
                        relAltFilter->reset();
                        sampleRateFilter->reset ();
                    }

                    usePressFilter      = true;
                    useAbsAltFilter     = true;
                    useRelAltFilter     = true;
                    useSampleRateFilter = true;
                    memset (buf, '\0', width);
                    sprintf(buf, "%8s", "Enabled");
                    Screen::Instance()->printText(FILTER_OFFSET, OPTIONS_LINE, buf);
                    memset (buf, '\0', width);
                    sprintf(buf, "%20s", FILTER_STRING_MAP[currentFilterType].c_str());
                    Screen::Instance()->printText(FILTER_TYPE_OFFSET, OPTIONS_LINE, buf);
                }
                break;
            case 't' :
                if (pressFilter != NULL)
                    delete pressFilter;
                if (absAltFilter != NULL)
                    delete absAltFilter;
                if (relAltFilter != NULL)
                    delete relAltFilter;
                if (sampleRateFilter != NULL)
                    delete sampleRateFilter;

                currentFilterType = (FILTER_TYPES) (((int32_t) currentFilterType) + 1);
                if (currentFilterType >= NUM_FILTER_TYPES)
                    currentFilterType = (FILTER_TYPES) 0;

                switch (currentFilterType)
                {
                    case SIMPLE_AVG_FILTER :
                        pressFilter = new SimpleAvgFilter<double>(pressFilterSize);
                        absAltFilter = new SimpleAvgFilter<double>(absAltFilterSize);
                        relAltFilter = new SimpleAvgFilter<double>(relAltFilterSize);
                        sampleRateFilter = new SimpleAvgFilter<double>(sampleRateFilterSize);
                        break;
                    case EXP_AVG_FILTER :
                        pressFilter = new ExpAvgFilter<double>(pressFilterAlpha, pressFilterStdDevInterval);
                        absAltFilter = new ExpAvgFilter<double>(absAltFilterAlpha, absAltFilterStdDevInterval);
                        relAltFilter = new ExpAvgFilter<double>(relAltFilterAlpha, relAltFilterStdDevInterval);
                        sampleRateFilter = new ExpAvgFilter<double>(sampleRateFilterAlpha,
                                                                    sampleRateFilterStdDevInterval);
                        break;
                    default:
                        devBus->destroy();
                        Screen::Instance()->destroy();
                        fprintf(stderr, "Error: Invalid filter type\n");
                        return 1;
                        break;
                }

                if (usePressFilter && useAbsAltFilter && useRelAltFilter && useSampleRateFilter)
                {
                    memset (buf, '\0', width);
                    sprintf(buf, "%20s", FILTER_STRING_MAP[currentFilterType].c_str());
                    Screen::Instance()->printText(FILTER_TYPE_OFFSET, OPTIONS_LINE, buf);
                }
                break;
            case 'r' :
                updateRefAlt = true;
                if (useRelAltFilter)
                    relAltFilter->reset();
                break;
            default :
                break;
        }

        // Atomic read
        pthread_mutex_lock (&eocData.m_dataMutex);

        bool val_data = eocData.m_val;
        int16_t raw_temp = eocData.m_temp;
        int32_t raw_pressure = eocData.m_pressure;
        int32_t sample_period_us = eocData.m_samplePeriodus;

        pthread_mutex_unlock (&eocData.m_dataMutex);

        // Don't print if not valid data
        if (!val_data)
            continue;

        //  Calculate sample rate
        double sample_rate_hz_unfiltered = 1.0 / (sample_period_us / 1000000.0);
        double sample_rate_hz_filtered, sample_rate_hz_filtered_std_dev;

        // Filter sample rate
        if (useSampleRateFilter)
        {
            sampleRateFilter->addValue (sample_rate_hz_unfiltered);
            sample_rate_hz_filtered = sampleRateFilter->getAvg();
            sample_rate_hz_filtered_std_dev = sampleRateFilter->getStdDev();
        }

        // Calculate the temperature and pressure
        double tempC;
        double pressurehPaUnfiltered, pressurehPaFiltered, pressurehPaFilteredStdDev;
        double absAltMUnfiltered, absAltMFiltered, absAltMFilteredStdDev;
        device.calcTempPressure(raw_temp, raw_pressure, &tempC, &pressurehPaUnfiltered);

        // Filter pressure
        if (usePressFilter)
        {
            pressFilter->addValue (pressurehPaUnfiltered);
            pressurehPaFiltered = pressFilter->getAvg();
            pressurehPaFilteredStdDev = pressFilter->getStdDev();
        }

        // Calculate approximate absolute altitude (uses average pressure at sea level)
        device.calcApproxAlt(pressurehPaUnfiltered, &absAltMUnfiltered);

        // Filter absolute altitude
        if (useAbsAltFilter)
        {
            absAltFilter->addValue (absAltMUnfiltered);
            absAltMFiltered = absAltFilter->getAvg();
            absAltMFilteredStdDev = absAltFilter->getStdDev();
        }

        // Set the reference altitude if this is the first measurement
        if (firstMeas)
        {
            updateRefAlt = true;
            firstMeas = false;
        }

        if (updateRefAlt)
        {
            if (useAbsAltFilter)
                referenceAltitude = absAltMFiltered;
            else
                referenceAltitude = absAltMUnfiltered;
        }

        // Calculate altitude relative to reference altitude
        double relAltMUnfiltered = absAltMUnfiltered - referenceAltitude;
        double relAltMFiltered, relAltMFilteredStdDev;

        // Filter relative altitude
        if (useRelAltFilter)
        {
            relAltFilter->addValue(relAltMUnfiltered);
            relAltMFiltered = relAltFilter->getAvg();
            relAltMFilteredStdDev = relAltFilter->getStdDev();
        }

        // Print new values to screen
        memset(buf, '\0', width);
        sprintf(buf, "%9.1fC", tempC);
        Screen::Instance()->printText(VALUE_OFFSET, TEMP_LINE, buf);

        memset(buf, '\0', width);
        if (usePressFilter)
            sprintf (buf, "%9.3fhPa    +/- %6.3fhPa", pressurehPaFiltered, pressurehPaFilteredStdDev);
        else
            sprintf (buf, "%9.3fhPa", pressurehPaUnfiltered);
        Screen::Instance()->printText(VALUE_OFFSET, PRESS_LINE, buf);

        memset(buf, '\0', width);
        if (useAbsAltFilter)
            sprintf (buf, "%9.3fm      +/- %6.3fm", absAltMFiltered, absAltMFilteredStdDev);
        else
            sprintf (buf, "%9.3fm", absAltMUnfiltered);
        Screen::Instance()->printText(VALUE_OFFSET, ABS_ALT_LINE, buf);

        memset(buf, '\0', width);
        if (useRelAltFilter)
            sprintf (buf, "%9.3fm      +/- %6.3fm", relAltMFiltered, relAltMFilteredStdDev);
        else
            sprintf (buf, "%9.3fm", relAltMUnfiltered);
        Screen::Instance()->printText(VALUE_OFFSET, REL_ALT_LINE, buf);

        memset(buf, '\0', width);
        if (useSampleRateFilter)
            sprintf (buf, "%9.2fHz     +/- %6.3fHz", sample_rate_hz_filtered, sample_rate_hz_filtered_std_dev);
        else
            sprintf (buf, "%9.2fHz", sample_rate_hz_unfiltered);
        Screen::Instance()->printText(VALUE_OFFSET, MEAS_RATE_LINE, buf);

        // Don't need to update screen too fast
        usleep(100);
    }

    device.unregisterListener (eocIntHandler);
    pthread_mutex_destroy (&eocData.m_dataMutex);

    if (pressFilter != NULL)
        delete pressFilter;
    if (absAltFilter != NULL)
        delete absAltFilter;
    if (relAltFilter != NULL)
        delete relAltFilter;
    if (sampleRateFilter != NULL)
        delete sampleRateFilter;

    device.destroy();
    #ifdef BEAGLEBONEBLACK
        intThread.end();
    #endif

    devBus->destroy();
    delete devBus;

    eocGPIO->destroy();
    delete eocGPIO;

    xclrGPIO->destroy();
    delete xclrGPIO;

    Screen::Instance()->destroy();

    return 0;
}

void eocIntHandler (const int16_t _temp, const int32_t _pressure, void* _data)
{
    struct IntData* _eocData = static_cast<struct IntData*>(_data);

    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    int32_t sample_period = current_time.tv_usec - _eocData->m_lastSampleTimeus;

    // Atomic update
    pthread_mutex_lock (&_eocData->m_dataMutex);

    _eocData->m_val = true;
    _eocData->m_temp = _temp;
    _eocData->m_pressure = _pressure;
    _eocData->m_samplePeriodus = sample_period;
    _eocData->m_lastSampleTimeus = current_time.tv_usec;

    pthread_mutex_unlock (&_eocData->m_dataMutex);
}
