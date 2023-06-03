/// @file      rayceGui.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <rayce.hpp>

using namespace rayce;

void imguiDraw(std::unique_ptr<RayceAppState>& appState)
{
    RAYCE_UNUSED(appState);
}

int main(int argc, char** argv)
{
    RayceOptions options;
    options.windowWidth = 1920;
    options.windowHeight = 1080;
    options.customGui = &imguiDraw;
    std::unique_ptr<RayceApp> pApplication = createApplication(argc, argv, options);

    if (!pApplication->initializeVulkan())
    {
        return 1;
    }

    pApplication->run();

    pApplication->shutdown();

    return 0;
}

