/// @file      window.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Window
    {
      public:
        DISABLE_COPY_MOVE_VK(Window)

        Window(int32 width, int32 height, const str& name);
        ~Window();

        int32 getWindowWidth()
        {
            return mWindowWidth;
        }

        int32 getWindowHeight()
        {
            return mWindowHeight;
        }

        GLFWwindow* getNativeWindowHandle()
        {
            return pWindow;
        }

        std::vector<const char*> getVulkanExtensions();

        bool shouldClose()
        {
            return glfwWindowShouldClose(pWindow);
        }

        void pollEvents()
        {
            glfwPollEvents();
        }

      private:
        int32 mWindowWidth;
        int32 mWindowHeight;

        GLFWwindow* pWindow;
    };
} // namespace rayce

#endif // WINDOW_HPP
