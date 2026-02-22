/*
  ==============================================================================

    Tempo.h
    Created: 18 Jan 2026 4:04:22pm
    Author:  Michael Evan
 
    Note:
    Tempo utility maintains BPM
    (default 120), can be updated from juce::AudioPlayHead and converts
    note-length indices (0..15) to milliseconds. Caller must ensure
    indices are within range.
   ==============================================================================
 */
#pragma once  // ensure this header is included only once per translation unit

#include <JuceHeader.h>  // JUCE core headers (AudioPlayHead, types, etc.)

// Tempo helper: reads host BPM and converts musical note lengths to milliseconds.
class Tempo
{
public:
    void reset() noexcept;         // restore default tempo (fallback)

    void update(const juce::AudioPlayHead* playhead) noexcept; // query host playhead for BPM

    double getMillisecondsForNoteLength(int index) const noexcept; // convert note index -> ms

    double getTempo() const noexcept   // return current BPM
    {
        return bpm;
    }

private:
    double bpm = 120.0;           // stored tempo in beats-per-minute (default 120 BPM)
};
