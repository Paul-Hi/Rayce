/// @file      surface.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef SURFACE_HPP
#define SURFACE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Surface
    {
      public:
        RAYCE_DISABLE_COPY_MOVE(Surface)

        Surface(VkInstance instance, GLFWwindow* nativeWindowHandle);
        ~Surface();

        VkSurfaceKHR getVkSurface() const
        {
            return mVkSurface;
        }

      private:
        VkSurfaceKHR mVkSurface;
        VkInstance mVkInstanceRef;
    };
} // namespace rayce

#endif // SURFACE_HPP
