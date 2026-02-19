/*
  ==============================================================================
    Parameters.h
    Created: 18 Dec 2025 11:40:41am
    Author:  Michael J Evan

    Defines a new class: Parameters to deal with all plug-in parameters
  ==============================================================================
*/

// This header included: C++ compiler only reades once per translation unit
#pragma once

#include <JuceHeader.h> // main JUCE include (Audio, GUI, DSP, etc.)

// ParameterID constants used to identify parameters in the APVTS (stable IDs)
const juce::ParameterID gainParamID { "gain", 1 };
const juce::ParameterID delayTimeParamID { "delayTime", 1 };
const juce::ParameterID mixParamID { "mix", 1 };
const juce::ParameterID feedbackParamID { "feedback", 1 };
const juce::ParameterID stereoParamID { "stereo", 1 };
const juce::ParameterID lowCutParamID { "lowCut", 1 };
const juce::ParameterID highCutParamID { "highCut", 1 };
const juce::ParameterID tempoSyncParamID { "tempoSync", 1 };
const juce::ParameterID delayNoteParamID { "delayNote", 1 };

// Parameters helper: holds runtime parameter values, smoothing, and ties to APVTS.
class Parameters
{
public:
    // Constructor: binds parameter pointers by looking them up in the host APVTS
    Parameters(juce::AudioProcessorValueTreeState& apvts);

    // Factory that creates the APVTS parameter layout (used when constructing APVTS)
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Lifecycle methods called from the processor
    void prepareToPlay(double sampleRate) noexcept; // init smoothing / sample-rate dependent values
    void reset() noexcept;                          // set defaults
    void update() noexcept;                         // pull values from APVTS parameters
    void smoothen() noexcept;                       // step smoothers (call per-sample or per-block)

    // Public runtime parameter values (exposed so audio thread can read them cheaply)
    float gain = 0.0f;         // linear gain (derived from dB parameter)
    float delayTime = 0.0f;    // smoothed delay time (ms)
    float mix = 1.0f;          // wet/dry mix (0..1)
    float feedback = 0.0f;     // feedback amount (signed -1..1)
    float panL = 0.0f;         // left write gain (equal-power panning)
    float panR = 1.0f;         // right write gain
    float lowCut = 20.0f;      // low cut cutoff (Hz)
    float highCut = 20000.0f;  // high cut cutoff (Hz)
    int delayNote = 0;         // index into note-length choices (0..15)
    bool tempoSync = false;    // whether delay is tempo-synced

    // Allowed delay range (ms)
    static constexpr float minDelayTime = 5.0f;
    static constexpr float maxDelayTime = 5000.0f;

    // Expose the APVTS parameter pointer for tempoSync so editor can add/remove listeners
    juce::AudioParameterBool* tempoSyncParam;

private:
    // Internal pointers to APVTS parameters (set by the constructor via dynamic cast)
    juce::AudioParameterFloat* gainParam;
    juce::LinearSmoothedValue<float> gainSmoother; // smooths gain changes

    juce::AudioParameterFloat* delayTimeParam;     // raw delay-time parameter (ms)

    float targetDelayTime = 0.0f; // the target (unsmoothed) delay time to approach
    float coeff = 0.0f;           // one-pole smoothing coefficient (computed from sampleRate)

    juce::AudioParameterFloat* mixParam;
    juce::LinearSmoothedValue<float> mixSmoother;

    juce::AudioParameterFloat* feedbackParam;
    juce::LinearSmoothedValue<float> feedbackSmoother;

    juce::AudioParameterFloat* stereoParam;
    juce::LinearSmoothedValue<float> stereoSmoother;

    juce::AudioParameterFloat* lowCutParam;
    juce::LinearSmoothedValue<float> lowCutSmoother;

    juce::AudioParameterFloat* highCutParam;
    juce::LinearSmoothedValue<float> highCutSmoother;

    juce::AudioParameterChoice* delayNoteParam; // choice list for note subdivisions (UI)
};
