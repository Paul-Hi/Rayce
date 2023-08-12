/// @file      camera.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <core/input.hpp>
#include <scene/camera.hpp>

using namespace rayce;

Camera::Camera(float aspect, float fovy, float zNear, float zFar, const vec3& position, const vec3& target, const std::shared_ptr<class Input> input)
    : mAspect(aspect)
    , mFovy(fovy)
    , mZNear(zNear)
    , mZFar(zFar)
    , mPosition(position)
    , mTarget(target)
{
    mInverseView       = lookAt(position, target, vec3(0.0f, 1.0f, 0.0f)).inverse();
    mInverseProjection = perspective(deg_to_rad(fovy), aspect, zNear, zFar).inverse();

    pInput              = input;
    vec3 rotationVector = (mTarget - mPosition).normalized();
    mRotation.y()       = acosf(-rotationVector.y());
    mRotation.x()       = acosf(rotationVector.x() / sinf(mRotation.y()));
    mWASD               = ivec4(0, 0, 0, 0);
    mLastMousePostion   = vec2(0.0, 0.0);
    mMoving             = false;
    mFirstClick         = true;
    input->registerCursorPositionCallback(
        [this](double xPosition, double yPosition)
        {
            if (pInput.expired())
            {
                RAYCE_LOG_WARN("Input not valid. Can not control camera.");
                return;
            }
            bool noRotation       = pInput.lock()->getMouseButton(EMouseButton::mouseButtonLeft) == EInputAction::release;
            vec2 diff             = vec2(xPosition, yPosition) - mLastMousePostion;
            bool offsetIrrelevant = diff.norm() < 1.0f; // In pixels.
            noRotation |= mFirstClick;
            mFirstClick = false;

            mLastMousePostion = vec2(xPosition, yPosition);

            if (noRotation || offsetIrrelevant)
                return;

            if (!noRotation)
            {
                vec2 rot = diff;
                rot.y() *= -1.0f;
                mRotation += rot * 0.005f;
                mRotation.y() = clamp(mRotation.y(), deg_to_rad(15.0f), deg_to_rad(165.0f));
                mRotation.x() = mRotation.x() < 0.0f ? mRotation.x() + deg_to_rad(360.0f) : mRotation.x();
                mRotation.x() = fmodf(mRotation.x(), deg_to_rad(360.0f));
                mMoving       = true;
                return;
            }
        });

    mSpeed = 2.0f;
    input->registerScrollCallback(
        [this](double, double yOffset)
        {
            if (yOffset < 0)
            {
                mSpeed -= 0.5f;
            }
            else
            {
                mSpeed += 0.5f;
            }
            mSpeed = clamp(mSpeed, 0.5f, 20.0f);
        });

    input->registerKeyCallback(
        [this](EKeyCode key, EInputAction action, EModifier mods)
        {
            if (pInput.expired())
            {
                RAYCE_LOG_WARN("Input not valid. Can not control camera.");
                return;
            }
            bool noKeyInput = pInput.lock()->getMouseButton(EMouseButton::mouseButtonLeft) == EInputAction::release;
            if (noKeyInput)
            {
                mWASD = ivec4(0, 0, 0, 0);
                return;
            }

            bool shiftPressed = (mods & EModifier::modifierShift) != EModifier::none;
            mMoving           = true;

            if (key == EKeyCode::keyW)
                mWASD.x() = +(int32)(action != EInputAction::release) * (shiftPressed ? 2 : 1);
            if (key == EKeyCode::keyA)
                mWASD.y() = +(int32)(action != EInputAction::release) * (shiftPressed ? 2 : 1);
            if (key == EKeyCode::keyS)
                mWASD.z() = -(int32)(action != EInputAction::release) * (shiftPressed ? 2 : 1);
            if (key == EKeyCode::keyD)
                mWASD.w() = -(int32)(action != EInputAction::release) * (shiftPressed ? 2 : 1);
            if (key == EKeyCode::keyLeftShift || key == EKeyCode::keyRightShift)
            {
                if (action == EInputAction::press)
                    mWASD *= 2;
                if (action == EInputAction::release)
                    mWASD /= 2;
                mWASD.x() = clamp(mWASD.x(), -2, 2);
                mWASD.y() = clamp(mWASD.y(), -2, 2);
                mWASD.z() = clamp(mWASD.z(), -2, 2);
                mWASD.w() = clamp(mWASD.w(), -2, 2);
            }
        });
}

Camera::~Camera()
{
}

void Camera::updateAspect(float aspect)
{
    mAspect            = aspect;
    mInverseView       = lookAt(mPosition, mTarget, vec3(0.0f, 1.0f, 0.0f)).inverse();
    mInverseProjection = perspective(deg_to_rad(mFovy), mAspect, mZNear, mZFar).inverse();
}

bool Camera::update(float dt)
{
    if (!mMoving && mWASD.norm() == 0)
    {
        return false;
    }

    vec3 rotationVector;
    rotationVector.x() = sinf(mRotation.y()) * cosf(mRotation.x());
    rotationVector.y() = -cosf(mRotation.y());
    rotationVector.z() = sinf(mRotation.y()) * sinf(mRotation.x());

    vec3 front = rotationVector.normalized();
    auto right = (vec3(0.0f, 1.0f, 0.0f).cross(front)).normalized();
    mPosition += right * (mWASD.y() + mWASD.w()) * mSpeed * dt;
    mPosition += front * (mWASD.x() + mWASD.z()) * mSpeed * dt;
    mTarget = mPosition + rotationVector;

    mInverseView       = lookAt(mPosition, mTarget, vec3(0.0f, 1.0f, 0.0f)).inverse();
    mInverseProjection = perspective(deg_to_rad(mFovy), mAspect, mZNear, mZFar).inverse();

    mMoving = false;
    return true;
}
