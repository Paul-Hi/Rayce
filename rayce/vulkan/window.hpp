/// @file      window.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Window
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Window)

        Window(int32 width, int32 height, const str& name);
        ~Window();

        int32 getWindowWidth() const
        {
            return mWindowWidth;
        }

        int32 getWindowHeight() const
        {
            return mWindowHeight;
        }

        GLFWwindow* getNativeWindowHandle() const
        {
            return pWindow;
        }

        std::vector<const char*> getVulkanExtensions() const;

        bool shouldClose() const
        {
            return glfwWindowShouldClose(pWindow);
        }

        void pollEvents() const
        {
            glfwPollEvents();
        }

        bool isMinimized() const
        {
            int32 width, height;
            glfwGetFramebufferSize(pWindow, &width, &height);
            return height == 0 && width == 0;
        }

        void waitEvents() const
        {
            glfwWaitEvents();
        }

    private:
        int32 mWindowWidth;
        int32 mWindowHeight;

        GLFWwindow* pWindow;
    };
} // namespace rayce

#endif // WINDOW_HPP
