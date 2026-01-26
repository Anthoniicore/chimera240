// SPDX-License-Identifier: GPL-3.0-only

#include <d3d9.h>

#include "specular_memes.hpp"
#include "../chimera.hpp"
#include "../signature/hook.hpp"
#include "../signature/signature.hpp"
#include "../event/game_loop.hpp"
#include "../rasterizer/rasterizer.hpp"
#include "../output/output.hpp" // <- necesario para console_output

extern "C" {
    void specular_light_draw_set_texture_asm() noexcept;
    void specular_light_adjust_ps_const_retail() noexcept;
    void specular_light_adjust_ps_const_custom() noexcept;
    void specular_lightmap_draw_set_constants_asm() noexcept;

    std::uint8_t *specular_light_is_spotlight = nullptr;
    std::byte *specular_constants_table = nullptr;
}

namespace Chimera {
    static Hook hook_set_texture;
    static Hook hook_set_constants;
    static Hook hook_lightmap_constants;

    void patch_specular_light_draw() noexcept {
        if (d3d9_device_caps->PixelShaderVersion < 0xffff0200) {
            console_output("Specular light fix skipped: Pixel Shader 2.0 not supported.");
            return;
        }

        auto *tex3_call = get_chimera().get_signature("env_specular_light_set_tex3").data() + 7;
        auto *tex1_call = get_chimera().get_signature("env_specular_light_set_tex1").data();

        // NOP redundant sampler assignment
        static const SigByte nop[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
        write_code_s(tex3_call, nop);

        // Redirect texture assignment
        write_jmp_call(tex1_call, hook_set_texture,
                       reinterpret_cast<const void *>(specular_light_draw_set_texture_asm),
                       nullptr, false);

        // Shader constant fix
        if (get_chimera().feature_present("client_custom_edition")) {
            specular_light_is_spotlight = *reinterpret_cast<std::uint8_t **>(
                get_chimera().get_signature("env_specular_light_spotlight_custom").data() + 1);
            auto *const_call = get_chimera().get_signature("env_specular_light_set_const_custom").data();
            write_jmp_call(const_call, hook_set_constants, nullptr,
                           reinterpret_cast<const void *>(specular_light_adjust_ps_const_custom), false);
        } else if (get_chimera().feature_present("client_retail_demo")) {
            specular_light_is_spotlight = *reinterpret_cast<std::uint8_t **>(
                get_chimera().get_signature("env_specular_light_spotlight_retail").data() + 2);
            auto *const_call = get_chimera().get_signature("env_specular_light_set_const_retail").data();
            write_jmp_call(const_call, hook_set_constants, nullptr,
                           reinterpret_cast<const void *>(specular_light_adjust_ps_const_retail), false);
        }
    }

    void patch_specular_lightmap() noexcept {
        if (get_chimera().feature_present("client_custom_edition")) {
            auto *ps_const_count = get_chimera().get_signature("env_specular_lightmap_set_const_custom").data() + 1;
            static const SigByte mod[] = { 0x04 };
            write_code_s(ps_const_count, mod);
        } else if (get_chimera().feature_present("client_retail_demo")) {
            auto *ps_const_count = get_chimera().get_signature("env_specular_lightmap_set_const_retail").data() + 1;
            specular_constants_table = *reinterpret_cast<std::byte **>(
                get_chimera().get_signature("env_specular_retail_const_table").data() + 1);
            write_jmp_call(ps_const_count + 50, hook_lightmap_constants,
                           reinterpret_cast<const void *>(specular_lightmap_draw_set_constants_asm), nullptr);
        }
    }

    void set_up_specular_light_fix() noexcept {
        add_game_start_event(patch_specular_light_draw);
        patch_specular_lightmap();
    }
}
