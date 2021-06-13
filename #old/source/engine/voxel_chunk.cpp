#include "renderer/render_chunk.h"
#include "voxel_chunk.h"
#include "baking_utils.h"


VoxelChunk::VoxelChunk() : VoxelChunk(ChunkPos::CHUNK_SIZE, ChunkPos::CHUNK_SIZE, ChunkPos::CHUNK_SIZE) {

}

VoxelChunk::VoxelChunk(int sizeX, int sizeY, int sizeZ) : pooledBuffer(PooledChunkBuffer(this)) {
    this->sizeX = sizeX;
    this->sizeY = sizeY;
    this->sizeZ = sizeZ;
    if (sizeX % SUB_REGION_TOTAL_SIZE != 0 || sizeY % SUB_REGION_TOTAL_SIZE != 0 || sizeZ % SUB_REGION_TOTAL_SIZE != 0) {
        throw std::invalid_argument("all chunk sizes must divide by 32");
    }

    voxelBuffer = new voxel_data_t[voxelBufferLen = sizeX * sizeY * sizeZ];
    rSizeX = sizeX / SUB_REGION_TOTAL_SIZE;
    rSizeY = sizeY / SUB_REGION_TOTAL_SIZE;
    rSizeZ = sizeZ / SUB_REGION_TOTAL_SIZE;
}

VoxelChunk::VoxelChunk(VoxelChunk const& other) : VoxelChunk(other.sizeX, other.sizeY, other.sizeZ) {
    copyFrom(other);
}

bool VoxelChunk::copyFrom(VoxelChunk const& other) {
    if (other.sizeX == sizeX && other.sizeY == sizeY && other.sizeZ == sizeZ) {
        setPos(other.position);
        memcpy(voxelBuffer, other.voxelBuffer, voxelBufferLen * sizeof(voxel_data_t));
        return true;
    }
    return false;
}

void VoxelChunk::setChunkSource(ChunkSource* source) {
    chunkSource = source;
}

void VoxelChunk::setPos(ChunkPos const& pos) {
    position = pos;
}

void VoxelChunk::setState(ChunkState newState) {
    state = newState;
}

bool VoxelChunk::isAvailableForRender() {
    return state == STATE_BAKED;
}



std::timed_mutex& VoxelChunk::getContentMutex() {
    return contentMutex;
}

void VoxelChunk::queueRegionUpdate(int x, int y, int z) {
    pooledBuffer.update();
    std::unique_lock<std::mutex> renderChunkLock(renderChunkMutex);
    if (renderChunk != nullptr) {
        renderChunk->queueRegionUpdate(x, y, z);
    }
}

void VoxelChunk::queueFullUpdate() {
    pooledBuffer.update();
    std::unique_lock<std::mutex> renderChunkLock(renderChunkMutex);
    if (renderChunk != nullptr) {
        renderChunk->queueFullUpdate();
    }
}



unsigned int VoxelChunk::calcNormal(int x, int y, int z) {
    if (getVoxelAt(x + 1, y, z) &&
            getVoxelAt(x - 1, y, z) &&
            getVoxelAt(x, y - 1, z) &&
            getVoxelAt(x, y + 1, z) &&
            getVoxelAt(x, y, z - 1) &&
            getVoxelAt(x, y, z + 1)) {
        return 0;
    }

    int nx = 0, ny = 0, nz = 0;
    int distribution[7] = { -1, -1, -2, 0, 2, 1, 1 };
    int weight[49] = {
            0, 1, 1, 1, 1, 1, 0,
            1, 1, 2, 2, 2, 1, 1,
            1, 2, 3, 4, 3, 2, 1,
            1, 2, 4, 6, 4, 2, 1,
            1, 2, 3, 4, 3, 2, 1,
            1, 1, 2, 2, 2, 1, 1,
            0, 1, 1, 1, 1, 1, 0,
    };

    int weight_sum = 0;
    for (int i = 0; i < 49; i++) {
        weight_sum += weight[i];
    }

    for (int xs = -3; xs <= 3; xs++) {
        for (int ys = -3; ys <= 3; ys++) {
            for (int zs = -3; zs <= 3; zs++) {
                if (getVoxelAt(xs + x, ys + y, zs + z) != 0) {
                    nx -= distribution[xs + 3] * weight[(ys + 3) * 7 + (zs + 3)];
                    ny -= distribution[ys + 3] * weight[(xs + 3) * 7 + (zs + 3)];
                    nz -= distribution[zs + 3] * weight[(xs + 3) * 7 + (ys + 3)];
                }
            }
        }
    }

    nx /= weight_sum / 7;
    ny /= weight_sum / 7;
    nz /= weight_sum / 7;

    return ChunkBakingUtils::normalToHash(nx, ny, nz);
}

void VoxelChunk::attachRenderChunk(RenderChunk* newRenderChunk) {
    if (renderChunk != nullptr) {
        renderChunk->_attach(nullptr);
    }

    renderChunkMutex.lock();
    renderChunk = newRenderChunk;
    renderChunkMutex.unlock();

    if (renderChunk != nullptr) {
        renderChunk->_attach(this);
    }
}

VoxelChunk::~VoxelChunk() {
    contentMutex.lock();
    bakedBuffer.releaseAndDestroyCache();
    attachRenderChunk(nullptr);
    delete(voxelBuffer);
    voxelBuffer = nullptr;
    contentMutex.unlock();
}
