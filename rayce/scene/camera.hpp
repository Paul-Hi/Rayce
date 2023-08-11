/// @file      camera.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYCE_CAMERA_HPP
#define RAYCE_CAMERA_HPP

namespace rayce
{
    class RAYCE_API_EXPORT Camera
    {
    public:
        Camera(float aspect, float fovy, float zNear, float zFar, const vec3& position, const vec3& target);
        ~Camera();

        mat4 getInverseView()
        {
            return mInverseView;
        }

        mat4 getInverseProjection()
        {
            return mInverseProjection;
        }

    private:
        float mAspect;
        float mFovy;
        float mZNear;
        float mZFar;
        vec3 mPosition;
        vec3 mTarget;

        mat4 mInverseView;
        mat4 mInverseProjection;
    };

} // namespace rayce

#endif // RAYCE_CAMERA_HPP
