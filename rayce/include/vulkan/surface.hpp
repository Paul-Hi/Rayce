/// @file      surface.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef SURFACE_HPP
#define SURFACE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Surface
    {
      public:
        Surface(VkInstance instance, GLFWwindow* nativeWindowHandle);
        ~Surface();

        VkSurfaceKHR getVkSurface()
        {
            return mVkSurface;
        }

      private:
        VkSurfaceKHR mVkSurface;
        VkInstance mVkInstanceRef;
    };
} // namespace rayce

#endif // SURFACE_HPP
