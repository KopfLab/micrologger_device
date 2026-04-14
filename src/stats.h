#pragma once
#include "uTypedef.h"

// exact histogram =======

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
    static constexpr dtypes::float64 NaN = std::numeric_limits<dtypes::float64>::quiet_NaN();

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

    bool mode(dtypes::float64 &result) const
    {
        if (FtotalCount < 2)
            return false;

        // ── mean and sigma — float64 to avoid precision loss in accumulation ──
        dtypes::float64 mean = 0.0, var = 0.0;
        for (int i = 0; i < RANGE; i++)
        {
            if (Fhistogram[i] == 0)
                continue;
            mean += (dtypes::float64)Fhistogram[i] * i;
        }
        mean /= FtotalCount;
        for (int i = 0; i < RANGE; i++)
        {
            if (Fhistogram[i] == 0)
                continue;
            dtypes::float64 d = i - mean;
            var += (dtypes::float64)Fhistogram[i] * d * d;
        }
        dtypes::float32 sigma = (dtypes::float32)sqrt(var / FtotalCount);

        // ── Silverman bandwidth — LUT for n^(-1/5), n=0..100 ─────────────
        static const dtypes::float32 LUT[] = {
            0, 1.f, .8706f, .8027f, .7579f, .7248f, .6988f, .6776f, .6598f, .6442f, .631f,
            .6194f, .6089f, .5994f, .5908f, .5829f, .5756f, .5689f, .5627f, .5569f, .5515f,
            .5464f, .5416f, .5371f, .5329f, .5289f, .5251f, .5215f, .5181f, .5148f, .5117f,
            .5088f, .506f, .5033f, .5007f, .4983f, .496f, .4937f, .4916f, .4895f, .4876f,
            .4857f, .4839f, .4821f, .4805f, .4789f, .4773f, .4758f, .4744f, .473f, .4717f,
            .4704f, .4692f, .4680f, .4668f, .4657f, .4646f, .4635f, .4624f, .4614f, .4604f,
            .4594f, .4584f, .4574f, .4565f, .4556f, .4547f, .4538f, .4529f, .4521f, .4512f,
            .4504f, .4496f, .4488f, .4480f, .4473f, .4465f, .4458f, .4451f, .4444f, .4437f,
            .4430f, .4423f, .4416f, .4410f, .4403f, .4397f, .4391f, .4384f, .4378f, .4372f,
            .4366f, .4360f, .4355f, .4349f, .4343f, .4338f, .4332f, .4327f, .4322f, .4317f};
        static constexpr int LUTMAX = 100;
        dtypes::uint32 n = (FtotalCount <= LUTMAX) ? FtotalCount : LUTMAX;
        dtypes::float32 h = 2.34f * sigma * LUT[n];
        if (h < 0.5f)
            h = 0.5f;

        // ── KDE peak search ───────────────────────────────────────────────
        int hBins = (int)h + 1;
        dtypes::float32 invH = 1.0f / h;
        dtypes::float32 bestDensity = -1.0f;
        dtypes::float32 bestX = 0.0f;
        int bestG = 0;

        for (int g = 0; g < RANGE; g++)
        {
            dtypes::float32 kde = 0.0f;
            int lo = g - hBins;
            if (lo < 0)
                lo = 0;
            int hi = g + hBins;
            if (hi >= RANGE)
                hi = RANGE - 1;
            for (int k = lo; k <= hi; k++)
            {
                if (Fhistogram[k] == 0)
                    continue;
                dtypes::float32 u = (dtypes::float32)(g - k) * invH;
                kde += (dtypes::float32)Fhistogram[k] * 0.75f * (1.0f - u * u);
            }
            if (kde > bestDensity)
            {
                bestDensity = kde;
                bestX = (dtypes::float32)g;
                bestG = g;
            }
        }

        // ── parabolic sub-bin refinement ──────────────────────────────────
        if (bestG > 0 && bestG < RANGE - 1)
        {
            dtypes::float32 km = 0.0f, k0 = bestDensity, k1 = 0.0f;
            int lo, hi;

            lo = bestG - 1 - hBins;
            if (lo < 0)
                lo = 0;
            hi = bestG - 1 + hBins;
            if (hi >= RANGE)
                hi = RANGE - 1;
            for (int k = lo; k <= hi; k++)
            {
                if (Fhistogram[k] == 0)
                    continue;
                dtypes::float32 u = (dtypes::float32)(bestG - 1 - k) * invH;
                km += (dtypes::float32)Fhistogram[k] * 0.75f * (1.0f - u * u);
            }

            lo = bestG + 1 - hBins;
            if (lo < 0)
                lo = 0;
            hi = bestG + 1 + hBins;
            if (hi >= RANGE)
                hi = RANGE - 1;
            for (int k = lo; k <= hi; k++)
            {
                if (Fhistogram[k] == 0)
                    continue;
                dtypes::float32 u = (dtypes::float32)(bestG + 1 - k) * invH;
                k1 += (dtypes::float32)Fhistogram[k] * 0.75f * (1.0f - u * u);
            }

            dtypes::float32 denom = km - 2.0f * k0 + k1;
            if (fabsf(denom) > 1e-12f)
                bestX += 0.5f * (km - k1) / denom;
        }

        result = (dtypes::float64)bestX + MIN_VAL;
        return true;
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
        dtypes::float64 variance = static_cast<dtypes::float64>(sumSq) / count - mean * mean;

        // return value
        _out.count = count;
        _out.mean = mean + MIN_VAL;
        _out.sd = (count > 1 && variance >= 0.0) ? sqrt(variance) : NaN;
        _out.sem = (count > 1 && variance >= 0.0) ? _out.sd / sqrt(count) : NaN;

        return true;
    }

    dtypes::uint32 totalCount() const
    {
        return FtotalCount;
    }
};

