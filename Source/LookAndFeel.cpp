/*
  ==============================================================================
    LookAndFeel.cpp
    Created: 12 Jan 2026
    Author:  Michael Evan
 
    Note:
    Implements the UI styling and rendering helpers:
     - defines and loads the embedded typeface,
     - provides color/font configuration,
     - implements custom LookAndFeel classes (RotaryKnob, Main, Button),
       including rotary knob drawing, slider textboxes, and button rendering.
     - uses a drop shadow and simple accessibility tweaks for slider editors.
  ==============================================================================
*/

#include "LookAndFeel.h" // header with declarations for colours, fonts and L&F classes

// Define the static Typeface pointer by creating a typeface from embedded binary data
// Note: (BinaryData::LatoMedium_ttf is the font file compiled into the binary).
//       (font file is in Assets folder)
const juce::Typeface::Ptr Fonts::typeface = juce::Typeface::createSystemTypefaceFor(
    BinaryData::LatoMedium_ttf, BinaryData::LatoMedium_ttfSize);

// Return a juce::Font using the shared typeface with the requested height.
juce::Font Fonts::getFont(float height)
{
    return juce::Font(typeface).withHeight(height);
}

// RotaryKnobLookAndFeel constructor: set up colours used by the knob drawing and text editing.
RotaryKnobLookAndFeel::RotaryKnobLookAndFeel()
{
    // Label text colour for knob labels
    setColour(juce::Label::textColourId, Colors::Knob::label);
    // Text inside the slider textbox
    setColour(juce::Slider::textBoxTextColourId, Colors::Knob::label);
    // Colour used to draw the active portion of the rotary track
    setColour(juce::Slider::rotarySliderFillColourId, Colors::Knob::trackActive);
    // Don't want a visible outline around the textbox, make it transparent
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    // Caret colour used by text editors created for the slider textbox
    setColour(juce::CaretComponent::caretColourId, Colors::Knob::caret);
}

//  ***** Custom rotary slider drawing routine override *****
//  Parameters:
//  g                - graphics context
//  x,y,width,height - bounding rectangle for the control
//  sliderPos        - normalized position [0..1]
//  rotaryStartAngle - start angle in radians
//  rotaryEndAngle   - end angle in radians
//  slider           - reference to the slider being drawn
void RotaryKnobLookAndFeel::drawRotarySlider(
     juce::Graphics& g,
     int x, int y, int width, [[maybe_unused]] int height,
     float sliderPos,
     float rotaryStartAngle, float rotaryEndAngle,
     juce::Slider& slider)
{
    // Create a square bounds and reduce inner margins for the knob face
    auto bounds = juce::Rectangle<int>(x, y, width, width).toFloat();
    auto knobRect = bounds.reduced(10.0f, 10.0f);

    // Create an ellipse path for the knob and draw a drop shadow under it
    auto path = juce::Path();
    path.addEllipse(knobRect);
    dropShadow.drawForPath(g, path);

    // Draw the outer outline of the knob
    g.setColour(Colors::Knob::outline);
    g.fillEllipse(knobRect);

    // Inner dial face with a vertical gradient for a subtle 3D look
    auto innerRect = knobRect.reduced(2.0f, 2.0f);
    auto gradient = juce::ColourGradient(
        Colors::Knob::gradientTop, 0.0f, innerRect.getY(),
        Colors::Knob::gradientBottom, 0.0f, innerRect.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillEllipse(innerRect);

    // Calculate geometry for the arc and dial indicator
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() / 2.0f;
    auto lineWidth = 3.0f;
    auto arcRadius = radius - lineWidth/2.0f;

    // Background arc path covering the full rotary range (inactive track)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x,
                                center.y,
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);

    // Stroke type for consistent rounded ends
    auto strokeType = juce::PathStrokeType(
        lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    
    // Draw the inactive track
    g.setColour(Colors::Knob::trackBackground);
    g.strokePath(backgroundArc, strokeType);

    // Calculate the dial (pointer) geometry and draw it
    auto dialRadius = innerRect.getHeight()/2.0f - lineWidth;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Point<float> dialStart(center.x + 10.0f * std::sin(toAngle),
                                 center.y - 10.0f * std::cos(toAngle));
    juce::Point<float> dialEnd(center.x + dialRadius * std::sin(toAngle),
                               center.y - dialRadius * std::cos(toAngle));

    juce::Path dialPath;
    dialPath.startNewSubPath(dialStart);
    dialPath.lineTo(dialEnd);
    g.setColour(Colors::Knob::dial);
    g.strokePath(dialPath, strokeType);

    // If the slider is enabled, draw the active arc representing the current value.
    if (slider.isEnabled()) {
        float fromAngle = rotaryStartAngle;
        // Optional behavior: draw arc starting from the middle of the range if requested
        if (slider.getProperties()["drawFromMiddle"]) {
            fromAngle += (rotaryEndAngle - rotaryStartAngle) / 2.0f;
        }

        juce::Path valueArc;
        valueArc.addCentredArc(center.x,
                               center.y,
                               arcRadius,
                               arcRadius,
                               0.0f,
                               fromAngle,
                               toAngle,
                               true);

        // Use the colour registered earlier (rotarySliderFillColourId) for the active arc
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(valueArc, strokeType);
    }
}

// Return font used for labels; currently uses the shared Fonts helper.
juce::Font RotaryKnobLookAndFeel::getLabelFont([[maybe_unused]] juce::Label& label)
{
    return Fonts::getFont();
}

// Small subclass of Label used for slider textboxes to tune behavior/accessibility.
class RotaryKnobLabel : public juce::Label
{
public:
    RotaryKnobLabel() : juce::Label() {}

    // Disable mouse-wheel changes for the label to avoid accidental edits.
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}

    // Accessibility: mark this label as "ignored" by assistive tech (no custom handler)
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    {
        return createIgnoredAccessibilityHandler(*this);
    }

    // Create a TextEditor used as the inline editor for the label (when editing numeric values)
    juce::TextEditor* createEditorComponent() override
    {
        auto* ed = new juce::TextEditor(getName());
        // Match the editor font to our LookAndFeel label font
        ed->applyFontToAllText(getLookAndFeel().getLabelFont(*this));
        // Copy the label's explicit colours to the text editor for consistent style
        copyAllExplicitColoursTo(*ed);

        // Tweak editor appearance/behavior
        ed->setBorder(juce::BorderSize<int>());
        ed->setIndents(2, 1);
        ed->setJustification(juce::Justification::centredTop);
        ed->setPopupMenuEnabled(false);
        ed->setInputRestrictions(8); // restrict input length
        return ed;
    }
};

