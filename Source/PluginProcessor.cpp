/*
  =====================================================================================================
 Author:  Michael J Evan
          CIS-595 Independent Study
          UMass Dartmouth
          Graduate Computer Science Dept
          Spring 2026
    Note:
    Implements the audio processing core of the plug-in
     - Manages AudioProcessor lifecycle (prepareToPlay, releaseResources).
     - Hosts the AudioProcessorValueTreeState (apvts) and Parameters helper for smoothing,
       parameter updates, and attachments used by the editor.
     - Allocates and manages delay lines, per-channel feedback, and filter state.
     - Implements processBlock: reads inputs, applies delay (tempo-syncable), feedback,
       filtering, mixing, gain, and level measurement; protects against denormals and
       unsafe sample values in debug builds.
     - Handles state save/restore (getStateInformation / setStateInformation) and plugin instantiation.

  =====================================================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProtectYourEars.h"

//==============================================================================
// Implementation of the audio processor
DelayAudioProcessor::DelayAudioProcessor() :
    AudioProcessor(
        BusesProperties()
           .withInput("Input", juce::AudioChannelSet::stereo(), true)   // declare stereo input bus
           .withOutput("Output", juce::AudioChannelSet::stereo(), true) // declare stereo output bus
    ),
    params(apvts) // initialize Parameters helper with the APVTS instance
{
    // Set filter types used later in the feedback path
    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

DelayAudioProcessor::~DelayAudioProcessor()
{
}

//==============================================================================
const juce::String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name; // plugin display name from Projucer/CMake defines
}

bool DelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0; // no infinite tail declared (could be >0 for long feedback tails)
}

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   // keep at least one program for host compatibility
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    params.prepareToPlay(sampleRate); // initialize smoothers etc.
    params.reset();                   // set initial parameter values

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = juce::uint32(samplesPerBlock);
    spec.numChannels = 2; // we expect stereo processing

    // compute maximum delay buffer size from max delay time (ms -> samples)
    double numSamples = Parameters::maxDelayTime / 1000.0 * sampleRate;
    int maxDelayInSamples = int(std::ceil(numSamples));

    delayLineL.setMaximumDelayInSamples(maxDelayInSamples); // allocate / set cap
    delayLineR.setMaximumDelayInSamples(maxDelayInSamples);
    delayLineL.reset();
    delayLineR.reset();

    // reset feedback accumulators
    feedbackL = 0.0f;
    feedbackR = 0.0f;

    // prepare filters with the process spec and clear their state
    lowCutFilter.prepare(spec);
    lowCutFilter.reset();

    highCutFilter.prepare(spec);
    highCutFilter.reset();

    // cached cutoff values to detect changes and avoid redundant set calls
    lastLowCut = -1.0f;
    lastHighCut = -1.0f;

    tempo.reset(); // reset tempo to default (120 BPM)

    levelL.reset(); // reset level meters/measurement
    levelR.reset();
}

void DelayAudioProcessor::releaseResources()
{
    // free resources if needed when playback stops
}

bool DelayAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    // Accept mono->mono, mono->stereo (upmix), and stereo->stereo only
    if (mainIn == mono && mainOut == mono) { return true; }
    if (mainIn == mono && mainOut == stereo) { return true; }
    if (mainIn == stereo && mainOut == stereo) { return true; }

    return false; // other layouts not supported
}

// *******************************************************************************
//
//  processBlock function: All DSP code is here .....
//
// *******************************************************************************

void DelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, [[maybe_unused]] juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; // avoid denormals on some CPUs
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have corresponding inputs to avoid garbage/noise.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    params.update();                 // pull latest parameter values from APVTS
    tempo.update(getPlayHead());     // update tempo from host playhead if available

    // compute tempo-synced delay time (ms) for the selected note value
    float syncedTime = float(tempo.getMillisecondsForNoteLength(params.delayNote));
    if (syncedTime > Parameters::maxDelayTime) { // clamp to allowed max
        syncedTime = Parameters::maxDelayTime;
    }

    float sampleRate = float(getSampleRate());

    // get input and output bus buffers and channel info
    auto mainInput = getBusBuffer(buffer, true, 0);
    auto mainInputChannels = mainInput.getNumChannels();
    auto isMainInputStereo = mainInputChannels > 1;
    const float* inputDataL = mainInput.getReadPointer(0);
    const float* inputDataR = mainInput.getReadPointer(isMainInputStereo ? 1 : 0);

    auto mainOutput = getBusBuffer(buffer, false, 0);
    auto mainOutputChannels = mainOutput.getNumChannels();
    auto isMainOutputStereo = mainOutputChannels > 1;
    float* outputDataL = mainOutput.getWritePointer(0);
    float* outputDataR = mainOutput.getWritePointer(isMainOutputStereo ? 1 : 0);

    float maxL = 0.0f; // peak trackers for meters
    float maxR = 0.0f;

    // per-sample processing loop (keeps smoothing/controls sample-accurate)
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        params.smoothen(); // advance smoothers and compute current param values

        // choose delay time (tempo-synced or manual) and convert to samples
        float delayTime = params.tempoSync ? syncedTime : params.delayTime;
        float delayInSamples = delayTime / 1000.0f * sampleRate;

        // update filters only when cutoff changed to save CPU
        if (params.lowCut != lastLowCut) {
            lowCutFilter.setCutoffFrequency(params.lowCut);
            lastLowCut = params.lowCut;
        }
        if (params.highCut != lastHighCut) {
            highCutFilter.setCutoffFrequency(params.highCut);
            lastHighCut = params.highCut;
        }

        // read dry input samples (supports mono input by duplicating channel 0)
        float dryL = inputDataL[sample];
        float dryR = inputDataR[sample];

        float mono = (dryL + dryR) * 0.5f; // use mono sum for the delay write

        // write into delay lines with panning + cross-feedback
        delayLineL.write(mono*params.panL + feedbackR);
        delayLineR.write(mono*params.panR + feedbackL);

        // read delayed samples (fractional-read supported by DelayLine)
        float wetL = delayLineL.read(delayInSamples);
        float wetR = delayLineR.read(delayInSamples);

        // compute feedback paths and run through tone filters
        feedbackL = wetL * params.feedback;
        feedbackL = lowCutFilter.processSample(0, feedbackL);
        feedbackL = highCutFilter.processSample(0, feedbackL);

        feedbackR = wetR * params.feedback;
        feedbackR = lowCutFilter.processSample(1, feedbackR);
        feedbackR = highCutFilter.processSample(1, feedbackR);

        // mix dry + wet according to mix param and apply output gain
        float mixL = dryL + wetL * params.mix;
        float mixR = dryR + wetR * params.mix;

        float outL = mixL * params.gain;
        float outR = mixR * params.gain;

        // write output samples
        outputDataL[sample] = outL;
        outputDataR[sample] = outR;

        // track peaks for meters
        maxL = std::max(maxL, std::abs(outL));
        maxR = std::max(maxR, std::abs(outR));
    }

#if JUCE_DEBUG
    protectYourEars(buffer); // debug guard to catch NaN/Inf/clipping during development
#endif

    // update measurement objects with observed peaks
    levelL.updateIfGreater(maxL);
    levelR.updateIfGreater(maxR);
}

//==============================================================================
bool DelayAudioProcessor::hasEditor() const
{
    return true; // plugin provides a GUI editor
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor (*this); // create and return the editor
}

//==============================================================================
void DelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // serialize APVTS state to XML and copy into destData for host preset storage
    copyXmlToBinary(*apvts.copyState().createXml(), destData);
}

void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // restore APVTS state from binary XML provided by host
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

//==============================================================================
// Factory function called by hosts to create plugin instances
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}
