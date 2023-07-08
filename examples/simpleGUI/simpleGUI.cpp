/// @file      simpleGUI.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "simpleGUI.hpp"

using namespace rayce;

SimpleGUI::SimpleGUI(const RayceOptions& options)
    : RayceApp::RayceApp(options)
{
}

bool SimpleGUI::onInitialize()
{
    return RayceApp::onInitialize();
}

bool SimpleGUI::onShutdown()
{
    return RayceApp::onShutdown();
}

void SimpleGUI::onUpdate()
{
    RayceApp::onUpdate();
}

void SimpleGUI::onFrameDraw()
{
    RayceApp::onFrameDraw();
}

void SimpleGUI::onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    RayceApp::onRender(commandBuffer, imageIndex);
}

void SimpleGUI::onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex)
{
    // Only call provided stuff - we do not want to share imgui context...
    RayceApp::onImGuiRender(commandBuffer, imageIndex);
}

int main(int argc, char** argv)
{
    RAYCE_UNUSED(argc);
    RAYCE_UNUSED(argv);

    RayceOptions options;
    options.windowWidth  = 1920;
    options.windowHeight = 1080;
    options.name         = "SimpleGUI";
#ifdef RAYCE_DEBUG
    options.enableValidationLayers = true;
#else
    options.enableValidationLayers = false;
#endif

    SimpleGUI application(options);

    application.run();

    return 0;
}
