/*
  ==============================================================================
    DelayLine.cpp
    Created: 18 Jan 2026 4:18:48pm
    Author:  Michael Evan
 
    Note:
    DelayLine.cpp implements a simple circular delay buffer with
    fractional (cubic) interpolation on reads.
  ==============================================================================
*/

#include <JuceHeader.h>   // include JUCE core utilities (jassert, etc.)
#include "DelayLine.h"    // class declaration for DelayLine

// Set the internal buffer size to accommodate the requested maximum delay in samples.
// Adds a small padding (2 samples) to allow safe fractional reads near the buffer edge.
void DelayLine::setMaximumDelayInSamples(int maxLengthInSamples)
{
    jassert(maxLengthInSamples > 0); // debug-time sanity check: requested length must be positive

    int paddedLength = maxLengthInSamples + 2; // add 2-sample padding for interpolation safety
    if (bufferLength < paddedLength) {         // only reallocate if current buffer is too small
        bufferLength = paddedLength;           // update stored buffer length
        buffer.reset(new float[size_t(bufferLength)]); // allocate new raw float array (unique_ptr takes ownership)
    }
}

// Reset the circular buffer indices and clear the buffer contents to silence.
void DelayLine::reset() noexcept
{
    writeIndex = bufferLength - 1;            // set writeIndex so next write increments to 0 (wrap behavior)

    for (size_t i = 0; i < size_t(bufferLength); ++i) { // clear every sample slot
        buffer[i] = 0.0f;                     // set to silence
    }
}

// Write a single sample into the buffer at the current write position (real-time safe).
void DelayLine::write(float input) noexcept
{
    jassert(bufferLength > 0);                // ensure buffer was allocated

    writeIndex += 1;                          // advance the write index
    if (writeIndex >= bufferLength) {        // wrap if we've reached the end
        writeIndex = 0;
    }

    buffer[size_t(writeIndex)] = input;      // store the input sample at the write position
}

// Read a delayed sample using fractional delay (cubic-style interpolation).
float DelayLine::read(float delayInSamples) const noexcept
{
    jassert(delayInSamples >= 1.0f);                     // require at least 1 sample delay
    jassert(delayInSamples <= bufferLength - 2.0f);     // ensure there's room for interpolation (padding)

    int integerDelay = int(delayInSamples);             // integer part of the delay

    // Calculate base read indices relative to writeIndex.
    // These pick four consecutive samples needed for the interpolation.
    int readIndexA = writeIndex - integerDelay + 1;     // nearest sample "A"
    int readIndexB = readIndexA - 1;                                // sample "B"
    int readIndexC = readIndexA - 2;                                // sample "C"
    int readIndexD = readIndexA - 3;                                // sample "D"

    // If any index is negative, wrap them by adding bufferLength so indices remain valid.
    // This nested structure updates D, C, B, A only if needed (minimizes branches).
    if (readIndexD < 0) {
        readIndexD += bufferLength;
        if (readIndexC < 0) {
            readIndexC += bufferLength;
            if (readIndexB < 0) {
                readIndexB += bufferLength;
                if (readIndexA < 0) {
                    readIndexA += bufferLength;
                }
            }
        }
    }

    // Fetch the four samples from the circular buffer used by the interpolation routine.
    float sampleA = buffer[size_t(readIndexA)];
    float sampleB = buffer[size_t(readIndexB)];
    float sampleC = buffer[size_t(readIndexC)];
    float sampleD = buffer[size_t(readIndexD)];

    // Compute fractional part between integerDelay and the requested delay.
    float fraction = delayInSamples - float(integerDelay);

    // The following compute slopes and coefficients for a 4-point interpolation
    // (a form of cubic interpolation—Hermite-like / cubic Lagrange mix).
    float slope0 = (sampleC - sampleA) * 0.5f;      // slope between A and C (approx derivative)
    float slope1 = (sampleD - sampleB) * 0.5f;      // slope between B and D
    float v = sampleB - sampleC;                            // difference between adjacent center samples
    float w = slope0 + v;                                           // helper term
    float a = w + v + slope1;                                    // cubic coefficient a
    float b = w + a;                                                  // cubic coefficient b
    float stage1 = a * fraction - b;                         // intermediate polynomial evaluation
    float stage2 = stage1 * fraction + slope0;      // next stage
    return stage2 * fraction + sampleB;               // final evaluated interpolated value, anchored at sampleB
}
