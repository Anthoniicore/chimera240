// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../../halo_data/camera.hpp"

namespace Chimera {

    void fp_motion_blur_before(CameraData &cam) noexcept;
    void fp_motion_blur_after(CameraData &cam) noexcept;
    void fp_motion_blur_clear() noexcept;

}
