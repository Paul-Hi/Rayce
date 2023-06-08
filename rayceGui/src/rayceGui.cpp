/// @file      rayceGui.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "rayceGui.hpp"

using namespace rayce;

RayceGui::RayceGui(const RayceOptions& options)
    : RayceApp::RayceApp(options)
{
}

bool RayceGui::onInitialize()
{
    return RayceApp::onInitialize();
}

bool RayceGui::onShutdown()
{
    return RayceApp::onShutdown();
}

void RayceGui::onUpdate()
{
    RayceApp::onUpdate();
}

void RayceGui::onFrameDraw()
{
    RayceApp::onFrameDraw();
}

void RayceGui::onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    RayceApp::onRender(commandBuffer, imageIndex);
}

void RayceGui::onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    RayceApp::onImGuiRender(commandBuffer, imageIndex);
}

int main(int argc, char** argv)
{
    RAYCE_UNUSED(argc);
    RAYCE_UNUSED(argv);

    RayceOptions options;
    options.windowWidth  = 1920;
    options.windowHeight = 1080;
    options.name         = "RayceGui";
#ifdef RAYCE_DEBUG
    options.enableValidationLayers = true;
#else
    options.enableValidationLayers = false;
#endif

    RayceGui application(options);

    application.run();

    return 0;
}
