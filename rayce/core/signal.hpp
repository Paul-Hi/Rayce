/// @file      signal.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2022
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYCE_SIGNAL_HPP
#define RAYCE_SIGNAL_HPP

namespace rayce
{
    /// @brief Implementation of a simple class providing signal slot functionality .
    /// @details Can be used to call multiple function callbacks.
    template <typename... Args>
    class RAYCE_API_EXPORT Signal
    {
        /// @brief Convenience typedef of the template function.
        using fun = std::function<void(Args...)>;

      public:
        /// @brief Connects a function to the signal that will be called on signal execution.
        /// @param[in] fn The function to callback.
        inline void connect(fun fn)
        {
            mObservers.push_back(fn);
        }

        /// @brief Executes the connected functions with given arguments.
        /// @param[in] args 0 to n arguments to call the functions with.
        void operator()(Args... args)
        {
            for (const auto& fn : mObservers)
            {
                fn(args...);
            }
        }

      private:
        /// @brief The connected functions.
        std::vector<fun> mObservers;
    };
} // namespace rayce

#endif // RAYCE_SIGNAL_HPP
