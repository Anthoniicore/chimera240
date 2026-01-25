// SPDX-License-Identifier: GPL-3.0-only

#include "../../math_trig/math_trig.hpp"

#include <optional>
#include <cstdint>
#include <cstring>

#include "fp.hpp"
#include "../../halo_data/pause.hpp"
#include "../../chimera.hpp"
#include "../../signature/signature.hpp"

namespace Chimera {

    struct FirstPersonNode {
        Quaternion orientation;
        Point3D position;
        float scale;
    };
    static_assert(sizeof(FirstPersonNode) == 0x20);

    static std::byte *first_person_nodes() noexcept {
        static std::optional<std::byte *> base;

        if(!base.has_value()) {
            base =
                **reinterpret_cast<std::byte ***>(
                    get_chimera().get_signature(
                        "first_person_node_base_address_sig"
                    ).data() + 2
                );
        }

        return base.value();
    }

    #define NODES_PER_BUFFER 128

    static FirstPersonNode last_frame[NODES_PER_BUFFER]{};
    static bool initialized = false;

    // CONSTANT smoothing (engine-safe)
    static constexpr float FP_POS_ALPHA   = 0.35f;
    static constexpr float FP_ROT_ALPHA   = 0.22f;
    static constexpr float FP_SCALE_ALPHA = 0.30f;

    static inline float clamp01(float v) noexcept {
        if(v < 0.0f) return 0.0f;
        if(v > 1.0f) return 1.0f;
        return v;
    }

    void interpolate_fp_before() noexcept {
        if(game_paused() || !interpolation_enabled)
            return;

        FirstPersonNode *fpn =
            reinterpret_cast<FirstPersonNode *>(first_person_nodes() + 0x8C);

        if(!initialized) {
            std::memcpy(last_frame, fpn, sizeof(last_frame));
            initialized = true;
            return;
        }

        for(std::size_t i = 0; i < NODES_PER_BUFFER; i++) {

            // Orientation — slightly tighter
            interpolate_quat(
                last_frame[i].orientation,
                fpn[i].orientation,
                fpn[i].orientation,
                FP_ROT_ALPHA
            );

            // Position — smoother
            interpolate_point(
                last_frame[i].position,
                fpn[i].position,
                fpn[i].position,
                FP_POS_ALPHA
            );

            // Scale
            fpn[i].scale =
                last_frame[i].scale +
                (fpn[i].scale - last_frame[i].scale) * FP_SCALE_ALPHA;
        }
    }

    void interpolate_fp_after() noexcept {
        if(game_paused() || !interpolation_enabled)
            return;

        FirstPersonNode *fpn =
            reinterpret_cast<FirstPersonNode *>(first_person_nodes() + 0x8C);

        std::memcpy(last_frame, fpn, sizeof(last_frame));
    }

    void interpolate_fp_clear() noexcept {
        initialized = false;
        std::memset(last_frame, 0, sizeof(last_frame));
    }

    void interpolate_fp_on_tick() noexcept {
        // FP interpolation is frame-only by design
    }
}
