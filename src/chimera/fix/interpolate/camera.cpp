// SPDX-License-Identifier: GPL-3.0-only

#include <algorithm>
#include <cstring>
#include <cmath>

#include "../../halo_data/object.hpp"
#include "../../halo_data/camera.hpp"
#include "../../halo_data/pause.hpp"
#include "../../halo_data/player.hpp"
#include "camera.hpp"
#include "interpolate.hpp"

#include "../../signature/signature.hpp"
#include "../../chimera.hpp"

namespace Chimera {

    struct InterpolatedCamera {
        CameraType type;
        ObjectID followed_object;
        CameraData data;
    };

    // Necesitamos 3 buffers para cúbica
    static InterpolatedCamera camera_buffers[3];

    static InterpolatedCamera *prev2_tick  = camera_buffers + 0;
    static InterpolatedCamera *prev1_tick  = camera_buffers + 1;
    static InterpolatedCamera *current_tick = camera_buffers + 2;

    static bool tick_passed = false;
    static bool skip = false;
    static bool rollback = false;

    extern bool spectate_enabled;

    // FPS safety
    static float last_alpha = 0.0f;
    constexpr float MIN_ALPHA_DELTA = 0.0001f;

    static inline float clamp01(float v) noexcept {
        return std::max(0.0f, std::min(1.0f, v));
    }

    void interpolate_camera_before() noexcept {
        if(game_paused() || !interpolation_enabled) {
            return;
        }

        float alpha = clamp01(interpolation_tick_progress);
        if(alpha - last_alpha <= MIN_ALPHA_DELTA) {
            return;
        }
        last_alpha = alpha;

        auto type = camera_type();

        if(tick_passed) {
            // rotar buffers
            std::swap(prev2_tick, prev1_tick);
            std::swap(prev1_tick, current_tick);

            static auto **followed_object =
                reinterpret_cast<ObjectID **>(
                    get_chimera().get_signature("followed_object_sig").data() + 10
                );

            current_tick->data = camera_data();
            current_tick->type = type;
            current_tick->followed_object = **followed_object;

            tick_passed = false;
            rollback = false;

            skip =
                (type == CameraType::CAMERA_CINEMATIC &&
                 current_tick->followed_object.is_null()) ||
                (current_tick->followed_object != prev1_tick->followed_object ||
                 current_tick->type != prev1_tick->type);

            if(!skip && type == CameraType::CAMERA_FIRST_PERSON) {
                skip =
                    distance_squared(
                        prev1_tick->data.position,
                        current_tick->data.position
                    ) > 25.0f;
            }
        }

        if(skip) {
            return;
        }

        auto &data = camera_data();

        bool vehicle_first_person = false;

        if(type == CameraType::CAMERA_FIRST_PERSON ||
           type == CameraType::CAMERA_DEBUG) {

            auto *player =
                PlayerTable::get_player_table().get_client_player();

            if(player) {
                auto *object =
                    ObjectTable::get_object_table()
                        .get_dynamic_object(player->object_id);

                if(object) {
                    vehicle_first_person = !object->parent.is_null();

                    if(type == CameraType::CAMERA_DEBUG &&
                       object->health >= 0.0f) {
                        skip = true;
                        return;
                    }
                }
            }
        }

        // ===== POSICIÓN (CÚBICA) =====
        interpolate_cubic(
            prev2_tick->data.position,
            prev1_tick->data.position,
            current_tick->data.position,
            current_tick->data.position, // fallback estable
            data.position,
            alpha
        );

        // ===== ORIENTACIÓN =====
        if(type != CameraType::CAMERA_FIRST_PERSON ||
           vehicle_first_person ||
           spectate_enabled) {

            interpolate_cubic(
                prev2_tick->data.orientation[0],
                prev1_tick->data.orientation[0],
                current_tick->data.orientation[0],
                current_tick->data.orientation[0],
                data.orientation[0],
                alpha
            );

            interpolate_cubic(
                prev2_tick->data.orientation[1],
                prev1_tick->data.orientation[1],
                current_tick->data.orientation[1],
                current_tick->data.orientation[1],
                data.orientation[1],
                alpha
            );

            rollback = true;
        }
    }

    void interpolate_camera_after() noexcept {
        if(skip || game_paused() || !interpolation_enabled) {
            return;
        }

        auto &data = camera_data();
        data.position = current_tick->data.position;

        if(rollback) {
            std::copy(
                current_tick->data.orientation,
                current_tick->data.orientation + 2,
                data.orientation
            );
            rollback = false;
        }
    }

    void interpolate_camera_clear() noexcept {
        skip = true;
        rollback = false;
        last_alpha = 0.0f;
        std::memset(camera_buffers, 0, sizeof(camera_buffers));
    }

    void interpolate_camera_on_tick() noexcept {
        tick_passed = true;
        last_alpha = 0.0f;
    }
}
