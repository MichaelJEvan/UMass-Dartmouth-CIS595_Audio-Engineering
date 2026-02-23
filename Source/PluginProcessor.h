/*
  ==============================================================================
 Author:  Michael J Evan
          CIS-595 Independent Study
          UMass Dartmouth
          Graduate Computer Science Dept
          Spring 2026
 
    This file contains the basic framework code for a JUCE plugin processor.
 
    Note: No MIDI functionality was implemented in this plug-in

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Parameters.h"  // parameter helpers + smoothing
#include "Tempo.h"       // tempo helper (reads host BPM / converts note lengths)
#include "DelayLine.h"   // circular delay buffer abstraction
#include "Measurement.h" // simple peak/level measurement utility

//==============================================================================
// Main audio processor for the delay plugin.
class DelayAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DelayAudioProcessor();                 // Constructor: set up APVTS, params, etc.
    ~DelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override; // allocate/reset DSP
    void releaseResources() override;                                     // free non-real-time resources

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override; // channel layout checks
   #endif

    
/* //////////////////////////////////////////////////////////////////////////////
 
    *** Most important function ! ***
 
    processBlock is where the DSP / audio processing code resides
 
*/
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override; // main audio processing

    //==============================================================================
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;


    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;


    void getStateInformation (juce::MemoryBlock& destData) override; // save plugin state
    void setStateInformation (const void* data, int sizeInBytes) override; // restore plugin state

    // APVTS holds parameters and state. Constructed inline using the Parameters helper.
    juce::AudioProcessorValueTreeState apvts {
        *this, nullptr, "Parameters", Parameters::createParameterLayout()
    };

    Parameters params;          // runtime values + smoothers derived from APVTS

    Measurement levelL, levelR; // simple level peak trackers for left/right

    
    //=============================================================================
private:
    
    DelayLine delayLineL, delayLineR; // per-channel delay buffers

    float feedbackL = 0.0f; // current feedback sample for left
    float feedbackR = 0.0f; // current feedback sample for right

    // StateVariable filters used in the feedback path for tone control
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;
    juce::dsp::StateVariableTPTFilter<float> highCutFilter;

    // cache last cutoff values to avoid redundant calls to setCutoffFrequency()
    float lastLowCut = -1.0f;
    float lastHighCut = -1.0f;

    Tempo tempo; // tempo helper used for tempo-synced delay times

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessor)
};
