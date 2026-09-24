#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

/**
 * @file ShadowMap.h
 * @brief Dedicated Depth Framebuffer and Texture encapsulation for real-time shadow mapping.
 * 
 * WHAT: Generates and binds 2D depth textures capturing the scene from the directional light's perspective.
 * WHY:  Fulfills core Computer Graphics requirement for real-time shadows, depth testing, and light-space transforms.
 * HOW:  Creates an FBO with a GL_DEPTH_COMPONENT24 texture, sets up orthographic light projection,
 *       and provides the Light-Space Matrix (LightProj * LightView) for two-pass shadow mapping.
 */
namespace SmartParking {

class ShadowMap {
public:
    ShadowMap();
    ~ShadowMap();

    // Prevent copying
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Move semantics
    ShadowMap(ShadowMap&& other) noexcept;
    ShadowMap& operator=(ShadowMap&& other) noexcept;

    bool init(int resolution = 2048);
    void cleanup();

    void bindForWriting();
    void unbind(int windowWidth, int windowHeight);
    void bindForReading(GLenum textureUnit = GL_TEXTURE1);

    // Light-Space Matrix calculation
    glm::mat4 getLightSpaceMatrix() const;
    glm::mat4 getLightProjectionMatrix() const;
    glm::mat4 getLightViewMatrix() const;

    // Getters and Setters
    GLuint getFBO() const { return m_fbo; }
    GLuint getDepthTexture() const { return m_depthTexture; }
    int getResolution() const { return m_resolution; }
    void setResolution(int res);

    const glm::vec3& getLightDirection() const { return m_lightDirection; }
    void setLightDirection(const glm::vec3& dir);

    float getBias() const { return m_shadowBias; }
    void setBias(float bias) { m_shadowBias = bias; }

    float getIntensity() const { return m_shadowIntensity; }
    void setIntensity(float intensity) { m_shadowIntensity = intensity; }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void toggleEnabled() { m_enabled = !m_enabled; }

    bool isDebugViewEnabled() const { return m_showDebugView; }
    void setDebugViewEnabled(bool show) { m_showDebugView = show; }
    void toggleDebugView() { m_showDebugView = !m_showDebugView; }

private:
    GLuint m_fbo = 0;
    GLuint m_depthTexture = 0;
    int m_resolution = 2048;

    // Sun directional light parameters
    glm::vec3 m_lightDirection = glm::normalize(glm::vec3(-0.45f, -0.85f, -0.30f));
    float m_lightDistance = 90.0f;
    float m_orthoExtentX = 85.0f; // Half-width covers entire parking lot
    float m_orthoExtentY = 75.0f; // Half-height covers facility depth
    float m_nearPlane = 1.0f;
    float m_farPlane = 220.0f;

    float m_shadowBias = 0.0018f;
    float m_shadowIntensity = 0.65f;
    bool m_enabled = true;
    bool m_showDebugView = false;
};

} // namespace SmartParking
