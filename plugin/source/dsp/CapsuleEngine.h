// MOON MIC Physical Capsule Engine — realtime C++ core (used by the plugin and by research/capsule_engine_cpp).
// Mirrors research/capsule_engine (Python reference) one to one; see that package for the physics notes.
// Realtime rules: no allocation, no locks, bounded worst-case iteration count per sample.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace moonmic::capsule
{

// ---- Koren triode law ------------------------------------------------------------------------------
struct KorenTriode
{
    double mu, ex, kg1, kp, kvb;

    double ip (double vgk, double vpk) const noexcept
    {
        if (vpk <= 0.0)
            return 0.0;
        const double z = kp * (1.0 / mu + vgk / std::sqrt (kvb + vpk * vpk));
        const double softplus = z > 0.0 ? z + std::log1p (std::exp (-z)) : std::log1p (std::exp (z));
        const double e1 = vpk / kp * softplus;
        return e1 > 0.0 ? std::pow (e1, ex) / kg1 : 0.0;
    }

    // Ip and its analytic partial derivatives dIp/dVgk, dIp/dVpk
    void ipPartials (double vgk, double vpk, double& ip, double& dg, double& dp) const noexcept
    {
        ip = dg = dp = 0.0;
        if (vpk <= 0.0)
            return;
        const double q = kvb + vpk * vpk;
        const double sq = std::sqrt (q);
        const double z = kp * (1.0 / mu + vgk / sq);
        // one exp for softplus and logistic: z > 0 -> e = exp(-z); else e = exp(z)
        const double e = std::exp (-std::abs (z));
        const double sp = (z > 0.0 ? z : 0.0) + std::log1p (e);
        const double sig = z > 0.0 ? 1.0 / (1.0 + e) : e / (1.0 + e);
        const double e1 = vpk / kp * sp;
        if (e1 <= 0.0)
            return;
        const double deg = vpk * sig / sq;
        const double dep = sp / kp - sig * vgk * vpk * vpk / (q * sq);
        ip = std::exp (ex * std::log (e1)) / kg1;
        const double k = ex * ip / e1;
        dg = k * deg;
        dp = k * dep;
    }
};

// 6AU6A, triode connection: fit to GE ET-T916A p.4 (data/manufacturer/tube_6au6a_triode.json). INFERRED.
inline KorenTriode tube6AU6A() noexcept
{
    return { 37.69414562041952, 1.2716812556403965, 276.91811448694085, 249.79360560139938, 222.64561898588695 };
}

// ---- Sony C-800G MA-55 common-cathode stage (schematic 3-2 values) ------------------------------------
struct StageCircuit
{
    double bPlus = 230.0, ra = 100e3, rk = 1e3, ck = 330e-6, cc = 1e-6;
    double rLoad = 100e3;   // primary-side load: UNKNOWN (Stage 3), parameter
    double turns = 7.0;     // T101 7:1 (JAS), step-down orientation assumed
};

class CommonCathodeStage
{
public:
    static constexpr int maxIter = 40;       // bracketed fallback
    static constexpr int maxFastIter = 6;    // warm-started Newton
    static constexpr double fastTol = 1e-10; // relative step; -200 dB re the plate current

    void prepare (const KorenTriode& t, const StageCircuit& c, double sampleRate, double gridDc = 0.0) noexcept
    {
        tube = t; circ = c; gridDcV = gridDc;
        const double T = 1.0 / sampleRate;
        gk = 2.0 * c.ck / T;
        rc = T / (2.0 * c.cc);
        gb = 1.0 / (c.rLoad + rc);
        reset();
    }

    void reset() noexcept
    {
        double ip, vp, vkk;
        ipGuess = -1.0;
        solve (gridDcV, 0.0, 0.0, 0.0, 0.0, ip, vp, vkk);
        ip0 = ip; vp0 = vp; vk0 = vkk;
        vk = vkk; ick = 0.0; vc = vp; ib = 0.0;
    }

    // grid AC voltage -> secondary voltage
    double processSample (double vg) noexcept
    {
        const double jk = gk * vk + ick;
        const double vcHist = vc + rc * ib;
        double ip, vp, vkk;
        solve (vg + gridDcV, jk, gk, vcHist, gb, ip, vp, vkk);
        const double ibNew = gb * (vp - vcHist);
        ick = gk * vkk - jk;
        vk = vkk;
        vc = vcHist + rc * ibNew;
        ib = ibNew;
        return ibNew * circ.rLoad / circ.turns;
    }

    /** Signed mid-band small-signal gain grid -> secondary (bypassed cathode, coupling capacitor short).
        Negative: the common-cathode stage inverts. */
    double smallSignalGain() const noexcept
    {
        double f, fg, fp;
        tube.ipPartials (gridDcV - vk0, vp0 - vk0, f, fg, fp);
        const double rAc = 1.0 / (1.0 / circ.ra + 1.0 / circ.rLoad);
        return -fg / (fp + 1.0 / rAc) / circ.turns;
    }

    double plateCurrent0() const noexcept { return ip0; }
    double plateVoltage0() const noexcept { return vp0; }
    double cathodeVoltage0() const noexcept { return vk0; }
    int lastIterations() const noexcept { return iterations; }

private:
    void solve (double vg, double jk, double gkIn, double vcHist, double gbIn,
                double& ipOut, double& vpOut, double& vkOut) noexcept
    {
        const double ak = 1.0 / circ.rk + gkIn;
        const double ap = 1.0 / circ.ra + gbIn;
        auto nodes = [&] (double ip, double& vp, double& vkk)
        {
            vkk = (ip + jk) / ak;
            vp = (circ.bPlus / circ.ra - ip + gbIn * vcHist) / ap;
        };
        auto resid = [&] (double ip)
        {
            double vp, vkk;
            nodes (ip, vp, vkk);
            return ip - tube.ip (vg - vkk, vp - vkk);
        };

        iterations = 0;
        const double invAk = 1.0 / ak, dConst = 1.0 / ap + invAk;

        // fast path: plain Newton from the previous sample's solution (typically 2-3 iterations).
        // r(ip) is strictly increasing; any step that leaves ip > 0 or stalls falls back to the bracket.
        if (ipGuess > 0.0)
        {
            double x = ipGuess;
            for (int k = 0; k < maxFastIter; ++k)
            {
                ++iterations;
                double vp, vkk, f, fg, fp;
                nodes (x, vp, vkk);
                tube.ipPartials (vg - vkk, vp - vkk, f, fg, fp);
                const double d = 1.0 + fg * invAk + fp * dConst;
                const double xn = x - (x - f) / d;
                if (! (xn > 0.0) || ! (d > 0.0))
                    break;
                if (std::abs (xn - x) <= fastTol * xn)
                {
                    ipGuess = ipOut = xn;
                    nodes (xn, vpOut, vkOut);
                    return;
                }
                x = xn;
            }
        }

        double lo = 0.0, hi = circ.bPlus / circ.ra + std::abs (gbIn * vcHist) + 1e-3;
        if (resid (lo) >= 0.0)
        {
            ipGuess = -1.0;                       // cut off: next sample starts from the bracket again
            ipOut = 0.0; nodes (0.0, vpOut, vkOut);
            return;
        }
        for (int k = 0; k < 8 && resid (hi) <= 0.0; ++k)
            hi *= 2.0;

        double x = 0.5 * (lo + hi);
        for (int k = 0; k < maxIter; ++k)
        {
            ++iterations;
            double vp, vkk, f, fg, fp;
            nodes (x, vp, vkk);
            tube.ipPartials (vg - vkk, vp - vkk, f, fg, fp);
            const double r = x - f;
            const double d = 1.0 + fg * invAk + fp * dConst;
            if (r > 0.0) hi = x; else lo = x;
            double xn = d > 0.0 ? x - r / d : 0.5 * (lo + hi);
            if (std::abs (xn - x) <= 1e-12 * std::max (x, 1e-6)) { x = xn; break; }   // before the bracket guard
            if (! (xn > lo && xn < hi))
                xn = 0.5 * (lo + hi);
            x = xn;
        }
        ipGuess = x;
        ipOut = x;
        nodes (x, vpOut, vkOut);
    }

    KorenTriode tube {};
    StageCircuit circ {};
    double gridDcV = 0.0, gk = 0.0, rc = 0.0, gb = 0.0;
    double ip0 = 0.0, vp0 = 0.0, vk0 = 0.0;
    double vk = 0.0, ick = 0.0, vc = 0.0, ib = 0.0;
    double ipGuess = -1.0;
    int iterations = 0;
};

// ---- Constant-charge capsule (quasi-static electrostatic transduction) -------------------------------
class ConstantChargeCapsule
{
public:
    enum class Shape { piston, clamped };

    void prepare (double uPeak94, double cpRatio, Shape s) noexcept
    {
        k = uPeak94 / std::sqrt (2.0);
        cp = cpRatio;
        for (int i = 0; i < n; ++i)
        {
            const double rho = (i + 0.5) / n;
            w[(size_t) i] = s == Shape::piston ? 1.0 : 1.0 - rho * rho;
            wa[(size_t) i] = 2.0 * rho / n;
        }
        cTotal0 = capRatio (0.0) + cp;
    }

    // pressure (Pa) -> V/V0 - 1
    double processSample (double p) const noexcept
    {
        const double u = std::clamp (k * p, -0.5, 0.5);
        return cTotal0 / (capRatio (u) + cp) - 1.0;
    }

private:
    static constexpr int n = 64;
    double capRatio (double u) const noexcept
    {
        double s = 0.0;
        for (int i = 0; i < n; ++i)
            s += wa[(size_t) i] / (1.0 - u * w[(size_t) i]);
        return s;
    }
    std::array<double, n> w {}, wa {};
    double k = 0.0, cp = 0.0, cTotal0 = 1.0;
};

} // namespace moonmic::capsule
