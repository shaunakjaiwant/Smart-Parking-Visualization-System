#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

/**
 * @file Mesh.h
 * @brief OpenGL Vertex Array and Buffer Object abstraction.
 * 
 * WHAT: Encapsulates vertex data, normals, texture coordinates, colors, and index buffers.
 * WHY:  Enables efficient hardware-accelerated batch rendering on the GPU (VBO/VAO/EBO).
 * HOW:  Uploads geometry to GPU VRAM using glBufferData, defines vertex attributes,
 *       and provides static factory methods for procedural 3D/2D primitives.
 */
namespace SmartParking {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec4 color;

    Vertex()
        : position(0.0f), normal(0.0f, 1.0f, 0.0f), texCoords(0.0f), color(1.0f) {}

    Vertex(glm::vec3 pos, glm::vec3 norm = glm::vec3(0.0f, 1.0f, 0.0f),
           glm::vec2 uv = glm::vec2(0.0f), glm::vec4 col = glm::vec4(1.0f))
        : position(pos), normal(norm), texCoords(uv), color(col) {}
};

class Mesh {
public:
    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GLenum drawMode = GL_TRIANGLES);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, GLenum drawMode = GL_TRIANGLES);
    void draw() const;
    void drawLines() const;

    GLuint getVAO() const { return m_vao; }
    size_t getIndexCount() const { return m_indexCount; }

    // Procedural primitive factories
    static std::unique_ptr<Mesh> createCube(glm::vec3 size = glm::vec3(1.0f), glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> createPlane(float width = 1.0f, float length = 1.0f, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> createCylinder(float radius = 0.5f, float height = 1.0f, int segments = 16, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> createSphere(float radius = 0.5f, int rings = 12, int sectors = 16, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> create2DQuad(float width = 1.0f, float height = 1.0f, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> create2DCircle(float radius = 1.0f, int segments = 24, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> createCone(float radius = 0.5f, float height = 1.0f, int segments = 16, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> createDirectionArrow(float length = 2.0f, float width = 1.2f, glm::vec4 color = glm::vec4(1.0f));
    static std::unique_ptr<Mesh> createPathRibbon(const std::vector<glm::vec3>& pathPoints, float ribbonWidth = 0.8f, glm::vec4 color = glm::vec4(0.0f, 0.8f, 1.0f, 0.9f));

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    size_t m_indexCount = 0;
    GLenum m_drawMode = GL_TRIANGLES;

    void cleanup();
};

} // namespace SmartParking
