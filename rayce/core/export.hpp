/// @file      export.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef EXPORT_HPP
/// @cond NO_DOC
#define EXPORT_HPP
/// @endcond

#ifdef RAYCE_SHARED

#if defined _WIN32 || defined __CYGWIN__
#ifdef RAYCE_LIBRARY
#ifdef __GNUC__
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT __attribute__((dllexport))
#else
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT __attribute__((dllimport))
#else
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT __declspec(dllimport)
#endif
#endif
/// @brief Hide symbols.
#define RAYCE_API_HIDDEN
#else
#if __GNUC__ >= 4
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT __attribute__((visibility("default")))
/// @brief Hide symbols.
#define RAYCE_API_HIDDEN __attribute__((visibility("hidden")))
#else
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT
/// @brief Hide symbols.
#define RAYCE_API_HIDDEN
#endif
#endif
#else
/// @brief Export/Import symbols.
#define RAYCE_API_EXPORT
/// @brief Hide symbols.
#define RAYCE_API_HIDDEN
#endif

#endif // EXPORT_HPP
