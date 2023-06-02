/// @file      rayceApp.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#ifndef RAYCE_APP_HPP
#define RAYCE_APP_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace rayce
{
    /// @brief Application state.
    class RayceApp
    {
      public:
        /// @brief Constructor.
        /// @param[in] width The width of the application.
        /// @param[in] height The height of the application.
        RayceApp(int32 width, int32 height);

        bool RAYCE_API_EXPORT initializeVulkan();

        bool RAYCE_API_EXPORT run();

        bool RAYCE_API_EXPORT shutdown();

      private:
        GLFWwindow* mWindow;
        int32 mWidth;
        int32 mHeight;

        VkInstance mInstance;
    };
} // namespace rayce

#endif // RAYCE_APP_HPP
