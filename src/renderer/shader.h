#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <cassert>


static std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) 
        throw std::runtime_error("Failed to open file: " + path.string());

    std::string content;
    content.resize(std::filesystem::file_size(path));

    if (!file.read(content.data(), content.size()))
        throw std::runtime_error("Failed to read file: " + path.string());

    return content;
}

class Shader
{
public:
    Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) : 
        m_vertexPath{vertexPath},
        m_fragmentPath{fragmentPath}
    {
        this->Reload();
    }

    ~Shader() {
        glDeleteProgram(m_ID);
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept
        : m_ID(other.m_ID),
        m_uniformMap(std::move(other.m_uniformMap)),
        m_vertexPath(std::move(other.m_vertexPath)),
        m_fragmentPath(std::move(other.m_fragmentPath))
    {
        other.m_ID = 0;
    }

    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            glDeleteProgram(m_ID);

            m_ID = other.m_ID;
            m_uniformMap = std::move(other.m_uniformMap);
            m_vertexPath = std::move(other.m_vertexPath);
            m_fragmentPath = std::move(other.m_fragmentPath);

            other.m_ID = 0;
        }
        return *this;
    }

    GLuint Compile(GLenum type, std::string_view code) {
        GLuint shader = glCreateShader(type);
        const char* src = code.data();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

            std::string log(logLength, '\0');
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());

            glDeleteShader(shader);

            std::cout << "Shader compilation failed:\n" + log << std::endl;
            return 0;
        }
        return shader;
    }

    GLuint Link(GLuint vertex, GLuint fragment) {
        auto ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        GLint success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLength = 0;
            glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &logLength);

            std::string log(logLength, '\0');
            glGetProgramInfoLog(ID, logLength, nullptr, log.data());
            std::cout << "Shader linking failed:\n" + log << std::endl;

            glDeleteProgram(ID);
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            return 0;
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);

        return ID;
    }

    void Reload() {
        const std::string vertexCode   = ReadTextFile(m_vertexPath);
        const std::string fragmentCode = ReadTextFile(m_fragmentPath);

        auto vertex = Compile(GL_VERTEX_SHADER, vertexCode);
        if (!vertex)
            return;

        auto fragment = Compile(GL_FRAGMENT_SHADER, fragmentCode);
        if (!fragment) {
            glDeleteShader(vertex); // clean up vertex shader if we failed here
            return;
        }

        auto newID = this->Link(vertex, fragment);
        if (newID != 0) {
            if (m_ID != 0) glDeleteProgram(m_ID);
            m_ID = newID;
            m_uniformMap.clear();
        }
        std::cout << "Shader reloaded" << std::endl;
    }

    void Activate() const
    {
        assert(m_ID != 0);
        glUseProgram(m_ID); 
    }

    // Utility uniform functions
    // ------------------------------------------------------------------------
    void SetBool(const std::string &name, bool value) const
    {         
        glUniform1i(this->GetLocation(name), (int)value); 
    }

    void SetInt(const std::string &name, int value) const
    { 
        glUniform1i(this->GetLocation(name), value); 
    }

    void SetFloat(const std::string &name, float value) const
    { 
        glUniform1f(this->GetLocation(name), value); 
    }

    void SetVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(this->GetLocation(name), 1, &value[0]); 
    }

    void SetVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(this->GetLocation(name), x, y); 
    }

    void SetVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(this->GetLocation(name), 1, &value[0]); 
    }

    void SetVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(this->GetLocation(name), x, y, z); 
    }

    void SetVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(this->GetLocation(name), 1, &value[0]); 
    }
    void SetVec4(const std::string &name, float x, float y, float z, float w) const
    { 
        glUniform4f(this->GetLocation(name), x, y, z, w); 
    }

    void SetMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(this->GetLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void SetMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(this->GetLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void SetMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(this->GetLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

private:
    GLuint m_ID{0};
    mutable std::unordered_map<std::string, GLint> m_uniformMap;
    std::filesystem::path m_vertexPath{};
    std::filesystem::path m_fragmentPath{};

    GLint GetLocation(const std::string& name) const
    {
        if (auto it = m_uniformMap.find(name);
            it != m_uniformMap.end())
            return it->second;

        GLint loc = glGetUniformLocation(m_ID, name.c_str());
        m_uniformMap[name] = loc;
        return loc;
    }
};
