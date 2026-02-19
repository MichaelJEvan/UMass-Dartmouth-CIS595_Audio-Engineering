/*
  ==============================================================================
  Michael J Evan
  CIS-595
 
    Note:
    - Constructs and lays out rotary knobs, groups, tempo sync button and level meter.
    - Hooks UI controls to the processor's AudioProcessorValueTreeState (attachments).
    - Listens to the tempoSync parameter and toggles delay time / note controls safely
    on the message thread (uses MessageManager::callAsync when needed).
    - Paints background using embedded images (BinaryData) and draws the header/logo.
    - Keeps visual state in sync with audio-side Parameters via the Parameters helpers.
  ==============================================================================
*/

#include "PluginProcessor.h" // editor <-> processor connection
#include "PluginEditor.h"    // this header's declarations

//==============================================================================
// Editor implementation
DelayAudioProcessorEditor::DelayAudioProcessorEditor (DelayAudioProcessor& p)
    : AudioProcessorEditor (&p),    // base constructor needs processor reference
      audioProcessor (p),           // store reference to the owning processor
      meter(p.levelL, p.levelR)     // initialize meter with processor's level trackers
{
    // Configure the Delay group UI
    delayGroup.setText("Delay");
    delayGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    delayGroup.addAndMakeVisible(delayTimeKnob);   // show manual time knob
    delayGroup.addChildComponent(delayNoteKnob);   // note knob is child but not immediately visible
    addAndMakeVisible(delayGroup);                 // add group to the editor

    // Configure the Feedback group UI
    feedbackGroup.setText("Feedback");
    feedbackGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    feedbackGroup.addAndMakeVisible(feedbackKnob);
    feedbackGroup.addAndMakeVisible(stereoKnob);
    feedbackGroup.addAndMakeVisible(lowCutKnob);
    feedbackGroup.addAndMakeVisible(highCutKnob);
    addAndMakeVisible(feedbackGroup);

    // Configure the Output group UI
    outputGroup.setText("Output");
    outputGroup.setTextLabelPosition(juce::Justification::horizontallyCentred);
    outputGroup.addAndMakeVisible(gainKnob);
    outputGroup.addAndMakeVisible(mixKnob);
    outputGroup.addAndMakeVisible(meter);
    addAndMakeVisible(outputGroup);

    // Tempo-sync toggle button
    tempoSyncButton.setButtonText("Sync");
    tempoSyncButton.setClickingTogglesState(true); // behaves like a toggle
    tempoSyncButton.setBounds(0, 0, 70, 27);       // initial size (will be repositioned in resized())
    tempoSyncButton.setLookAndFeel(ButtonLookAndFeel::get()); // custom button L&F
    delayGroup.addAndMakeVisible(tempoSyncButton); // place button inside delay group

    setSize(500, 330); // ***** Plug-in fixed window size *****

    //setLookAndFeel for the entire editor (custom look & feel instance)
    setLookAndFeel(&mainLF);

    // Ensure the UI reflects current tempoSync state on startup
    updateDelayKnobs(audioProcessor.params.tempoSyncParam->get());
    // Listen for changes to the tempoSync parameter so the UI updates automatically
    audioProcessor.params.tempoSyncParam->addListener(this);
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
    // Clean up listener to avoid dangling callbacks
    audioProcessor.params.tempoSyncParam->removeListener(this);
    setLookAndFeel(nullptr); // restore default look & feel before destruction
}

//==============================================================================
void DelayAudioProcessorEditor::paint (juce::Graphics& g)
{
  
    // Load background image from BinaryData (embedded resource)
    auto Aurora = juce::ImageCache::getFromMemory(
        BinaryData::aurora_png, BinaryData::aurora_pngSize);
    auto fillType = juce::FillType(Aurora, juce::AffineTransform::scale(1.0f));

    g.setFillType(fillType);         // set the fill to the image pattern
    g.fillRect(getLocalBounds());    // paint the background

    // Draw a header strip
    auto rect = getLocalBounds().withHeight(40);
    g.setColour(Colors::header);
    g.fillRect(rect);

    // Draw Logo.png file centered at top (image loaded from BinaryData)
    auto image = juce::ImageCache::getFromMemory(
        BinaryData::Logo_png, BinaryData::Logo_pngSize);

    int destWidth = image.getWidth() / 2;   // scale down the logo
    int destHeight = image.getHeight() / 2;
    g.drawImage(image,
                getWidth() / 2 - destWidth / 2, 0, destWidth, destHeight, // destination rect
                0, 0, image.getWidth(), image.getHeight());               // source rect
}

void DelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    int y = 50;     // top margin below header
    int height = bounds.getHeight() - 60;   // available height for groups

    // Position the main groups
    delayGroup.setBounds(10, y, 110, height);       // left column
    outputGroup.setBounds(bounds.getWidth() - 160, y, 150, height);         // right column
    feedbackGroup.setBounds(delayGroup.getRight() + 10, y,                  // middle area
                            outputGroup.getX() - delayGroup.getRight() - 20,
                            height);

    // Position controls inside groups (absolute positions relative to each group's origin)
    delayTimeKnob.setTopLeftPosition(20, 20);
    tempoSyncButton.setTopLeftPosition(20, delayTimeKnob.getBottom() + 10);
    delayNoteKnob.setTopLeftPosition(delayTimeKnob.getX(), delayTimeKnob.getY());

    mixKnob.setTopLeftPosition(20, 20);
    gainKnob.setTopLeftPosition(mixKnob.getX(), mixKnob.getBottom() + 10);

    feedbackKnob.setTopLeftPosition(20, 20);
    stereoKnob.setTopLeftPosition(feedbackKnob.getRight() + 20, 20);
    lowCutKnob.setTopLeftPosition(feedbackKnob.getX(), feedbackKnob.getBottom() + 10);
    highCutKnob.setTopLeftPosition(lowCutKnob.getRight() + 20, lowCutKnob.getY());

    // Meter positioned inside output group (x is relative to group's left)
    meter.setBounds(outputGroup.getWidth() - 45, 30, 30, gainKnob.getBottom() - 30);
}

// Parameter listener callback (value changed)
void DelayAudioProcessorEditor::parameterValueChanged(int, float value)
{
    // If already on the message thread, update immediately
    if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        updateDelayKnobs(value != 0.0f);
    } else {
        // If called from another thread (audio thread), post to message thread
        juce::MessageManager::callAsync([this, value]
        {
            updateDelayKnobs(value != 0.0f);
        });
    }
}

// Toggle visibility of manual vs. note-based delay controls based on tempo sync state
void DelayAudioProcessorEditor::updateDelayKnobs(bool tempoSyncActive)
{
    delayTimeKnob.setVisible(!tempoSyncActive); // hide manual ms (milliseconde) knob when sync active
    delayNoteKnob.setVisible(tempoSyncActive);  // show note-choice knob when sync active
}
