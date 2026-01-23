// SPDX-License-Identifier: GPL-3.0-only

#include <cmath>

#include "../../chimera.hpp"
#include "../../signature/hook.hpp"
#include "../../signature/signature.hpp"
#include "../../halo_data/pause.hpp"
#include "../../event/camera.hpp"
#include "../../event/frame.hpp"
#include "../../event/tick.hpp"
#include "../../event/revert.hpp"

#include "antenna.hpp"
#include "camera.hpp"
#include "flag.hpp"
#include "fp.hpp"
#include "light.hpp"
#include "object.hpp"
#include "particle.hpp"
#include "interpolate.hpp"

namespace Chimera {

    // Alpha [0..1] since last tick
    float interpolation_tick_progress = 0.0f;

    static float *first_person_camera_tick_rate = nullptr;
    bool interpolation_enabled = false;

    // ---- 240 FPS STABILITY ----
    static float last_interp_progress = 0.0f;
    constexpr float MIN_PROGRESS_DELTA = 0.0001f;

    static inline float clamp01(float v) noexcept {
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    // ===== TICK =====
    static void on_tick() noexcept {
        if(game_paused()) {
            return;
        }

        interpolate_antenna_on_tick();
        interpolate_flag_on_tick();
        interpolate_fp_on_tick();
        interpolate_light_on_tick();
        interpolate_object_on_tick();
        interpolate_camera_on_tick();
        interpolate_particle_on_tick();

        interpolation_tick_progress = 0.0f;
        last_interp_progress = 0.0f;

        float tick_rate = effective_tick_rate();
        if(*first_person_camera_tick_rate != tick_rate) {
            overwrite(first_person_camera_tick_rate, tick_rate);
        }
    }

    // ===== PRE-FRAME (INTERPOLATION) =====
    static void on_preframe() noexcept {
        if(game_paused()) {
            return;
        }

        float raw = clamp01(get_tick_progress());

        // Ensure monotonic progress (critical at high FPS)
        if(raw < last_interp_progress) {
            raw = last_interp_progress;
        }

        float delta = raw - last_interp_progress;
        if(delta < MIN_PROGRESS_DELTA) {
            return; // skip insignificant frames (prevents jitter)
        }

        interpolation_tick_progress = raw;
        last_interp_progress = raw;

        interpolate_antenna_before();
        interpolate_flag_before();
        interpolate_light_before();
        interpolate_object_before();
        interpolate_particle();
    }

    // ===== FRAME END (ROLLBACK) =====
    static void on_frame() noexcept {
        if(game_paused()) {
            return;
        }

        interpolate_antenna_after();
        interpolate_object_after();
        interpolate_particle_after();
    }

    void clear_buffers() noexcept {
        interpolate_object_clear();
        interpolate_particle_clear();
        interpolate_light_clear();
        interpolate_flag_clear();
        interpolate_camera_clear();
        interpolate_fp_clear();
    }

    void set_up_interpolation() noexcept {
        static auto *fp_interp_ptr = get_chimera().get_signature("fp_interp_sig").data();
        static Hook fp_interp_hook;

        first_person_camera_tick_rate =
            *reinterpret_cast<float **>(
                get_chimera().get_signature("fp_cam_tick_rate_sig").data() + 2
            );

        add_tick_event(on_tick);
        add_preframe_event(on_preframe);
        add_frame_event(on_frame);
        add_precamera_event(interpolate_camera_before);
        add_camera_event(interpolate_camera_after);

        write_jmp_call(
            fp_interp_ptr,
            fp_interp_hook,
            reinterpret_cast<const void *>(interpolate_fp_before),
            reinterpret_cast<const void *>(interpolate_fp_after)
        );

        // Disable Halo built-in FP interpolation
        overwrite(
            get_chimera().get_signature("camera_interpolation_sig").data() + 0xF,
            static_cast<unsigned char>(0xEB)
        );

        add_revert_event(clear_buffers);
        interpolation_enabled = true;
    }

    void disable_interpolation() noexcept {
        get_chimera().get_signature("fp_interp_sig").rollback();
        get_chimera().get_signature("camera_interpolation_sig").rollback();

        remove_tick_event(on_tick);
        remove_preframe_event(on_preframe);
        remove_frame_event(on_frame);
        remove_precamera_event(interpolate_camera_before);
        remove_camera_event(interpolate_camera_after);
        remove_revert_event(clear_buffers);

        interpolation_enabled = false;
    }
}
