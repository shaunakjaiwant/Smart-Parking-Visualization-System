#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @file Camera.h
 * @brief Dual-mode (2D Orthographic / 3D Perspective) interactive camera.
 * 
 * WHAT: Manages view and projection matrices, user navigation (WASDQE + Mouse),
 *       and smooth transitions between 2D top-down view and 3D perspective view.
 * WHY:  Fulfills the core CG requirement for viewing pipeline transformations:
 *       World Space -> View (Eye) Space -> Clip Space -> Screen Coordinates.
 * HOW:  Maintains position, yaw, pitch, FOV, and ortho zoom. Uses glm::lookAt
 *       for view matrix, glm::perspective for 3D, and glm::ortho for 2D.
 */
namespace SmartParking {

enum class CameraMode {
    MODE_2D_TOPDOWN,
    MODE_3D_PERSPECTIVE
};

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 35.0f, 45.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = -40.0f);

    void setMode(CameraMode mode);
    CameraMode getMode() const { return m_mode; }
    void toggleMode();

    // View and Projection transformations
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Movement & Interaction
    void processKeyboard(char direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    void reset();
    void setPresetView(int preset); // Presets 1..6 per Section 21

    // Setters / Getters
    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& pos) { m_position = pos; updateCameraVectors(); }
    glm::vec3 getFront() const { return m_front; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    float getZoom() const { return m_zoom; }
    float getOrthoScale() const { return m_orthoScale; }
    glm::vec2 getOrthoPan() const { return m_orthoPan; }
    void pan2D(float dx, float dy);

    // Camera settings
    float movementSpeed = 25.0f;
    float mouseSensitivity = 0.12f;

private:
    CameraMode m_mode = CameraMode::MODE_3D_PERSPECTIVE;

    // 3D parameters
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;
    float m_zoom = 45.0f; // FOV in degrees

    // 2D orthographic parameters
    glm::vec2 m_orthoPan = glm::vec2(0.0f, 0.0f);
    float m_orthoScale = 45.0f; // half-height extent

    // Default reset states
    glm::vec3 m_default3DPosition = glm::vec3(0.0f, 40.0f, 50.0f);
    float m_default3DYaw = -90.0f;
    float m_default3DPitch = -42.0f;

    void updateCameraVectors();
};

} // namespace SmartParking
