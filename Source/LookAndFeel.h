/*
  ==============================================================================
    LookAndFeel.h
    Created: 12 Jan 2026 11:52:57am
    Author:  Michael Evan
             CIS-595 Independent Study
             UMass Dartmouth Graduate Computer Science Dept.
             Spring 2026
  ==============================================================================
*/

#pragma once            // include guard: ensure header included only once

#include <JuceHeader.h> // main JUCE include (GUI, Colours, Fonts, LookAndFeel, etc.)

// Named color palette used throughout the UI to keep styling consistent.
namespace Colors
{
    // background & header colors
    const juce::Colour background { 245, 240, 235 }; // overall editor background colour (RGB)
    const juce::Colour header { 0, 0, 77 };                     // header bar colour (dark blue)

    namespace Knob
    {
        const juce::Colour trackBackground { 205, 200, 195 };  // inactive arc portion of rotary
        const juce::Colour trackActive { 240, 100, 219 };           // active arc portion of rotary
        const juce::Colour outline { 0, 0, 255 };                           // outline colour used for knob border
        const juce::Colour gradientTop {158, 12, 232};              // top colour for dial face gradient
        const juce::Colour gradientBottom { 240, 235, 230 };  // bottom colour for dial face gradient
        const juce::Colour dial { 100, 175, 255 };                      // fallback dial colour
        const juce::Colour dropShadow { 195, 190, 185 };      // shadow colour behind dial
        const juce::Colour label { 255, 255, 255 };                    // knob label text colour (white)
        const juce::Colour textBoxBackground { 235, 227,240 };  // textbox background behind slider value
        const juce::Colour value { 15, 136, 191 };                           // numeric value text colour
        const juce::Colour caret { 12, 169, 232 };                           // caret colour in text editors
    }

    namespace Group
    {
        const juce::Colour label { 232, 216, 247 };  // group box label colour
        const juce::Colour outline { 158, 12, 232 }; // group box border outline colour
    }

    namespace Button
    {
        const juce::Colour text { 158, 12, 235 };            // default button text colour
        const juce::Colour textToggled { 40, 40, 40 };       // button text when toggled
        const juce::Colour background { 235, 227, 240 };     // button background normal
        const juce::Colour backgroundToggled { 204, 153, 255 };// button background when toggled
        const juce::Colour outline { 235, 230, 225 };        // button outline colour
    }

    namespace LevelMeter
    {
        const juce::Colour background { 235, 227, 240 }; // meter background
        const juce::Colour tickLine { 138, 33, 207 };    // tick/line colour for meter scale
        const juce::Colour tickLabel {138, 12, 232};     // labels for the meter ticks
        const juce::Colour tooLoud { 226, 74, 81 };      // red colour for clipping/too loud indicator
        const juce::Colour levelOK { 65, 206, 88 };      // green for normal levels
    }
}

// Small Fonts helper: provides a shared font for labels (singleton style).
class Fonts
{
public:
    Fonts() = delete;                    // delete constructor to prevent instantiation

    static juce::Font getFont(float height = 20.0f); // returns a font of requested height

private:
    static const juce::Typeface::Ptr typeface; // underlying typeface pointer (defined in .cpp)
};

// Custom LookAndFeel for rotary knobs.
class RotaryKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RotaryKnobLookAndFeel();             // constructor: will configure colours/fonts/shadows as needed

    // singleton accessor to reuse one shared lookAndFeel instance
    static RotaryKnobLookAndFeel* get()
    {
        static RotaryKnobLookAndFeel instance; // static local ensures single instance
        return &instance;
    }

    // override to custom-draw the rotary slider (knob)
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    // override to provide label font for knob labels
    juce::Font getLabelFont(juce::Label&) override;

    // override to create custom textbox below the slider (returns pointer to Label or TextEditor)
    juce::Label* createSliderTextBox(juce::Slider&) override;

    // We intentionally don't draw the default text editor outline (empty override).
    void drawTextEditorOutline(juce::Graphics&, int, int, juce::TextEditor&) override { }
    // Provide custom background fill for text editors (we override to style value textbox)
    void fillTextEditorBackground(juce::Graphics&, int width, int height, juce::TextEditor&) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnobLookAndFeel)
    // disables copying/moving and adds leak detector helpful during debugging

    // Drop shadow used when drawing knobs; constructed with color, radius and offset
    juce::DropShadow dropShadow { Colors::Knob::dropShadow, 6, { 0, 3 } };
};

// Primary LookAndFeel for the main UI (labels, general widgets).
class MainLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MainLookAndFeel();                   // constructor: set colours/fonts for general UI

    juce::Font getLabelFont(juce::Label&) override; // override to return common label font

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainLookAndFeel)
};

// Button-specific LookAndFeel for text buttons (custom rendering).
class ButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ButtonLookAndFeel();                 // constructor: initialize button styles

    // singleton accessor for button L&F (reused across instances)
    static ButtonLookAndFeel* get()
    {
        static ButtonLookAndFeel instance;
        return &instance;
    }

    // override to draw a custom button background (rounded rect, gradients, etc.)
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // override to draw button's text (controls font, colour, alignment)
    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonLookAndFeel)
};
