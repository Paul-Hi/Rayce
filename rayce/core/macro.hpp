/// @file      macro.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef MACRO_HPP
#define MACRO_HPP

#include <iostream>
#include <core/log.hpp>
#include <stdio.h>
#include <core/types.hpp>

/// @brief Macro for unused parameters.
#define RAYCE_UNUSED(x) (void)x

#define RAYCE_DISABLE_COPY_MOVE(className)              \
    className(const className&)            = delete; \
    className(className&&)                 = delete; \
    className& operator=(const className&) = delete; \
    className& operator=(className&&)      = delete;

#ifdef RAYCE_DEBUG

/// @brief Macro for assertions.
/// @details Checks and prints out the expression on fail.
//! An additional message can be added.
//! If the asserted expression is true, nothing happens.
//! Assertions are only enabled in debug mode.
#define RAYCE_ASSERT(expression, ...)                                                                                                                                                             \
    ((void)(!(expression) && (RAYCE_ABORT("\nAssertion '{0}' failed in function {1}, file {2}, line {3}.\nMessage: '{4}. Pause.'", #expression, __func__, __FILE__, __LINE__, __VA_ARGS__), 1) && \
            (std::cin.get(), 1) && (std::abort(), 1)))

#else

/// @brief Macro for assertions.
/// @details Checks and prints out the expression on fail.
//! An additional message can be added.
//! If the asserted expression is true, nothing happens.
//! Assertions are only enabled in debug mode.
#define RAYCE_ASSERT(expression, ...) \
    do                                \
    {                                 \
        (void)sizeof(expression);     \
    } while (0)

#endif

#endif // MACRO_HPP
