#pragma once
#include <cmath>
#include <array>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// ScaleEngine
// Given a detected frequency and a key + scale, returns the target frequency.
// ─────────────────────────────────────────────────────────────────────────────
class ScaleEngine
{
public:
    // Scale intervals in semitones (relative to key root)
    static const std::vector<int>& getIntervals(int scaleIndex)
    {
        // 0=Chromatic, 1=Major, 2=Minor, 3=Pentatonic, 4=Blues, 5=Dorian
        static const std::vector<std::vector<int>> scales = {
            { 0,1,2,3,4,5,6,7,8,9,10,11 },         // Chromatic
            { 0,2,4,5,7,9,11 },                      // Major
            { 0,2,3,5,7,8,10 },                      // Minor (Natural)
            { 0,2,4,7,9 },                            // Major Pentatonic
            { 0,3,5,6,7,10 },                         // Blues
            { 0,2,3,5,7,9,10 }                        // Dorian
        };
        return scales[scaleIndex % (int)scales.size()];
    }

    // Convert frequency to MIDI note number (float, e.g. 69.0 = A4 = 440Hz)
    static float freqToMidi(float freq)
    {
        if (freq <= 0.0f) return -1.0f;
        return 69.0f + 12.0f * std::log2(freq / 440.0f);
    }

    // Convert MIDI note to frequency
    static float midiToFreq(float midi)
    {
        return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
    }

    // Snap a MIDI note to the nearest allowed note in the scale
    static float snapToScale(float midiNote, int keyIndex, int scaleIndex)
    {
        const auto& intervals = getIntervals(scaleIndex);

        // Find the octave base
        int noteClass = (int)std::round(midiNote) % 12;
        if (noteClass < 0) noteClass += 12;
        int octave = (int)(std::round(midiNote) / 12);

        // Build allowed notes around this octave
        float bestNote = midiNote;
        float bestDist = 999.0f;

        for (int oct = octave - 1; oct <= octave + 1; ++oct)
        {
            for (int interval : intervals)
            {
                float candidate = (float)(oct * 12 + keyIndex + interval);
                float dist = std::abs(candidate - midiNote);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestNote = candidate;
                }
            }
        }
        return bestNote;
    }

    // Main entry: given detected freq, return target freq
    // Returns -1 if input is invalid.
    static float getTargetFreq(float detectedFreq, int keyIndex, int scaleIndex)
    {
        if (detectedFreq <= 0.0f) return -1.0f;

        float midiIn  = freqToMidi(detectedFreq);
        float midiOut = snapToScale(midiIn, keyIndex, scaleIndex);
        return midiToFreq(midiOut);
    }

    // Note name for display (C, C#, D, ...)
    static std::string noteName(float freq)
    {
        static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        if (freq <= 0.0f) return "--";
        float midi = freqToMidi(freq);
        int note = (int)std::round(midi) % 12;
        if (note < 0) note += 12;
        int octave = (int)(std::round(midi) / 12) - 1;
        return std::string(names[note]) + std::to_string(octave);
    }
};
