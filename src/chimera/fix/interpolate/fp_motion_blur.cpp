// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/camera.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;

    static constexpr float BLUR_STRENGTH = 0.70f;

    void fp_motion_blur_before() noexcept {
        auto *cam = get_camera_data();
        if(!cam) return;

        last_pitch = cam->orientation[0].x;
        last_yaw   = cam->orientation[0].y;
    }

    void fp_motion_blur_after() noexcept {
        auto *cam = get_camera_data();
        if(!cam) return;

        float pitch = cam->orientation[0].x;
        float yaw   = cam->orientation[0].y;

        float dp = pitch - last_pitch;
        float dy = yaw   - last_yaw;

        cam->orientation[0].x -= dp * BLUR_STRENGTH;
        cam->orientation[0].y -= dy * BLUR_STRENGTH;
    }

    void fp_motion_blur_clear() noexcept {
        last_pitch = 0.0f;
        last_yaw   = 0.0f;
    }
}