// single peak (mode) detection =======

struct TkdeMode
{
    dtypes::float64 peak = std::numeric_limits<dtypes::float64>::quiet_NaN();
    dtypes::float64 sd = std::numeric_limits<dtypes::float64>::quiet_NaN();
};

template <int32_t ADC_MAX, int32_t MAX_SAMPLES, int32_t N_BOOTS>
class TpeakStats
{

private:
    TexactHistogram<0, ADC_MAX> _hist;
    TexactHistogram<0, ADC_MAX> _bHist;
    dtypes::uint16 _data[MAX_SAMPLES];
    dtypes::float32 _bPeaks[N_BOOTS];
    int _n;
    dtypes::uint32 _lcg;

    static constexpr dtypes::float64 NaN = std::numeric_limits<dtypes::float64>::quiet_NaN();

    inline int _randi(int n)
    {
        _lcg = _lcg * 1664525u + 1013904223u;
        return (int)(_lcg % (dtypes::uint32)n);
    }

public:
    TpeakStats() : _n(0), _lcg(12345u) {}

    void reset()
    {
        _n = 0;
        _hist.reset();
    }

    bool add(dtypes::uint16 x)
    {
        if (_n >= MAX_SAMPLES)
            return false;
        _hist.add((dtypes::int32)x);
        _data[_n++] = x;
        return true;
    }

    int count() const { return _n; }
    bool full() const { return _n >= MAX_SAMPLES; }

    bool calculate(TkdeMode &result, dtypes::uint32 seed = 12345u)
    {
        if (_n == 0)
            return false;

        // only one sample — return it directly, sd is undefined
        if (_n == 1)
        {
            result.peak = (dtypes::float64)_data[0];
            result.sd = NaN;
            return true;
        }

        // primary estimate
        dtypes::float64 singlePass;
        if (!_hist.mode(singlePass))
            return false;

        // bootstrap — resample _data[], rebuild histogram per replicate
        dtypes::float64 bMean = 0.0;
        for (int b = 0; b < N_BOOTS; b++)
        {
            _bHist.reset();
            for (int i = 0; i < _n; i++)
                _bHist.add((dtypes::int32)_data[_randi(_n)]);
            dtypes::float64 peak;
            _bPeaks[b] = _bHist.mode(peak) ? (dtypes::float32)peak : (dtypes::float32)singlePass;
            bMean += _bPeaks[b];
        }
        bMean /= N_BOOTS;

        // BC bootstrap peak and std dev of bootstrap peaks
        dtypes::float64 bVar = 0.0;
        for (int b = 0; b < N_BOOTS; b++)
        {
            dtypes::float64 d = _bPeaks[b] - bMean;
            bVar += d * d;
        }

        result.peak = 2.0 * singlePass - bMean; // bias-corrected bootstrap peak
        result.sd = sqrt(bVar / N_BOOTS);       // std dev of bootstrap peaks
        return true;
    }
};