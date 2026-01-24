// SPDX-License-Identifier: GPL-3.0-only

#ifndef CHIMERA_INTERPOLATE_HPP
#define CHIMERA_INTERPOLATE_HPP

namespace Chimera {
    // Interpolation progress between ticks (0.0 → 1.0)
    // Updated every frame
    extern float interpolation_tick_progress;

    // Whether Chimera interpolation is currently enabled
    extern bool interpolation_enabled;

    // Setup / teardown
    void set_up_interpolation() noexcept;
    void disable_interpolation() noexcept;

    inline float interpolate_cubic(
        float p0,
        float p1,
        float p2,
        float p3,
        float t
    ) noexcept {
        const float t2 = t * t;
        const float t3 = t2 * t;

        return 0.5f * (
            (2.0f * p1) +
            (-p0 + p2) * t +
            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
    }

 }
#endif
