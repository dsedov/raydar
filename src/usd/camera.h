

#ifndef RD_USD_CAMERA_H
#define RD_USD_CAMERA_H

#include <cstdio>
#include <vector>

#include "../data/hittable.h"
#include "../data/hittable_list.h"
#include "../data/bvh.h"

#include "../core/camera.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdGeom/metrics.h>

namespace rd::usd::camera {
    extern rd::core::camera extractCameraProperties(const pxr::UsdGeomCamera& usd_camera);
}
#endif