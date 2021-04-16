#ifndef VOXEL_ENGINE_UTIL_H
#define VOXEL_ENGINE_UTIL_H

#include <vector>
#include <string>
#include <glad/glad.h>

#include "common/types.h"


namespace gl {
    const std::string SHADER_DIR = "../shaders/";

    class Texture {
    public:
        GLuint handle = 0;
        int width, height;

        Texture(int width, int height, int internalFormat, int format, int dataType, void *data = nullptr);

        Texture(Texture &&texture);

        Texture(const Texture &) = delete;

        ~Texture();
    };

    class BufferTexture {
    public:
        GLuint handle = 0, buffer_handle = 0;
        int size;

        BufferTexture(int size, GLbyte *data, GLenum mode = GL_DYNAMIC_DRAW);

        void bind() const;

        ~BufferTexture();
    };

    class Mesh {
        struct Vertex {
            float x, y, z;
            float r, g, b, a;
            float u, v;
        };

        GLuint vboHandle = 0, vaoHandle = 0;
        GLuint vaoLength = 0;
        bool isValid = false;
    public:
        GLuint primitive = GL_TRIANGLES;
        std::vector<Vertex> vertices;
        std::vector<int> indices;

        Mesh();

        ~Mesh();

        void invalidate();

        void rebuild();

        void render();

        void unloadGraphics();
    };

    class Shader {
        GLuint programHandle;
    public:
        Shader(std::string const &vertex, std::string const &fragment, std::vector<std::string> const &defines = {});

        Shader(std::string const &name, std::vector<std::string> const &defines = {});

        void use() const;

        ~Shader();

        GLuint getUniform(char const *name) const;
    };

    class ComputeShader {
    public:
        GLuint programHandle;

        ComputeShader(std::string const &source, std::vector<std::string> const &defines = {});

        void use() const;

        ~ComputeShader();
    };

    class Buffer {
    public:
        GLuint handle;

        Buffer();
        ~Buffer();
        void setData(size_t size, void *data, GLuint bufferType = GL_SHADER_STORAGE_BUFFER,
                     GLuint accessType = GL_DYNAMIC_DRAW);
    };

    template <typename T>
    class ComputeShaderUniform : public Buffer {
    public:
        T data;

        void updateAndBind(int index,
                GLuint bufferType = GL_SHADER_STORAGE_BUFFER,
                GLuint accessType = GL_DYNAMIC_DRAW) {
            setData(sizeof(data), &data, bufferType, accessType);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, handle);
        }
    };

    class RenderToTexture {
    public:
        int width, height;

        GLuint frameBufferHandle;
        GLuint depthBufferHandle;
        std::vector<Texture> outputTextures;

        RenderToTexture(int width, int height, int textureCount);
        void startRenderToTexture();
        void drawFullScreenQuad();
        void endRenderToTexture();
    };
}

#define UNIFORM_HANDLE(VARIABLE_NAME, SHADER, UNIFORM_NAME) static GLint VARIABLE_NAME = 0; if (VARIABLE_NAME == 0) VARIABLE_NAME = (SHADER).getUniform(UNIFORM_NAME);

#define GLSL_BUFFER_ALIGN4 __attribute__ ((aligned(4)))
#define GLSL_BUFFER_ALIGN8 __attribute__ ((aligned(8)))
#define GLSL_BUFFER_ALIGN16 __attribute__ ((aligned(16)))
#define GLSL_BUFFER_ALIGN GLSL_BUFFER_ALIGN16


#endif //VOXEL_ENGINE_UTIL_H
