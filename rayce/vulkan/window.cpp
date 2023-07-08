/// @file      window.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/window.hpp>

using namespace rayce;

Window::Window(int32 width, int32 height, const str& name)
    : mWindowWidth(width)
    , mWindowHeight(height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, name.c_str(), nullptr, nullptr);

    RAYCE_CHECK_NOTNULL(pWindow, "Creating GLFW window failed!");

    RAYCE_LOG_INFO("Created window of size (%d, %d)!", mWindowWidth, mWindowHeight);
}

Window::~Window()
{
    if (pWindow)
    {
        glfwDestroyWindow(pWindow);
    }
    glfwTerminate();
}

std::vector<const char*> Window::getVulkanExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}
