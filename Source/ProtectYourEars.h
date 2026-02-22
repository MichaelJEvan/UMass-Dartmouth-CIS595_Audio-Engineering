#pragma once

#include <JuceHeader.h>

// Silences the buffer if bad or loud values are detected in the output buffer.
// Use during debugging to avoid blowing out your eardrums on headphones!!!!! Ouch!


// Notes:
// - This function is intended as a debug/safety helper; DBG is relatively CPU expensive and should be disabled in release builds.
// - Clearing the whole buffer is a blunt but safe action to prevent loud output; consider more targeted handling in production.
// - Running these checks sample-by-sample on the audio thread can be CPU-heavy; just keeping for development builds only.


inline void protectYourEars(juce::AudioBuffer<float>& buffer)
{
    // Only emit the detailed one-time warning for the first out-of-range sample
    bool firstWarning = true;

    // iterate channels and samples to validate audio data
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel); // writable pointer to this channel's samples
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float x = channelData[sample];
            bool silence = false;

            // NaN/infinite checks — these indicate broken processing or divide-by-zero, etc....
            if (std::isnan(x)) {
                DBG("!!! WARNING: nan detected in audio buffer, silencing !!!");
                silence = true;
            } else if (std::isinf(x)) {
                DBG("!!! WARNING: inf detected in audio buffer, silencing !!!");
                silence = true;
            }
            // Absolute-safety clamp: values beyond [-2, 2] are treated as catastrophic (screaming feedback)
            else if (x < -2.0f || x > 2.0f) {
                DBG("!!! WARNING: sample out of range, silencing !!!");
                silence = true;
            }
            // Soft warning for values outside [-1, 1] (audio clipping region); only log the first occurrence
            else if (x < -1.0f || x > 1.0f) {
                if (firstWarning) {
                    DBG("!!! WARNING: sample out of range: " << x << " !!!");
                    firstWarning = false;
                }
            }

            // If a fatal condition was detected, clear the entire buffer and return immediately
            if (silence) {
                buffer.clear(); // zero all channels/samples to avoid sending dangerous audio to the host/speakers
                return;         // stop checking further — we've already mitigated the issue
            }
        }
    }
}
