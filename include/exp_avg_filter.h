/*
 * Filename: exp_avg_filter.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for a exponential moving average filter
 */

#ifndef EMBED_EXP_AVG_FILTER_H
#define EMBED_EXP_AVG_FILTER_H

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include <list>

#include "avg_filter.h"

namespace embed
{

template <class T> class ExpAvgFilter : public AvgFilter<T>
{
 public:
     ExpAvgFilter(double _alpha, uint32_t _stdDevInterval);
     ~ExpAvgFilter ();

     void reset();
     void addValue (T _val);
     double getAvg ();
     double getStdDev ();
 private:
    uint32_t            m_numSamples;
    std::list<T>        m_samples;
    uint32_t            m_sampleCount;
    double              m_alpha;
    double              m_sT;
};

}

#endif
