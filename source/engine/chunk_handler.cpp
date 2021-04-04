#include "chunk_handler.h"


bool ChunkHandler::isValidPosition(ChunkPos const& pos) {
    return true;
}

bool ChunkHandler::requestChunk(VoxelChunk& chunk) {
    return true;
}

Vec2i ChunkHandler::getRequestLockRadius() {
    return Vec2i(0, 0);
}

void ChunkHandler::bakeChunk(VoxelChunk &chunk) {

}

Vec2i ChunkHandler::getBakeLockRadius() {
    return Vec2i(1, 0);
}

void ChunkHandler::unloadChunk(VoxelChunk& chunk) {

}