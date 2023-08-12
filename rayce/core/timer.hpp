/// @file      Timer.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYCE_TIMER_HPP
#define RAYCE_TIMER_HPP

#include <chrono>

namespace rayce
{
    /// @cond NO_DOC
    using clock_type = typename std::conditional<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>::type;
    /// @endcond
    /// @brief Typedef of a timestep.
    using timestep = std::chrono::time_point<clock_type>;

    /// @brief Some small Timer class.
    class RAYCE_API_EXPORT Timer
    {
      public:
        ~Timer() = default;

        /// @brief Returns the elapsed time since the last start() call.
        /// @return The elapsed time since the last start() call in microseconds.
        inline std::chrono::microseconds elapsedMicroseconds() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(clock_type::now() - mStartTime);
        }

        /// @brief Returns the elapsed time since the last start() call.
        /// @return The elapsed time since the last start() call in milliseconds.
        inline std::chrono::milliseconds elapsedMilliseconds() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - mStartTime);
        }

        /// @brief Returns the elapsed time since the last start() call.
        /// @return The elapsed time since the last start() call in seconds.
        inline std::chrono::seconds elapsedSeconds() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(clock_type::now() - mStartTime);
        }

        /// @brief Starts the @a Timer.
        inline void start()
        {
            mStartTime = clock_type::now();
        }

        /// @brief Restarts the @a Timer.
        inline void restart()
        {
            mStartTime = clock_type::now();
        }

      private:
        /// @brief The time the Timer started.
        timestep mStartTime;
    };
} // namespace rayce

#endif // RAYCE_TIMER_HPP
