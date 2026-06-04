#pragma once
#include <vector>
#include <array>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// PitchShifter
// Granular synthesis pitch shifter with Hanning-windowed grains.
// Inspired by the approach in bemtorres/opentune (MIT).
// Zero-latency, lock-free, real-time safe.
// ─────────────────────────────────────────────────────────────────────────────
class PitchShifter
{
public:
    PitchShifter();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process a mono channel in-place.
    // pitchRatio : target/current frequency ratio (e.g. 1.05 = +1 semitone approx)
    // retuneSpeed: 0=robot (instant), 100=slow/natural
    // humanize   : 0-100, adds subtle pitch randomisation
    void processMono(float* buffer, int numSamples,
                     float pitchRatio, float retuneSpeed, float humanize);

    // Smooth the current pitch ratio toward target (called per block)
    float smoothPitchRatio(float current, float target, float retuneSpeed, int numSamples);

private:
    // Grain parameters
    static constexpr int GRAIN_SIZE  = 2048;          // samples per grain
    static constexpr int HOP_SIZE    = GRAIN_SIZE / 4; // grain spacing
    static constexpr int RING_SIZE   = GRAIN_SIZE * 8; // ring buffer size
    static constexpr int NUM_VOICES  = 2;              // overlapping grains

    struct Voice
    {
        double readHead  = 0.0;  // fractional read position in ring
        int    grainPhase = 0;   // sample position inside current grain
        bool   active    = false;
    };

    std::array<Voice, NUM_VOICES> voices;

    std::vector<float> ringBuffer;   // input ring buffer
    std::vector<float> hannWindow;   // pre-computed Hanning window
    std::vector<float> outputAccum;  // overlap-add accumulator

    int    writeHead       = 0;
    int    nextGrainSample = 0;

    double sampleRate         = 44100.0;
    float  currentPitchRatio  = 1.0f;
    float  humanisePhase      = 0.0f;

    float  readRing(double pos) const;
    void   buildHannWindow();
    float  randPhase();          // cheap LCG random -1..1
    uint32_t lcgState = 12345u;
};
