// SPDX-License-Identifier: GPL-3.0-only

#ifndef CHIMERA_INTERPOLATE_FP_HPP
#define CHIMERA_INTERPOLATE_FP_HPP

#include "interpolate.hpp"

namespace Chimera {

    /**
     * Interpolate first person (visual only).
     * Uses interpolation_tick_progress (0.0 → 1.0).
     */
    void interpolate_fp_before() noexcept;

    /**
     * Restore original first person state after rendering.
     */
    void interpolate_fp_after() noexcept;

    /**
     * Clear interpolation buffers.
     * Should be called on state changes (map load, revert, etc.).
     */
    void interpolate_fp_clear() noexcept;

    /**
     * Mark that a new tick has passed (swap buffers next frame).
     */
    void interpolate_fp_on_tick() noexcept;
}

#endif