// Create a custom slider textbox Label instance (owned by the slider).
juce::Label* RotaryKnobLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto l = new RotaryKnobLabel();
    l->setJustificationType(juce::Justification::centred);
    l->setKeyboardType(juce::TextInputTarget::decimalKeyboard); // numeric keyboard on touch devices
    // Set colours for the label/texteditor using our colour palette
    l->setColour(juce::Label::textColourId, slider.findColour(juce::Slider::textBoxTextColourId));
    l->setColour(juce::TextEditor::textColourId, Colors::Knob::value);
    l->setColour(juce::TextEditor::highlightedTextColourId, Colors::Knob::value);
    l->setColour(juce::TextEditor::highlightColourId, slider.findColour(juce::Slider::rotarySliderFillColourId));
    l->setColour(juce::TextEditor::backgroundColourId, Colors::Knob::textBoxBackground);
    return l;
}

// Fill the background of the slider text editor with a rounded rectangle.
void RotaryKnobLookAndFeel::fillTextEditorBackground(
    juce::Graphics& g, [[maybe_unused]] int width, [[maybe_unused]] int height,
    juce::TextEditor& textEditor)
{
    g.setColour(Colors::Knob::textBoxBackground);
    g.fillRoundedRectangle(textEditor.getLocalBounds().reduced(4, 0).toFloat(), 4.0f);
}

// MainLookAndFeel constructor: set group component colours.
MainLookAndFeel::MainLookAndFeel()
{
    setColour(juce::GroupComponent::textColourId, Colors::Group::label);
    setColour(juce::GroupComponent::outlineColourId, Colors::Group::outline);
}

// Return the shared font for general labels.
juce::Font MainLookAndFeel::getLabelFont([[maybe_unused]] juce::Label& label)
{
    return Fonts::getFont();
}

// ButtonLookAndFeel constructor: configure button colour roles.
ButtonLookAndFeel::ButtonLookAndFeel()
{
    setColour(juce::TextButton::textColourOffId, Colors::Button::text);
    setColour(juce::TextButton::textColourOnId, Colors::Button::textToggled);
    setColour(juce::TextButton::buttonColourId, Colors::Button::background);
    setColour(juce::TextButton::buttonOnColourId, Colors::Button::backgroundToggled);
}

// Custom button background drawing: rounded rectangle with optional pressed offset.
void ButtonLookAndFeel::drawButtonBackground(
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour& backgroundColour,
    [[maybe_unused]] bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerSize = bounds.getHeight() * 0.25f; // corner radius proportional to height
    auto buttonRect = bounds.reduced(1.0f, 1.0f).withTrimmedBottom(1.0f);

    // If button is pressed, slightly offset to mimic depth
    if (shouldDrawButtonAsDown) {
        buttonRect.translate(0.0f, 1.0f);
    }

    // Fill the button background and draw an outline
    g.setColour(backgroundColour);
    g.fillRoundedRectangle(buttonRect, cornerSize);

    g.setColour(Colors::Button::outline);
    g.drawRoundedRectangle(buttonRect, cornerSize, 2.0f);
}

// Custom button text drawing: choose colour based on toggle state and draw centered text.
void ButtonLookAndFeel::drawButtonText(
    juce::Graphics& g,
    juce::TextButton& button,
    [[maybe_unused]] bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto buttonRect = bounds.reduced(1.0f, 1.0f).withTrimmedBottom(1.0f);

    if (shouldDrawButtonAsDown) {
        buttonRect.translate(0.0f, 1.0f);
    }

    // Choose toggled vs. untoggled text colour
    if (button.getToggleState()) {
        g.setColour(button.findColour(juce::TextButton::textColourOnId));
    } else {
        g.setColour(button.findColour(juce::TextButton::textColourOffId));
    }

    // Draw the text using the shared font and center it
    g.setFont(Fonts::getFont());
    g.drawText(button.getButtonText(), buttonRect, juce::Justification::centred);
}
