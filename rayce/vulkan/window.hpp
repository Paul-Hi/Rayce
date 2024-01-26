/// @file      window.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef WINDOW_HPP
#define WINDOW_HPP

namespace rayce
{
    class RAYCE_API_EXPORT Window
    {
    public:
        RAYCE_DISABLE_COPY_MOVE(Window)

        Window(int32 width, int32 height, const str& name, const std::shared_ptr<class Input> input);
        ~Window();

        int32 getWindowWidth() const
        {
            return mWindowData.width;
        }

        int32 getWindowHeight() const
        {
            return mWindowData.height;
        }

        GLFWwindow* getNativeWindowHandle() const
        {
            return mWindowData.pWindow;
        }

        std::vector<const char*> getVulkanExtensions() const;

        bool shouldClose() const
        {
            return glfwWindowShouldClose(mWindowData.pWindow);
        }

        void close() const
        {
            glfwSetWindowShouldClose(mWindowData.pWindow, GLFW_TRUE);
        }

        void pollEvents() const
        {
            glfwPollEvents();
        }

        bool isMinimized() const
        {
            int32 width, height;
            glfwGetFramebufferSize(mWindowData.pWindow, &width, &height);
            return height == 0 && width == 0;
        }

        void waitEvents() const
        {
            glfwWaitEvents();
        }

    private:
        struct WindowData
        {
            /// @brief The native handle to the GLFWWindow.
            GLFWwindow* pWindow;

            /// @brief Horizontal screen position.
            int32 x;
            /// @brief Vertical screen position.
            int32 y;
            /// @brief @a Window width.
            int32 width;
            /// @brief @a Window height.
            int32 height;
            /// @brief @a Window title.
            const char* title;

            /// @brief The input.
            std::weak_ptr<class Input> pInput;
        } mWindowData; ///< The internal data for the Window.
    };
} // namespace rayce

#endif // WINDOW_HPP
