// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/camera.hpp"
#include "../../event/camera.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;

    static constexpr float BLUR_STRENGTH = 0.30f;

    /**
     * This is called automatically by Chimera every frame
     * for the render camera (NOT the logical camera).
     */
    void on_camera(CameraData &cam) noexcept {
        float pitch = cam.orientation[0].x;
        float yaw   = cam.orientation[0].y;

        float dp = pitch - last_pitch;
        float dy = yaw   - last_yaw;

        cam.orientation[0].x -= dp * BLUR_STRENGTH;
        cam.orientation[0].y -= dy * BLUR_STRENGTH;

        last_pitch = pitch;
        last_yaw   = yaw;
    }
}




