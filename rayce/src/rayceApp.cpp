/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#include "rayceApp.hpp"

using namespace rayce;

RayceApp::RayceApp(int32 width, int32 height)
    : mWidth(width)
    , mHeight(height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mWindow = glfwCreateWindow(width, height, "RayceApp", nullptr, nullptr);

    CHECK_NOTNULL_F(mWindow, "Creating GLFW window failed!");

    LOG_F(INFO, "Created RayceApp of size (%d, %d)!", width, height);
}

bool RayceApp::initializeVulkan()
{
    // Initialize the vulkan instance

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "RayceApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Rayce";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32 glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount   = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount       = 0;

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
    {
        LOG_F(INFO, "Creating Vulkan instance failed!");
        return false;
    }

    LOG_F(INFO, "Created Vulkan instance!");

    return true;
}

bool RayceApp::run()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
    }
    return true;
}

bool RayceApp::shutdown()
{
    if (mInstance)
    {
        vkDestroyInstance(mInstance, nullptr);
    }

    glfwDestroyWindow(mWindow);
    glfwTerminate();

    LOG_F(INFO, "Shutdown RayceApp!");

    return true;
}
