#include "SimpleFFT.h"

#include <cassert>
#include <cmath>
#include <numbers>

namespace moonmic
{

SimpleFFT::SimpleFFT (int order) : n (1 << order)
{
    bitrev.resize ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        int r = 0;
        for (int b = 0; b < order; ++b)
            if (i & (1 << b))
                r |= 1 << (order - 1 - b);
        bitrev[(size_t) i] = r;
    }

    twiddles.resize ((size_t) n / 2);
    for (int k = 0; k < n / 2; ++k)
        twiddles[(size_t) k] = std::polar (1.0, -2.0 * std::numbers::pi * k / n);
}

void SimpleFFT::transform (std::vector<std::complex<double>>& x, bool inverse) const
{
    assert ((int) x.size() == n);

    for (int i = 0; i < n; ++i)
        if (i < bitrev[(size_t) i])
            std::swap (x[(size_t) i], x[(size_t) bitrev[(size_t) i]]);

    for (int len = 2; len <= n; len <<= 1)
    {
        const int half = len / 2, step = n / len;
        for (int start = 0; start < n; start += len)
            for (int k = 0; k < half; ++k)
            {
                auto w = twiddles[(size_t) (k * step)];
                if (inverse) w = std::conj (w);
                const auto u = x[(size_t) (start + k)];
                const auto v = x[(size_t) (start + k + half)] * w;
                x[(size_t) (start + k)] = u + v;
                x[(size_t) (start + k + half)] = u - v;
            }
    }

    if (inverse)
        for (auto& v : x)
            v /= (double) n;
}

} // namespace moonmic
