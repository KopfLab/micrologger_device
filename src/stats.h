#pragma once
#include "uTypedef.h"

// histogram =======

struct TpercentileStats
{
    dtypes::float64 mean = 0;
    dtypes::float64 sd = 0;
    dtypes::float64 sem = 0;
    dtypes::uint32 count = 0;
};

template <int32_t MIN_VAL, int32_t MAX_VAL>
class TexactHistogram
{

private:
    static constexpr int32_t RANGE = MAX_VAL - MIN_VAL + 1;
    dtypes::uint32 Fhistogram[RANGE];
    dtypes::uint32 FtotalCount;

public:
    TexactHistogram()
    {
        reset();
    }

    void reset()
    {
        FtotalCount = 0;
        for (int i = 0; i < RANGE; i++)
            Fhistogram[i] = 0;
    }

    void add(int32_t v)
    {
        // Clip
        if (v < MIN_VAL)
            v = MIN_VAL;
        if (v > MAX_VAL)
            v = MAX_VAL;

        size_t idx = v - MIN_VAL;

        if (Fhistogram[idx] != UINT32_MAX)
            Fhistogram[idx]++;

        if (FtotalCount != UINT32_MAX)
            FtotalCount++;
    }

    // Mean of samples between two fractions (0.0 - 1.0)
    // Example: (0.9, 1.0) = top 10%
    bool const percentileStats(TpercentileStats &_out, double _low = 0.0, double _high = 1.0)
    {
        // safety checks
        if (_low < 0.0 || _high > 1.0)
            return false;
        if (_low >= _high)
            return false;
        if (FtotalCount == 0)
            return false;

        // Convert fractions to ranks (1-based)
        dtypes::uint32 start_n = static_cast<dtypes::uint32>(round(FtotalCount * _low));
        dtypes::uint32 end_n = static_cast<dtypes::uint32>(round(FtotalCount * _high));
        dtypes::uint32 current_n = 0;
        if (start_n >= end_n)
            return false;

        // Walk through the histogram
        dtypes::uint32 count = 0;
        int64_t sum = 0;
        int64_t sumSq = 0;
        for (size_t i = 0; i < RANGE; i++)
        {
            // ignore 0s
            dtypes::uint32 n = Fhistogram[i];
            if (n == 0)
                continue;

            // calculate amount of overlap with the range
            dtypes::uint32 next_n = current_n + n;
            if (next_n <= start_n || current_n > end_n)
            {
                // no overlap
                current_n = next_n;
                continue;
            }

            // are we in the range of interest?
            if (current_n < start_n && next_n > end_n)
                n = end_n - start_n + 1; // yes outside both sides
            else if (current_n < start_n && next_n <= end_n)
                n = next_n - start_n + 1; // outside one side
            else if (current_n >= start_n && next_n > end_n)
                n = end_n - current_n + 1; // outside the other side

            // add
            count += n;
            sum += i * n;
            sumSq += i * i * n;

            // are we done?
            current_n = next_n;
            if (current_n >= end_n)
                break;
        }

        // did we get any counts?
        if (count == 0)
            return false;

        // return
        dtypes::float64 mean = static_cast<dtypes::float64>(sum) / count;
        dtypes::float64 variance = static_cast<double>(sumSq) / count - mean * mean;

        // return value
        _out.count = count;
        _out.mean = mean + MIN_VAL;
        _out.sd = (count > 1 && variance >= 0.0) ? std::sqrt(variance) : std::numeric_limits<dtypes::float64>::quiet_NaN();
        _out.sem = (count > 1 && variance >= 0.0) ? _out.sd / sqrt(_out.count) : std::numeric_limits<dtypes::float64>::quiet_NaN();

        return true;
    }

    dtypes::uint32 totalCount() const
    {
        return FtotalCount;
    }
};
