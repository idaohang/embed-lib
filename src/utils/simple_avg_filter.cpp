/*
 * Filename: simple_avg_filter.cpp
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Implementation file for a simple moving average filter
 */

#include "simple_avg_filter.h"

using namespace embed;

template <class T> SimpleAvgFilter<T>::SimpleAvgFilter (uint32_t _numSamples) :
    m_numSamples(_numSamples),
    m_samples (),
    m_sampleCount (0)
{
}

template <class T> SimpleAvgFilter<T>::~SimpleAvgFilter ()
{
}

template <class T> void SimpleAvgFilter<T>::reset()
{
    m_samples.clear();
    m_sampleCount = 0;

    assert (m_sampleCount == m_samples.size());
}

template <class T> void SimpleAvgFilter<T>::addValue (T _val)
{
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

template <class T> double SimpleAvgFilter<T>::getAvg ()
{
    assert (m_sampleCount == m_samples.size());

    if (m_sampleCount <= 0)
        return nan("");

    T sum = 0;
    for (typename std::list<T>::const_iterator it = m_samples.begin(); it != m_samples.end(); it++)
        sum += (*it);

    return (((double) sum) / ((double) m_sampleCount));
}

template <class T> double SimpleAvgFilter<T>::getStdDev ()
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
template class SimpleAvgFilter<double>;
