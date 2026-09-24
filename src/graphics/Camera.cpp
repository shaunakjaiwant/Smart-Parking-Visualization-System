#include "Camera.h"
#include <algorithm>

namespace SmartParking {

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position),
      m_worldUp(up),
      m_yaw(yaw),
      m_pitch(pitch),
      m_front(glm::vec3(0.0f, -0.6f, -0.8f)) {
    m_default3DPosition = position;
    m_default3DYaw = yaw;
    m_default3DPitch = pitch;
    updateCameraVectors();
}

void Camera::setMode(CameraMode mode) {
    m_mode = mode;
}

void Camera::toggleMode() {
    m_mode = (m_mode == CameraMode::MODE_3D_PERSPECTIVE) ?
             CameraMode::MODE_2D_TOPDOWN : CameraMode::MODE_3D_PERSPECTIVE;
}

glm::mat4 Camera::getViewMatrix() const {
    if (m_mode == CameraMode::MODE_2D_TOPDOWN) {
        // Looking straight down onto the XZ plane from high above Y axis
        glm::vec3 eye(m_orthoPan.x, 100.0f, m_orthoPan.y);
        glm::vec3 center(m_orthoPan.x, 0.0f, m_orthoPan.y);
        glm::vec3 up(0.0f, 0.0f, -1.0f); // -Z is "up" on the screen in 2D top-down
        return glm::lookAt(eye, center, up);
    } else {
        return glm::lookAt(m_position, m_position + m_front, m_up);
    }
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    if (m_mode == CameraMode::MODE_2D_TOPDOWN) {
        float xHalf = m_orthoScale * aspectRatio;
        float yHalf = m_orthoScale;
        return glm::ortho(-xHalf, xHalf, -yHalf, yHalf, 0.1f, 300.0f);
    } else {
        return glm::perspective(glm::radians(m_zoom), aspectRatio, 0.1f, 500.0f);
    }
}

void Camera::processKeyboard(char direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;

    if (m_mode == CameraMode::MODE_3D_PERSPECTIVE) {
        if (direction == 'W') m_position += m_front * velocity;
        if (direction == 'S') m_position -= m_front * velocity;
        if (direction == 'A') m_position -= m_right * velocity;
        if (direction == 'D') m_position += m_right * velocity;
        if (direction == 'Q') m_position -= m_worldUp * velocity;
        if (direction == 'E') m_position += m_worldUp * velocity;
    } else {
        // 2D Pan
        if (direction == 'W') m_orthoPan.y -= velocity;
        if (direction == 'S') m_orthoPan.y += velocity;
        if (direction == 'A') m_orthoPan.x -= velocity;
        if (direction == 'D') m_orthoPan.x += velocity;
    }
}

void Camera::pan2D(float dx, float dy) {
    m_orthoPan.x += dx;
    m_orthoPan.y += dy;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    if (m_mode != CameraMode::MODE_3D_PERSPECTIVE) {
        return; // In 2D, mouse dragging is used for panning or selecting
    }

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    if (constrainPitch) {
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    if (m_mode == CameraMode::MODE_3D_PERSPECTIVE) {
        m_zoom -= yoffset * 2.0f;
        m_zoom = std::clamp(m_zoom, 10.0f, 80.0f);
    } else {
        m_orthoScale -= yoffset * 3.0f;
        m_orthoScale = std::clamp(m_orthoScale, 10.0f, 150.0f);
    }
}

void Camera::reset() {
    m_position = m_default3DPosition;
    m_yaw = m_default3DYaw;
    m_pitch = m_default3DPitch;
    m_zoom = 45.0f;
    m_orthoPan = glm::vec2(0.0f, 0.0f);
    m_orthoScale = 45.0f;
    updateCameraVectors();
}

void Camera::setPresetView(int preset) {
    switch (preset) {
        case 1: // 2D Top-Down Mode
            setMode(CameraMode::MODE_2D_TOPDOWN);
            m_orthoPan = glm::vec2(0.0f, 0.0f);
            m_orthoScale = 48.0f;
            break;
        case 2: // 3D Perspective Default View
            setMode(CameraMode::MODE_3D_PERSPECTIVE);
            reset();
            break;
        case 3: // Full Building / Elevated Overview
            setMode(CameraMode::MODE_3D_PERSPECTIVE);
            m_position = glm::vec3(0.0f, 52.0f, 56.0f);
            m_yaw = -90.0f;
            m_pitch = -44.0f;
            m_zoom = 45.0f;
            updateCameraVectors();
            break;
        case 4: // Ground Floor / South Rows A & B
            setMode(CameraMode::MODE_3D_PERSPECTIVE);
            m_position = glm::vec3(-12.0f, 22.0f, 28.0f);
            m_yaw = -75.0f;
            m_pitch = -32.0f;
            m_zoom = 45.0f;
            updateCameraVectors();
            break;
        case 5: // Level 1 / North Rows C & D
            setMode(CameraMode::MODE_3D_PERSPECTIVE);
            m_position = glm::vec3(12.0f, 22.0f, 10.0f);
            m_yaw = -105.0f;
            m_pitch = -32.0f;
            m_zoom = 45.0f;
            updateCameraVectors();
            break;
        case 6: // Entrance Gate & Main Aisle View
            setMode(CameraMode::MODE_3D_PERSPECTIVE);
            m_position = glm::vec3(-46.0f, 12.0f, 22.0f);
            m_yaw = -30.0f;
            m_pitch = -16.0f;
            m_zoom = 45.0f;
            updateCameraVectors();
            break;
        default:
            break;
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace SmartParking
