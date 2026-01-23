// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/camera.hpp"
#include "../../halo_data/pause.hpp"

#include "fp_motion_blur.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;
    static bool  has_last   = false;

    constexpr float BLUR_STRENGTH = 0.35f;

    void fp_motion_blur_before() noexcept {
        if(game_paused()) {
            return;
        }

        auto &cam = camera_data;

        float pitch = cam.orientation[0].x;
        float yaw   = cam.orientation[0].y;

        if(!has_last) {
            last_pitch = pitch;
            last_yaw   = yaw;
            has_last   = true;
            return;
        }

        float pitch_delta = pitch - last_pitch;
        float yaw_delta   = yaw   - last_yaw;

        cam.orientation[0].x = pitch - pitch_delta * BLUR_STRENGTH;
        cam.orientation[0].y = yaw   - yaw_delta   * BLUR_STRENGTH;
    }

    void fp_motion_blur_after() noexcept {
        if(!has_last || game_paused()) {
            return;
        }

        auto &cam = camera_data;

        cam.orientation[0].x = last_pitch;
        cam.orientation[0].y = last_yaw;

        last_pitch = cam.orientation[0].x;
        last_yaw   = cam.orientation[0].y;
    }

    void fp_motion_blur_clear() noexcept {
        has_last = false;
    }

    void fp_motion_blur_on_tick() noexcept {
        has_last = false;
    }

}
