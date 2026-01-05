/**
 * DopplerShiftTest.cpp
 *
 * Unit test for Doppler shift stability in SpatialProcessor.
 * Verifies that Doppler shift calculations remain bounded and stable
 * across various velocity configurations.
 *
 * Part of Three-System Plan Phase 3.
 */

#include "../dsp/SpatialProcessor.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace monument::dsp;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int kBlockSize = 512;
    constexpr int kNumLines = 8;
    constexpr float kEpsilon = 0.0001f;
    constexpr float kMaxExpectedShift = 2400.0f; // ±50ms @ 48kHz
}

void testDopplerShiftBounds()
{
    std::cout << "Testing Doppler shift bounds...\n";

    SpatialProcessor spatial;
    spatial.prepare(kSampleRate, kBlockSize, kNumLines);
    spatial.setEnabled(true);
    spatial.setDopplerScale(1.0f); // Full Doppler effect

    // Test maximum positive velocity (moving away)
    spatial.setVelocity(0, 1.0f);
    spatial.process();
    float shiftAway = spatial.getDopplerShift(0);

    // Test maximum negative velocity (moving toward)
    spatial.setVelocity(1, -1.0f);
    spatial.process();
    float shiftToward = spatial.getDopplerShift(1);

    // Verify shifts are within expected bounds
    assert(std::abs(shiftAway) <= kMaxExpectedShift);
    assert(std::abs(shiftToward) <= kMaxExpectedShift);

    // Verify positive velocity gives positive shift (delay increases)
    assert(shiftAway > 0.0f);

    // Verify negative velocity gives negative shift (delay decreases)
    assert(shiftToward < 0.0f);

    // Verify symmetry
    assert(std::abs(std::abs(shiftAway) - std::abs(shiftToward)) < kEpsilon);

    std::cout << "  ✓ Max velocity away: " << shiftAway << " samples\n";
    std::cout << "  ✓ Max velocity toward: " << shiftToward << " samples\n";
    std::cout << "  ✓ Shifts within bounds [±" << kMaxExpectedShift << "]\n";
}

void testDopplerShiftScaling()
{
    std::cout << "\nTesting Doppler scale parameter...\n";

    SpatialProcessor spatial;
    spatial.prepare(kSampleRate, kBlockSize, kNumLines);
    spatial.setEnabled(true);
    spatial.setVelocity(0, 1.0f); // Full velocity

    // Test at 100% scale
    spatial.setDopplerScale(1.0f);
    spatial.process();
    float shift100 = spatial.getDopplerShift(0);

    // Test at 50% scale
    spatial.setDopplerScale(0.5f);
    spatial.process();
    float shift50 = spatial.getDopplerShift(0);

    // Test at 0% scale (disabled)
    spatial.setDopplerScale(0.0f);
    spatial.process();
    float shift0 = spatial.getDopplerShift(0);

    // Verify scaling relationship
    assert(std::abs(shift50 - shift100 * 0.5f) < kEpsilon);
    assert(std::abs(shift0) < kEpsilon);

    std::cout << "  ✓ 100% scale: " << shift100 << " samples\n";
    std::cout << "  ✓ 50% scale: " << shift50 << " samples\n";
    std::cout << "  ✓ 0% scale: " << shift0 << " samples\n";
    std::cout << "  ✓ Scaling proportional\n";
}

void testDopplerShiftDisabled()
{
    std::cout << "\nTesting disabled spatial processor...\n";

    SpatialProcessor spatial;
    spatial.prepare(kSampleRate, kBlockSize, kNumLines);
    spatial.setEnabled(false); // Disable spatial processing
    spatial.setVelocity(0, 1.0f);
    spatial.setDopplerScale(1.0f);
    spatial.process();

    float shift = spatial.getDopplerShift(0);

    // When disabled, Doppler should return zero
    assert(std::abs(shift) < kEpsilon);

    std::cout << "  ✓ Shift with spatial disabled: " << shift << " samples\n";
    std::cout << "  ✓ Zero shift confirmed\n";
}

