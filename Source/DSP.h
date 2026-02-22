/*
  ==============================================================================
    DSP.h
    Created: 18 Jan 2026
    Author:  Michael Evan
 
    Note:   DSP.h provides a tiny, header-only helper that computes equal‑power
            panning gains for left and right channels. Call panningEqualPower
            with panning in [-1..1]; it writes left/right gain values (≈0..1)
            via reference.
 
            The function is inline so it can be defined in the header
            without violating C++ One Definition Rule.
  ==============================================================================
*/

#pragma once // ensure this header is included only once per translation unit

#include <cmath>    // std::cos, std::sin used for gain calculations

// note: inline used because the implementation lives in the header
// note: constant 0.7853981633974483f = pi / 4
// output gains are written to left and right via reference (float&)
inline void panningEqualPower(float panning, float& left, float& right)
{
    // Map panning [-1..1] to angle in radians.
    // When panning == -1 -> x = 0       -> left = cos(0)=1, right = sin(0)=0 (full left)
    // When panning ==  0 -> x = pi/4    -> left = right = 0.707... (center, equal power)
    // When panning == +1 -> x = pi/2    -> left = cos(pi/2)=0, right = 1 (full right)
    float x = 0.7853981633974483f * (panning + 1.0f);

    // Equal-power panning uses cos/sin of the mapped angle to produce gains.
    left = std::cos(x);
    right = std::sin(x);
}
