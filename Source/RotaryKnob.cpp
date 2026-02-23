/*
  ===================================================================================================

    RotaryKnob.cpp
    Created: 22 Dec 2025
    Author:  Michael J Evan
             CIS-595 Independent Study
             UMass Dartmouth
             Graduate Computer Science Dept
             Spring 2026
 
    Note:
    Implements a reusable rotary knob UI component.
     - Wraps a juce::Slider and juce::Label and binds the slider to the APVTS via a SliderAttachment.
     - Configures rotary style, text box, size and custom LookAndFeel.
     - Sets rotary angle range and an optional "drawFromMiddle" property for bipolar-style arcs.
     - Intended for use in the plugin editor to provide consistent, skinnable parameter controls.
 
  ===================================================================================================
*/

#include <JuceHeader.h>
#include "RotaryKnob.h"
#include "LookAndFeel.h" // custom LookAndFeel used to draw the knob

RotaryKnob::RotaryKnob(const juce::String& text,
                       juce::AudioProcessorValueTreeState& apvts,
                       const juce::ParameterID& parameterID,
                       bool drawFromMiddle)
    : attachment(apvts, parameterID.getParamID(), slider) // ties the slider to an APVTS parameter
{
    slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag); // rotary control with drag
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16); // textbox below knob; fixed size
    slider.setBounds(0, 0, 70, 86); // initial bounds for layout
    addAndMakeVisible(slider); // ensure the slider is visible as a child component

    label.setText(text, juce::NotificationType::dontSendNotification); // static label text
    label.setJustificationType(juce::Justification::horizontallyCentred); // centered under knob
    label.setBorderSize(juce::BorderSize<int>{ 0, 0, 2, 0 }); // small border/padding for visual spacing
    label.attachToComponent(&slider, false); // attach label to the slider (not on left)
    addAndMakeVisible(label);

    setSize(70, 110); // component size (width, height)

    setLookAndFeel(RotaryKnobLookAndFeel::get()); // use your custom LookAndFeel singleton; must outlive component

    float pi = juce::MathConstants<float>::pi;
    slider.setRotaryParameters(1.25f * pi, 2.75f * pi, true); // startAngle, endAngle in radians; clockwise = true

    slider.getProperties().set("drawFromMiddle", drawFromMiddle); // custom property used by the LookAndFeel to alter drawing
}

RotaryKnob::~RotaryKnob()
{
    // empty destructor - ownership of lookAndFeel, attachment, etc. handled elsewhere
}

void RotaryKnob::resized()
{
    slider.setTopLeftPosition(0, 24); // position slider inside this component (leave space for label)
}
