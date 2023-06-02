/// @file      rayce_gui.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#include <rayce.hpp>

using namespace rayce;

/// @brief Portable main.
/// @param argc Argument count.
/// @param argv Arguments
/// @return 0 on success, or error code.
int mainFunction(int argc, char** argv)
{
    RAYCE_UNUSED(argc);
    RAYCE_UNUSED(argv);

    testbed();

    return 0;
}

#ifdef RAYCE_WINMAIN
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    return mainFunction(__argc, __argv);
}
#else
int main(int argc, char** argv)
{
    return mainFunction(argc, argv);
}
#endif // RAYCE_WINMAIN
