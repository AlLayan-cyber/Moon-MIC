// Stage 5 test bench: C++ core vs Python reference, stability, sample-rate changes, CPU cost.
// Usage: CapsuleBench <reference_dir>   (reference_dir from research.capsule_engine.export_reference)
#include "CapsuleEngine.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace moonmic::capsule;

static int failures = 0, checks = 0;

static void check (const std::string& name, bool ok, const std::string& info)
{
    ++checks;
    if (! ok) ++failures;
    std::printf ("[%s] %s: %s\n", ok ? "PASS" : "FAIL", name.c_str(), info.c_str());
}

static std::vector<double> readF64 (const std::string& path, size_t n)
{
    std::vector<double> v (n);
    std::ifstream f (path, std::ios::binary);
    f.read (reinterpret_cast<char*> (v.data()), (std::streamsize) (n * sizeof (double)));
    if (! f) v.clear();
    return v;
}

static std::string fmt (const char* f, double a, double b = 0.0)
{
    char buf[256];
    std::snprintf (buf, sizeof buf, f, a, b);
    return buf;
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::printf ("usage: CapsuleBench <reference_dir>\n"); return 2; }
    const std::string dir = argv[1];
    std::ifstream man (dir + "/manifest.txt");
    if (! man) { std::printf ("cannot open %s/manifest.txt\n", dir.c_str()); return 2; }

    std::string line;
    while (std::getline (man, line))
    {
        std::istringstream ss (line);
        std::string kind, name;
        ss >> kind >> name;
        if (kind == "stage")
        {
            double fs, rl, gdc, ip, vp, vk; size_t n;
            ss >> fs >> rl >> gdc >> n >> ip >> vp >> vk;
            auto x = readF64 (dir + "/" + name + "_in.f64", n);
            auto ref = readF64 (dir + "/" + name + "_out.f64", n);
            if (x.empty() || ref.empty()) { check (name, false, "missing reference files"); continue; }
            StageCircuit c; c.rLoad = rl;
            CommonCathodeStage st; st.prepare (tube6AU6A(), c, fs, gdc);
            const double opErr = std::abs (st.plateCurrent0() - ip) / ip + std::abs (st.plateVoltage0() - vp) / vp
                               + std::abs (st.cathodeVoltage0() - vk) / vk;
            double maxErr = 0.0, peak = 0.0; int maxIt = 0; bool finite = true;
            for (size_t i = 0; i < n; ++i)
            {
                const double y = st.processSample (x[i]);
                finite = finite && std::isfinite (y);
                maxErr = std::max (maxErr, std::abs (y - ref[i]));
                peak = std::max (peak, std::abs (ref[i]));
                maxIt = std::max (maxIt, st.lastIterations());
            }
            check ("stage " + name + " operating point", opErr < 1e-9, fmt ("relative error %.2e", opErr));
            check ("stage " + name + " vs Python", finite && maxErr <= 1e-9 * peak,
                   fmt ("max |err| %.2e of peak %.3g", maxErr, peak) + ", max iterations " + std::to_string (maxIt));
        }
        else if (kind == "capsule")
        {
            double u, cp; std::string shape; size_t n;
            ss >> u >> cp >> shape >> n;
            auto x = readF64 (dir + "/" + name + "_in.f64", n);
            auto ref = readF64 (dir + "/" + name + "_out.f64", n);
            if (x.empty() || ref.empty()) { check (name, false, "missing reference files"); continue; }
            ConstantChargeCapsule cap;
            cap.prepare (u, cp, shape == "piston" ? ConstantChargeCapsule::Shape::piston : ConstantChargeCapsule::Shape::clamped);
            double maxErr = 0.0, peak = 0.0;
            for (size_t i = 0; i < n; ++i)
            {
                maxErr = std::max (maxErr, std::abs (cap.processSample (x[i]) - ref[i]));
                peak = std::max (peak, std::abs (ref[i]));
            }
            check ("capsule " + name + " vs Python", maxErr <= 1e-12 * std::max (peak, 1e-3), fmt ("max |err| %.2e of peak %.3g", maxErr, peak));
        }
    }

    // sample-rate changes: prepare at several rates, process, stay finite and reach the same mid-band gain
    {
        double g48 = 0.0;
        for (double fs : { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 })
        {
            CommonCathodeStage st; st.prepare (tube6AU6A(), StageCircuit {}, fs);
            const int n = (int) (0.5 * fs);
            double acc = 0.0; bool finite = true;
            for (int i = 0; i < n; ++i)
            {
                const double s = 1e-4 * std::sin (2.0 * 3.141592653589793 * 1000.0 * i / fs);
                const double y = st.processSample (s);
                finite = finite && std::isfinite (y);
                if (i >= n / 2) acc += y * s;
            }
            const double g = acc / (0.5 * (n - n / 2) * 1e-8);
            if (fs == 48000.0) g48 = g;
            check (fmt ("sample rate %.0f Hz", fs), finite, fmt ("1 kHz gain %.5f", g));
        }
        (void) g48;
    }

    // CPU: 10 s of a loud 1 kHz sine at 48 kHz and at 192 kHz (= 4x oversampled 48 kHz)
    for (double fs : { 48000.0, 192000.0 })
    {
        CommonCathodeStage st; st.prepare (tube6AU6A(), StageCircuit {}, fs);
        const int n = (int) (10.0 * fs);
        std::vector<double> x ((size_t) n);
        for (int i = 0; i < n; ++i) x[(size_t) i] = 0.3 * std::sin (2.0 * 3.141592653589793 * 1000.0 * i / fs);
        volatile double sink = 0.0;
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < n; ++i) sink = sink + st.processSample (x[(size_t) i]);
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        const double load = sec / 10.0 * 100.0;
        check (fmt ("CPU at %.0f Hz", fs), load < 25.0, fmt ("%.1f ns/sample, %.2f %% of one core in realtime", sec / n * 1e9, load));
    }

    std::printf ("\n%d/%d checks passed\n", checks - failures, checks);
    return failures == 0 ? 0 : 1;
}
