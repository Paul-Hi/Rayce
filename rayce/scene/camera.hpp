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
        Camera(float aspect, float fovy, float zNear, float zFar, float lensRadius, float focalDistance, const vec3& position, const vec3& target, const std::shared_ptr<class Input> input);
        ~Camera();

        void updateAspect(float aspect);

        bool onImGuiRender();
        bool update(float dt);

        mat4 getInverseView()
        {
            return mInverseView;
        }

        mat4 getInverseProjection()
        {
            return mInverseProjection;
        }

        float getNear()
        {
            return mZNear;
        }

        float getFar()
        {
            return mZFar;
        }

        float getLensRadius()
        {
            return mLensRadius;
        }

        float getFocalDistance()
        {
            return mFocalDistance;
        }

    private:
        float mAspect;
        float mFovy;
        float mZNear;
        float mZFar;
        vec3 mPosition;
        vec3 mTarget;

        float mLensRadius;
        float mFocalDistance;

        mat4 mInverseView;
        mat4 mInverseProjection;

        std::weak_ptr<class Input> pInput;

        // FIXME: Might extract this into a camera controller

        vec2 mRotation;
        ivec4 mWASD;
        vec2 mLastMousePosition;
        float mSpeed;
        bool mMoving;
        bool mFirstClick;
    };

} // namespace rayce

#endif // RAYCE_CAMERA_HPP
