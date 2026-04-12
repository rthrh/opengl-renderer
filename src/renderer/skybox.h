#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image.h>

#include <vector>
#include <string>

#include "renderer/shader.h"
#include "utils/logger.h"
#include "renderer/shapes.h"

class Skybox {
public:
    Skybox(std::string path, Shader& equirectShader, GLsizei cubeSize = 1024) :
        _cubeSize(cubeSize)
    {
        initBuffers();
        initTextureHDR(std::move(path));
        initCubemap();
        initCube();

        equirectToEnvMap(equirectShader);
    }

    ~Skybox() {
        //TODO FBO, RBO, HDR texture can be removed after init
        glDeleteTextures(1, &_envCubemap);
        glDeleteTextures(1, &_hdrTexture);
        glDeleteFramebuffers(1, &_captureFBO);
        glDeleteRenderbuffers(1, &_captureRBO);
        glDeleteVertexArrays(1, &_cubeVAO);
        glDeleteBuffers(1, &_cubeVBO);
    }

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    void Draw(Shader& skyboxShader, glm::mat4 view, glm::mat4 projection) {
        skyboxShader.Activate();
        skyboxShader.SetMat4("view", view);
        skyboxShader.SetMat4("projection", projection);

        glBindTextureUnit(6, _envCubemap); //TODO texture slot 6 for skybox
        renderCube();
    }

private:
    // Initializes capture FBO and RBO
    void initBuffers() {
        glCreateFramebuffers(1, &_captureFBO);
        glCreateRenderbuffers(1, &_captureRBO);
        glNamedRenderbufferStorage(_captureRBO, GL_DEPTH_COMPONENT24, _cubeSize, _cubeSize);
        glNamedFramebufferRenderbuffer(_captureFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _captureRBO);
    }

    void initTextureHDR(const std::string& path) {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        float *data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glCreateTextures(GL_TEXTURE_2D, 1, &_hdrTexture);
            glTextureStorage2D(_hdrTexture, 1, GL_RGB16F, width, height);
            glTextureSubImage2D(_hdrTexture, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);
            glTextureParameteri(_hdrTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(_hdrTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(_hdrTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(_hdrTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            throw std::runtime_error("Failed to load HDR image");
        }
    }

    // setup cubemap to render to and attach to framebuffer
    void initCubemap() {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &_envCubemap);
        glTextureStorage2D(_envCubemap, 1, GL_RGB16F, _cubeSize, _cubeSize);
        glTextureParameteri(_envCubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_envCubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_envCubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_envCubemap, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_envCubemap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // Init cube geometry
    void initCube() {
        glCreateVertexArrays(1, &_cubeVAO);
        glCreateBuffers(1, &_cubeVBO);
        glNamedBufferStorage(_cubeVBO, sizeof(cube_map_vertices), cube_map_vertices, 0);

        glVertexArrayVertexBuffer(_cubeVAO, 0, _cubeVBO, 0, 8 * sizeof(float));

        glVertexArrayAttribFormat(_cubeVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribFormat(_cubeVAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glVertexArrayAttribFormat(_cubeVAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));

        glVertexArrayAttribBinding(_cubeVAO, 0, 0);
        glVertexArrayAttribBinding(_cubeVAO, 1, 0);
        glVertexArrayAttribBinding(_cubeVAO, 2, 0);

        glEnableVertexArrayAttrib(_cubeVAO, 0);
        glEnableVertexArrayAttrib(_cubeVAO, 1);
        glEnableVertexArrayAttrib(_cubeVAO, 2);
    }

    // Converts equirectangular map to cube map
    void equirectToEnvMap(Shader& equirectShader) {
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        equirectShader.Activate();
        equirectShader.SetInt("equirectangularMap", 0);
        equirectShader.SetMat4("projection", captureProjection);
        glBindTextureUnit(0, _hdrTexture);

        glViewport(0, 0, _cubeSize, _cubeSize);
        glBindFramebuffer(GL_FRAMEBUFFER, _captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectShader.SetMat4("view", captureViews[i]);
            glNamedFramebufferTextureLayer(_captureFBO, GL_COLOR_ATTACHMENT0, _envCubemap, 0, i);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            this->renderCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void renderCube() {
        glBindVertexArray(_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36); // 36 cubemap triangles
    }

    GLuint _captureFBO = 0;
    GLuint _captureRBO = 0;
    GLuint _envCubemap = 0;
    GLuint _hdrTexture = 0;
    GLuint _cubeVAO = 0;
    GLuint _cubeVBO = 0;
    GLsizei _cubeSize = 0;
};