#include "chunk_pos.h"


ChunkPos::ChunkPos() : x(0), y(0), z(0) {

}

ChunkPos::ChunkPos(int x, int y, int z) : x(x), y(y), z(z) {

}

ChunkPos::ChunkPos(ChunkPos const& o) : x(o.x), y(o.y), z(o.z) {

}

ChunkPos::ChunkPos(Vec3 v) :
        x(int(floor(v.x / ChunkPos::CHUNK_SIZE))),
        y(int(floor(v.y / ChunkPos::CHUNK_SIZE))),
        z(int(floor(v.z / ChunkPos::CHUNK_SIZE))) {

}

bool ChunkPos::operator==(const ChunkPos &other) const {
    return x == other.x && y == other.y && z == other.z;
}
