/*
  ==============================================================================
    Parameters.cpp
    Created: 18 Dec 2025 11:40:41am
    Author:  Michael Evan
             CIS-595 Independent Study
             UMass Dartmouth
             Graduate Computer Science Dept
             Spring 2026

    Implementation of the Parameters helper:
    - looks up APVTS parameters
    - provides string formatting/parsing for the UI
    - maintains runtime copies of parameter values and smoothers
  ==============================================================================
*/

#include "Parameters.h"

//"DSP.h"
//header-only helper that computes equal‑power
//panning gains for left and right channels.
#include "DSP.h"


// Helper template to safely cast an APVTS parameter to the expected type.
// Asserts in debug builds if the parameter is missing or the type is wrong.
template<typename T>
static void castParameter(juce::AudioProcessorValueTreeState& apvts,
                          const juce::ParameterID& id, T& destination)
{
    destination = dynamic_cast<T>(apvts.getParameter(id.getParamID()));
    jassert(destination);  // parameter does not exist or wrong type
}

// Formatting function for delay-time display in the UI.
// Chooses ms (milliseconds) or s (seconds) and formats with appropriate # of decimals.
static juce::String stringFromMilliseconds(float value, int)
{
    if (value < 10.0f) {
        return juce::String(value, 2) + " ms";
    } else if (value < 100.0f) {
        return juce::String(value, 1) + " ms";
    } else if (value < 1000.0f) {
        return juce::String(int(value)) + " ms";
    } else {
        return juce::String(value * 0.001f, 2) + " s";
    }
}

// Parse user-entered delay-time strings: accepts "s" suffix (seconds) or plain numbers (ms).
// Also treats small numbers as seconds if below minDelayTime.
static float millisecondsFromString(const juce::String& text)
{
    float value = text.getFloatValue();
    if (!text.endsWithIgnoreCase("ms")) {
        if (text.endsWithIgnoreCase("s") || value < Parameters::minDelayTime) {
            return value * 1000.0f;
        }
    }
    return value;
}

// Simple formatting helpers for dB, percent, and Hz parameters etc... used by sliders/textboxes.
static juce::String stringFromDecibels(float value, int)
{
    return juce::String(value, 1) + " dB";
}

static juce::String stringFromPercent(float value, int)
{
    return juce::String(int(value)) + " %";
}

static juce::String stringFromHz(float value, int)
{
    if (value < 1000.0f) {
        return juce::String(int(value)) + " Hz";
    } else if (value < 10000.0f) {
        return juce::String(value / 1000.0f, 2) + " k";
    } else {
        return juce::String(value / 1000.0f, 1) + " k";
    }
}

// Parse a frequency string; values < 20 are assumed to be kHz shorthand (e.g. "20" -> 20000).
static float hzFromString(const juce::String& str)
{
    float value = str.getFloatValue();
    if (value < 20.0f) {
        return value * 1000.0f;
    }
    return value;
}

// Constructor: locate and bind APVTS parameters to internal pointers.
// Uses castParameter to assert on missing/wrong-type parameters at startup.
Parameters::Parameters(juce::AudioProcessorValueTreeState& apvts)
{
    castParameter(apvts, gainParamID, gainParam);
    castParameter(apvts, delayTimeParamID, delayTimeParam);
    castParameter(apvts, mixParamID, mixParam);
    castParameter(apvts, feedbackParamID, feedbackParam);
    castParameter(apvts, stereoParamID, stereoParam);
    castParameter(apvts, lowCutParamID, lowCutParam);
    castParameter(apvts, highCutParamID, highCutParam);
    castParameter(apvts, tempoSyncParamID, tempoSyncParam);
    castParameter(apvts, delayNoteParamID, delayNoteParam);
}

