#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace SmartParking {

Shader::Shader()
    : m_programId(0) {
}

Shader::~Shader() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
}

Shader::Shader(Shader&& other) noexcept
    : m_programId(other.m_programId),
      m_uniformLocationCache(std::move(other.m_uniformLocationCache)) {
    other.m_programId = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_programId != 0) {
            glDeleteProgram(m_programId);
        }
        m_programId = other.m_programId;
        m_uniformLocationCache = std::move(other.m_uniformLocationCache);
        other.m_programId = 0;
    }
    return *this;
}

std::string Shader::readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "[Shader Error] Failed to open shader file: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vCode = readFile(vertexPath);
    std::string fCode = readFile(fragmentPath);

    if (vCode.empty() || fCode.empty()) {
        return false;
    }

    return loadFromSource(vCode, fCode);
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
        m_uniformLocationCache.clear();
    }

    const char* vShaderCode = vertexSource.c_str();
    const char* fShaderCode = fragmentSource.c_str();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    if (!checkCompileErrors(vertex, "VERTEX")) {
        glDeleteShader(vertex);
        return false;
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    if (!checkCompileErrors(fragment, "FRAGMENT")) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertex);
    glAttachShader(m_programId, fragment);
    glLinkProgram(m_programId);

    bool linkOk = checkCompileErrors(m_programId, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return linkOk;
}

void Shader::use() const {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void Shader::unuse() const {
    glUseProgram(0);
}

GLint Shader::getUniformLocation(const std::string& name) {
    auto it = m_uniformLocationCache.find(name);
    if (it != m_uniformLocationCache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_programId, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

void Shader::setBool(const std::string& name, bool value) {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

bool Shader::checkCompileErrors(GLuint shader, const std::string& type) {
    GLint success;
    GLchar infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "[Shader Error] Compilation failure for type: " << type << "\n"
                      << infoLog << "\n----------------------------------------" << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "[Shader Error] Program linking failure:\n"
                      << infoLog << "\n----------------------------------------" << std::endl;
            return false;
        }
    }
    return true;
}

} // namespace SmartParking
