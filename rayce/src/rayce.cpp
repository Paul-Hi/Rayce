/// @file      rayce.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#include <rayce.hpp>

using namespace rayce;

RAYCE_API_HIDDEN bool rayce::init()
{
    return true;
}

RAYCE_API_HIDDEN bool rayce::shutdown()
{
    return true;
}

RAYCE_API_EXPORT std::unique_ptr<RayceApp> rayce::createApplication(int32 argc, char** argv, int32 width, int32 height)
{
    loguru::init(argc, argv);
    return std::make_unique<RayceApp>(width, height);
}

// Shared library attach/detach functions

#ifdef WIN32

RAYCE_API_EXPORT BOOL DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    RAYCE_UNUSED(hinstDLL);
    RAYCE_UNUSED(lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        return rayce::init() ? TRUE : FALSE;
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        return rayce::shutdown() ? TRUE : FALSE;
    }

    return TRUE;
}

#else if LINUX

RAYCE_API_EXPORT __attribute__((constructor)) bool soAttach()
{
    return rayce::init();
}

RAYCE_API_EXPORT __attribute__((destructor)) bool soDetach()
{
    return rayce::shutdown();
}

#endif