// Build the APVTS parameter layout: defines parameters (IDs, names, ranges, defaults)
// and attaches user-facing string formatting / parsing where useful.
juce::AudioProcessorValueTreeState::ParameterLayout Parameters::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Output gain in dB (-12..+12), displayed using dB formatter.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        gainParamID,
        "Output Gain",
        juce::NormalisableRange<float> { -12.0f, 12.0f },
        0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromDecibels)
    ));

    // Delay time in milliseconds with custom text parsing/formatting (ms / s).
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        delayTimeParamID,
        "Delay Time",
        juce::NormalisableRange<float> { minDelayTime, maxDelayTime, 0.001f, 0.25f },
        100.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(stringFromMilliseconds)
            .withValueFromStringFunction(millisecondsFromString)
    ));

    // Mix and feedback expressed as 0..100 (percent) parameters in the UI.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        mixParamID,
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        100.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        feedbackParamID,
        "Feedback",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)
    ));

    // Stereo/panning control shown as -100..100%
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        stereoParamID,
        "Stereo",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction(stringFromPercent)
    ));

    // Low cut and high cut frequency parameters with human-readable formatting
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        lowCutParamID,
        "Low Cut",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(stringFromHz)
            .withValueFromStringFunction(hzFromString)
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        highCutParamID,
        "High Cut",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20000.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction(stringFromHz)
            .withValueFromStringFunction(hzFromString)
    ));

    // Simple boolean toggle for tempo sync
    layout.add(std::make_unique<juce::AudioParameterBool>(
        tempoSyncParamID, "Tempo Sync", false));

    // Choice list for note subdivisions used when tempoSync is active.
    juce::StringArray noteLengths = {
        "1/32",
        "1/16 trip",
        "1/32 dot",
        "1/16",
        "1/8 trip",
        "1/16 dot",
        "1/8",
        "1/4 trip",
        "1/8 dot",
        "1/4",
        "1/2 trip",
        "1/4 dot",
        "1/2",
        "1/1 trip",
        "1/2 dot",
        "1/1",
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        delayNoteParamID, "Delay Note", noteLengths, 9)); // default index = 9 (1/4)

    return layout;
}

// prepareToPlay: initialize smoothers and compute the one-pole coefficient used
// by the custom smoothing implemented for delayTime.
void Parameters::prepareToPlay(double sampleRate) noexcept
{
    double duration = 0.02; // smoother ramp time for linear smoothers (20 ms)
    gainSmoother.reset(sampleRate, duration);

    // coeff approximates a one-pole smoother; smaller when sampleRate is higher.
    coeff = 1.0f - std::exp(-1.0f / (0.2f * float(sampleRate)));

    // initialize other linear smoothers with the same ramp duration
    mixSmoother.reset(sampleRate, duration);
    feedbackSmoother.reset(sampleRate, duration);
    stereoSmoother.reset(sampleRate, duration);
    lowCutSmoother.reset(sampleRate, duration);
    highCutSmoother.reset(sampleRate, duration);
}

// reset: set runtime values to sensible defaults and prime smoothers with current APVTS values.
void Parameters::reset() noexcept
{
    gain = 0.0f;
    gainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));

    delayTime = 0.0f; // targetDelayTime will be initialized on the first update

    mix = 1.0f;
    mixSmoother.setCurrentAndTargetValue(mixParam->get() * 0.01f); // UI is 0..100 -> 0..1

    feedback = 0.0f;
    feedbackSmoother.setCurrentAndTargetValue(feedbackParam->get() * 0.01f);

    panL = 0.0f; // equal-power panning defaults: left=0, right=1
    panR = 1.0f;
    stereoSmoother.setCurrentAndTargetValue(stereoParam->get() * 0.01f);

    lowCut = 20.0f;
    lowCutSmoother.setCurrentAndTargetValue(lowCutParam->get());

    highCut = 20000.0f;
    highCutSmoother.setCurrentAndTargetValue(highCutParam->get());
}

// update: called (typically at block start) to read raw APVTS values and set targets
// for the smoothers. Does not advance smoothers — smoothen() does that per-sample.
void Parameters::update() noexcept
{
    gainSmoother.setTargetValue(juce::Decibels::decibelsToGain(gainParam->get()));

    // raw target delay time read from parameter; if delayTime is uninitialized (0)
    // we set it immediately to avoid a jump on first frame.
    targetDelayTime = delayTimeParam->get();
    if (delayTime == 0.0f) {
        delayTime = targetDelayTime;
    }

    mixSmoother.setTargetValue(mixParam->get() * 0.01f);
    feedbackSmoother.setTargetValue(feedbackParam->get() * 0.01f);
    stereoSmoother.setTargetValue(stereoParam->get() * 0.01f);
    lowCutSmoother.setTargetValue(lowCutParam->get());
    highCutSmoother.setTargetValue(highCutParam->get());

    // copy choice index and tempo sync flag for quick access on the audio thread
    delayNote = delayNoteParam->getIndex();
    tempoSync = tempoSyncParam->get();
}

// smoothen: step the smoothers / apply one-pole smoothing for delayTime.
// Intended to be called per-sample inside the audio loop.
void Parameters::smoothen() noexcept
{
    gain = gainSmoother.getNextValue();

    // simple one-pole toward the target delay time (ms)
    delayTime += (targetDelayTime - delayTime) * coeff;

    mix = mixSmoother.getNextValue();
    feedback = feedbackSmoother.getNextValue();

    // compute equal-power panning gains from the smoothed stereo value
    panningEqualPower(stereoSmoother.getNextValue(), panL, panR);

    lowCut = lowCutSmoother.getNextValue();
    highCut = highCutSmoother.getNextValue();
}
