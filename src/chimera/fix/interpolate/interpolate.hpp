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
}

#endif
