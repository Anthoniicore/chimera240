// SPDX-License-Identifier: GPL-3.0-only

#include "../../signature/signature.hpp"
#include "../../halo_data/object.hpp"
#include "../../halo_data/camera.hpp"
#include "../../math_trig/math_trig.hpp"
#include "../../chimera.hpp"

#include "interpolate.hpp"
#include "object.hpp"

namespace Chimera {

#define OBJECT_BUFFER_SIZE 2048

    struct InterpolatedObject {
        bool interpolate = false;
        bool interpolated_this_frame = false;

        std::size_t children_count = 0;
        std::size_t children[OBJECT_BUFFER_SIZE];

        TagID tag_id;
        std::uint16_t index;

        Point3D center;

        std::size_t node_count;
        ModelNode nodes[MAX_NODES];

        float device_position = 0.0f;
    };

    static InterpolatedObject object_buffers[2][OBJECT_BUFFER_SIZE] = {};
    static auto *current_tick  = object_buffers[0];
    static auto *previous_tick = object_buffers[1];

    static bool tick_passed = false;

    static void copy_objects() noexcept;
    static void interpolate_object(std::size_t);

    // ------------------------------------------------------------

    void interpolate_object_before() noexcept {
        if(tick_passed) {
            std::swap(current_tick, previous_tick);
            copy_objects();
            tick_passed = false;
        }

        static auto **visible_object_count =
            reinterpret_cast<std::uint32_t **>(
                get_chimera().get_signature("visible_object_count_sig").data() + 3);

        static auto **visible_object_array =
            reinterpret_cast<ObjectID **>(
                get_chimera().get_signature("visible_object_ptr_sig").data() + 3);

        auto count = **visible_object_count;

        for(std::size_t i = 0; i < count; i++) {
            interpolate_object((*visible_object_array)[i].index.index);
        }
    }

    // ------------------------------------------------------------

    static void interpolate_object(std::size_t index) {
        extern float interpolation_tick_progress;

        if(index >= OBJECT_BUFFER_SIZE) return;

        auto &cur  = current_tick[index];
        auto &prev = previous_tick[index];

        if(!cur.interpolate || cur.interpolated_this_frame) return;

        auto *object =
            ObjectTable::get_object_table().get_dynamic_object(index);

        if(!object) return;

        if(cur.tag_id != object->tag_id ||
           prev.tag_id != cur.tag_id ||
           cur.index != prev.index)
            return;

        // ---- micro-correction killer (CRÍTICO)
        if(distance_squared(prev.center, cur.center) < 0.0005f) {
            return;
        }

        cur.interpolated_this_frame = true;

        // ---- children first
        for(std::size_t i = 0; i < cur.children_count; i++) {
            interpolate_object(cur.children[i]);
        }

        // ---- camera distance scaling
        Point3D cam = camera_data().position;
        float dist2 = distance_squared(cam, cur.center);

        float alpha = interpolation_tick_progress;
        if(dist2 < 4.0f) {
            // cerca = lineal pura
            alpha = clamp01(alpha);
        }

        // ---- center interpolation (SIEMPRE)
        interpolate_point(prev.center, cur.center,
                          object->center_position, alpha);

        // ---- bipeds remotos: NO skeleton
        bool is_remote_biped =
            object->type == OBJECT_TYPE_BIPED &&
            !object->is_local_player;

        if(is_remote_biped) {
            return;
        }

        // ---- no parented physics jitter
        if(!object->parent.is_null() &&
           object->type == OBJECT_TYPE_BIPED) {
            return;
        }

        auto *nodes = object->nodes();
        if(!nodes) return;

        for(std::size_t n = 0; n < cur.node_count; n++) {
            auto &node_cur  = cur.nodes[n];
            auto &node_prev = prev.nodes[n];
            auto &node      = nodes[n];

            interpolate_point(
                node_prev.position,
                node_cur.position,
                node.position,
                alpha
            );

            node.scale =
                node_prev.scale +
                (node_cur.scale - node_prev.scale) * alpha;

            interpolate_quat(
                node_prev.rotation,
                node_cur.rotation,
                node.rotation,
                alpha
            );
        }
    }

    // ------------------------------------------------------------

    static void copy_objects() noexcept {
        auto &table = ObjectTable::get_object_table();

        ObjectID parent_map[OBJECT_BUFFER_SIZE] = { HaloID::null_id() };

        for(std::size_t i = 0; i < OBJECT_BUFFER_SIZE; i++) {
            auto &obj = current_tick[i];

            obj.interpolate = false;
            obj.interpolated_this_frame = false;
            obj.children_count = 0;

            auto *dyn = table.get_dynamic_object(i);
            if(!dyn) continue;

            obj.index  = table.first_element[i].id;
            obj.tag_id = dyn->tag_id;
            obj.center = dyn->center_position;

            auto *nodes = dyn->nodes();
            if(!nodes) continue;

            // ---- node count
            if(dyn->type == OBJECT_TYPE_PROJECTILE) {
                obj.node_count = 1;
            }
            else {
                auto *tag = get_tag(obj.tag_id.index.index);
                if(!tag) continue;

                auto model_id =
                    *reinterpret_cast<TagID *>(tag->data + 0x34);
                auto *model = get_tag(model_id);
                if(!model) continue;

                obj.node_count =
                    *reinterpret_cast<std::uint32_t *>(model->data + 0xB8);
            }

            std::copy(nodes, nodes + obj.node_count, obj.nodes);

            // ---- interpolation distance caps
            static constexpr float MAX_DIST2_BIPED = 2.5f * 2.5f;
            static constexpr float MAX_DIST2_OTHER = 7.5f * 7.5f;

            float max_dist2 =
                dyn->type == OBJECT_TYPE_BIPED
                ? MAX_DIST2_BIPED
                : MAX_DIST2_OTHER;

            obj.interpolate =
                distance_squared(obj.center, previous_tick[i].center)
                < max_dist2;

            // ---- device machine fix
            if(dyn->type == OBJECT_TYPE_DEVICE_MACHINE) {
                obj.device_position =
                    reinterpret_cast<DeviceMachineDynamicObject *>(dyn)
                        ->device_position;

                if(obj.interpolate) {
                    obj.interpolate =
                        !(obj.device_position < 0.002f &&
                          previous_tick[i].device_position > 0.9f);
                }
            }

            if(!dyn->parent.is_null()) {
                parent_map[i] = dyn->parent;
            }
        }

        // ---- build parent -> children
        for(std::size_t i = 0; i < OBJECT_BUFFER_SIZE; i++) {
            if(parent_map[i].is_null()) continue;

            auto &parent =
                current_tick[parent_map[i].index.index];

            parent.children[parent.children_count++] = i;
        }
    }

    // ------------------------------------------------------------

    void interpolate_object_after() noexcept {
        auto &table = ObjectTable::get_object_table();

        for(std::size_t i = 0; i < table.current_size; i++) {
            auto &obj = current_tick[i];
            if(!obj.interpolated_this_frame) continue;

            obj.interpolated_this_frame = false;

            auto *dyn = table.get_dynamic_object(i);
            if(!dyn) continue;

            dyn->center_position = obj.center;
            std::copy(obj.nodes, obj.nodes + obj.node_count, dyn->nodes());
        }
    }

    void interpolate_object_clear() noexcept {
        for(std::size_t i = 0; i < OBJECT_BUFFER_SIZE; i++) {
            current_tick[i].interpolate = false;
            current_tick[i].interpolated_this_frame = false;
            current_tick[i].index = 0;
        }
    }

    void interpolate_object_on_tick() noexcept {
        tick_passed = true;
    }
}
