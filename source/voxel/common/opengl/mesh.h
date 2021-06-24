#ifndef VOXEL_ENGINE_MESH_H
#define VOXEL_ENGINE_MESH_H

#include <vector>
#include <glad/glad.h>

#include "voxel/common/math/vec.h"
#include "voxel/common/math/color.h"
#include "voxel/common/opengl/buffer.h"


namespace voxel {
namespace opengl {

/*
 * Represents vertex format struct field, that is bound to vertex attribute.
 * Because this is really basic implementation, for more convenient use offset and size must be given in bytes divided
 * by 4, to match size of 32-bit floats/ints:
 *
 * for example float {Vec3 pos, Vec2 uv} are defined as {GL_FLOAT, 0, 3}, {GL_FLOAT, 3, 2}
 * */
struct VertexField {
    // type of the field: GL_FLOAT/GL_INT/...
    GLenum type;
    // offset of the vertex field (offset in bytes / 4)
    GLuint offset;
    // size of the vertex field (size in bytes / 4)
    GLuint size;
};

// default vertex format struct, that contains all required basic vertex attributes
struct DefaultVertexFormat {
    // 0) position of the vertex offset=0, size=3
    math::Vec3f position;
    // 1) normal vector of the vertex offset=3, size=3
    math::Vec3f normal;
    // 2) texture coordinate of the vertex offset=6, size=2
    math::Vec2f uv;
    // 3) RGBA color of the vertex offset=8, size=4
    math::Color color;
    // total size = 12 * 4 = 48

    // vertex format must declare
    inline static std::vector<VertexField> fields() {
        return {
                {GL_FLOAT, 0, 3}, // position, index 0
                {GL_FLOAT, 3, 3}, // normal, index 1
                {GL_FLOAT, 6, 2}, // uv, index 2
                {GL_FLOAT, 8, 4}, // color, index 3
        };
    }
};

/*
 * Really basic implementation of mesh to render simple shapes using shaders, can be edited and marked for rebuild,
 * and rendered. Render will call rebuild method if required, or it can be called manually.
 *
 * Mesh vertex format is passed as a template argument, it must be a struct with all required data and
 * declaring static std::vector<VertexField> fields() method, that returns all it fields, to be bound to vertex attributes:
 * vertex field at index 0 is bound to attribute 0, at index 1 to attribute 1 and so on.
 */
template<typename V>
class Mesh {
public:

private:
    std::vector<V> m_vertices;
    std::vector<GLuint> m_indices;

    bool m_dirty = false;

    GLuint m_vertex_array_handle = 0;
    Buffer m_vertex_buffer;
    Buffer m_index_buffer;

public:

    Mesh() : m_vertex_buffer(GL_ARRAY_BUFFER), m_index_buffer(GL_ELEMENT_ARRAY_BUFFER) {
        glGenVertexArrays(1, &m_vertex_array_handle);
    }

    Mesh(const Mesh<V>& other) : Mesh() {
        m_vertices = other.m_vertices;
        m_indices = other.m_indices;
        m_dirty = true;
    }

    Mesh(Mesh<V>&& other) :
        m_vertices(std::move(other.m_vertices)),
        m_indices(std::move(other.m_indices)),
        m_vertex_buffer(std::move(other.m_vertex_buffer)),
        m_index_buffer(std::move(other.m_index_buffer)) {

        m_dirty = other.m_dirty;
        m_vertex_array_handle = other.m_vertex_array_handle;
        other.m_vertex_array_handle = 0;
    }

    ~Mesh() {
        if (m_vertex_array_handle != 0) {
            glBindVertexArray(m_vertex_array_handle);
            int index = 0;
            for (const VertexField& vertex_field : V::fields()) {
                glDisableVertexAttribArray(index++);
            }
            glBindVertexArray(0);
            glDeleteVertexArrays(1, &m_vertex_array_handle);
            m_vertex_array_handle = 0;
        }
    }

    std::vector<V>& getVertices() {
        return m_vertices;
    }

    std::vector<GLuint>& getIndices() {
        return m_indices;
    }

    void invalidate() {
        m_dirty = true;
    }

    void rebuild() {
        if (m_dirty) {
            m_dirty = false;

            glBindVertexArray(m_vertex_array_handle);
            m_vertex_buffer.setData(m_vertices.size() * sizeof(V), &m_vertices[0], GL_STATIC_DRAW, false);
            m_index_buffer.setData(m_indices.size() * sizeof(GLuint), &m_indices[0], GL_STATIC_DRAW, false);

            int index = 0;
            for (const VertexField& vertex_field : V::fields()) {
                glVertexAttribPointer(index, vertex_field.size, vertex_field.type, GL_FALSE, sizeof(V), reinterpret_cast<void*>(vertex_field.offset * 4));
                glEnableVertexAttribArray(index);
                index++;
            }

            glBindVertexArray(0);
        }
    }

    void render() {
        rebuild();
        glBindVertexArray(m_vertex_array_handle);
        m_vertex_buffer.bindBuffer();
        m_index_buffer.bindBuffer();
        glDrawElements(GL_TRIANGLES, m_index_buffer.getAllocatedSize() / sizeof(GLuint), GL_UNSIGNED_INT, NULL);
        m_index_buffer.unbindBuffer();
        m_vertex_buffer.unbindBuffer();
        glBindVertexArray(0);
    }
};

} // opengl
} // voxel

#endif //VOXEL_ENGINE_MESH_H
