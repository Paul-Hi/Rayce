/// @file      rayce.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023

#ifndef RAYCE_HPP
#define RAYCE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <rayceApp.hpp>
#include <types.hpp>

#ifdef WIN32
#ifndef UNICODE
#define UNICODE
#endif // UNICODE

#define NOMINMAX
#include <windows.h>

#endif // WIN32

namespace rayce
{
    /// @brief Initializes the shared library.
    RAYCE_API_HIDDEN bool init();

    /// @brief Shuts the shared library down.
    RAYCE_API_HIDDEN bool shutdown();

    /// @brief Creates and returns a \a RayceApp.
    /// @param[in] argc The number of command line parameters.
    /// @param[in] argv List of command line parameters.
    /// @param[in] width The width of the window to create.
    /// @param[in] height The height of the window to create.
    /// @return A pointer to the created \a RayceApp.
    RAYCE_API_EXPORT std::unique_ptr<RayceApp> createApplication(int32 argc, char** argv, int32 width, int32 height);
} // namespace rayce

#endif // RAYCE_HPP
