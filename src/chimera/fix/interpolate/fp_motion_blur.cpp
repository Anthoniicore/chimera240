// SPDX-License-Identifier: GPL-3.0-only

#include <cmath>

#include "../../halo_data/camera.hpp"
#include "../../event/camera.hpp"
#include "../../event/frame.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;

    static float saved_pitch = 0.0f;
    static float saved_yaw   = 0.0f;

    static constexpr float BLUR_STRENGTH = 0.55f;

    // ===== BEFORE RENDER =====
    void fp_motion_blur_before(CameraData &cam) noexcept {
        float pitch = cam.orientation[0].x;
        float yaw   = cam.orientation[0].y;

        float dp = pitch - last_pitch;
        float dy = yaw   - last_yaw;

        // Guardar estado real
        saved_pitch = pitch;
        saved_yaw   = yaw;

        // Aplicar blur VISUAL
        cam.orientation[0].x -= dp * BLUR_STRENGTH;
        cam.orientation[0].y -= dy * BLUR_STRENGTH;

        last_pitch = pitch;
        last_yaw   = yaw;
    }

    // ===== AFTER RENDER (ROLLBACK) =====
    void fp_motion_blur_after(CameraData &cam) noexcept {
        cam.orientation[0].x = saved_pitch;
        cam.orientation[0].y = saved_yaw;
    }

    void fp_motion_blur_clear() noexcept {
        last_pitch = last_yaw = 0.0f;
    }
}





