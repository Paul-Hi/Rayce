/// @file      types.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef TYPES_HPP
#define TYPES_HPP

#include <Eigen/Core>
#include <Eigen/Eigen>
#include <export.hpp>
#include <macro.hpp>
#include <stdint.h>
#include <string.h>

#pragma warning(push, 0)
#define NOMINMAX
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma warning(pop)

namespace rayce
{
    /// @brief Typedef for one byte.
    using byte = unsigned char;
    /// @brief Typedef for 16 bit integers.
    using int16 = short int;
    /// @brief Typedef for 32 bit integers.
    using int32 = int;
    /// @brief Typedef for 64 bit integers.
    using int64 = long long;
    /// @brief Typedef for 16 bit unsigned integers.
    using uint16 = unsigned short int;
    /// @brief Typedef for 32 bit unsigned integers.
    using uint32 = unsigned int;
    /// @brief Typedef for 64 bit unsigned integers.
    using uint64 = unsigned long long;
    /// @brief Typedef for size_t.
    using ptr_size = size_t;

    /// @brief Typedef for a 3 component double vector.
    using dvec3 = Eigen::Vector3<double>;
    /// @brief Typedef for a 3x3 double matrix.
    using dmat3 = Eigen::Matrix3<double>;
    /// @brief Typedef for a 2 component double vector.
    using dvec2 = Eigen::Vector2<double>;

    /// @brief Typedef for string.
    using str = std::string;

    /// @cond NO_COND

    template <typename e>
    struct bit_mask_operations
    {
        static const bool enable = false;
    };

/// @brief Macro used to enable safe bitmask operations on enum classes.
#define COMMON_ENABLE_BITMASK_OPERATIONS(e)  \
    template <>                              \
    struct bit_mask_operations<e>            \
    {                                        \
        static constexpr bool enable = true; \
    };

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator|(e lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator&(e lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator^(e lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e>::type operator~(e lhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        return static_cast<e>(~static_cast<underlying>(lhs));
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e&>::type operator|=(e& lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        lhs = static_cast<e>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
        return lhs;
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e&>::type operator&=(e& lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        lhs = static_cast<e>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
        return lhs;
    }

    template <typename e>
    typename std::enable_if<bit_mask_operations<e>::enable, e&>::type operator^=(e& lhs, e rhs)
    {
        typedef typename std::underlying_type<e>::type underlying;
        lhs = static_cast<e>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
        return lhs;
    }

    /// @endcond

/// @brief Pi.
#define PI 3.1415926535897932384626433832795_d
/// @brief Pi times two.
#define TWO_PI (2.0_d * PI)
/// @brief Pi divided by two.
#define HALF_PI (0.5_d * PI)

} // namespace rayce

#endif // TYPES_HPP
