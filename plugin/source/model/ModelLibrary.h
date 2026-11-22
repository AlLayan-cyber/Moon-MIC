#pragma once

#include <string>
#include <vector>

namespace moonmic
{

/** One measured microphone configuration (model + polar pattern) at the reference distance r0.
    Data source and processing: tools/export/export_signature_models.py (format MMIC v1). */
struct MicSet
{
    std::string id;           // e.g. "u87ai_cardioid"
    std::string displayName;  // dev build label (trademark use pending legal review)
    std::string pattern;      // Cardioid / Omni / Bidirect / Hyper / fixed
    float lfLimitHz = 0.0f;   // below this the data is held (real LF roll-off UNKNOWN)

    std::vector<float> magDb;    // [numAngles * numFreqs]
    std::vector<float> phaseRad; // [numAngles * numFreqs], unwrapped along frequency
    std::vector<float> bRe, bIm; // first-order gradient component b(f), [numFreqs]
};

/** Immutable library of SIGNATURE models parsed from the embedded MMIC binary. */
class ModelLibrary
{
public:
    /** Parses an MMIC v1 blob. Returns false (and leaves the library empty) on any format error. */
    bool load (const void* data, size_t numBytes, std::string& error);

    /** Loads the model data embedded in the binary (MoonMicData). */
    bool loadEmbedded (std::string& error);

    int numSets() const noexcept                          { return (int) sets.size(); }
    const MicSet& set (int index) const                   { return sets[(size_t) index]; }
    int indexOf (const std::string& id) const noexcept;

    const std::vector<float>& frequencies() const noexcept { return freqs; }
    int numAngles() const noexcept                        { return (int) anglesDeg.size(); }
    float referenceDistance() const noexcept              { return r0; }
    float speedOfSound() const noexcept                   { return cSound; }

private:
    std::vector<MicSet> sets;
    std::vector<float> freqs, anglesDeg;
    float r0 = 0.5f, cSound = 343.0f;
};

} // namespace moonmic
