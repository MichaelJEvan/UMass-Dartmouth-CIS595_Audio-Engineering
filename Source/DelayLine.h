/*
  ==============================================================================
     DelayLine.h
     Created: 18 Jan 2026
     Author:  Michael J Evan
              CIS-595 Independent Study
              UMass Dartmouth
              Graduate Computer Science Dept
              Spring 2026
  ==============================================================================
*/

#pragma once    // include guard: ensure this header is included only once

#include <memory>   // for std::unique_ptr used to own the circular buffer

// Simple circular delay buffer class (mono) providing write and fractional-read access.
class DelayLine
{
public:
    // Allocate or resize the internal buffer to hold at least maxLengthInSamples samples.
    // Typically called from prepareToPlay with sampleRate * maxDelaySeconds.
    void setMaximumDelayInSamples(int maxLengthInSamples);

    // Clear the buffer and reset indices (safe to call from real-time code).
    void reset() noexcept;

    // Write a sample into the delay buffer at the current write position and advance the index.
    // Should be real-time safe (no allocations).
    void write(float input) noexcept;

    // Read a delayed sample. delayInSamples can be fractional (implementation typically
    // does linear or higher-order interpolation). The method is const and real-time safe.
    float read(float delayInSamples) const noexcept;

    // Return the current buffer length in samples (capacity).
    int getBufferLength() const noexcept
    {
        return bufferLength;
    }

private:
    std::unique_ptr<float[]> buffer; // owned raw float array for the circular buffer
    int bufferLength = 0;            // capacity of the buffer in samples
    int writeIndex = 0;              // index where the most recent value was written (next write will overwrite at this pos)
};
