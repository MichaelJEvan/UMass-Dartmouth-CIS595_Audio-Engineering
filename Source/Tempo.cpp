/*
==============================================================================
     Tempo.cpp
     Created: 18 Jan 2026 4:05:17pm
     Author:  Michael J Evan
              CIS-595 Independent Study
              UMass Dartmouth
              Graduate Computer Science Dept
              Spring 2026
    Note:
    Provides a table of 16 note-length multipliers (indices 0..15) ...
    computes milliseconds per note given the current BPM. BPM defaults
    to 120 and is updated from  juce::AudioPlayHead when available
    from the host DAW.

    Use: getMillisecondsForNoteLength(index) — array index must be in [0,15].
==============================================================================
 */

#include "Tempo.h"

// Varies note representations below
// Each is a multiplier expressed in unites of quarter-notes (beats)
// The code computes milliseconds as: ms = 60000.0 * multiplier / bpm.
// (60000 / bpm = milliseconds per quarter note)
// Multiplying by the multiplier gives the ms for the chosen note length.

static std::array<double, 16> noteLengthMultipliers =
{
    0.125,        //  0 = 1/32 note
    0.5 / 3.0,    //  1 = 1/16 triplet
    0.1875,       //  2 = 1/32 dotted
    0.25,         //  3 = 1/16 note
    1.0 / 3.0,    //  4 = 1/8 triplet
    0.375,        //  5 = 1/16 dotted
    0.5,          //  6 = 1/8 note
    2.0 / 3.0,    //  7 = 1/4 triplet
    0.75,         //  8 = 1/8 dotted
    1.0,          //  9 = 1/4 note
    4.0 / 3.0,    // 10 = 1/2 triplet
    1.5,          // 11 = 1/4 dotted
    2.0,          // 12 = 1/2 note
    8.0 / 3.0,    // 13 = 1/1 triplet
    3.0,          // 14 = 1/2 dotted
    4.0,          // 15 = 1/1 = whole note
};

void Tempo::reset() noexcept
{
    bpm = 120.0;                // set default tempo to 120 BPM (fallback/initial value)
}

void Tempo::update(const juce::AudioPlayHead* playhead) noexcept
{
    reset();                    // start from default; if playhead provides BPM we'll override it

    if (playhead == nullptr) {  // if host didn't provide a playhead pointer, nothing to do
        return;
    }

    const auto opt = playhead->getPosition(); // ask the host for current transport/position info
    if (!opt.hasValue()) {                    // if host didn't return valid position data, bail out
        return;
    }

    const auto& pos = *opt;                   // dereference the optional PositionInfo

    if (pos.getBpm().hasValue()) {     // if the PositionInfo includes a BPM value...
        bpm = *pos.getBpm();                 // ...use it to update our internal tempo
    }
}

double Tempo::getMillisecondsForNoteLength(int index) const noexcept
{
    // Convert a musical note-length (via noteLengthMultipliers[index]) to milliseconds:
    // 60000 ms per minute * multiplier / bpm = delay time in ms for that note value.
    // Note: this uses the global noteLengthMultipliers array; there is no bounds check here,
    // so must ensure 'index' is in the 0..15 range.
    return 60000.0 * noteLengthMultipliers[size_t(index)] / bpm;
}
