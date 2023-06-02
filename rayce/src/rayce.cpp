/// @file      rayce.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#include <rayce.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

bool rayce::init()
{
    return true;
}

bool rayce::shutdown()
{
    return true;
}

bool rayce::testbed()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Rayce", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return true;
}
