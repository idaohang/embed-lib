/*
 * Filename: avg_filter.h
 * Date Created: 11/13/2014
 * Author: Michael McKeown
 * Description: Header file for average filter interface
 */

#ifndef EMBED_AVG_FILTER_H
#define EMBED_AVG_FILTER_H

namespace embed
{

template <class T> class AvgFilter
{
 public:
    virtual ~AvgFilter() {}

    virtual void reset() = 0;
    virtual void addValue (T _val) = 0;
    virtual double getAvg() = 0;
    virtual double getStdDev() = 0;
 private:
};

}

#endif
