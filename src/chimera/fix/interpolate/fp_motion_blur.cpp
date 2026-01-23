// SPDX-License-Identifier: GPL-3.0-only

#include "fp_motion_blur.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;
    static bool  valid_last = false;

    static constexpr float BLUR_STRENGTH = 0.6f;

    // Se llama ANTES del render
    void fp_motion_blur_before(CameraData &cam) noexcept {

        // Solo primera persona
        if(cam.type != CameraType::FIRST_PERSON) {
            return;
        }

        float pitch = cam.orientation[0].x;
        float yaw   = cam.orientation[0].y;

        if(!valid_last) {
            last_pitch = pitch;
            last_yaw   = yaw;
            valid_last = true;
            return;
        }

        float dp = pitch - last_pitch;
        float dy = yaw   - last_yaw;

        // Aplicamos blur SOLO visual
        cam.orientation[0].x -= dp * BLUR_STRENGTH;
        cam.orientation[0].y -= dy * BLUR_STRENGTH;
    }

    // Se llama DESPUÉS del render
    void fp_motion_blur_after(CameraData &cam) noexcept {

        if(cam.type != CameraType::FIRST_PERSON) {
            return;
        }

        last_pitch = cam.orientation[0].x;
        last_yaw   = cam.orientation[0].y;
    }

    void fp_motion_blur_clear() noexcept {
        valid_last = false;
        last_pitch = 0.0f;
        last_yaw   = 0.0f;
    }

}



