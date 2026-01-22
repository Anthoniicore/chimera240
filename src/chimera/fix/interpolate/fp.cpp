// SPDX-License-Identifier: GPL-3.0-only

#include "../../math_trig/math_trig.hpp"

#include <optional>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>

#include "fp.hpp"
#include "../../halo_data/pause.hpp"
#include "../../chimera.hpp"
#include "../../signature/signature.hpp"
#include "interpolate.hpp"

namespace Chimera {

    struct FirstPersonNode {
        Quaternion orientation;
        Point3D position;
        float scale;
    };
    static_assert(sizeof(FirstPersonNode) == 0x20);

    static std::byte *first_person_nodes() noexcept {
        static std::optional<std::byte *> first_person_nodes_opt;

        if(!first_person_nodes_opt.has_value()) {
            first_person_nodes_opt =
                **reinterpret_cast<std::byte ***>(
                    get_chimera().get_signature("first_person_node_base_address_sig").data() + 2
                );
        }

        return first_person_nodes_opt.value();
    }

    #define NODES_PER_BUFFER 128
    static FirstPersonNode fp_buffers[2][NODES_PER_BUFFER] = {};

    static FirstPersonNode *current_tick  = fp_buffers[0];
    static FirstPersonNode *previous_tick = fp_buffers[1];

    static bool skip = false;
    static bool revert = false;
    static bool tick_passed = false;

    // 240 FPS safety
    static float last_alpha = 0.0f;
    constexpr float MIN_ALPHA_DELTA = 0.0001f;

    static inline float clamp01(float v) noexcept {
        if(v < 0.0f) return 0.0f;
        if(v > 1.0f) return 1.0f;
        return v;
    }

    void interpolate_fp_before() noexcept {
        if(game_paused() || !interpolation_enabled) {
            return;
        }

        FirstPersonNode *fpn =
            reinterpret_cast<FirstPersonNode *>(first_person_nodes() + 0x8C);

        float alpha = clamp01(interpolation_tick_progress);

        // Skip meaningless frames (critical at 240 FPS)
        float delta = alpha - last_alpha;
        if(delta <= MIN_ALPHA_DELTA) {
            return;
        }
        last_alpha = alpha;

        if(tick_passed) {
            static std::uint32_t last_weapon = ~0u;
            static std::uint32_t &current_weapon =
                *reinterpret_cast<std::uint32_t *>(first_person_nodes() + 8);

            skip = !*reinterpret_cast<std::uint32_t *>(first_person_nodes())
                   || last_weapon != current_weapon;

            if(revert) {
                skip = true;
                revert = false;
            }

            std::swap(current_tick, previous_tick);

            last_weapon = current_weapon;
            tick_passed = false;

            std::copy(fpn, fpn + NODES_PER_BUFFER, current_tick);
        }

        if(!skip) {
            for(int i = 0; i < NODES_PER_BUFFER; i++) {
                interpolate_quat(
                    previous_tick[i].orientation,
                    current_tick[i].orientation,
                    fpn[i].orientation,
                    alpha
                );

                interpolate_point(
                    previous_tick[i].position,
                    current_tick[i].position,
                    fpn[i].position,
                    alpha
                );

                fpn[i].scale =
                    previous_tick[i].scale +
                    (current_tick[i].scale - previous_tick[i].scale) * alpha;
            }
        }
    }

    void interpolate_fp_after() noexcept {
        if(!skip && !game_paused() && interpolation_enabled) {
            FirstPersonNode *fpn =
                reinterpret_cast<FirstPersonNode *>(first_person_nodes() + 0x8C);

            std::copy(current_tick, current_tick + NODES_PER_BUFFER, fpn);
        }
    }

    void interpolate_fp_clear() noexcept {
        skip = true;
        revert = true;
        last_alpha = 0.0f;
        std::memset(fp_buffers, 0, sizeof(fp_buffers));
    }

    void interpolate_fp_on_tick() noexcept {
        tick_passed = true;
        last_alpha = 0.0f;
    }
}

