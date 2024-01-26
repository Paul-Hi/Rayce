/// @file      utils.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

#ifndef UTILS_HPP
#define UTILS_HPP

namespace rayce
{
    inline uint32 quickAlign(uint32 value, uint32 alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    template <typename Pred>
    ptr_size interval(ptr_size sz, const Pred& pred)
    {
        using signed_ptr_size = std::make_signed_t<ptr_size>;
        signed_ptr_size size = (signed_ptr_size)sz - 2, first = 1;
        while (size > 0)
        {
            ptr_size half = (ptr_size)size >> 1, middle = first + half;
            bool predResult = pred(middle);
            first           = predResult ? middle + 1 : first;
            size            = predResult ? size - (half + 1) : half;
        }
        return (ptr_size)std::clamp((signed_ptr_size)first - 1, (signed_ptr_size)0, (signed_ptr_size)sz - 2);
    }

    inline str trim(const str& string)
    {
        str::size_type
            start = string.find_first_not_of(" \t\r\n"),
            end   = string.find_last_not_of(" \t\r\n");

        return string.substr(start == str::npos ? 0 : start, end == str::npos ? string.length() - 1 : end - start + 1);
    }
} // namespace rayce

#endif // UTILS_HPP
