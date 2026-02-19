/*
  ==============================================================================

    RotaryKnob.h
    Created: 22 Dec 2025 3:23:13pm
    Author:  Michael Evan

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Reusable rotary knob component that wraps a juce::Slider + Label
// and connects the slider to an APVTS parameter via a SliderAttachment.
class RotaryKnob  : public juce::Component
{
public:
    // text: label shown under the knob
    // apvts: the AudioProcessorValueTreeState that owns the parameter
    // parameterID: identifies which parameter to attach the slider to
    // drawFromMiddle: optional flag passed to the LookAndFeel via slider properties
    RotaryKnob(const juce::String& text,
               juce::AudioProcessorValueTreeState& apvts,
               const juce::ParameterID& parameterID,
               bool drawFromMiddle = false);

    ~RotaryKnob() override;

    void resized() override; // lay out child components

    juce::Slider slider; // the visible rotary control
    juce::Label label;   // text label attached to the slider

    // Attachment binds slider <-> APVTS parameter. Must be constructed after
    // slider exists (hence declared after it) and keeps the connection alive.
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
    // Disables copy/move and adds a leak detector (helpful during development)
};
