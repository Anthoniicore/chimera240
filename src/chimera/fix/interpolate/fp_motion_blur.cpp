// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/camera.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;

    static constexpr float BLUR_STRENGTH = 0.70f;

    void fp_motion_blur_before() noexcept {
        if(!camera_data) {
            return;
        }

        last_pitch = camera_data->orientation[0].x;
        last_yaw   = camera_data->orientation[0].y;
    }

    void fp_motion_blur_after() noexcept {
        if(!camera_data) {
            return;
        }

        float pitch = camera_data->orientation[0].x;
        float yaw   = camera_data->orientation[0].y;

        float dp = pitch - last_pitch;
        float dy = yaw   - last_yaw;

        camera_data->orientation[0].x -= dp * BLUR_STRENGTH;
        camera_data->orientation[0].y -= dy * BLUR_STRENGTH;
    }

    void fp_motion_blur_clear() noexcept {
        last_pitch = 0.0f;
        last_yaw   = 0.0f;
    }
}


