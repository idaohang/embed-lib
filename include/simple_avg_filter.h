/*
 * Filename: simple_avg_filter.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for a simple moving average filter
 */

#ifndef EMBED_SIMPLE_AVG_FILTER_H
#define EMBED_SIMPLE_AVG_FILTER_H

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include <list>

#include "avg_filter.h"

namespace embed
{

template <class T> class SimpleAvgFilter : public AvgFilter<T>
{
 public:
     SimpleAvgFilter(uint32_t _numSamples);
     ~SimpleAvgFilter ();

     void reset();
     void addValue (T _val);
     double getAvg ();
     double getStdDev ();
 private:
    uint32_t            m_numSamples;
    std::list<T>        m_samples;
    uint32_t            m_sampleCount;
};

}

#endif
