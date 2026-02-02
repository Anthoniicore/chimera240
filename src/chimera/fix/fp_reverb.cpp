// SPDX-License-Identifier: GPL-3.0-only

#include "custom_map_lobby_fix.hpp"
#include "../signature/hook.hpp"
#include "../signature/signature.hpp"
#include "../chimera.hpp"
#include "../output/output.hpp"
#include "../halo_data/camera.hpp"
#include "../halo_data/player.hpp"
#include "../halo_data/object.hpp"

namespace Chimera {
    extern "C" void override_fp_reverb_position_asm() noexcept;

    extern "C" void move_current_fp_sound_to_camera(std::uint32_t object_id, Point3D *sound_position) noexcept {
        // Solo aplicamos en primera persona
        if(camera_type() != CameraType::CAMERA_FIRST_PERSON) {
            return;
        }

        // ID inválido
        if(object_id == 0xFFFFFFFF) {
            return;
        }

        // Obtener jugador local
        auto *player = PlayerTable::get_player_table().get_client_player();
        if(!player) {
            return;
        }

        auto &ot = ObjectTable::get_object_table();
        auto *player_object = reinterpret_cast<UnitDynamicObject *>(ot.get_dynamic_object(player->object_id));
        if(!player_object) {
            return;
        }

        // Verificar si el objeto corresponde al jugador o a cualquiera de sus armas
        bool is_us = (object_id == player->object_id.whole_id);
        if(!is_us) {
            for(auto &w : player_object->weapons) {
                if(w.whole_id == object_id) {
                    is_us = true;
                    break;
                }
            }
        }

        // Si es nuestro cuerpo o cualquier arma que tengamos, mover sonido a la cámara
        if(is_us) {
            auto &d = camera_data();
            *sound_position = d.position;
        }
    }

    void set_up_fp_reverb_fix() noexcept {
        auto &chimera = get_chimera();
        if(!chimera.feature_present("client_fp_reverb")) {
            return;
        }

        auto *first_person_reverb_1 = chimera.get_signature("first_person_reverb_1_sig").data();
        auto *first_person_reverb_2 = chimera.get_signature("first_person_reverb_2_sig").data();

        // Habilitar reverb
        static constexpr const SigByte tell_lies_on_the_internet[] = { 0x66, 0xB8, 0x0D, 0x00 };
        write_code_s(first_person_reverb_1, tell_lies_on_the_internet);

        // Hook para redirigir sonidos
        static Hook hook;
        write_jmp_call(reinterpret_cast<std::byte *>(first_person_reverb_2), hook, nullptr, reinterpret_cast<const void *>(override_fp_reverb_position_asm), false);
    }

    void disable_fp_reverb_fix() noexcept {
        auto &chimera = get_chimera();
        chimera.get_signature("first_person_reverb_1_sig").rollback();
        chimera.get_signature("first_person_reverb_2_sig").rollback();
    }
}

