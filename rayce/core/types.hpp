/// @file      types.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef TYPES_HPP
/// @cond NO_DOC
#define TYPES_HPP
/// @endcond

#include <Eigen/Core>
#include <Eigen/Eigen>
#include <Eigen/Geometry>
#include <core/export.hpp>
#include <core/iconsFA6.hpp>
#include <core/macro.hpp>
#include <stdint.h>
#include <string.h>

/// @cond NO_DOC
#pragma warning(push, 0)
#define NOMINMAX
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma warning(pop)
/// @endcond

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
    /// @brief Typedef for ptr_size.
    using ptr_size = size_t;

    /// @brief Typedef for a 4 component float vector.
    using vec4 = Eigen::Vector4<float>;
    /// @brief Typedef for a 4x4 float matrix.
    using mat4 = Eigen::Matrix4<float>;
    /// @brief Typedef for a 3 component float vector.
    using vec3 = Eigen::Vector3<float>;
    /// @brief Typedef for a 3x3 float matrix.
    using mat3 = Eigen::Matrix3<float>;
    /// @brief Typedef for a 2 component float vector.
    using vec2 = Eigen::Vector2<float>;
    /// @brief Typedef for a 2x2 matrix.
    using mat2 = Eigen::Matrix2<float>;
    //! @brief Type alias for a Eigen::Quaternionf.
    using quat = Eigen::Quaternionf;

    /// @brief Typedef for string.
    using str = std::string;

    /// @cond NO_DOC

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
#define PI 3.1415926535897932384626433832795
/// @brief Pi times two.
#define TWO_PI (2.0_d * PI)
/// @brief Pi divided by two.
#define HALF_PI (0.5_d * PI)

    // Utility functions

    /// @brief Convert degrees to radians.
    /// @param[in] degrees Angle in degrees.
    /// @return Angle in radians.
    inline float deg_to_rad(const float& degrees)
    {
        return degrees * (static_cast<float>(PI) / 180.0f);
    }

    /// @brief Convert radians to degrees.
    /// @param[in] radians Angle in radians.
    /// @return Angle in degrees.
    inline float rad_to_deg(const float& radians)
    {
        return radians * (180.0f / static_cast<float>(PI));
    }

    /// @brief Convert @a vec3 in degrees to radians.
    /// @param[in] degrees @a vec3 angle in degrees.
    /// @return @a vec3 angles in radians.
    inline vec3 deg_to_rad(const vec3& degrees)
    {
        return degrees * (PI / 180.0f);
    }

    /// @brief Convert @a vec3 in radians to degrees.
    /// @param[in] radians @a vec3 angles in radians.
    /// @return @a vec3 angles in degrees.
    inline vec3 rad_to_deg(const vec3& radians)
    {
        return radians * (180.0f / PI);
    }

    /// @brief Convert @a vec4 in degrees to radians.
    /// @param[in] degrees @a vec4 angle in degrees.
    /// @return @a vec4 angles in radians.
    inline vec4 deg_to_rad(const vec4& degrees)
    {
        return degrees * (PI / 180.0f);
    }

    /// @brief Convert @a vec4 in radians to degrees.
    /// @param[in] radians @a vec4 angles in radians.
    /// @return @a vec4 angles in degrees.
    inline vec4 rad_to_deg(const vec4& radians)
    {
        return radians * (180.0f / PI);
    }

    /// @brief Clamp value between low and high.
    /// @param[in] v The value to clamp.
    /// @param[in] lo Lower limit.
    /// @param[in] hi Upper limit.
    /// @param[in] comp Compare function.
    /// @return The clamped value.
    template <typename T, typename Compare>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp)
    {
        return comp(v, lo) ? lo : comp(hi, v) ? hi
                                              : v;
    }

    /// @brief Clamp value between low and high.
    /// @param[in] v The value to clamp.
    /// @param[in] lo Lower limit.
    /// @param[in] hi Upper limit.
    /// @return The clamped value.
    template <typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return rayce::clamp(v, lo, hi, std::less<T>());
    }

    /// @brief Return the absolute value of given @a vec3.
    /// @param[in] v The @a vec3 get the absolute value for.
    /// @return The absolute @a vec3 value of v.
    inline vec3 abs(const vec3& v)
    {
        return vec3(std::abs(v.x()), std::abs(v.y()), std::abs(v.z()));
    }

    /// @brief Return the absolute value of given @a vec4.
    /// @param[in] v The @a vec4 get the absolute value for.
    /// @return The absolute @a vec4 value of v.
    inline vec4 abs(const vec4& v)
    {
        return vec4(std::abs(v.x()), std::abs(v.y()), std::abs(v.z()), std::abs(v.w()));
    }

    /// @brief Minimum of two values.
    /// @param[in] a The first value.
    /// @param[in] b The second value.
    /// @return The minimum of both values.
    template <typename T, typename U>
    typename std::common_type<T, U>::type min(const T& a, const U& b)
    {
        return (a < b) ? a : b;
    }

    /// @brief Maximum of two values.
    /// @param[in] a The first value.
    /// @param[in] b The second value.
    /// @return The maximum of both values.
    template <typename T, typename U>
    typename std::common_type<T, U>::type max(const T& a, const U& b)
    {
        return (a > b) ? a : b;
    }

    /// @brief Minimum of two @a vec3.
    /// @param[in] a The first @a vec3.
    /// @param[in] b The second @a vec3.
    /// @return The minimum of both @a vec3.
    template <>
    inline vec3 min<vec3, vec3>(const vec3& a, const vec3& b)
    {
        return vec3(rayce::min(a.x(), b.x()), rayce::min(a.y(), b.y()), rayce::min(a.z(), b.z()));
    }

    /// @brief Maximum of two @a vec3.
    /// @param[in] a The first @a vec3.
    /// @param[in] b The second @a vec3.
    /// @return The maximum of both @a vec3.
    template <>
    inline vec3 max<vec3, vec3>(const vec3& a, const vec3& b)
    {
        return vec3(rayce::max(a.x(), b.x()), rayce::max(a.y(), b.y()), rayce::max(a.z(), b.z()));
    }

    /// @brief Minimum of two @a vec4.
    /// @param[in] a The first @a vec4.
    /// @param[in] b The second @a vec4.
    /// @return The minimum of both @a vec4.
    template <>
    inline vec4 min<vec4, vec4>(const vec4& a, const vec4& b)
    {
        return vec4(rayce::min(a.x(), b.x()), rayce::min(a.y(), b.y()), rayce::min(a.z(), b.z()), rayce::min(a.w(), b.w()));
    }

    /// @brief Maximum of two @a vec4.
    /// @param[in] a The first @a vec4.
    /// @param[in] b The second @a vec4.
    /// @return The maximum of both @a vec4.
    template <>
    inline vec4 max<vec4, vec4>(const vec4& a, const vec4& b)
    {
        return vec4(rayce::max(a.x(), b.x()), rayce::max(a.y(), b.y()), rayce::max(a.z(), b.z()), rayce::max(a.w(), b.w()));
    }

    /// @brief Create a perspective matrix (Vulkan).
    /// @param[in] fovy The vertical field of view in radians.
    /// @param[in] aspect The aspect ratio.
    /// @param[in] zNear Near plane depth.
    /// @param[in] zFar Far plane depth.
    /// @return An Vulkan perspective matrix.
    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> perspective(Scalar fovy, Scalar aspect, Scalar zNear, Scalar zFar)
    {
        Eigen::Transform<Scalar, 3, Eigen::Projective> tr;
        tr.matrix().setZero();
        assert(aspect > 0);
        assert(zFar > zNear);
        assert(zNear > 0);
        Scalar tan_half_fovy = std::tan(fovy / static_cast<Scalar>(2));
        Scalar focalLength   = static_cast<Scalar>(1) / (tan_half_fovy);
        tr(0, 0)             = focalLength / aspect;
        tr(1, 1)             = -focalLength;
        tr(2, 2)             = zFar / (zNear - zFar);
        tr(3, 2)             = -static_cast<Scalar>(1);
        tr(2, 3)             = -(zFar * zNear) / (zFar - zNear);
        return tr.matrix();
    }

    /// @brief Create a view matrix.
    /// @param[in] eye Eye position.
    /// @param[in] center The target position to look at.
    /// @param[in] up Up vector.
    /// @return A view matrix.
    template <typename Derived>
    Eigen::Matrix<typename Derived::Scalar, 4, 4> lookAt(const Derived& eye, const Derived& center, const Derived& up)
    {
        typedef Eigen::Matrix<typename Derived::Scalar, 4, 4> Matrix4;
        typedef Eigen::Matrix<typename Derived::Scalar, 3, 1> Vector3;
        typedef Eigen::Matrix<typename Derived::Scalar, 4, 1> Vector4;

        const Vector3 z = (center - eye).normalized();
        const Vector3 x = (z.cross(up)).normalized();
        const Vector3 y = x.cross(z);

        Matrix4 lA;
        lA << x.x(), x.y(), x.z(), -x.dot(eye), y.x(), y.y(), y.z(), -y.dot(eye), -z.x(), -z.y(), -z.z(), z.dot(eye), 0.0, 0.0, 0.0, 1.0;

        return lA;
    }

    /// @brief Create a scale matrix.
    /// @param[in] x Scaling in x direction.
    /// @param[in] y Scaling in y direction.
    /// @param[in] z Scaling in z direction.
    /// @return The scale matrix.
    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> scale(Scalar x, Scalar y, Scalar z)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setZero();
        tr(0, 0) = x;
        tr(1, 1) = y;
        tr(2, 2) = z;
        tr(3, 3) = 1;
        return tr.matrix();
    }

    /// @brief Create a translation matrix.
    /// @param[in] x Translation in x direction.
    /// @param[in] y Translation in y direction.
    /// @param[in] z Translation in z direction.
    /// @return The translation matrix.
    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> translate(Scalar x, Scalar y, Scalar z)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setIdentity();
        tr(0, 3) = x;
        tr(1, 3) = y;
        tr(2, 3) = z;
        return tr.matrix();
    }

    /// @brief Create a scale matrix.
    /// @param[in] scale Scaling in x,y,z direction.
    /// @return The scale matrix.
    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> scale(const Eigen::Vector3<Scalar>& scale)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setZero();
        tr(0, 0) = scale.x();
        tr(1, 1) = scale.y();
        tr(2, 2) = scale.z();
        tr(3, 3) = 1;
        return tr.matrix();
    }

    /// @brief Create a translation matrix.
    /// @param[in] translation Translation in x,y,z direction.
    /// @return The translation matrix.
    template <typename Scalar>
    Eigen::Matrix<Scalar, 4, 4> translate(const Eigen::Vector3<Scalar>& translation)
    {
        Eigen::Transform<Scalar, 3, Eigen::Affine> tr;
        tr.matrix().setIdentity();
        tr(0, 3) = translation.x();
        tr(1, 3) = translation.y();
        tr(2, 3) = translation.z();
        return tr.matrix();
    }

    //! @brief Create a @a vec3 from one value.
    //! @param[in] value The value to fill the @a vec3 with.
    //! @return The created @a vec3.
    inline vec3 makeVec3(const float& value)
    {
        vec3 v;
        v << value, value, value;
        return v;
    }

    /*
    /// @brief Calculate a rotation matrix from a quaternion.
    /// @param[in] rotation The rotation quaternion.
    /// @return The rotation matrix for the given quaternion.
    inline mat4 quaternion_to_mat4(const quat& rotation)
    {
        mat4 m              = Eigen::Matrix4f::Identity();
        m.block(0, 0, 3, 3) = rotation.toRotationMatrix();
        return m;
    }

    /// @brief Decompose a transformation matrix in scale, rotation and translation.
    /// @param[in] input The transformation matrix to decompose.
    /// @param[out] out_scale Output vector for scale.
    /// @param[out] out_rotation Output quaternion for rotation.
    /// @param[out] out_translation Output vector for translation.
    inline void decompose_transformation(const mat4& input, vec3& out_scale, quat& out_rotation, vec3& out_translation)
    {
        out_translation = input.col(3).head<3>();
        mat3 rot        = input.block(0, 0, 3, 3);
        out_scale.x()   = rot.col(0).norm();
        out_scale.y()   = rot.col(1).norm();
        out_scale.z()   = (rot.determinant() < 0.0 ? -1 : 1) * rot.col(2).norm();

        rot.col(0) /= out_scale.x();
        rot.col(1) /= out_scale.y();
        rot.col(2) /= out_scale.z();

        out_rotation = quat(rot);
    }
    */

} // namespace rayce

#endif // TYPES_HPP
