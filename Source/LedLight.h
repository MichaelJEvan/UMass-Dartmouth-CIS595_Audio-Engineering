/* ***********************************************************
 
 Michael J Evan
 CIS-595 Independent Study
 UMass Dartmouth
 Spring 2026
 Led Sync light code finalized: Feb 22, 2026
 LED with adjustable center size.
 Draws glow inside the component bounds.
 
*********************************************************** */


#pragma once
#include <JuceHeader.h>


//
class LedLight : public juce::Component
{
public:
    LedLight()
    {
        setOpaque(false);                       // allow transparency so glow doesn't show a square
        setInterceptsMouseClicks(false, false);
    }

    void setState(bool shouldBeOn)
    {
        if (state != shouldBeOn) { state = shouldBeOn; repaint(); }
    }

    bool getState() const noexcept { return state; }

    void setColour(juce::Colour c) { colour = c; repaint(); }

    // centerScale: fraction of the component diameter used for the solid center (0.2 .. 0.95).
    void setCenterScale(float scale)
    {
        centerScale = juce::jlimit(0.2f, 0.95f, scale);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float diameter = std::min(r.getWidth(), r.getHeight());

        // inner diameter for the solid disk
        const float innerDiameter = diameter * centerScale;

        // glow amount is the leftover space; keep it inside bounds
        const float glowDiameter = juce::jmax(0.0f, diameter - innerDiameter);

        juce::Rectangle<float> inner = r.withSizeKeepingCentre(innerDiameter, innerDiameter);

        if (state && glowDiameter > 0.5f)
        {
            // draw a soft glow (kept inside component bounds)
            g.setColour(colour.withAlpha(0.35f));
            g.fillEllipse(inner.expanded(glowDiameter * 0.5f, glowDiameter * 0.5f));
        }

        // main circle (center)
        g.setColour(state ? colour : juce::Colours::darkgrey);      // inactive color grey
        g.fillEllipse(inner);

        // rim
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.drawEllipse(inner, 1.0f);
    }

private:
    bool state { false };
    juce::Colour colour { juce::Colours::limegreen };
    float centerScale { 0.66f }; // default: center is 66% of component diameter

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LedLight)
};
