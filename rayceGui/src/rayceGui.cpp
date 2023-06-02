/// @file      rayceGui.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#include <rayce.hpp>

using namespace rayce;

/// @brief Portable main.
/// @param[in] argc Argument count.
/// @param[in] argv Arguments
/// @return 0 on success, or error code.
int mainFunction(int argc, char** argv)
{
    std::unique_ptr<RayceApp> pApplication = createApplication(argc, argv, 1920, 1080);

    if (!pApplication->initializeVulkan())
    {
        return 1;
    }

    pApplication->run();

    pApplication->shutdown();

    std::cin.get();

    return 0;
}

#ifdef RAYCE_WINMAIN
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    return mainFunction(__argc, __argv);
}
#else
int main(int argc, char** argv)
{
    return mainFunction(argc, argv);
}
#endif // RAYCE_WINMAIN
