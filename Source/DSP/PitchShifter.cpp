#include "PitchShifter.h"
#include <cstring>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
PitchShifter::PitchShifter()
{
    ringBuffer.resize(RING_SIZE, 0.0f);
    hannWindow.resize(GRAIN_SIZE, 0.0f);
    outputAccum.resize(RING_SIZE, 0.0f);
    buildHannWindow();
}

void PitchShifter::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
}

void PitchShifter::reset()
{
    std::fill(ringBuffer.begin(),   ringBuffer.end(),   0.0f);
    std::fill(outputAccum.begin(),  outputAccum.end(),  0.0f);
    writeHead        = 0;
    nextGrainSample  = 0;
    currentPitchRatio = 1.0f;
    humanisePhase    = 0.0f;

    for (auto& v : voices)
    {
        v.active     = false;
        v.readHead   = 0.0;
        v.grainPhase = 0;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void PitchShifter::buildHannWindow()
{
    for (int i = 0; i < GRAIN_SIZE; ++i)
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (GRAIN_SIZE - 1)));
}

float PitchShifter::readRing(double pos) const
{
    int   i0 = (int)pos;
    float f  = (float)(pos - i0);
    int   a  = ((i0)     + RING_SIZE) % RING_SIZE;
    int   b  = ((i0 + 1) + RING_SIZE) % RING_SIZE;
    return ringBuffer[a] + f * (ringBuffer[b] - ringBuffer[a]);
}

float PitchShifter::randPhase()
{
    lcgState = lcgState * 1664525u + 1013904223u;
    return ((float)(lcgState >> 16) / 32768.0f) - 1.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Smooth pitch ratio (exponential slew)
// retuneSpeed 0 = instant (robot), 100 = slow (natural)
// ─────────────────────────────────────────────────────────────────────────────
float PitchShifter::smoothPitchRatio(float current, float target,
                                      float retuneSpeed, int numSamples)
{
    // Map speed 0-100 → time constant in samples
    // speed=0  → ~8 ms (robot), speed=100 → ~300 ms (natural)
    float msTarget = 8.0f + retuneSpeed * 2.92f;          // 8 … 300 ms
    float tc       = msTarget * 0.001f * (float)sampleRate;
    float alpha    = 1.0f - std::exp(-(float)numSamples / tc);
    return current + alpha * (target - current);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main processing — mono, in-place
// ─────────────────────────────────────────────────────────────────────────────
void PitchShifter::processMono(float* buffer, int numSamples,
                                float pitchRatio, float retuneSpeed, float humanize)
{
    // Smooth pitch ratio
    currentPitchRatio = smoothPitchRatio(currentPitchRatio, pitchRatio,
                                          retuneSpeed, numSamples);

    // Humanize: subtle LFO-style pitch wobble (like a real singer)
    float humanizeAmt = humanize / 100.0f * 0.004f;   // max ±0.4%
    humanisePhase += (float)numSamples * 5.0f / (float)sampleRate; // 5 Hz LFO
    if (humanisePhase > 2.0f * 3.14159265f) humanisePhase -= 2.0f * 3.14159265f;
    float humanizeMod = 1.0f + humanizeAmt * std::sin(humanisePhase + randPhase() * 0.1f);
    float effectiveRatio = currentPitchRatio * humanizeMod;

    // ── Write input into ring buffer ──────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        ringBuffer[writeHead] = buffer[i];
        writeHead = (writeHead + 1) % RING_SIZE;
    }

    // Clear output buffer for this block
    std::fill(buffer, buffer + numSamples, 0.0f);

    // ── Grain scheduling ──────────────────────────────────────────────────
    // Trigger a new grain every HOP_SIZE samples
    // Each grain reads from the ring at ratio-adjusted speed

    for (int s = 0; s < numSamples; ++s)
    {
        // Trigger a new grain if needed
        if (nextGrainSample <= 0)
        {
            nextGrainSample = HOP_SIZE;

            // Find an idle voice
            for (auto& v : voices)
            {
                if (!v.active)
                {
                    // Start reading from 'now' in the ring (minus latency compensation)
                    v.readHead   = (double)((writeHead - numSamples + s - GRAIN_SIZE / 2 + RING_SIZE) % RING_SIZE);
                    v.grainPhase = 0;
                    v.active     = true;
                    break;
                }
            }
        }
        --nextGrainSample;

        // ── Synthesise all active voices ──────────────────────────────────
        float out = 0.0f;
        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Read sample at read head with linear interpolation
            float sample = readRing(v.readHead);

            // Apply Hanning window
            float win = hannWindow[v.grainPhase];
            out += sample * win;

            // Advance read head by pitch ratio
            v.readHead += (double)effectiveRatio;
            if (v.readHead >= RING_SIZE) v.readHead -= RING_SIZE;

            ++v.grainPhase;
            if (v.grainPhase >= GRAIN_SIZE)
                v.active = false;
        }

        // Normalise overlap (NUM_VOICES grains overlap at any time)
        buffer[s] = out / (float)NUM_VOICES;
    }
}
