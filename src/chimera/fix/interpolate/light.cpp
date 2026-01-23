// SPDX-License-Identifier: GPL-3.0-only

#include <cstring>

#include "../../halo_data/light.hpp"

#include "light.hpp"
#include "interpolate.hpp"

namespace Chimera {

    // =========================
    // Light data
    // =========================
    #define MAX_LIGHT 0x380

    struct InterpolatedLight {
        bool interpolate = false;
        Point3D position;
        Point3D orientation[2];
        std::uint32_t some_counter = 0;
    };

    static InterpolatedLight light_buffers[2][MAX_LIGHT];

    // These are pointers to each buffer. These swap every tick.
    static InterpolatedLight *current_tick  = light_buffers[0];
    static InterpolatedLight *previous_tick = light_buffers[1];

    // If true, a tick has passed and it's time to re-copy the light data.
    static bool tick_passed = false;

    void interpolate_light_before() noexcept {
        auto &light_table = LightTable::get_light_table();

        if(tick_passed) {
            // Swap buffers
            if(current_tick == light_buffers[0]) {
                current_tick  = light_buffers[1];
                previous_tick = light_buffers[0];
            }
            else {
                current_tick  = light_buffers[0];
                previous_tick = light_buffers[1];
            }

            tick_passed = false;

            // Copy data
            for(size_t i = 0; i < MAX_LIGHT; i++) {
                current_tick[i].interpolate = false;

                auto *light = light_table.get_element(i);
                if(!light) {
                    continue;
                }

                auto &cur  = current_tick[i];
                auto &prev = previous_tick[i];

                cur.some_counter = light->some_counter;

                // 🔧 FIX: inicializar previous_tick la primera vez
                if(prev.some_counter == 0) {
                    prev.some_counter     = cur.some_counter;
                    prev.position         = light->position;
                    prev.orientation[0]   = light->orientation[0];
                    prev.orientation[1]   = light->orientation[1];
                }

                if(cur.some_counter > prev.some_counter) {
                    cur.interpolate     = true;
                    cur.position        = light->position;
                    cur.orientation[0]  = light->orientation[0];
                    cur.orientation[1]  = light->orientation[1];
                }
            }
        }

        // =========================
        // Interpolate
        // =========================
        extern float interpolation_tick_progress;

        for(size_t i = 0; i < light_table.current_size && i < MAX_LIGHT; i++) {
            auto &cur  = current_tick[i];
            auto &prev = previous_tick[i];
            auto &mem  = light_table.first_element[i];

            if(cur.interpolate && prev.interpolate) {
                interpolate_point(prev.orientation[0], cur.orientation[0], mem.orientation[0], interpolation_tick_progress);
                interpolate_point(prev.orientation[1], cur.orientation[1], mem.orientation[1], interpolation_tick_progress);
                interpolate_point(prev.position,       cur.position,       mem.position,       interpolation_tick_progress);
            }
        }
    }

    void interpolate_light_clear() noexcept {
        std::memset(light_buffers, 0, sizeof(light_buffers));
    }

    void interpolate_light_on_tick() noexcept {
        tick_passed = true;
    }
}
