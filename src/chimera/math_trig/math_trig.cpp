// SPDX-License-Identifier: GPL-3.0-only

#include "math_trig.hpp"
#include <cmath>
#include <algorithm>

namespace Chimera {

    // =========================
    // Helpers
    // =========================
    static inline float clamp01(float v) noexcept {
        return std::max(0.0f, std::min(1.0f, v));
    }

    // =========================
    // Color
    // =========================
    ColorRGB::ColorRGB(float r, float g, float b) noexcept :
        red(clamp01(r)), green(clamp01(g)), blue(clamp01(b)) {}

    ColorRGB::ColorRGB(const ColorByte &o) noexcept :
        red(o.red / 255.0f),
        green(o.green / 255.0f),
        blue(o.blue / 255.0f) {}

    ColorRGB::ColorRGB(const ColorARGB &o) noexcept :
        red(o.red), green(o.green), blue(o.blue) {}

    ColorARGB::ColorARGB(float a, float r, float g, float b) noexcept :
        alpha(clamp01(a)),
        red(clamp01(r)),
        green(clamp01(g)),
        blue(clamp01(b)) {}

    ColorARGB::ColorARGB(const ColorByte &o) noexcept :
        alpha(o.alpha / 255.0f),
        red(o.red / 255.0f),
        green(o.green / 255.0f),
        blue(o.blue / 255.0f) {}

    ColorARGB::ColorARGB(const ColorRGB &o) noexcept :
        alpha(1.0f), red(o.red), green(o.green), blue(o.blue) {}

    ColorByte::ColorByte(float a, float r, float g, float b) noexcept :
        blue(static_cast<unsigned char>(clamp01(b) * 255)),
        green(static_cast<unsigned char>(clamp01(g) * 255)),
        red(static_cast<unsigned char>(clamp01(r) * 255)),
        alpha(static_cast<unsigned char>(clamp01(a) * 255)) {}

    ColorByte::ColorByte(unsigned char a, unsigned char r, unsigned char g, unsigned char b) noexcept :
        blue(b), green(g), red(r), alpha(a) {}

    ColorByte::ColorByte(const ColorRGB &o) noexcept :
        ColorByte(1.0f, o.red, o.green, o.blue) {}

    ColorByte::ColorByte(const ColorARGB &o) noexcept :
        ColorByte(o.alpha, o.red, o.green, o.blue) {}

    // =========================
    // Quaternion <-> Matrix
    // =========================
    Quaternion::Quaternion(const RotationMatrix &m) noexcept {
        float tr = m.v[0].x + m.v[1].y + m.v[2].z;

        if(tr > 0.0f) {
            float S = std::sqrt(tr + 1.0f) * 2.0f;
            w = 0.25f * S;
            x = (m.v[2].y - m.v[1].z) / S;
            y = (m.v[0].z - m.v[2].x) / S;
            z = (m.v[1].x - m.v[0].y) / S;
        }
        else if((m.v[0].x > m.v[1].y) && (m.v[0].x > m.v[2].z)) {
            float S = std::sqrt(1.0f + m.v[0].x - m.v[1].y - m.v[2].z) * 2.0f;
            w = (m.v[2].y - m.v[1].z) / S;
            x = 0.25f * S;
            y = (m.v[0].y + m.v[1].x) / S;
            z = (m.v[0].z + m.v[2].x) / S;
        }
        else if(m.v[1].y > m.v[2].z) {
            float S = std::sqrt(1.0f + m.v[1].y - m.v[0].x - m.v[2].z) * 2.0f;
            w = (m.v[0].z - m.v[2].x) / S;
            x = (m.v[0].y + m.v[1].x) / S;
            y = 0.25f * S;
            z = (m.v[1].z + m.v[2].y) / S;
        }
        else {
            float S = std::sqrt(1.0f + m.v[2].z - m.v[0].x - m.v[1].y) * 2.0f;
            w = (m.v[1].x - m.v[0].y) / S;
            x = (m.v[0].z + m.v[2].x) / S;
            y = (m.v[1].z + m.v[2].y) / S;
            z = 0.25f * S;
        }
    }

    RotationMatrix::RotationMatrix() noexcept {}

