/// @file      surface.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/surface.hpp>

using namespace rayce;

Surface::Surface(VkInstance instance, GLFWwindow* nativeWindowHandle)
    : mVkInstanceRef(instance)
{
    RAYCE_CHECK_VK(glfwCreateWindowSurface(instance, nativeWindowHandle, nullptr, &mVkSurface), "Creating window surface failed!");

    RAYCE_LOG_INFO("Created window surface!");
}

Surface::~Surface()
{
    if (mVkSurface)
    {
        vkDestroySurfaceKHR(mVkInstanceRef, mVkSurface, nullptr);
    }
}
