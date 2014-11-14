/*
 * Filename: bbb_bmp085_sync_test.cpp
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: A test program that prints values received from the BMP085
 *              pressure and temperature sensor.
 */

#include <sys/time.h>
#include <map>

#include "bmp085.h"
#include "bbb_i2c.h"
#include "avg_filter.h"
#include "simple_avg_filter.h"
#include "exp_avg_filter.h"
#include "screen.h"

using namespace embed;

int main (int argc, char *argv[])
{
    // Setup TUI
    if (!Screen::Instance()->init())
        return 1;

    int32_t width, height;
    Screen::Instance()->size(width, height);

    // Initialize I2C bus
    BBBI2C     devBus(1);
    devBus.init();

    // Initialize BMP device, passing it the I2C bus
    BMP085  device(&devBus);
    device.init();
    device.setOSSR(BMP085::OSSR_ULTRA_HIGH_RES);

    // Sample rate variables
    struct timeval last_measure, current_measure;
    gettimeofday(&last_measure, NULL);

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
    const double pressFilterAlpha = 0.75;
    const double absAltFilterAlpha = 0.75;
    const double relAltFilterAlpha = 0.75;
    const double sampleRateFilterAlpha = 0.75;

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
    double referenceAltitude = 0;

    // Line content defintionsdefinitions
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
    Screen::Instance()->printTextCenter(0, width - 1, TITLE_LINE, "Quadcopter Prototyping: BMP085 Test");
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

    // Main loop
    bool firstIter = true;
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
                                pressFilter = new ExpAvgFilter<double>(pressFilterAlpha, pressFilterSize);
                                absAltFilter = new ExpAvgFilter<double>(absAltFilterAlpha, absAltFilterSize);
                                relAltFilter = new ExpAvgFilter<double>(relAltFilterAlpha, relAltFilterSize);
                                sampleRateFilter = new ExpAvgFilter<double>(sampleRateFilterAlpha,
                                                                            sampleRateFilterSize);
                                break;
                            default:
                                devBus.destroy();
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
                        pressFilter = new ExpAvgFilter<double>(pressFilterAlpha, pressFilterSize);
                        absAltFilter = new ExpAvgFilter<double>(absAltFilterAlpha, absAltFilterSize);
                        relAltFilter = new ExpAvgFilter<double>(relAltFilterAlpha, relAltFilterSize);
                        sampleRateFilter = new ExpAvgFilter<double>(sampleRateFilterAlpha,
                                                                    sampleRateFilterSize);
                        break;
                    default:
                        devBus.destroy();
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

        // Read raw temperature and pressure
        int16_t raw_temp = device.readRawTempSync();
        int32_t raw_pressure = device.readRawPressureSync ();

        //  Calculate sample rate
        gettimeofday(&current_measure, NULL);
        int32_t sample_period_us = current_measure.tv_usec - last_measure.tv_usec;
        double sample_rate_hz_unfiltered = 1.0 / (sample_period_us / 1000000.0);
        double sample_rate_hz_filtered, sample_rate_hz_filtered_std_dev;
        memcpy(&last_measure, &current_measure, sizeof(struct timeval));

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

        // Set the reference altitude if this is the first iteration
        if (firstIter)
        {
            referenceAltitude = absAltMUnfiltered;
            firstIter = false;
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
    }

    if (pressFilter != NULL)
        delete pressFilter;
    if (absAltFilter != NULL)
        delete absAltFilter;
    if (relAltFilter != NULL)
        delete relAltFilter;
    if (sampleRateFilter != NULL)
        delete sampleRateFilter;

    devBus.destroy();

    Screen::Instance()->destroy();

    return 0;
}
