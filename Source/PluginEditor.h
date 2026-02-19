/*
  ==============================================================================

     Michael J Evan
     CIS-595
 
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>          // JUCE main include (UI, audio, DSP, etc.)
#include "PluginProcessor.h"     // processor declarations (DelayAudioProcessor)
#include "Parameters.h"          // parameter IDs, ranges, helpers
#include "RotaryKnob.h"          // reusable rotary knob component
#include "LookAndFeel.h"         // custom LookAndFeel implementations
#include "LevelMeter.h"          // simple level meter widget

//==============================================================================
/*
  Editor for the DelayAudioProcessor. Also listens to parameter changes.
*/
class DelayAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                   private juce::AudioProcessorParameter::Listener
{
public:
    DelayAudioProcessorEditor (DelayAudioProcessor&); // constructor takes reference to the processor
    ~DelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;   // paint the UI background / custom graphics
    void resized() override;                 // layout children when the editor is resized

private:
    // AudioProcessorParameter::Listener callbacks:
    void parameterValueChanged(int, float) override;      // called when a parameter value changes
    void parameterGestureChanged(int, bool) override { }  // gesture begin/end (unused here)

    void updateDelayKnobs(bool tempoSyncActive); // helper to enable/disable or update delay-related knobs

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DelayAudioProcessor& audioProcessor; // ref to owning processor (must outlive editor)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessorEditor)
    // disallow copying and enable leak detection in debug builds

    // NOTE: these in-class initializers reference audioProcessor; in practice you must
    // ensure these members are initialized in the correct order (typically via the
    // editor constructor initializer list). Keep the same declaration order as shown.

    RotaryKnob gainKnob { "Gain", audioProcessor.apvts, gainParamID, true };      // master/trim gain
    RotaryKnob mixKnob { "Mix", audioProcessor.apvts, mixParamID };               // wet/dry mix
    RotaryKnob delayTimeKnob { "Time", audioProcessor.apvts, delayTimeParamID };  // manual delay time (ms)
    RotaryKnob feedbackKnob { "Feedback", audioProcessor.apvts, feedbackParamID, true }; // feedback amount
    RotaryKnob stereoKnob { "Stereo", audioProcessor.apvts, stereoParamID, true };// stereo/panning control
    RotaryKnob lowCutKnob { "Low Cut", audioProcessor.apvts, lowCutParamID };     // tone control - low cut
    RotaryKnob highCutKnob { "High Cut", audioProcessor.apvts, highCutParamID };  // tone control - high cut
    RotaryKnob delayNoteKnob { "Note", audioProcessor.apvts, delayNoteParamID };  // note choice for tempo sync

    juce::TextButton tempoSyncButton; // toggle button for tempo sync on/off

    // Binds the button to the tempoSync parameter in the APVTS (keeps UI <-> state in sync)
    juce::AudioProcessorValueTreeState::ButtonAttachment tempoSyncAttachment {
        audioProcessor.apvts, tempoSyncParamID.getParamID(), tempoSyncButton
    };

    juce::GroupComponent delayGroup, feedbackGroup, outputGroup; // grouped UI panels

    MainLookAndFeel mainLF; // instance of custom look-and-feel for the editor

    LevelMeter meter; // visual level meter (single instance shown in the UI)
};
