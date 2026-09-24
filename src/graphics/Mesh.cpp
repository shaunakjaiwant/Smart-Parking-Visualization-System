#include "Mesh.h"
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace SmartParking {

Mesh::Mesh() {
}

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GLenum drawMode) {
    upload(vertices, indices, drawMode);
}

Mesh::~Mesh() {
    cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao),
      m_vbo(other.m_vbo),
      m_ebo(other.m_ebo),
      m_indexCount(other.m_indexCount),
      m_drawMode(other.m_drawMode) {
    other.m_vao = 0;
    other.m_vbo = 0;
    other.m_ebo = 0;
    other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanup();
        m_vao = other.m_vao;
        m_vbo = other.m_vbo;
        m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        m_drawMode = other.m_drawMode;

        other.m_vao = 0;
        other.m_vbo = 0;
        other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Mesh::cleanup() {
    if (m_ebo != 0) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
    if (m_vbo != 0) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao != 0) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    m_indexCount = 0;
}

void Mesh::upload(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GLenum drawMode) {
    cleanup();

    m_drawMode = drawMode;
    m_indexCount = indices.size();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Attribute 0: Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Attribute 1: Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Attribute 2: TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    // Attribute 3: Color
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (m_vao != 0 && m_indexCount > 0) {
        glBindVertexArray(m_vao);
        glDrawElements(m_drawMode, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void Mesh::drawLines() const {
    if (m_vao != 0 && m_indexCount > 0) {
        glBindVertexArray(m_vao);
        glDrawElements(GL_LINES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

std::unique_ptr<Mesh> Mesh::createCube(glm::vec3 size, glm::vec4 color) {
    float hx = size.x * 0.5f;
    float hy = size.y * 0.5f;
    float hz = size.z * 0.5f;

    std::vector<Vertex> vertices = {
        // Front (+Z)
        { {-hx, -hy,  hz}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, color },
        { { hx, -hy,  hz}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, color },
        { { hx,  hy,  hz}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, color },
        { {-hx,  hy,  hz}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, color },
        // Back (-Z)
        { { hx, -hy, -hz}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, color },
        { {-hx, -hy, -hz}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, color },
        { {-hx,  hy, -hz}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, color },
        { { hx,  hy, -hz}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, color },
        // Top (+Y)
        { {-hx,  hy,  hz}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, color },
        { { hx,  hy,  hz}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, color },
        { { hx,  hy, -hz}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, color },
        { {-hx,  hy, -hz}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, color },
        // Bottom (-Y)
        { {-hx, -hy, -hz}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, color },
        { { hx, -hy, -hz}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, color },
        { { hx, -hy,  hz}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, color },
        { {-hx, -hy,  hz}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, color },
        // Left (-X)
        { {-hx, -hy, -hz}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, color },
        { {-hx, -hy,  hz}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, color },
        { {-hx,  hy,  hz}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, color },
        { {-hx,  hy, -hz}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, color },
        // Right (+X)
        { { hx, -hy,  hz}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, color },
        { { hx, -hy, -hz}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, color },
        { { hx,  hy, -hz}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, color },
        { { hx,  hy,  hz}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, color }
    };

    std::vector<unsigned int> indices;
    indices.reserve(36);
    for (unsigned int i = 0; i < 6; ++i) {
        unsigned int offset = i * 4;
        indices.push_back(offset + 0);
        indices.push_back(offset + 1);
        indices.push_back(offset + 2);
        indices.push_back(offset + 2);
        indices.push_back(offset + 3);
        indices.push_back(offset + 0);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createPlane(float width, float length, glm::vec4 color) {
    float hw = width * 0.5f;
    float hl = length * 0.5f;

    std::vector<Vertex> vertices = {
        { {-hw, 0.0f, -hl}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, color },
        { { hw, 0.0f, -hl}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, color },
        { { hw, 0.0f,  hl}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, color },
        { {-hw, 0.0f,  hl}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, color }
    };

    std::vector<unsigned int> indices = { 0, 2, 1, 0, 3, 2 };
    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createCylinder(float radius, float height, int segments, glm::vec4 color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float hh = height * 0.5f;
    float step = 2.0f * glm::pi<float>() / static_cast<float>(segments);

    // Center vertices for top and bottom caps
    unsigned int topCenterIdx = 0;
    vertices.push_back({ {0.0f, hh, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}, color });
    unsigned int botCenterIdx = 1;
    vertices.push_back({ {0.0f, -hh, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}, color });

    // Side vertices
    unsigned int sideStartIdx = 2;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * step;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        glm::vec3 norm = glm::normalize(glm::vec3(x, 0.0f, z));
        float u = static_cast<float>(i) / segments;

        // Top ring
        vertices.push_back({ {x, hh, z}, norm, {u, 1.0f}, color });
        // Bottom ring
        vertices.push_back({ {x, -hh, z}, norm, {u, 0.0f}, color });
    }

    // Indices for sides
    for (int i = 0; i < segments; ++i) {
        unsigned int currentTop = sideStartIdx + i * 2;
        unsigned int currentBot = currentTop + 1;
        unsigned int nextTop = currentTop + 2;
        unsigned int nextBot = currentTop + 3;

        indices.push_back(currentTop);
        indices.push_back(currentBot);
        indices.push_back(nextTop);

        indices.push_back(nextTop);
        indices.push_back(currentBot);
        indices.push_back(nextBot);

        // Cap triangles
        indices.push_back(topCenterIdx);
        indices.push_back(currentTop);
        indices.push_back(nextTop);

        indices.push_back(botCenterIdx);
        indices.push_back(nextBot);
        indices.push_back(currentBot);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createSphere(float radius, int rings, int sectors, glm::vec4 color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            float y = sin(-glm::half_pi<float>() + glm::pi<float>() * r * R);
            float x = cos(2.0f * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);
            float z = sin(2.0f * glm::pi<float>() * s * S) * sin(glm::pi<float>() * r * R);

            glm::vec3 pos = glm::vec3(x, y, z) * radius;
            glm::vec3 norm = glm::normalize(pos);
            vertices.push_back({ pos, norm, {s * S, r * R}, color });
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            unsigned int cur = r * sectors + s;
            unsigned int next = cur + sectors;

            indices.push_back(cur);
            indices.push_back(next);
            indices.push_back(next + 1);

            indices.push_back(cur);
            indices.push_back(next + 1);
            indices.push_back(cur + 1);
        }
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::create2DQuad(float width, float height, glm::vec4 color) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;

    std::vector<Vertex> vertices = {
        { {-hw, 0.0f, -hh}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, color },
        { { hw, 0.0f, -hh}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, color },
        { { hw, 0.0f,  hh}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, color },
        { {-hw, 0.0f,  hh}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, color }
    };

    std::vector<unsigned int> indices = { 0, 1, 2, 2, 3, 0 };
    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::create2DCircle(float radius, int segments, glm::vec4 color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    vertices.push_back({ {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f}, color });
    float step = 2.0f * glm::pi<float>() / segments;

    for (int i = 0; i <= segments; ++i) {
        float angle = i * step;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back({ {x, 0.0f, z}, {0.0f, 1.0f, 0.0f}, {0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)}, color });
    }

    for (int i = 1; i <= segments; ++i) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createPathRibbon(const std::vector<glm::vec3>& pathPoints, float ribbonWidth, glm::vec4 color) {
    if (pathPoints.size() < 2) {
        return nullptr;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float halfW = ribbonWidth * 0.5f;

    for (size_t i = 0; i < pathPoints.size(); ++i) {
        glm::vec3 dir(0.0f);
        if (i + 1 < pathPoints.size()) {
            dir += glm::normalize(pathPoints[i + 1] - pathPoints[i]);
        }
        if (i > 0) {
            dir += glm::normalize(pathPoints[i] - pathPoints[i - 1]);
        }
        dir = glm::normalize(dir);

        glm::vec3 right = glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::length(right) < 0.001f) {
            right = glm::vec3(1.0f, 0.0f, 0.0f);
        } else {
            right = glm::normalize(right);
        }

        glm::vec3 pLeft = pathPoints[i] - right * halfW;
        glm::vec3 pRight = pathPoints[i] + right * halfW;

        // Slight elevation above ground to prevent Z-fighting
        pLeft.y += 0.05f;
        pRight.y += 0.05f;

        vertices.push_back({ pLeft, {0.0f, 1.0f, 0.0f}, {0.0f, static_cast<float>(i)}, color });
        vertices.push_back({ pRight, {0.0f, 1.0f, 0.0f}, {1.0f, static_cast<float>(i)}, color });
    }

    for (size_t i = 0; i < pathPoints.size() - 1; ++i) {
        unsigned int idx0 = static_cast<unsigned int>(i * 2);
        unsigned int idx1 = idx0 + 1;
        unsigned int idx2 = idx0 + 2;
        unsigned int idx3 = idx0 + 3;

        indices.push_back(idx0);
        indices.push_back(idx1);
        indices.push_back(idx2);

        indices.push_back(idx1);
        indices.push_back(idx3);
        indices.push_back(idx2);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createCone(float radius, float height, int segments, glm::vec4 color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float hh = height * 0.5f;
    float step = 2.0f * glm::pi<float>() / segments;

    // Apex vertex (index 0)
    vertices.push_back({ {0.0f, hh, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}, color });

    // Base center vertex (index 1)
    vertices.push_back({ {0.0f, -hh, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f}, color });

    // Base perimeter vertices (indices 2 to segments + 2)
    float slope = radius / height;
    for (int i = 0; i <= segments; ++i) {
        float angle = i * step;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        glm::vec3 sideNorm = glm::normalize(glm::vec3(cos(angle), slope, sin(angle)));
        vertices.push_back({ {x, -hh, z}, sideNorm, {static_cast<float>(i) / segments, 0.0f}, color });
    }

    // Side triangles & base cap
    for (int i = 0; i < segments; ++i) {
        unsigned int cur = 2 + i;
        unsigned int next = 2 + i + 1;

        // Side cone triangle
        indices.push_back(0); // Apex
        indices.push_back(cur);
        indices.push_back(next);

        // Base disk triangle
        indices.push_back(1); // Base center
        indices.push_back(next);
        indices.push_back(cur);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> Mesh::createDirectionArrow(float length, float width, glm::vec4 color) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfL = length * 0.5f;
    float halfW = width * 0.5f;
    float shaftHalfW = halfW * 0.35f;
    float splitZ = halfL * 0.05f;

    // Shaft quad (4 vertices)
    vertices.push_back({ {-shaftHalfW, 0.0f, -halfL}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, color });
    vertices.push_back({ { shaftHalfW, 0.0f, -halfL}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, color });
    vertices.push_back({ { shaftHalfW, 0.0f,  splitZ}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, color });
    vertices.push_back({ {-shaftHalfW, 0.0f,  splitZ}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, color });

    indices.push_back(0); indices.push_back(2); indices.push_back(1);
    indices.push_back(0); indices.push_back(3); indices.push_back(2);

    // Head triangle (3 vertices)
    unsigned int headStart = 4;
    vertices.push_back({ {-halfW, 0.0f, splitZ}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, color });
    vertices.push_back({ { halfW, 0.0f, splitZ}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, color });
    vertices.push_back({ { 0.0f,  0.0f,  halfL}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}, color });

    indices.push_back(headStart + 0);
    indices.push_back(headStart + 2);
    indices.push_back(headStart + 1);

    return std::make_unique<Mesh>(vertices, indices);
}

} // namespace SmartParking
