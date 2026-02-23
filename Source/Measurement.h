/*
  ==============================================================================
    Measurement.h
    Created: 18 Jan 2026
    Author:  Michael Evan
             CIS-595 Independent Study
             UMass Dartmouth
             Graduate Computer Science Dept
             Spring 2026
 
    Note:   header-only, thread-safe peak tracker storing a floating-point
            value in an atomic. It's safe to update from the audio thread
            and read/reset from the UI thread (or vice versa).
            updateIfGreater uses an atomic compare-exchange loop to
            publish only larger values without locking.
  ==============================================================================
*/

#pragma once        // ensure this header is included only once per TU

#include <atomic>   // std::atomic used for lock-free thread-safety

struct Measurement
{
    // Reset the stored value to 0.0 (atomic store)
    void reset() noexcept
    {
        value.store(0.0f);
    }

    // Atomically update the stored value only if newValue is greater than the current value.
    // Uses compare_exchange_weak in a loop to implement a lock-free 'max' operation.
    void updateIfGreater(float newValue) noexcept
    {
        auto oldValue = value.load(); // read current value
        // while newValue is greater than oldValue, try to replace oldValue with newValue.
        // compare_exchange_weak updates oldValue on failure, so the loop retries until success or condition no longer holds.
        while (newValue > oldValue && !value.compare_exchange_weak(oldValue, newValue));
    }

    // Atomically read the current value and reset it to 0.0, returning the previous value.
    // Useful for periodic polling (e.g., UI reading peak once per frame).
    float readAndReset() noexcept
    {
        return value.exchange(0.0f);
    }

    std::atomic<float> value; // atomic storage for the measured value (default-initialized by user)
};
