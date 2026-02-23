/*
  ==============================================================================
    LevelMeter.h
    Created: 21 Jan 2026
    Michael J Evan
    UMass Dartmouth
    Graduate Computer Science Department
    CIS-595 Independent Study
    Spring 2026

    Simple UI component
    Displays L/R level meters
    Driven by Measurement objects.
  ==============================================================================
*/

#pragma once                                       // include guard

#include <JuceHeader.h>                            // JUCE core GUI/audio includes
#include "Measurement.h"                           // thread-safe peak measurement helper

// LevelMeter: a Component that polls Measurements and draws vertical bar meters.
// Inherits juce::Timer privately to poll at a fixed refresh rate.
class LevelMeter  : public juce::Component, private juce::Timer
{
public:
    // Constructor takes references to the left and right Measurement objects (no ownership)
    LevelMeter(Measurement& measurementL, Measurement& measurementR);
    ~LevelMeter() override;       // destructor (virtual via override)

    void paint (juce::Graphics&) override;         // paint the meter graphics
    void resized() override;     // handle layout changes

private:
    void timerCallback() override;                 // Timer callback to poll measurements periodically

    // Convert dB level to a pixel position
    // Used to map dB -> vertical meter coordinate
    int positionForLevel(float dbLevel) const noexcept
    {
        return int(std::round(juce::jmap(dbLevel, maxdB, mindB, maxPos, minPos)));
    }

    void drawLevel(juce::Graphics& g, float level, int x, int width); // draw one channel's meter
    
    // Update smoothed linear level and compute dB value for rendering
    void updateLevel(float newLevel, float& smoothedLevel, float& leveldB) const;

    Measurement& measurementL;   // reference to left-channel measurement (audio thread writes)
    Measurement& measurementR;  // reference to right-channel measurement

    // dB range shown on the meter
    static constexpr float maxdB = 6.0f;           // top of scale (0 dB is typical, +6 allows headroom)
    static constexpr float mindB = -60.0f;         // bottom of visible scale
    static constexpr float stepdB = 6.0f;          // tick step in dB for scale markings

    float maxPos = 0.0f;                           // pixel position representing maxdB (set in resized)
    float minPos = 0.0f;                           // pixel position representing mindB (set in resized)

    static constexpr float clampdB = -120.0f;      // clamp floor used for very small values
    static constexpr float clampLevel = 0.000001f; // linear level corresponding to -120 dB

    float dbLevelL;                                // last computed dB for left channel
    float dbLevelR;                                // last computed dB for right channel

    static constexpr int refreshRate = 60;         // UI refresh rate in Hz for the timer

    float decay = 0.0f;                    // per-frame decay used in smoothing logic
    float levelL = clampLevel;          // smoothed linear level left (starts near silence)
    float levelR = clampLevel;         // smoothed linear level right

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter) // disable copy, enable leak detection
};
