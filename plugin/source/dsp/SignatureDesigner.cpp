#include "SignatureDesigner.h"
#include "SimpleFFT.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace moonmic
{

namespace
{
    using cd = std::complex<double>;

    cd G (double f, double r, double c)
    {
        const double k = 2.0 * std::numbers::pi * std::max (f, 1.0) / c;
        return 1.0 + 1.0 / cd (0.0, k * r);
    }

    /** Linear interpolation of y(x) on ascending x, clamped at both ends; xq in log-frequency. */
    double interpLog (const std::vector<float>& f, const std::vector<float>& y, double fq)
    {
        if (fq <= f.front()) return y.front();
        if (fq >= f.back())  return y.back();
        const auto it = std::upper_bound (f.begin(), f.end(), (float) fq);
        const size_t i1 = (size_t) (it - f.begin()), i0 = i1 - 1;
        const double x0 = std::log (f[i0]), x1 = std::log (f[i1]);
        const double t = (std::log (fq) - x0) / (x1 - x0);
        return y[i0] + t * (y[i1] - y[i0]);
    }
}

std::vector<float> SignatureDesigner::targetMagnitudeDb (const ModelLibrary& lib, int setIndex,
                                                         float angleDeg, float distanceM)
{
    const auto& s = lib.set (setIndex);
    const auto& f = lib.frequencies();
    const size_t nF = f.size();
    const int nA = lib.numAngles();

    double a = std::fmod ((double) angleDeg, 360.0);
    if (a < 0.0) a += 360.0;
    a /= 5.0;
    const int i0 = (int) std::floor (a) % nA;
    const int i1 = (i0 + 1) % nA;
    const double t = a - std::floor (a);
    const double cosT = std::cos ((double) angleDeg * std::numbers::pi / 180.0);
    const double r0 = lib.referenceDistance(), c = lib.speedOfSound();

    std::vector<float> out (nF);
    for (size_t k = 0; k < nF; ++k)
    {
        auto H = [&] (int ai)
        {
            const size_t idx = (size_t) ai * nF + k;
            return std::polar (std::pow (10.0, s.magDb[idx] / 20.0), (double) s.phaseRad[idx]);
        };
        cd h = (1.0 - t) * H (i0) + t * H (i1);
        const cd b ((double) s.bRe[k], (double) s.bIm[k]);
        h += b * cosT * (G (f[k], distanceM, c) / G (f[k], r0, c) - 1.0);
        out[k] = (float) (20.0 * std::log10 (std::abs (h) + 1e-12));
    }
    return out;
}

int SignatureDesigner::firLengthFor (double sampleRate)
{
    int len = 256;
    while (len < (int) std::ceil (firSeconds * sampleRate))
        len <<= 1;
    return len;
}

SignatureDesigner::Result SignatureDesigner::design (const ModelLibrary& lib, int setIndex,
                                                     float angleDeg, float distanceM, double sampleRate)
{
    Result res;
    res.targetMagDb = targetMagnitudeDb (lib, setIndex, angleDeg, distanceM);
    const auto& f = lib.frequencies();

    // Pink-weighted level (equal weight per log-frequency point) between 100 Hz and 10 kHz.
    double acc = 0.0;
    int cnt = 0;
    for (size_t k = 0; k < f.size(); ++k)
        if (f[k] >= 100.0f && f[k] <= 10000.0f)
        {
            acc += std::pow (10.0, res.targetMagDb[k] / 10.0);
            ++cnt;
        }
    res.autoGainDb = cnt > 0 ? (float) (-10.0 * std::log10 (acc / cnt)) : 0.0f;

    // Minimum-phase FIR from the target magnitude via the real cepstrum.
    const int firLength = firLengthFor (sampleRate);
    int order = 0;
    while ((1 << order) < firLength * 4) ++order;     // design FFT = 4x FIR length
    const SimpleFFT fft (order);
    const int N = fft.size();
    std::vector<cd> X ((size_t) N);
    for (int k = 0; k <= N / 2; ++k)
    {
        const double fk = k * sampleRate / N;
        const double db = interpLog (f, res.targetMagDb, std::max (fk, 1.0));
        X[(size_t) k] = std::log (std::pow (10.0, db / 20.0));
        if (k > 0 && k < N / 2)
            X[(size_t) (N - k)] = X[(size_t) k];
    }
    fft.inverse (X);                                   // real cepstrum
    for (int nIdx = 1; nIdx < N / 2; ++nIdx) X[(size_t) nIdx] *= 2.0;
    for (int nIdx = N / 2 + 1; nIdx < N; ++nIdx) X[(size_t) nIdx] = 0.0;
    X[0] = X[0].real();
    X[(size_t) N / 2] = X[(size_t) N / 2].real();
    fft.forward (X);
    for (auto& v : X) v = std::exp (v);
    fft.inverse (X);

    res.fir.resize ((size_t) firLength);
    const int fadeStart = firLength * 3 / 4;
    for (int nIdx = 0; nIdx < firLength; ++nIdx)
    {
        double w = 1.0;
        if (nIdx >= fadeStart)
            w = 0.5 * (1.0 + std::cos (std::numbers::pi * (nIdx - fadeStart) / (firLength - fadeStart)));
        res.fir[(size_t) nIdx] = (float) (X[(size_t) nIdx].real() * w);
    }
    return res;
}

} // namespace moonmic
