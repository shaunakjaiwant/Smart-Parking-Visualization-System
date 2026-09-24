#include "ShadowMap.h"

namespace SmartParking {

ShadowMap::ShadowMap() {
}

ShadowMap::~ShadowMap() {
    cleanup();
}

ShadowMap::ShadowMap(ShadowMap&& other) noexcept
    : m_fbo(other.m_fbo),
      m_depthTexture(other.m_depthTexture),
      m_resolution(other.m_resolution),
      m_lightDirection(other.m_lightDirection),
      m_lightDistance(other.m_lightDistance),
      m_orthoExtentX(other.m_orthoExtentX),
      m_orthoExtentY(other.m_orthoExtentY),
      m_nearPlane(other.m_nearPlane),
      m_farPlane(other.m_farPlane),
      m_shadowBias(other.m_shadowBias),
      m_shadowIntensity(other.m_shadowIntensity),
      m_enabled(other.m_enabled),
      m_showDebugView(other.m_showDebugView) {
    other.m_fbo = 0;
    other.m_depthTexture = 0;
}

ShadowMap& ShadowMap::operator=(ShadowMap&& other) noexcept {
    if (this != &other) {
        cleanup();

        m_fbo = other.m_fbo;
        m_depthTexture = other.m_depthTexture;
        m_resolution = other.m_resolution;
        m_lightDirection = other.m_lightDirection;
        m_lightDistance = other.m_lightDistance;
        m_orthoExtentX = other.m_orthoExtentX;
        m_orthoExtentY = other.m_orthoExtentY;
        m_nearPlane = other.m_nearPlane;
        m_farPlane = other.m_farPlane;
        m_shadowBias = other.m_shadowBias;
        m_shadowIntensity = other.m_shadowIntensity;
        m_enabled = other.m_enabled;
        m_showDebugView = other.m_showDebugView;

        other.m_fbo = 0;
        other.m_depthTexture = 0;
    }
    return *this;
}

void ShadowMap::cleanup() {
    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

bool ShadowMap::init(int resolution) {
    cleanup();
    m_resolution = resolution > 0 ? resolution : 2048;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_resolution, m_resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // Bilinear filtering for smooth PCF
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clamp to border color (1.0 = maximum depth = no shadow outside light frustum)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

    // Depth-only FBO: no color buffer attachments
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[ShadowMap Error] Framebuffer initialization incomplete. Status: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "[ShadowMap] Successfully initialized " << m_resolution << "x" << m_resolution 
              << " depth FBO (ID: " << m_fbo << ")." << std::endl;
    return true;
}

void ShadowMap::setResolution(int res) {
    if (res != m_resolution && (res == 1024 || res == 2048 || res == 4096)) {
        init(res);
    }
}

void ShadowMap::setLightDirection(const glm::vec3& dir) {
    if (glm::length(dir) > 0.001f) {
        m_lightDirection = glm::normalize(dir);
    }
}

void ShadowMap::bindForWriting() {
    glViewport(0, 0, m_resolution, m_resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::unbind(int windowWidth, int windowHeight) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
}

void ShadowMap::bindForReading(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}

glm::mat4 ShadowMap::getLightProjectionMatrix() const {
    return glm::ortho(-m_orthoExtentX, m_orthoExtentX, -m_orthoExtentY, m_orthoExtentY, m_nearPlane, m_farPlane);
}

glm::mat4 ShadowMap::getLightViewMatrix() const {
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    glm::vec3 lightPos = target - m_lightDirection * m_lightDistance;
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    // If light direction is nearly parallel to Y, use alternate up vector
    if (std::abs(glm::dot(m_lightDirection, up)) > 0.95f) {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    return glm::lookAt(lightPos, target, up);
}

glm::mat4 ShadowMap::getLightSpaceMatrix() const {
    return getLightProjectionMatrix() * getLightViewMatrix();
}

} // namespace SmartParking
