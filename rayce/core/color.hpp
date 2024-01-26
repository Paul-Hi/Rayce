/// @file      color.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

// A mixture between mitsuba, pbrtv4 and Falcor

#ifndef RAYCE_COLOR_HPP
#define RAYCE_COLOR_HPP

namespace rayce
{
    // assumes all input/outputs are in sRGB, which uses the ITU-R Rec. BT.709 (Rec.709) color space.

    const mat3 ColorTransformRGBtoXYZRec709{
        { 0.4123907992659595, 0.3575843393838780, 0.1804807884018343 },
        { 0.2126390058715104, 0.7151686787677559, 0.0721923153607337 },
        { 0.0193308187155918, 0.1191947797946259, 0.9505321522496608 }
    };

    const mat3 ColorTransformXYZtoRGBRec709{
        { 3.2409699419045213, -1.5373831775700935, -0.4986107602930033 },
        { -0.9692436362808798, 1.8759675015077206, 0.0415550574071756 },
        { 0.0556300796969936, -0.2039769588889765, 1.0569715142428784 }
    };

    inline RAYCE_API_EXPORT vec3 RGBRec709toXYZ(const vec3& c)
    {
        return ColorTransformRGBtoXYZRec709 * c;
    }

    inline RAYCE_API_EXPORT vec3 XYZtoRGBRec709(const vec3& c)
    {
        return ColorTransformXYZtoRGBRec709 * c;
    }
};

#endif // RAYCE_COLOR_HPP