#pragma once
#include <vector>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// PitchDetector
// Implements the YIN algorithm for reliable monophonic pitch detection.
// Designed for vocal range (70 Hz – 1100 Hz).
// ─────────────────────────────────────────────────────────────────────────────
class PitchDetector
{
public:
    PitchDetector();

    void prepare(double sampleRate);
    void reset();

    // Feed new audio samples. Returns detected frequency in Hz, or -1 if
    // silence / undetected.
    float process(const float* samples, int numSamples);

    // RMS of last block (0-1), for meter display
    float getRMS() const { return lastRMS; }

private:
    static constexpr int   ANALYSIS_SIZE  = 2048;
    static constexpr float YIN_THRESHOLD  = 0.15f;
    static constexpr float NOISE_GATE_RMS = 0.008f;
    static constexpr float VOCAL_MIN_HZ   = 70.0f;
    static constexpr float VOCAL_MAX_HZ   = 1100.0f;

    double sampleRate = 44100.0;
    float  lastRMS    = 0.0f;

    // Internal analysis ring-buffer
    std::vector<float> analysisBuffer;
    int                writePos = 0;
    int                fillCount = 0;

    // YIN d' buffer
    std::vector<float> yinBuffer;

    // Run YIN on current analysis buffer. Returns freq or -1.
    float runYIN();

    // Cumulative mean normalised difference
    void  cmndf(const float* buf, int halfSize);

    // Parabolic interpolation around tau minimum
    float parabolicInterp(int tau, int halfSize) const;
};
