#pragma once

#include <complex>
#include <vector>

namespace moonmic
{

/** Small deterministic radix-2 complex FFT used for offline filter design (not on the audio thread).
    Forward: X[k] = sum x[n] e^{-j2pi kn/N}. Inverse includes the 1/N scale. */
class SimpleFFT
{
public:
    explicit SimpleFFT (int order);

    int size() const noexcept { return n; }
    void forward (std::vector<std::complex<double>>& data) const { transform (data, false); }
    void inverse (std::vector<std::complex<double>>& data) const { transform (data, true); }

private:
    void transform (std::vector<std::complex<double>>& data, bool inverse) const;

    int n;
    std::vector<int> bitrev;
    std::vector<std::complex<double>> twiddles; // e^{-j2pi k/N}, k < N/2
};

} // namespace moonmic
