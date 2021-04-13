#include <iostream>
#include <fstream>
#include <streambuf>

#include "util.h"


namespace gl {
    Texture::Texture(int width, int height, int internalFormat, int format, int dataType, void* data) {
        this->width = width;
        this->height = height;

        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    Texture::Texture(Texture&& other) {
        this->width = other.width;
        this->height = other.height;
        this->handle = other.handle;
        other.handle = 0;
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


    std::string resolveShaderSource(std::string const& source) {
        std::ifstream inputStream(source);
        if (inputStream.good()) {
            return std::string(std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
        } else {
            std::ifstream inputStream2(SHADER_DIR + source);
            if (inputStream2.good()) {
                return std::string(std::istreambuf_iterator<char>(inputStream2), std::istreambuf_iterator<char>());
            }
        }
        return source;
    }

    Shader::Shader(std::string const& vertexS, std::string const& fragmentS, std::vector<std::string> const& defines) {
        std::string vertex = resolveShaderSource(vertexS);
        std::string fragment = resolveShaderSource(fragmentS);

        std::string definesSource = "";
        for (auto const& define : defines) {
            definesSource += "#define " + define + "\n";
        }

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vertexSources[2] = { definesSource.c_str(), vertex.c_str() };
        glShaderSource(vertexShader, 2, vertexSources, nullptr);
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
        const char* fragmentSources[2] = { definesSource.c_str(), fragment.c_str() };
        glShaderSource(fragmentShader, 2, fragmentSources, nullptr);
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

    Shader::Shader(std::string const& name, std::vector<std::string> const& defines) : Shader(name + ".vert", name + ".frag", defines) {

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


    ComputeShader::ComputeShader(std::string const& sourceS) {
        std::string source = resolveShaderSource(sourceS);

        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
        const char* sources[1] = { source.c_str() };
        glShaderSource(shader, 1, sources, nullptr);
        glCompileShader(shader);

        // Check for compile time errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
        }

        // Link shaders
        programHandle = glCreateProgram();
        glAttachShader(programHandle, shader);
        glLinkProgram(programHandle);

        // Check for linking errors
        glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(programHandle, 512, nullptr, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(shader);
    }

    void ComputeShader::use() const {
        glUseProgram(programHandle);
    }

    ComputeShader::~ComputeShader() {
        if (programHandle != 0) {
            glDeleteProgram(programHandle);
        }
    }


    RenderToTexture::RenderToTexture(int width, int height, int textureCount) :
        width(width), height(height) {

        glGenFramebuffers(1, &frameBufferHandle);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);

        glGenRenderbuffers(1, &depthBufferHandle);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBufferHandle);
        glad_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferHandle);

        // create textures
        for (int i = 0; i < textureCount; i++) {
            outputTextures.emplace_back(Texture(width, height, GL_RGB, GL_RGB, GL_FLOAT, nullptr));
        }

        // attach textures to framebuffer
        GLenum attachment = GL_COLOR_ATTACHMENT0;
        std::vector<GLenum> drawBuffers;
        for (Texture const& texture : outputTextures) {
            glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.handle, 0);
            drawBuffers.emplace_back(attachment);
            attachment++;
        }
        glDrawBuffers(drawBuffers.size(), &drawBuffers[0]);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "failed to create framebuffer!\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void RenderToTexture::startRenderToTexture() {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
        glClearColor(1.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);
    }

    void RenderToTexture::drawFullScreenQuad() {
        static Mesh mesh;
        static bool isCreated = false;
        if (!isCreated) {
            mesh.vertices.push_back({-1, -1, -1, 1, 1, 1, 1, 0, 0});
            mesh.vertices.push_back({1, -1, -1, 1, 1, 1, 1, 1, 0});
            mesh.vertices.push_back({1, 1, -1, 1, 1, 1, 1, 1, 1});
            mesh.vertices.push_back({-1, -1, -1, 1, 1, 1, 1, 0, 0});
            mesh.vertices.push_back({1, 1, -1, 1, 1, 1, 1, 1, 1});
            mesh.vertices.push_back({-1, 1, -1, 1, 1, 1, 1, 0, 1});
            mesh.rebuild();
            isCreated = true;
        }
        mesh.render();
    }

    void RenderToTexture::endRenderToTexture() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}