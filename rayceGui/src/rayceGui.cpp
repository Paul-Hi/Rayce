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

void RayceGui::onRender()
{
    RayceApp::onRender();
}

void RayceGui::onImGuiRender()
{
    RayceApp::onImGuiRender();
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
