/// @file      instanceData.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef INSTANCE_DATA_HPP
#define INSTANCE_DATA_HPP

namespace rayce
{
    struct RAYCE_API_EXPORT InstanceData
    {
        uint64 indexReference;
        uint64 vertexReference;
        uint32 materialId;

        int32 pad[3];
    };
} // namespace rayce

#endif // INSTANCE_DATA_HPP
