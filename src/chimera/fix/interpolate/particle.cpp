// SPDX-License-Identifier: GPL-3.0-only

#include "../../halo_data/particle.hpp"
#include "../../math_trig/math_trig.hpp"

#include "particle.hpp"

namespace Chimera {

    struct InterpolatedParticle {
        bool interpolate = false;
        bool interpolated_this_frame = false;
        Point3D position;
    };

    #define PARTICLE_BUFFER_SIZE 1024
    static InterpolatedParticle particle_buffers[2][PARTICLE_BUFFER_SIZE] = {};

    static auto *current_tick = particle_buffers[0];
    static auto *previous_tick = particle_buffers[1];

    static bool tick_passed = false;

    void interpolate_particle() noexcept {
        auto &particle_table = ParticleTable::get_particle_table();

        if(tick_passed) {
            std::swap(current_tick, previous_tick);

            for(std::size_t i = 0; i < PARTICLE_BUFFER_SIZE; i++) {
                auto *particle = particle_table.get_element(i);
                auto &dst = current_tick[i];

                dst.interpolate = false;
                dst.interpolated_this_frame = false;

                if(!particle) {
                    continue;
                }

                dst.position = particle->position;
                dst.interpolate = (particle->unknown0 & 0xFFFF) != 0;
            }

            tick_passed = false;
        }

        extern float interpolation_tick_progress;

        for(std::size_t i = 0; i < particle_table.current_size && i < PARTICLE_BUFFER_SIZE; i++) {
            auto *particle = particle_table.get_element(i);
            auto &cur = current_tick[i];
            auto &prev = previous_tick[i];

            if(!particle) {
                continue;
            }

            if(cur.interpolate && prev.interpolate) {
                interpolate_point(
                    prev.position,
                    cur.position,
                    particle->position,
                    interpolation_tick_progress
                );
                cur.interpolated_this_frame = true;
            }
        }
    }

    void interpolate_particle_after() noexcept {
        auto &particle_table = ParticleTable::get_particle_table();

        for(std::size_t i = 0; i < particle_table.current_size && i < PARTICLE_BUFFER_SIZE; i++) {
            auto *particle = particle_table.get_element(i);
            auto &cur = current_tick[i];

            if(!particle) {
                continue;
            }

            if(cur.interpolated_this_frame) {
                particle->position = cur.position;
                cur.interpolated_this_frame = false;
            }
        }
    }

    void interpolate_particle_clear() noexcept {
        for(auto &buf : particle_buffers) {
            for(auto &p : buf) {
                p.interpolate = false;
                p.interpolated_this_frame = false;
            }
        }
    }

    void interpolate_particle_on_tick() noexcept {
        tick_passed = true;
    }
}
