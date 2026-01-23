
// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/camera.hpp"
#include "../../halo_data/pause.hpp"
#include "fp_motion_blur.hpp"

namespace Chimera {

    static float last_yaw = 0.0f;
    static float last_pitch = 0.0f;

    static float blurred_yaw = 0.0f;
    static float blurred_pitch = 0.0f;

    static bool initialized = false;

    constexpr float BLUR_STRENGTH = 0.15f;

    void fp_motion_blur_before() noexcept {
        if(game_paused()) {
            return;
        }

        auto &cam = camera_data();

        if(!initialized) {
            last_yaw = cam.orientation[0].yaw;
            last_pitch = cam.orientation[0].pitch;
            blurred_yaw = last_yaw;
            blurred_pitch = last_pitch;
            initialized = true;
            return;
        }

        float yaw_delta = cam.orientation[0].yaw - last_yaw;
        float pitch_delta = cam.orientation[0].pitch - last_pitch;

        blurred_yaw += yaw_delta * BLUR_STRENGTH;
        blurred_pitch += pitch_delta * BLUR_STRENGTH;

        last_yaw = cam.orientation[0].yaw;
        last_pitch = cam.orientation[0].pitch;

        cam.orientation[0].yaw = blurred_yaw;
        cam.orientation[0].pitch = blurred_pitch;
    }

    void fp_motion_blur_after() noexcept {
        if(game_paused()) {
            return;
        }

        auto &cam = camera_data();
        cam.orientation[0].yaw = last_yaw;
        cam.orientation[0].pitch = last_pitch;
    }

    void fp_motion_blur_clear() noexcept {
        initialized = false;
    }
}


