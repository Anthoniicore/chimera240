// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/camera.hpp"
#include "../../event/camera.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;
    static bool  valid_last = false;

    static constexpr float BLUR_STRENGTH = 0.65f;

    void fp_motion_blur_before() noexcept {
        // NO tocamos la cámara lógica
        // solo guardamos estado
        auto &cam = get_first_person_camera();

        if(!valid_last) {
            last_pitch = cam.orientation[0].x;
            last_yaw   = cam.orientation[0].y;
            valid_last = true;
            return;
        }

        float dp = cam.orientation[0].x - last_pitch;
        float dy = cam.orientation[0].y - last_yaw;

        cam.orientation[0].x -= dp * BLUR_STRENGTH;
        cam.orientation[0].y -= dy * BLUR_STRENGTH;
    }

    void fp_motion_blur_after() noexcept {
        // restauramos valores EXACTOS
        auto &cam = get_first_person_camera();

        last_pitch = cam.orientation[0].x;
        last_yaw   = cam.orientation[0].y;
    }

    void fp_motion_blur_clear() noexcept {
        valid_last = false;
        last_pitch = 0.0f;
        last_yaw   = 0.0f;
    }

}




