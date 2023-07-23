/// @file      material.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

namespace rayce
{
    struct RAYCE_API_EXPORT Material
    {
        vec4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        vec3 emissiveColor{ 0.0f, 0.0f, 0.0f };
        // alphaMode;
        // alphaCutoff;
        // doubleSided;
        float metallicFactor{ 0.5f };
        float roughnessFactor{ 0.5f };

        int32 baseColorTextureId{ -1 };
        int32 metallicRoughnessTextureId{ -1 };
        int32 normalTextureId{ -1 };
        // int32 occlusionTextureId{-1};
        int32 emissiveTextureId{ -1 };
    };
} // namespace rayce

#endif // MATERIAL_HPP
