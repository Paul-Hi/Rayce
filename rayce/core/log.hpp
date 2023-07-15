/// @file      log.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef LOG_HPP
#define LOG_HPP

#define LOGURU_EXPORT RAYCE_API_EXPORT
#include <core/loguru.hpp>

#define RAYCE_LOG_INFO(...) LOG_F(INFO, __VA_ARGS__)
#define RAYCE_LOG_WARN(...) LOG_F(WARNING, __VA_ARGS__)
#define RAYCE_LOG_ERROR(...) LOG_F(ERROR, __VA_ARGS__)

#define RAYCE_LOG_DINFO(...) DLOG_F(INFO, __VA_ARGS__)
#define RAYCE_LOG_DWARN(...) DLOG_F(WARNING, __VA_ARGS__)
#define RAYCE_LOG_DERROR(...) DLOG_F(ERROR, __VA_ARGS__)

#define RAYCE_ABORT(...) ABORT_F(__VA_ARGS__)
#define RAYCE_CHECK(test, ...) CHECK_F(test, __VA_ARGS__)
#define RAYCE_CHECK_NOTNULL(x, ...) CHECK_NOTNULL_F(x, __VA_ARGS__)
#define RAYCE_CHECK_EQ(a, b, ...) CHECK_EQ_F(a, b, __VA_ARGS__)
#define RAYCE_CHECK_NE(a, b, ...) CHECK_NE_F(a, b, __VA_ARGS__)
#define RAYCE_CHECK_LT(a, b, ...) CHECK_LT_F(a, b, __VA_ARGS__)
#define RAYCE_CHECK_LE(a, b, ...) CHECK_LE_F(a, b, __VA_ARGS__)
#define RAYCE_CHECK_GT(a, b, ...) CHECK_GT_F(a, b, __VA_ARGS__)
#define RAYCE_CHECK_GE(a, b, ...) CHECK_GE_F(a, b, __VA_ARGS__)

#define RAYCE_CHECK_VK(x, info, ...) LOGURU_PREDICT_TRUE((x) == VK_SUCCESS) ? (void)0 : loguru::log_and_abort(0, "VULKAN CHECK FAILED (" #x "):  " info "  ", __FILE__, __LINE__, ##__VA_ARGS__)

#define RAYCE_TRY_VK(x, info, ...) LOGURU_PREDICT_TRUE((x) == VK_SUCCESS) ? (void)0 : loguru::log(-1, __FILE__, __LINE__, "VULKAN TRY FAILED (" #x "):  " info "  "##__VA_ARGS__)

#define RAYCE_DCHECK(test, ...) DCHECK_F(test, __VA_ARGS__)
#define RAYCE_DCHECK_NOTNULL(x, ...) DCHECK_NOTNULL_F(x, __VA_ARGS__)
#define RAYCE_DCHECK_EQ(a, b, ...) DCHECK_EQ_F(a, b, __VA_ARGS__)
#define RAYCE_DCHECK_NE(a, b, ...) DCHECK_NE_F(a, b, __VA_ARGS__)
#define RAYCE_DCHECK_LT(a, b, ...) DCHECK_LT_F(a, b, __VA_ARGS__)
#define RAYCE_DCHECK_LE(a, b, ...) DCHECK_LE_F(a, b, __VA_ARGS__)
#define RAYCE_DCHECK_GT(a, b, ...) DCHECK_GT_F(a, b, __VA_ARGS__)
#define RAYCE_DCHECK_GE(a, b, ...) DCHECK_GE_F(a, b, __VA_ARGS__)

#ifdef LOGURU_DEBUG_CHECKS
#define RAYCE_DCHECK_VK(x, info, ...) RAYCE_CHECK_VK(x, info, __VA_ARGS__)
#define RAYCE_DTRY_VK(x, info, ...) RAYCE_TRY_VK(x, info, __VA_ARGS__)
#else
#define RAYCE_DCHECK_VK(x, info, ...)
#define RAYCE_DTRY_VK(x, info, ...)
#endif
#endif // LOG_HPP
