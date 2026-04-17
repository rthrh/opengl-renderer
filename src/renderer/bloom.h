#pragma once
#include <glad/glad.h>
#include <iostream>

class Bloom {
public:
    Bloom(int scrWidth, int scrHeight) :
        _width(scrWidth), _height(scrHeight)
    {
        // configure (floating point) framebuffers
        glGenFramebuffers(1, &_hdrFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, _hdrFBO);
        // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
        glGenTextures(2, _colorBuffers);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindTexture(GL_TEXTURE_2D, _colorBuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            // attach texture to framebuffer
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _colorBuffers[i], 0);
        }
        // create and attach depth buffer (renderbuffer)
        glGenRenderbuffers(1, &_rboDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, _rboDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _width, _height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _rboDepth);
        // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        if (auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer not complete!" << std::endl;
            Error("[Bloom]: Framebuffer incomplete: {} ", status);
            throw std::runtime_error("[Bloom]: Framebuffer incomplete");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ping-pong-framebuffer for blurring
        glGenFramebuffers(2, _pingpongFBO);
        glGenTextures(2, _pingpongColorbuffers);
        for (unsigned int i = 0; i < 2; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, _pingpongFBO[i]);
            glBindTexture(GL_TEXTURE_2D, _pingpongColorbuffers[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _width, _height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _pingpongColorbuffers[i], 0);

            if (auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cout << "Framebuffer not complete!" << std::endl;
                Error("[Bloom]: Framebuffer incomplete: {} ", status);
                throw std::runtime_error("[Bloom]: Framebuffer incomplete");
            }
        }

    }

    ~Bloom() {
        glDeleteFramebuffers(2, _pingpongFBO);
        glDeleteFramebuffers(1, &_hdrFBO);
        glDeleteFramebuffers(1, &_rboDepth);
        glDeleteTextures(2, _pingpongColorbuffers);
        glDeleteTextures(2, _colorBuffers);

    }

    void BindFramebuffer(int index) {
        glBindFramebuffer(GL_FRAMEBUFFER, _pingpongFBO[index]);
    }

    void BindHdrFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, _hdrFBO);
    }

    GLuint GetTexture(int index) {
        return _colorBuffers[index];
    }
    GLuint GetPingPongTexture(int index) {
        return _pingpongColorbuffers[index];
    }

    GLuint GetHdrFBO() {
        return _hdrFBO;
    }



    Bloom(const Bloom&) = delete;
    Bloom& operator=(const Bloom&) = delete;


private:
    int _width, _height;
    GLuint _hdrFBO = 0;
    GLuint _pingpongFBO[2] = {0, 0};
    GLuint _pingpongColorbuffers[2] = {0, 0};
    GLuint _colorBuffers[2] = {0, 0};
    GLuint _rboDepth = 0;
};
