/// @file      loadHelper.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef LOAD_HELPER_HPP
#define LOAD_HELPER_HPP

namespace rayce
{
    template <typename T>
    static void extendVector(std::vector<T>& toExtend, const std::vector<T>& with)
    {
        toExtend.reserve(toExtend.size() + with.size());
        toExtend.insert(toExtend.end(), with.begin(), with.end());
    }

    static VkFormat getImageFormat(uint32 components, bool srgb)
    {
        // FIXME: This is really basic...
        if (srgb)
        {
            // clang-format off
            return components == 1 ? VK_FORMAT_R8_SRGB :
                    (components == 2 ? VK_FORMAT_R8G8_SRGB :
                    (components == 3 ? VK_FORMAT_R8G8B8_SRGB :
                    VK_FORMAT_R8G8B8A8_SRGB));
            // clang-format on
        }
        else
        {
            // clang-format off
            return components == 1 ? VK_FORMAT_R8_UNORM :
                    (components == 2 ? VK_FORMAT_R8G8_UNORM :
                    (components == 3 ? VK_FORMAT_R8G8B8_UNORM :
                    VK_FORMAT_R8G8B8A8_UNORM));
            // clang-format on
        }
    }
}

#endif // LOAD_HELPER_HPP
