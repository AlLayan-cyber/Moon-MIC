#include "ModelLibrary.h"

#include <MoonMicData.h>
#include <cstdint>
#include <cstring>

namespace moonmic
{

namespace
{
    struct Reader
    {
        const unsigned char* p;
        size_t remaining;

        bool bytes (void* dst, size_t n)
        {
            if (n > remaining) return false;
            std::memcpy (dst, p, n);
            p += n;
            remaining -= n;
            return true;
        }

        bool u32 (uint32_t& v)  { return bytes (&v, 4); }   // little-endian host assumed (x86/ARM)
        bool f32 (float& v)     { return bytes (&v, 4); }

        bool floats (std::vector<float>& v, size_t n)
        {
            v.resize (n);
            return bytes (v.data(), n * sizeof (float));
        }

        bool str (std::string& s)
        {
            uint32_t n = 0;
            if (! u32 (n) || n > 4096) return false;
            s.resize (n);
            return bytes (s.data(), n);
        }
    };
}

bool ModelLibrary::load (const void* data, size_t numBytes, std::string& error)
{
    sets.clear();
    Reader r { static_cast<const unsigned char*> (data), numBytes };

    char magic[4] {};
    uint32_t version = 0, nSets = 0, nFreq = 0, nAngles = 0;

    if (! r.bytes (magic, 4) || std::memcmp (magic, "MMIC", 4) != 0) { error = "bad magic"; return false; }
    if (! r.u32 (version) || version != 1)                           { error = "unsupported version"; return false; }
    if (! r.u32 (nSets) || ! r.u32 (nFreq) || ! r.u32 (nAngles)
        || ! r.f32 (r0) || ! r.f32 (cSound))                          { error = "truncated header"; return false; }
    if (nSets == 0 || nFreq < 2 || nAngles != 72)                     { error = "invalid dimensions"; return false; }
    if (! r.floats (freqs, nFreq) || ! r.floats (anglesDeg, nAngles)) { error = "truncated grid"; return false; }

    std::vector<MicSet> parsed (nSets);
    for (auto& s : parsed)
    {
        if (! r.str (s.id) || ! r.str (s.displayName) || ! r.str (s.pattern) || ! r.f32 (s.lfLimitHz)
            || ! r.floats (s.magDb, (size_t) nAngles * nFreq)
            || ! r.floats (s.phaseRad, (size_t) nAngles * nFreq)
            || ! r.floats (s.bRe, nFreq) || ! r.floats (s.bIm, nFreq))
        {
            error = "truncated set data";
            return false;
        }
    }

    if (r.remaining != 0) { error = "trailing bytes"; return false; }

    sets = std::move (parsed);
    return true;
}

bool ModelLibrary::loadEmbedded (std::string& error)
{
    return load (MoonMicData::moonmic_signature_v1_bin,
                 (size_t) MoonMicData::moonmic_signature_v1_binSize, error);
}

int ModelLibrary::indexOf (const std::string& id) const noexcept
{
    for (size_t i = 0; i < sets.size(); ++i)
        if (sets[i].id == id)
            return (int) i;
    return -1;
}

} // namespace moonmic
