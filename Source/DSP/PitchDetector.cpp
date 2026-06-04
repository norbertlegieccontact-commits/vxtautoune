#include "PitchDetector.h"
#include <cmath>
#include <algorithm>
#include <numeric>

PitchDetector::PitchDetector()
{
    analysisBuffer.resize(ANALYSIS_SIZE, 0.0f);
    yinBuffer.resize(ANALYSIS_SIZE / 2, 0.0f);
}

void PitchDetector::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void PitchDetector::reset()
{
    std::fill(analysisBuffer.begin(), analysisBuffer.end(), 0.0f);
    writePos  = 0;
    fillCount = 0;
    lastRMS   = 0.0f;
}

float PitchDetector::process(const float* samples, int numSamples)
{
    // ── Compute RMS ───────────────────────────────────────────────────────
    float sumSq = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        sumSq += samples[i] * samples[i];
    lastRMS = std::sqrt(sumSq / (float)numSamples);

    // Noise gate
    if (lastRMS < NOISE_GATE_RMS)
        return -1.0f;

    // ── Fill ring buffer ──────────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        analysisBuffer[writePos] = samples[i];
        writePos = (writePos + 1) % ANALYSIS_SIZE;
        if (fillCount < ANALYSIS_SIZE) ++fillCount;
    }

    // Don't analyse until we have enough data
    if (fillCount < ANALYSIS_SIZE)
        return -1.0f;

    return runYIN();
}

// ─────────────────────────────────────────────────────────────────────────────
// YIN pitch estimation
// ─────────────────────────────────────────────────────────────────────────────
void PitchDetector::cmndf(const float* buf, int halfSize)
{
    // Step 1: Difference function + CMNDF in one pass
    yinBuffer[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau < halfSize; ++tau)
    {
        float d = 0.0f;
        for (int i = 0; i < halfSize; ++i)
        {
            float diff = buf[i] - buf[i + tau];
            d += diff * diff;
        }
        runningSum += d;
        yinBuffer[tau] = (runningSum > 0.0f) ? (d * tau / runningSum) : 0.0f;
    }
}

float PitchDetector::parabolicInterp(int tau, int halfSize) const
{
    if (tau <= 0 || tau >= halfSize - 1)
        return static_cast<float>(tau);

    float s0 = yinBuffer[tau - 1];
    float s1 = yinBuffer[tau];
    float s2 = yinBuffer[tau + 1];

    float denom = 2.0f * (s0 - 2.0f * s1 + s2);
    if (std::abs(denom) < 1e-8f)
        return static_cast<float>(tau);

    return static_cast<float>(tau) + (s0 - s2) / denom;
}

float PitchDetector::runYIN()
{
    // Copy ring buffer into a flat array for analysis
    std::vector<float> flat(ANALYSIS_SIZE);
    for (int i = 0; i < ANALYSIS_SIZE; ++i)
        flat[i] = analysisBuffer[(writePos + i) % ANALYSIS_SIZE];

    const int halfSize = ANALYSIS_SIZE / 2;
    cmndf(flat.data(), halfSize);

    // ── Find first dip below threshold ────────────────────────────────────
    int tau = 2;
    while (tau < halfSize)
    {
        if (yinBuffer[tau] < YIN_THRESHOLD)
        {
            // Descend to local minimum
            while (tau + 1 < halfSize && yinBuffer[tau + 1] < yinBuffer[tau])
                ++tau;
            break;
        }
        ++tau;
    }

    if (tau >= halfSize)
        return -1.0f;   // No pitch found

    // ── Parabolic interpolation ───────────────────────────────────────────
    float refinedTau = parabolicInterp(tau, halfSize);
    if (refinedTau <= 0.0f)
        return -1.0f;

    float freq = static_cast<float>(sampleRate) / refinedTau;

    // ── Vocal range gate ─────────────────────────────────────────────────
    if (freq < VOCAL_MIN_HZ || freq > VOCAL_MAX_HZ)
        return -1.0f;

    return freq;
}
