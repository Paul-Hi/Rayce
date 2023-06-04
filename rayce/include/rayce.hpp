/// @file      rayce.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

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
} // namespace rayce

#endif // RAYCE_HPP
