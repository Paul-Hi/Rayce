/// @file      surface.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef SURFACE_HPP
#define SURFACE_HPP

namespace rayce
{
    /// @brief Wrapper for a vulkan surface.
    class RAYCE_API_EXPORT Surface
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Surface)

        /// @brief Constructs a new vulkan @a Surface.
        /// @param[in] instance Handle to the vulkan instance.
        /// @param[in] nativeWindowHandle Native platform window handle.
        Surface(VkInstance instance, GLFWwindow* nativeWindowHandle);
        /// @brief Destructor.
        ~Surface();

        /// @brief Returns the underlying native vulkan surface handle.
        /// @return The underlying native vulkan surface handle.
        VkSurfaceKHR getVkSurface() const
        {
            return mVkSurface;
        }

    private:
        /// @brief Vulkan surface handle.
        VkSurfaceKHR mVkSurface;
        /// @brief Vulkan instance reference handle.
        VkInstance mVkInstanceRef;
    };
} // namespace rayce

#endif // SURFACE_HPP
