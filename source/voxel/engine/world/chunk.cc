#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>

#include "chunk.h"


namespace voxel {
namespace world {

Chunk::Chunk(ChunkPosition position) : m_position(position) {
    m_buffer_voxel_span = HEADER_SIZE + 512 * TREE_NODE_SIZE;
    m_buffer_size = m_buffer_voxel_span + 4096 * VOXEL_SIZE;

    m_buffer_tree_offset = HEADER_SIZE;
    m_buffer_voxels_offset = m_buffer_voxel_span;

    // allocate buffer
    m_buffer = static_cast<u32*>(calloc(m_buffer_size, sizeof(i32)));
    // header
    m_buffer[0] = 0x80000000u; // magic constant
    m_buffer[1] = 0u;          // reserved
    m_buffer[2] = 3u;          // empty child at idx 0
}

Chunk::~Chunk() {
    delete(m_buffer);
}

const ChunkPosition& Chunk::getPosition() const {
    return m_position;
}

const u32* Chunk::getBuffer() const {
    return m_buffer;
}

const i32 Chunk::getBufferSize() const {
    return m_buffer_size;
}


void Chunk::preallocate(int32_t tree_nodes, int32_t voxels) {
    // std::cout << "preallocate " << tree_nodes << ", " << voxels << "\n";

    // calculate required tree and voxel span sizes and new buffer size
    i32 tree_nodes_span_size = std::max(m_buffer_voxel_span - HEADER_SIZE, tree_nodes * TREE_NODE_SIZE);
    i32 voxel_span_size = std::max(m_buffer_size - m_buffer_voxel_span, voxels * VOXEL_SIZE);
    i32 new_buffer_size = HEADER_SIZE + tree_nodes_span_size + voxel_span_size;

    // if more buffer space is required
    if (new_buffer_size > m_buffer_size) {
        // reallocate buffer
        m_buffer = static_cast<u32*>(realloc(m_buffer, new_buffer_size * sizeof(u32)));
        m_buffer_size = new_buffer_size;

        // voxel span shift = new voxel span offset - old voxel span offset
        i32 voxel_span_shift = (tree_nodes_span_size + HEADER_SIZE) - m_buffer_voxel_span;

        // if voxel span must be shifted
        if (voxel_span_shift != 0) {
            // shift header pointer if required
            if (m_buffer[2] >= m_buffer_voxel_span) {
                m_buffer[2] += voxel_span_shift;
            }
            // iterate over all child pointers in all tree nodes
            for (i32 i = HEADER_SIZE; i < m_buffer_tree_offset; i += TREE_NODE_SIZE) {
                for (i32 j = 2; j < TREE_NODE_SIZE; j++) {
                    // if child is pointing to voxel span, shift it
                    if (m_buffer[i + j] + i >= m_buffer_voxel_span) {
                        m_buffer[i + j] += voxel_span_shift;
                    }
                }
            }

            // shift voxel span
            memmove(
                    m_buffer + m_buffer_voxel_span + voxel_span_shift,               // dst: new buffer voxel span offset
                    m_buffer + m_buffer_voxel_span,                                  // src: old buffer voxel span offset
                    (m_buffer_voxels_offset - m_buffer_voxel_span) * sizeof(u32)    // size: end of voxel span - buffer voxel span
            );
            m_buffer_voxel_span += voxel_span_shift;
            m_buffer_voxels_offset += voxel_span_shift;
        }
    }
}

u32 Chunk::_getAllocatedNodeSpanSize() {
    return (m_buffer_voxel_span - HEADER_SIZE) / TREE_NODE_SIZE;
}

u32 Chunk::_getAllocatedVoxelSpanSize() {
    return (m_buffer_size - m_buffer_voxel_span) / VOXEL_SIZE;
}

u32 Chunk::_allocateNewNode(u32 color, u32 material) {
    // if remaining space for nodes < tree node size, allocate more space
    if (m_buffer_voxel_span - m_buffer_tree_offset < TREE_NODE_SIZE) {
        u32 allocated_nodes = _getAllocatedNodeSpanSize();
        u32 allocated_voxels = _getAllocatedVoxelSpanSize();
        preallocate(allocated_nodes + 128, allocated_voxels + 256);
    }

    u32 ptr = m_buffer_tree_offset;
    m_buffer_tree_offset += TREE_NODE_SIZE;
    m_buffer[ptr] = color | 0x80000000u;
    m_buffer[ptr + 1] = material;
    memset(m_buffer + ptr + 2, 0, sizeof(i32) * (TREE_NODE_SIZE - 2));
    return ptr;
}

u32 Chunk::_allocateNewVoxel(u32 color, u32 material) {
    // if remaining space for nodes < tree node size, allocate more space
    if (m_buffer_size - m_buffer_voxels_offset < VOXEL_SIZE) {
        u32 allocated_nodes = _getAllocatedNodeSpanSize();
        u32 allocated_voxels = _getAllocatedVoxelSpanSize();
        preallocate(allocated_nodes, allocated_voxels + 1024);
    }

    u32 ptr = m_buffer_voxels_offset;
    m_buffer_voxels_offset += VOXEL_SIZE;
    m_buffer[ptr] = color | 0x40000000u;
    m_buffer[ptr + 1] = material;
    return ptr;
}

void Chunk::setVoxel(VoxelPosition position) {
    u32 tree_ptr = 0;

    // recursively allocate all required tree nodes
    // after loop exits, tree_ptr must contain pointer to the parent tree node of required voxel
    for (i8 i = position.scale; i > 0; i--) {
        // get idx from i bit of position (from highest to lowest)
        u8 idx = ((((position.x >> i) & 1) << 0) | (((position.y >> i) & 1) << 1) | (((position.z >> i) & 1) << 2)) ^ 7;

        // it is guaranteed, that m_buffer[tree_ptr] is a tree node

        // get child ptr offset
        u32 child_link_ptr = tree_ptr + 2 + idx;
        u32 child = m_buffer[child_link_ptr];
        // if child exists - proceed to it
        if (child != 0) {
            tree_ptr += child;
            // if child is not a tree node, override it with newly allocated tree node
            if (!(m_buffer[tree_ptr] & 0x80000000u)) {
                // remove old
                m_buffer[tree_ptr] = 0;
                // allocate new
                u32 next = _allocateNewNode(0, 0);
                m_buffer[child_link_ptr] = next - (tree_ptr - child);
                tree_ptr = next;
            }
        // if child does not exist - allocate it
        } else {
            u32 next = _allocateNewNode(0, 0);
            m_buffer[child_link_ptr] = next - tree_ptr;
            tree_ptr = next;
        }
    }

    // get ptr to voxel from tree_ptr containing last tree node and first bit of position as idx;
    u8 idx = (((position.x & 1) << 0) | ((position.y & 1) << 1) | ((position.z & 1) << 2)) ^ 7;
    u32 child = m_buffer[tree_ptr + 2 + idx];

    // if it is already allocated
    if (child != 0) {
        // proceed and override it
        tree_ptr += child;
        m_buffer[tree_ptr] = 1;
        m_buffer[tree_ptr + 1] = 2;
    } else {
        // otherwise allocate new one
        u32 next = _allocateNewVoxel(1, 2);
        m_buffer[tree_ptr + 2 + idx] = next - tree_ptr;
    }
}


} // world
} // voxel