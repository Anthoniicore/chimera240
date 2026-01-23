#include "../../event/camera.hpp"
#include "../../event/frame.hpp"
#include "../../halo_data/camera.hpp"

namespace Chimera {

    static float last_pitch = 0.0f;
    static float last_yaw   = 0.0f;

    static constexpr float BLUR_STRENGTH = 0.35f;

    static void on_camera(CameraData &cam) noexcept {
        float pitch = cam.orientation[0].x;
        float yaw   = cam.orientation[0].y;

        float dp = pitch - last_pitch;
        float dy = yaw   - last_yaw;

        cam.orientation[0].x -= dp * BLUR_STRENGTH;
        cam.orientation[0].y -= dy * BLUR_STRENGTH;

        last_pitch = pitch;
        last_yaw   = yaw;
    }

    static void on_frame_end() noexcept {
        // No restauramos nada porque Chimera
        // reconstruye la cámara cada frame
    }

    void fp_motion_blur_init() noexcept {
        event::camera::subscribe(on_camera);
        event::frame::subscribe(on_frame_end);
    }
}


