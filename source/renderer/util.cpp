
#include <iostream>
#include <fstream>
#include <streambuf>

#include "util.h"

namespace gl {
    Texture::Texture(int width, int height, int mode, GLbyte *data) {
        this->width = width;
        this->height = height;
        this->mode = mode;

        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    Texture::~Texture() {
        if (handle != 0) {
            glDeleteTextures(1, &handle);
        }
    }


    BufferTexture::BufferTexture(int size, GLbyte* data, GLenum mode) {
        this->size = size;

        glGenBuffers(1, &buffer_handle);
        glBindBuffer(GL_TEXTURE_BUFFER, buffer_handle);
        glBufferData(GL_TEXTURE_BUFFER, size, data, mode);
        glGenTextures(1, &handle);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    void BufferTexture::bind() const {
        glBindTexture(GL_TEXTURE_BUFFER, handle);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer_handle);
    }

    BufferTexture::~BufferTexture() {
        if (handle != 0) {
            glDeleteTextures(1, &handle);
        }
        if (buffer_handle != 0) {
            glDeleteBuffers(1, &buffer_handle);
        }
    }


    Mesh::Mesh() {

    }

    void Mesh::invalidate() {
        isValid = false;
    }

    void Mesh::render() {
        if (!isValid) {
            rebuild();
        }
        glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
        glBindVertexArray(vaoHandle);
        glDrawArrays(GL_TRIANGLES, 0, vaoLength);
        glBindVertexArray(0);
    }

    void Mesh::rebuild() {
        if (vaoHandle == 0) {
            glGenVertexArrays(1, &vaoHandle);
        }
        if (vboHandle == 0) {
            glGenBuffers(1, &vboHandle);
        }

        glBindVertexArray(vaoHandle);
        glBindBuffer(GL_ARRAY_BUFFER, vboHandle);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), (GLfloat*) &(vertices[0]), GL_STATIC_DRAW);
        vaoLength = vertices.size();

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) (sizeof(GLfloat) * 0)); // position
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) (sizeof(GLfloat) * 3)); // color
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*) (sizeof(GLfloat) * 7)); // uv

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        isValid = true;
    }

    void Mesh::unloadGraphics() {
        if (vaoHandle != 0) {
            glDeleteVertexArrays(1, &vaoHandle);
        }
        if (vboHandle != 0) {
            glDeleteBuffers(1, &vboHandle);
        }
    }

    Mesh::~Mesh() {
        unloadGraphics();
    }


    Shader::Shader(const char *vertexS, const char *fragmentS) {
        std::string vertex = vertexS;
        std::string fragment = fragmentS;
        std::ifstream vertexInputStream(vertexS), fragmentInputStream(fragment);
        if (vertexInputStream.good()) {
            vertex = std::string(std::istreambuf_iterator<char>(vertexInputStream), std::istreambuf_iterator<char>());
        }
        if (fragmentInputStream.good()) {
            fragment = std::string(std::istreambuf_iterator<char>(fragmentInputStream), std::istreambuf_iterator<char>());
        }

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexCS = vertex.c_str();
        glShaderSource(vertexShader, 1, &vertexCS, nullptr);
        glCompileShader(vertexShader);
        // Check for compile time errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // Fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fragmentCS = fragment.c_str();
        glShaderSource(fragmentShader, 1, &fragmentCS, nullptr);
        glCompileShader(fragmentShader);
        // Check for compile time errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        // Link shaders
        programHandle = glCreateProgram();
        glAttachShader(programHandle, vertexShader);
        glAttachShader(programHandle, fragmentShader);
        glLinkProgram(programHandle);
        // Check for linking errors
        glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programHandle, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Shader::use() const {
        glUseProgram(programHandle);
    }

    GLuint Shader::getUniform(const char *name) const {
        return glGetUniformLocation(programHandle, name);
    }

    Shader::~Shader() {
        if (programHandle != 0) {
            glDeleteProgram(programHandle);
        }
    }
}