void testDopplerShiftStability()
{
    std::cout << "\nTesting Doppler shift stability over time...\n";

    SpatialProcessor spatial;
    spatial.prepare(kSampleRate, kBlockSize, kNumLines);
    spatial.setEnabled(true);
    spatial.setDopplerScale(0.8f);
    spatial.setVelocity(0, 0.6f); // Moderate velocity

    // Process multiple blocks and verify shift remains stable
    constexpr int kNumBlocks = 100;
    float firstShift = 0.0f;
    bool stable = true;

    for (int i = 0; i < kNumBlocks; ++i)
    {
        spatial.process();
        float shift = spatial.getDopplerShift(0);

        if (i == 0)
            firstShift = shift;

        // Verify shift doesn't drift (constant velocity = constant shift)
        if (std::abs(shift - firstShift) > kEpsilon)
        {
            stable = false;
            std::cout << "  ✗ Shift drifted at block " << i
                      << ": expected " << firstShift
                      << ", got " << shift << "\n";
        }
    }

    assert(stable);
    std::cout << "  ✓ Shift stable over " << kNumBlocks << " blocks: "
              << firstShift << " samples\n";
}

void testDopplerShiftPerLine()
{
    std::cout << "\nTesting independent Doppler per delay line...\n";

    SpatialProcessor spatial;
    spatial.prepare(kSampleRate, kBlockSize, kNumLines);
    spatial.setEnabled(true);
    spatial.setDopplerScale(1.0f);

    // Set different velocities for each line
    for (int i = 0; i < kNumLines; ++i)
    {
        float velocity = -1.0f + (2.0f * i) / (kNumLines - 1); // Range [-1, +1]
        spatial.setVelocity(i, velocity);
    }

    spatial.process();

    // Verify each line has independent shift
    for (int i = 0; i < kNumLines; ++i)
    {
        float shift = spatial.getDopplerShift(i);
        float expectedVelocity = -1.0f + (2.0f * i) / (kNumLines - 1);

        // Verify shift sign matches velocity sign
        if (expectedVelocity > 0.0f)
            assert(shift > 0.0f);
        else if (expectedVelocity < 0.0f)
            assert(shift < 0.0f);
        else
            assert(std::abs(shift) < kEpsilon);

        std::cout << "  ✓ Line " << i << " velocity=" << expectedVelocity
                  << " → shift=" << shift << " samples\n";
    }
}

void testDopplerShiftClipping()
{
    std::cout << "\nTesting Doppler shift clipping at extremes...\n";

    SpatialProcessor spatial;
    spatial.prepare(kSampleRate, kBlockSize, kNumLines);
    spatial.setEnabled(true);
    spatial.setDopplerScale(1.0f);

    // Test beyond valid velocity range (should be clamped internally)
    spatial.setVelocity(0, 2.0f); // Beyond [−1, +1]
    spatial.process();
    float shiftOverflow = spatial.getDopplerShift(0);

    // Should not exceed maximum shift
    assert(shiftOverflow <= kMaxExpectedShift);

    std::cout << "  ✓ Velocity=2.0 (clamped) → shift=" << shiftOverflow << " samples\n";
    std::cout << "  ✓ Shift clamped to safe range\n";
}

int main()
{
    std::cout << "========================================\n";
    std::cout << "Doppler Shift Stability Test\n";
    std::cout << "Three-System Plan Phase 3\n";
    std::cout << "========================================\n\n";

    try
    {
        testDopplerShiftBounds();
        testDopplerShiftScaling();
        testDopplerShiftDisabled();
        testDopplerShiftStability();
        testDopplerShiftPerLine();
        testDopplerShiftClipping();

        std::cout << "\n========================================\n";
        std::cout << "✓ All Doppler shift tests passed!\n";
        std::cout << "========================================\n";

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n✗ Test failed: " << e.what() << "\n";
        return 1;
    }
}
