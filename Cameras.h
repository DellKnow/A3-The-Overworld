// Cameras.h
#ifndef A3_CAMERAS_H
#define A3_CAMERAS_H

#include <CSCI441/PerspectiveCamera.hpp>

/// \desc camera that orbits a target point at a given radius; theta/phi come from mouse drags
class ArcballCam final : public CSCI441::PerspectiveCamera {
public:
    explicit ArcballCam(const GLfloat aspectRatio = 1.0f) : PerspectiveCamera(aspectRatio) {
        mCameraTheta  = 0.8f;
        mCameraPhi    = 2.0f;           // > pi/2 puts the camera above the target
        mCameraRadius = 15.0f;
        mCameraUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
        mCameraLookAtPoint = glm::vec3(0.0f);
        recomputeOrientation();
    }

    /// \desc follow a new target (the hero in T4) while keeping the same angles and distance
    void setTarget(const glm::vec3& target) {
        setLookAtPoint(target);
        recomputeOrientation();
    }

    /// \desc orbit around the target
    void rotate(const GLfloat dTheta, const GLfloat dPhi) override {
        mCameraTheta += dTheta;
        mCameraPhi = glm::clamp(mCameraPhi + dPhi, MIN_PHI, MAX_PHI);
        recomputeOrientation();
    }

    /// \desc spherical (theta, phi, radius) around the look-at point -> camera position
    void recomputeOrientation() override {
        const glm::vec3 offset(  mCameraRadius * glm::sin(mCameraTheta) * glm::sin(mCameraPhi),
                                -mCameraRadius * glm::cos(mCameraPhi),
                                -mCameraRadius * glm::cos(mCameraTheta) * glm::sin(mCameraPhi) );
        mCameraPosition = mCameraLookAtPoint + offset;
        computeViewMatrix();
    }

    /// \desc zoom in; radius is clamped so the camera never passes through or loses the target
    void moveForward(const GLfloat amount) override {
        mCameraRadius = glm::clamp(mCameraRadius - amount, MIN_RADIUS, MAX_RADIUS);
        recomputeOrientation();
    }
    void moveBackward(const GLfloat amount) override { moveForward(-amount); }

private:
    static constexpr GLfloat MIN_RADIUS = 3.0f;
    static constexpr GLfloat MAX_RADIUS = 40.0f;

    // phi = pi/2 is level with the target, phi = pi is straight overhead
    static constexpr GLfloat MIN_PHI = 1.65f;   // ~5 deg above the horizon
    static constexpr GLfloat MAX_PHI = 3.09f;   // ~3 deg short of straight overhead
};

#endif // A3_CAMERAS_H
