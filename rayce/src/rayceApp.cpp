/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "rayceApp.hpp"

using namespace rayce;

static std::vector<const char*> sValidationLayers = { "VK_LAYER_KHRONOS_validation" };

RayceApp::RayceApp(const RayceOptions& options)
{
    mWindowWidth            = options.windowWidth;
    mWindowHeight           = options.windowHeight;
    mEnableValidationLayers = options.enableValidationLayers;

    pWindow   = std::make_unique<Window>(mWindowWidth, mWindowHeight, options.name);
    std::vector<const char*> windowExtensions = pWindow->getVulkanExtensions();
    pInstance = std::make_unique<Instance>(mEnableValidationLayers, windowExtensions, sValidationLayers);

    RAYCE_CHECK(onInitialize(), "onInitialize() failed!");

    RAYCE_LOG_INFO("Created RayceApp!");
}

RayceApp::~RayceApp()
{
    RAYCE_CHECK(onShutdown(), "onShutdown() failed!");
    RAYCE_LOG_INFO("Destroyed RayceApp!");
};

bool RayceApp::run()
{
    while (!pWindow->shouldClose())
    {
        pWindow->pollEvents();

        onUpdate();
        onRender();
        onImGuiRender();
    }
    return true;
}

bool RayceApp::onInitialize()
{
    return true;
}

bool RayceApp::onShutdown()
{
    return true;
}

void RayceApp::onUpdate() {}

void RayceApp::onRender() {}

void RayceApp::onImGuiRender() {}
