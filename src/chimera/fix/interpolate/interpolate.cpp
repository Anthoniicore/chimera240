// SPDX-License-Identifier: GPL-3.0-only

#include <cctype>
#include <cmath>

#include "../../chimera.hpp"
#include "../../signature/hook.hpp"
#include "../../signature/signature.hpp"
#include "../../halo_data/multiplayer.hpp"
#include "../../halo_data/pause.hpp"
#include "../../event/camera.hpp"
#include "../../event/frame.hpp"
#include "../../event/tick.hpp"
#include "../../event/revert.hpp"
#include "../../halo_data/game_engine.hpp"
#include "../../output/output.hpp"

#include "antenna.hpp"
#include "camera.hpp"
#include "flag.hpp"
#include "fp.hpp"
#include "light.hpp"
#include "object.hpp"
#include "particle.hpp"

#include "interpolate.hpp"

namespace Chimera {

    // Progress since last tick (0.0 → 1.0)
    float interpolation_tick_progress = 0.0f;

    static float *first_person_camera_tick_rate = nullptr;
    bool interpolation_enabled = false;

    // ====== 240 FPS SAFETY ======
    static float last_interp_progress = 0.0f;

    // ~8 ms delay @ 30 ticks/sec (Valorant-like buffer)
    constexpr float INTERP_DELAY = 0.25f;

    constexpr float MIN_PROGRESS_DELTA = 0.0001f;

    static inline float clamp01(float v) noexcept {
        if(v < 0.0f) return 0.0f;
        if(v > 1.0f) return 1.0f;
        return v;
    }

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

        float current_tick_rate = effective_tick_rate();
        if(*first_person_camera_tick_rate != current_tick_rate) {
            overwrite(first_person_camera_tick_rate, current_tick_rate);
        }
    }

    static void on_preframe() noexcept {
        if(game_paused()) {
            return;
        }

        float raw = get_tick_progress();

        // Apply interpolation delay (critical for 240 FPS stability)
        float delayed = raw - INTERP_DELAY;

        interpolation_tick_progress = clamp01(delayed);

        float delta = interpolation_tick_progress - last_interp_progress;
        if(delta <= MIN_PROGRESS_DELTA) {
            return; // skip meaningless frames (prevents float jitter)
        }

        last_interp_progress = interpolation_tick_progress;

        interpolate_antenna_before();
        interpolate_flag_before();
        interpolate_light_before();
        interpolate_object_before();
        interpolate_particle();
    }

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

        // Disable built-in FP interpolation
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
