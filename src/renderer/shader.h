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



class Shader
{
public:
    explicit Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath, const std::filesystem::path& geometryPath = "") : 
        _vertexPath{vertexPath},
        _fragmentPath{fragmentPath},
        _geometryPath{geometryPath}
    {
        this->Reload();
    }

    ~Shader() {
        glDeleteProgram(_ID);
    }

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept
        : _ID(other._ID),
        _uniformMap(std::move(other._uniformMap)),
        _vertexPath(std::move(other._vertexPath)),
        _fragmentPath(std::move(other._fragmentPath)),
        _geometryPath(std::move(other._geometryPath))
    {
        other._ID = 0;
    }

    Shader& operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            glDeleteProgram(_ID);

            _ID = other._ID;
            _uniformMap = std::move(other._uniformMap);
            _vertexPath = std::move(other._vertexPath);
            _fragmentPath = std::move(other._fragmentPath);
            _geometryPath = std::move(other._geometryPath);

            other._ID = 0;
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

    GLuint Link(GLuint vertex, GLuint fragment, GLuint geometry) {
        auto ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometry) glAttachShader(ID, geometry);

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
            if (geometry) glDeleteShader(geometry);
            return 0;
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometry) glDeleteShader(geometry);

        return ID;
    }

    void Reload() {
        const std::string vertexCode   = ReadFile(_vertexPath);
        const std::string fragmentCode = ReadFile(_fragmentPath);
        const std::string geometryCode = !_geometryPath.empty() ? ReadFile(_geometryPath) : "";
        Info("Reloading {} {} {}", _vertexPath.c_str(), _fragmentPath.c_str(), _geometryPath.c_str());

        auto vertex = Compile(GL_VERTEX_SHADER, vertexCode);
        if (!vertex)
            return;

        auto fragment = Compile(GL_FRAGMENT_SHADER, fragmentCode);
        if (!fragment) {
            glDeleteShader(vertex); // clean up vertex shader if we failed here
            return;
        }

        GLuint geometry = 0;
        if (!geometryCode.empty()) {
            geometry = Compile(GL_GEOMETRY_SHADER, geometryCode);
            if (!geometry) {
                glDeleteShader(fragment); // clean up previous shaders
                glDeleteShader(vertex);
                return;
            }
        }

        auto newID = this->Link(vertex, fragment, geometry);
        if (newID != 0) {
            if (_ID != 0) glDeleteProgram(_ID);
            _ID = newID;
            _uniformMap.clear();
        }
        Info("Shader reloaded");
    }

    void Activate() const
    {
        assert(_ID != 0);
        glUseProgram(_ID); 
    }

    // Uniform setters
    void SetBool(const std::string &name, bool value) const {         
        glUniform1i(this->GetLocation(name), (int)value); 
    }

    void SetInt(const std::string &name, int value) const { 
        glUniform1i(this->GetLocation(name), value); 
    }

    void SetFloat(const std::string &name, float value) const { 
        glUniform1f(this->GetLocation(name), value); 
    }

    void SetVec2(const std::string &name, const glm::vec2 &value) const { 
        glUniform2fv(this->GetLocation(name), 1, &value[0]); 
    }

    void SetVec2(const std::string &name, float x, float y) const { 
        glUniform2f(this->GetLocation(name), x, y); 
    }

    void SetVec3(const std::string &name, const glm::vec3 &value) const { 
        glUniform3fv(this->GetLocation(name), 1, &value[0]); 
    }

    void SetVec3(const std::string &name, float x, float y, float z) const { 
        glUniform3f(this->GetLocation(name), x, y, z); 
    }

    void SetVec4(const std::string &name, const glm::vec4 &value) const { 
        glUniform4fv(this->GetLocation(name), 1, &value[0]); 
    }

    void SetVec4(const std::string &name, float x, float y, float z, float w) const { 
        glUniform4f(this->GetLocation(name), x, y, z, w); 
    }

    void SetMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(this->GetLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void SetMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(this->GetLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

    void SetMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(this->GetLocation(name), 1, GL_FALSE, &mat[0][0]);
    }

private:
    GLuint _ID{0};
    mutable std::unordered_map<std::string, GLint> _uniformMap;
    std::filesystem::path _vertexPath{};
    std::filesystem::path _fragmentPath{};
    std::filesystem::path _geometryPath{};

    GLint GetLocation(const std::string& name) const
    {
        if (auto it = _uniformMap.find(name);
            it != _uniformMap.end())
            return it->second;

        GLint loc = glGetUniformLocation(_ID, name.c_str());
        _uniformMap[name] = loc;
        return loc;
    }

    // Reads shader files recursively and appends content 
    static std::string ReadFile(std::filesystem::path path, int depth = 0) {
        if (depth > 3) throw std::runtime_error("Include limit exceeded");

        std::ifstream file(path);
        std::string line, result, tag, name;

        while (std::getline(file, line)) {
            std::istringstream ss(line);
            // extract filename
            if (ss >> tag && tag == "#include" && ss >> std::quoted(name)) {
                result += ReadFile(path.parent_path() / name, depth + 1) + "\n";
            } else {
                result += line + "\n";
            }
        }
        return result;
    }
};
