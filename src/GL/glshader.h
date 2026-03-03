#include <glad/glad.h>



struct GLShader {
    GLuint id;
    explicit GLShader(GLenum type) : id(glCreateShader(type)) {}
    ~GLShader() { if(id) glDeleteShader(id);}
};