/// @file      camera.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <scene/camera.hpp>

using namespace rayce;

Camera::Camera(float aspect, float fovy, float zNear, float zFar, const vec3& position, const vec3& target)
    : mAspect(aspect)
    , mFovy(fovy)
    , mZNear(zNear)
    , mZFar(zFar)
    , mPosition(position)
    , mTarget(target)
{
    mInverseView       = lookAt(position, target, vec3(0.0f, 1.0f, 0.0f)).inverse();
    mInverseProjection = perspective(deg_to_rad(fovy), aspect, zNear, zFar).inverse();
    RAYCE_LOG_INFO("Created camera!");
}

Camera::~Camera()
{
}
