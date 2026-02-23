/*
  ==============================================================================
    LevelMeter.cpp
    Created: 21 Jan 2026
    Author:  Michael Evan
             CIS-595 Independent Study
             UMass Dartmouth Graduate Computer Science Dept.
             Spring 2026
    Note:
    Implements the LevelMeter UI component.
    - Polls Measurement (atomic) peak values at refreshRate Hz via a Timer.
    - Applies fast attack / exponential release smoothing to linear levels.
    - Converts smoothed linear levels to dB and maps them to pixel positions.
    - Draws left/right vertical meters, tick lines and dB labels.
    - Safe to use with the audio thread updating Measurement (readAndReset is atomic).
  ==============================================================================
*/

#include <JuceHeader.h>     // JUCE core (Graphics, Timer, etc.)
#include "LevelMeter.h"     // header for this component
#include "LookAndFeel.h"    // colours and fonts used for drawing

// Constructor: bind measurement references and initialize dB display values
LevelMeter::LevelMeter(Measurement& measurementL_, Measurement& measurementR_)
    : measurementL(measurementL_), measurementR(measurementR_),
      dbLevelL(clampdB), dbLevelR(clampdB) // initialize displayed dB to clamp floor
{
    setOpaque(true);                      // component fully repaints its background
    startTimerHz(refreshRate);            // start timer to poll measurements at refreshRate Hz
    // decay factor for smoothing (derived from time constant 0.2s and timer rate)
    decay = 1.0f - std::exp(-1.0f / (float(refreshRate) * 0.2f));
}

LevelMeter::~LevelMeter()
{
    // destructor: nothing special to do (timer stops automatically on destruction)
}

// Paint the meter UI
void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();            // local drawing bounds

    g.fillAll(Colors::LevelMeter::background);       // fill background with meter background colour

    drawLevel(g, dbLevelL, 0, 7);                    // draw left channel meter at x=0, width=7
    drawLevel(g, dbLevelR, 9, 7);                    // draw right channel meter at x=9, width=7

    g.setFont(Fonts::getFont(10.0f));                // use shared font for tick labels
    // draw tick lines and labels from maxdB down to mindB in steps of stepdB
    for (float db = maxdB; db >= mindB; db -= stepdB) {
        int y = positionForLevel(db);                // compute y position for this dB value

        g.setColour(Colors::LevelMeter::tickLine);  // tick line colour
        g.fillRect(0, y, 16, 1);                    // draw horizontal tick line

        g.setColour(Colors::LevelMeter::tickLabel); // tick label colour
        g.drawSingleLineText(juce::String(int(db)), bounds.getWidth(), y + 3,
                             juce::Justification::right); // draw dB text right-aligned
    }
}

// Handle component resizing: compute pixel positions for top/bottom of meter scale
void LevelMeter::resized()
{
    maxPos = 4.0f;                       // top pixel for maxdB (4px from top)
    minPos = float(getHeight()) - 4.0f;  // bottom pixel for mindB (4px from bottom)
}

// Timer callback: poll measurements, update smoothing, and trigger repaint
void LevelMeter::timerCallback()
{
    updateLevel(measurementL.readAndReset(), levelL, dbLevelL); // get L peak and update L state
    updateLevel(measurementR.readAndReset(), levelR, dbLevelR); // get R peak and update R state

    repaint(); // schedule a paint to reflect new levels
}

// Draw a single channel's vertical level bar.
// level: dB value to plot, x: left position, width: bar width
void LevelMeter::drawLevel(juce::Graphics& g, float level, int x, int width)
{
    int y = positionForLevel(level);             // y coordinate for the top of the bar
    if (level > 0.0f) {                          // level above 0 dB -> "too loud" region present
        int y0 = positionForLevel(0.0f);         // y coordinate representing 0 dB
        g.setColour(Colors::LevelMeter::tooLoud); // color for >0 dB (red)
        g.fillRect(x, y, width, y0 - y);         // draw red portion above 0 dB
        g.setColour(Colors::LevelMeter::levelOK); // color for <=0 dB (green)
        g.fillRect(x, y0, width, getHeight() - y0); // draw green portion below 0 dB
    } else if (y < getHeight()) {                // level at or below 0 dB but within visible range
        g.setColour(Colors::LevelMeter::levelOK); // green for normal levels
        g.fillRect(x, y, width, getHeight() - y); // draw green bar from y down to bottom
    }
}

// Update smoothed linear level and compute its dB representation.
// newLevel: freshly read linear RMS/peak level (0..1), smoothedLevel: ongoing smoothed linear value,
// leveldB: output dB value (clamped).
void LevelMeter::updateLevel(float newLevel, float& smoothedLevel, float& leveldB) const
{
    if (newLevel > smoothedLevel) {
        smoothedLevel = newLevel;                 // instantaneous attack (no smoothing up)
    } else {
        smoothedLevel += (newLevel - smoothedLevel) * decay; // exponential release (smoothing down)
    }
    if (smoothedLevel > clampLevel) {             // avoid log(0) and extremely small values
        leveldB = juce::Decibels::gainToDecibels(smoothedLevel); // convert linear to dB
    } else {
        leveldB = clampdB;      // use clamp floor for near-zero levels
    }
}