    RotationMatrix::RotationMatrix(const Quaternion &q) noexcept {
        float sqw = q.w*q.w;
        float sqx = q.x*q.x;
        float sqy = q.y*q.y;
        float sqz = q.z*q.z;

        float invs = 1.0f / (sqx + sqy + sqz + sqw);

        v[0].x = ( sqx - sqy - sqz + sqw) * invs;
        v[1].y = (-sqx + sqy - sqz + sqw) * invs;
        v[2].z = (-sqx - sqy + sqz + sqw) * invs;

        float tmp1 = q.x*q.y;
        float tmp2 = q.z*q.w;
        v[1].x = 2.0f * (tmp1 + tmp2) * invs;
        v[0].y = 2.0f * (tmp1 - tmp2) * invs;

        tmp1 = q.x*q.z;
        tmp2 = q.y*q.w;
        v[2].x = 2.0f * (tmp1 - tmp2) * invs;
        v[0].z = 2.0f * (tmp1 + tmp2) * invs;

        tmp1 = q.y*q.z;
        tmp2 = q.x*q.w;
        v[2].y = 2.0f * (tmp1 + tmp2) * invs;
        v[1].z = 2.0f * (tmp1 - tmp2) * invs;
    }

    // =========================
    // Interpolation
    // =========================
    void interpolate_quat(const Quaternion &a, const Quaternion &b, Quaternion &out, float t) noexcept {
        float cos_half = a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
        Quaternion bb = b;

        if(cos_half < 0.0f) {
            cos_half = -cos_half;
            bb.w = -bb.w; bb.x = -bb.x; bb.y = -bb.y; bb.z = -bb.z;
        }

        if(cos_half > 0.9995f) {
            out.w = a.w + t*(bb.w - a.w);
            out.x = a.x + t*(bb.x - a.x);
            out.y = a.y + t*(bb.y - a.y);
            out.z = a.z + t*(bb.z - a.z);
        }
        else {
            float half_theta = std::acos(cos_half);
            float sin_half = std::sqrt(1.0f - cos_half*cos_half);

            float r0 = std::sin((1.0f - t) * half_theta) / sin_half;
            float r1 = std::sin(t * half_theta) / sin_half;

            out.w = a.w*r0 + bb.w*r1;
            out.x = a.x*r0 + bb.x*r1;
            out.y = a.y*r0 + bb.y*r1;
            out.z = a.z*r0 + bb.z*r1;
        }

        float inv = 1.0f / std::sqrt(out.w*out.w + out.x*out.x + out.y*out.y + out.z*out.z);
        out.w *= inv; out.x *= inv; out.y *= inv; out.z *= inv;
    }

    void interpolate_point(const Point3D &a, const Point3D &b, Point3D &o, float t) noexcept {
        o.x = a.x + (b.x - a.x) * t;
        o.y = a.y + (b.y - a.y) * t;
        o.z = a.z + (b.z - a.z) * t;
    }

    // =========================
    // Math helpers
    // =========================
    float distance_squared(float x1, float y1, float x2, float y2) noexcept {
        float dx = x1 - x2;
        float dy = y1 - y2;
        return dx*dx + dy*dy;
    }

    float distance_squared(float x1, float y1, float z1, float x2, float y2, float z2) noexcept {
        float dx = x1 - x2;
        float dy = y1 - y2;
        float dz = z1 - z2;
        return dx*dx + dy*dy + dz*dz;
    }

    float distance_squared(const Point3D &a, const Point3D &b) noexcept {
        return distance_squared(a.x, a.y, a.z, b.x, b.y, b.z);
    }

    float magnitude_squared(const Point3D &a) noexcept {
        return a.x*a.x + a.y*a.y + a.z*a.z;
    }

    long fast_ftol(float f) noexcept {
        long i = static_cast<long>(f);
        return (f - i < 0.5f) ? i : i + 1;
    }

    void pixel32_to_real_argb_color(std::uint32_t p, ColorARGB *c) noexcept {
        c->alpha = ((p >> 24) & 0xFF) / 255.0f;
        c->red   = ((p >> 16) & 0xFF) / 255.0f;
        c->green = ((p >> 8)  & 0xFF) / 255.0f;
        c->blue  = ((p >> 0)  & 0xFF) / 255.0f;
    }

    double counter_time_elapsed(const LARGE_INTEGER &a, const LARGE_INTEGER &b) noexcept {
        static LARGE_INTEGER freq{};
        if(freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
        return double(b.QuadPart - a.QuadPart) / freq.QuadPart;
    }

    double counter_time_elapsed(const LARGE_INTEGER &a) noexcept {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return counter_time_elapsed(a, now);
    }

}

