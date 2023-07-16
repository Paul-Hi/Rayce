/// @file      log.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef LOG_HPP
/// @cond NO_DOC
#define LOG_HPP
/// @endcond

#define LOGURU_EXPORT RAYCE_API_EXPORT
#include <core/loguru.hpp>

/// @brief Info log.
#define RAYCE_LOG_INFO(...) LOG_F(INFO, __VA_ARGS__)
/// @brief Warning log.
#define RAYCE_LOG_WARN(...) LOG_F(WARNING, __VA_ARGS__)
/// @brief Error log.
#define RAYCE_LOG_ERROR(...) LOG_F(ERROR, __VA_ARGS__)

/// @brief Debug info log.
#define RAYCE_LOG_DINFO(...) DLOG_F(INFO, __VA_ARGS__)
/// @brief Debug warn log.
#define RAYCE_LOG_DWARN(...) DLOG_F(WARNING, __VA_ARGS__)
/// @brief Debug error log.
#define RAYCE_LOG_DERROR(...) DLOG_F(ERROR, __VA_ARGS__)

/// @brief Abort while logging error.
#define RAYCE_ABORT(...) ABORT_F(__VA_ARGS__)
/// @brief Check and abort if fails.
#define RAYCE_CHECK(test, ...) CHECK_F(test, __VA_ARGS__)
/// @brief Check not null and abort if fails.
#define RAYCE_CHECK_NOTNULL(x, ...) CHECK_NOTNULL_F(x, __VA_ARGS__)
/// @brief Check equality and abort if fails.
#define RAYCE_CHECK_EQ(a, b, ...) CHECK_EQ_F(a, b, __VA_ARGS__)
/// @brief Check not equal and abort if fails.
#define RAYCE_CHECK_NE(a, b, ...) CHECK_NE_F(a, b, __VA_ARGS__)
/// @brief Check less then and abort if fails.
#define RAYCE_CHECK_LT(a, b, ...) CHECK_LT_F(a, b, __VA_ARGS__)
/// @brief Check less then or equals and abort if fails.
#define RAYCE_CHECK_LE(a, b, ...) CHECK_LE_F(a, b, __VA_ARGS__)
/// @brief Check greater then and abort if fails.
#define RAYCE_CHECK_GT(a, b, ...) CHECK_GT_F(a, b, __VA_ARGS__)
/// @brief Check greater then or equals and abort if fails.
#define RAYCE_CHECK_GE(a, b, ...) CHECK_GE_F(a, b, __VA_ARGS__)

/// @brief Check vulkan status and abort if fails.
#define RAYCE_CHECK_VK(x, info, ...) LOGURU_PREDICT_TRUE((x) == VK_SUCCESS) ? (void)0 : loguru::log_and_abort(0, "VULKAN CHECK FAILED (" #x "):  " info "  ", __FILE__, __LINE__, ##__VA_ARGS__)

/// @brief Check vulkan status and log if it fails without aborting.
#define RAYCE_TRY_VK(x, info, ...) LOGURU_PREDICT_TRUE((x) == VK_SUCCESS) ? (void)0 : loguru::log(-1, __FILE__, __LINE__, "VULKAN TRY FAILED (" #x "):  " info "  "##__VA_ARGS__)

#ifdef LOGURU_DEBUG_CHECKS
/// @brief Debug check and abort if fails.
#define RAYCE_DCHECK(test, ...) DCHECK_F(test, __VA_ARGS__)
/// @brief Debug check not null and abort if fails.
#define RAYCE_DCHECK_NOTNULL(x, ...) DCHECK_NOTNULL_F(x, __VA_ARGS__)
/// @brief Debug check equality and abort if fails.
#define RAYCE_DCHECK_EQ(a, b, ...) DCHECK_EQ_F(a, b, __VA_ARGS__)
/// @brief Debug check not equal and abort if fails.
#define RAYCE_DCHECK_NE(a, b, ...) DCHECK_NE_F(a, b, __VA_ARGS__)
/// @brief Debug check less then and abort if fails.
#define RAYCE_DCHECK_LT(a, b, ...) DCHECK_LT_F(a, b, __VA_ARGS__)
/// @brief Debug check less then or equals and abort if fails.
#define RAYCE_DCHECK_LE(a, b, ...) DCHECK_LE_F(a, b, __VA_ARGS__)
/// @brief Debug check greater then and abort if fails.
#define RAYCE_DCHECK_GT(a, b, ...) DCHECK_GT_F(a, b, __VA_ARGS__)
/// @brief Debug check greater then or equals and abort if fails.
#define RAYCE_DCHECK_GE(a, b, ...) DCHECK_GE_F(a, b, __VA_ARGS__)
/// @brief Debug heck vulkan status and abort if fails.
#define RAYCE_DCHECK_VK(x, info, ...) RAYCE_CHECK_VK(x, info, __VA_ARGS__)
/// @brief Debug check vulkan status and log if it fails without aborting.
#define RAYCE_DTRY_VK(x, info, ...) RAYCE_TRY_VK(x, info, __VA_ARGS__)
#else
/// @brief Debug check and abort if fails.
#define RAYCE_DCHECK(test, ...)
/// @brief Debug check not null and abort if fails.
#define RAYCE_DCHECK_NOTNULL(x, ...)
/// @brief Debug check equality and abort if fails.
#define RAYCE_DCHECK_EQ(a, b, ...)
/// @brief Debug check not equal and abort if fails.
#define RAYCE_DCHECK_NE(a, b, ...)
/// @brief Debug check less then and abort if fails.
#define RAYCE_DCHECK_LT(a, b, ...)
/// @brief Debug check less then or equals and abort if fails.
#define RAYCE_DCHECK_LE(a, b, ...)
/// @brief Debug check greater then and abort if fails.
#define RAYCE_DCHECK_GT(a, b, ...)
/// @brief Debug check greater then or equals and abort if fails.
#define RAYCE_DCHECK_GE(a, b, ...)
/// @brief Debug heck vulkan status and abort if fails.
#define RAYCE_DCHECK_VK(x, info, ...)
/// @brief Debug check vulkan status and log if it fails without aborting.
#define RAYCE_DTRY_VK(x, info, ...)
#endif
#endif // LOG_HPP
