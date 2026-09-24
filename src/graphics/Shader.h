#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

/**
 * @file Shader.h
 * @brief OpenGL Shader Program Manager.
 * 
 * WHAT: Loads, compiles, links, and manages GLSL vertex and fragment shaders.
 * WHY:  Essential for OpenGL modern core profile pipeline (programmable pipeline).
 * HOW:  Reads shader code, compiles with glCompileShader, checks GL_COMPILE_STATUS,
 *       links with glLinkProgram, and caches uniform locations for fast updates.
 */
namespace SmartParking {

class Shader {
public:
    Shader();
    ~Shader();

    // Prevent unintended shallow copies
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Allow move semantics
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

    void use() const;
    void unuse() const;
    GLuint getProgramId() const { return m_programId; }
    bool isValid() const { return m_programId != 0; }

    // Uniform setters
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat4(const std::string& name, const glm::mat4& mat);

private:
    GLuint m_programId;
    std::unordered_map<std::string, GLint> m_uniformLocationCache;

    GLint getUniformLocation(const std::string& name);
    bool checkCompileErrors(GLuint shader, const std::string& type);
    std::string readFile(const std::string& filePath);
};

} // namespace SmartParking
