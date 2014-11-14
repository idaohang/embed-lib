/*
 * Filename: exp_avg_filter.cpp
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Implementation file for a exponential moving average filter
 */

#include "exp_avg_filter.h"

using namespace embed;

template <class T> ExpAvgFilter<T>::ExpAvgFilter (double _alpha, uint32_t _stdDevInterval)
  : m_numSamples(_stdDevInterval),
    m_samples (),
    m_sampleCount (0),
    m_alpha (_alpha),
    m_sT (0.0)
{
}

template <class T> ExpAvgFilter<T>::~ExpAvgFilter ()
{
}

template <class T> void ExpAvgFilter<T>::reset()
{
    m_samples.clear();
    m_sampleCount = 0;
    m_sT = 0.0;

    assert (m_sampleCount == m_samples.size());
}

template <class T> void ExpAvgFilter<T>::addValue (T _val)
{
    m_sT = (m_alpha * _val) + ((1.0 - m_alpha) * m_sT);

    if (m_sampleCount < m_numSamples)
    {
        m_samples.push_back(_val);
        m_sampleCount++;
    }
    else
    {
        m_samples.pop_front();
        m_samples.push_back(_val);
    }

    assert (m_sampleCount == m_samples.size());
}

template <class T> double ExpAvgFilter<T>::getAvg ()
{
    return m_sT;
}

template <class T> double ExpAvgFilter<T>::getStdDev ()
{
    assert (m_sampleCount == m_samples.size());

    if (m_sampleCount <= 0)
        return nan("");

    double average = getAvg();

    double sq_diff_sum = 0;
    for (typename std::list<T>::const_iterator it = m_samples.begin(); it != m_samples.end(); it++)
        sq_diff_sum += pow((((double) (*it)) - average), 2);

    return sqrt(sq_diff_sum / ((double) m_sampleCount));
}

// Define possible usages for separate file compilation (stupid C++ problem)
template class ExpAvgFilter<double>;
