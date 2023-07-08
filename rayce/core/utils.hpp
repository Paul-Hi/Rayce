/// @file      utils.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef UTILS_HPP
#define UTILS_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    inline uint32 quickAlign(uint32 value, uint32 alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

} // namespace rayce

#endif // UTILS_HPP