#include <iostream>
#include "riff_file_format.h"

#if VOXEL_ENGINE_ENABLE_DEBUG_VERBOSE
#define VERBOSE(X) X
#else
#define VERBOSE(X)
#endif


namespace voxel {
namespace format {

RiffStyleFileFormat::RiffChunk RiffStyleFileFormat::_readRiffChunkRecursive(std::istream& istream, i32 depth) {
    VERBOSE({
        std::cout << "\n";
        for (i32 i = 0; i < depth; i++) {
            std::cout << "    ";
        }
    });
    VERBOSE(std::cout << "[" << istream.tellg() << "] read RIFF chunk:");

    // read id
    RiffChunk chunk;
    istream.read(chunk.id, 4);
    chunk.id[4] = 0;
    VERBOSE(std::cout << " id=" << chunk.id);

    // read size
    i32 chunk_size;
    i32 children_size;
    istream.read(reinterpret_cast<char*>(&chunk_size), sizeof(i32));
    istream.read(reinterpret_cast<char*>(&children_size), sizeof(i32));
    chunk.total_size = chunk_size + children_size + 12;

    VERBOSE(std::cout << " chunk_size=" << chunk_size);
    VERBOSE(std::cout << " children_size=" << children_size);

    // read chunk bytes
    chunk.bytes.resize(chunk_size);
    istream.read(reinterpret_cast<char*>(&chunk.bytes[0]), chunk_size);

    // read all children
    if (children_size > 0) {
        VERBOSE(std::cout << " {");
        while (children_size > 0) {
            auto& child = chunk.children.emplace_back(std::move(_readRiffChunkRecursive(istream, depth + 1)));
            children_size -= child.total_size;
            VERBOSE(std::cout << " | " << chunk.id << ": remaining children bytes: " << children_size << " + " << child.total_size);
        }
        VERBOSE({
            std::cout << "\n";
            for (i32 i = 0; i < depth; i++) {
                std::cout << "    ";
            }
            std::cout << "}";
        });
    }

    return chunk;
}

std::vector<VoxelModel> RiffStyleFileFormat::read(std::istream& istream) {
    RiffStyleFileFormat::RiffFile riff_file;

    // read type
    istream.read(riff_file.type, 4);
    riff_file.type[4] = 0;

    // read version
    istream.read(reinterpret_cast<char*>(&riff_file.version), sizeof(u32));

    // read root chunks
    while (true) {
        std::streampos pos = istream.tellg();
        istream.seekg( 0, std::ios::end );
        std::streamsize len = istream.tellg() - pos;
        istream.seekg( pos );
        if (len < 12) {
            break;
        }
        riff_file.chunks.emplace_back(std::move(_readRiffChunkRecursive(istream)));
    }

    // pass to riff reader
    return readRiff(riff_file);
}

std::vector<VoxelModel> RiffStyleFileFormat::readRiff(RiffFile& riff_file) {
    return {};
}

} // format
} // voxel