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

    static InterpolatedCamera camera_buffers[2] = {}; // Volvemos a 2 buffers físicos
    static InterpolatedCamera *current_tick  = &camera_buffers[0];
    static InterpolatedCamera *previous_tick = &camera_buffers[1];

    static bool tick_passed = false;
    static bool skip = false;
    static bool rollback = false;

    extern bool spectate_enabled;
    static float last_alpha = -1.0f;

    static ObjectID** get_followed_object_ptr() noexcept {
        static ObjectID** ptr = nullptr;
        if (!ptr) {
            auto sig = get_chimera().get_signature("followed_object_sig");
            if (sig.data()) ptr = reinterpret_cast<ObjectID **>(sig.data() + 10);
        }
        return ptr;
    }

    void interpolate_camera_before() noexcept {
        if(game_paused() || !interpolation_enabled) return;

        float alpha = std::clamp(interpolation_tick_progress, 0.0f, 1.0f);
        
        // No retornar prematuramente si alpha es 0 o 1, para asegurar actualización
        if(std::abs(alpha - last_alpha) < 0.00001f && alpha > 0.0f && alpha < 1.0f) return;
        last_alpha = alpha;

        auto type = camera_type();
        auto** followed_ptr = get_followed_object_ptr();

        if(tick_passed) {
            std::swap(current_tick, previous_tick);
            
            current_tick->data = camera_data();
            current_tick->type = type;
            current_tick->followed_object = followed_ptr ? **followed_ptr : ObjectID();

            tick_passed = false;
            rollback = false;

            // Solo saltar si hay un cambio drástico de tipo de cámara
            skip = (current_tick->type != previous_tick->type);

            if(!skip) {
                float dist_sq = distance_squared(previous_tick->data.position, current_tick->data.position);
                // Si el salto es mayor a 30 unidades (teleport), saltamos interpolación
                if (dist_sq > 900.0f) skip = true;
            }
        }

        if(skip) {
            // Si saltamos, forzamos la posición actual para evitar el "stutter"
            return;
        }

        auto &data = camera_data();
        
        // INTERPOLACIÓN DE POSICIÓN (Forzada para máxima suavidad)
        interpolate_point(previous_tick->data.position, current_tick->data.position, data.position, alpha);

        // INTERPOLACIÓN DE ORIENTACIÓN
        // En primera persona a pie, Halo maneja la rotación muy rápido. 
        // Solo interpolamos si no es FP pura o si estamos en vehículo/espectador.
        bool vehicle_fp = false;
        if(auto *player = PlayerTable::get_player_table().get_client_player()) {
            if(auto *object = ObjectTable::get_object_table().get_dynamic_object(player->object_id)) {
                vehicle_fp = !object->parent.is_null();
            }
        }

        if(type != CameraType::CAMERA_FIRST_PERSON || vehicle_fp || spectate_enabled) {
            // Interpolar vectores de orientación (Forward y Up)
            interpolate_point(previous_tick->data.orientation[0], current_tick->data.orientation[0], data.orientation[0], alpha);
            interpolate_point(previous_tick->data.orientation[1], current_tick->data.orientation[1], data.orientation[1], alpha);
            rollback = true;
        }
    }

    void interpolate_camera_after() noexcept {
        if(skip || game_paused() || !interpolation_enabled) return;

        auto &data = camera_data();
        // Siempre restaurar para que el motor de física no se vuelva loco
        data.position = current_tick->data.position;

        if(rollback) {
            std::memcpy(data.orientation, current_tick->data.orientation, sizeof(data.orientation));
            rollback = false;
        }
    }

    void interpolate_camera_clear() noexcept {
        skip = true;
        rollback = false;
        last_alpha = -1.0f;
        std::memset(camera_buffers, 0, sizeof(camera_buffers));
    }

    void interpolate_camera_on_tick() noexcept {
        tick_passed = true;
    }
}
