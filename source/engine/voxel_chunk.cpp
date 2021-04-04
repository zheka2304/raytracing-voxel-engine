#include "renderer/render_chunk.h"
#include "voxel_chunk.h"


VoxelChunk::VoxelChunk() : VoxelChunk(ChunkPos::CHUNK_SIZE, ChunkPos::CHUNK_SIZE, ChunkPos::CHUNK_SIZE) {

}

VoxelChunk::VoxelChunk(int sizeX, int sizeY, int sizeZ) {
    this->sizeX = sizeX;
    this->sizeY = sizeY;
    this->sizeZ = sizeZ;
    if (sizeX % SUB_REGION_TOTAL_SIZE != 0 || sizeY % SUB_REGION_TOTAL_SIZE != 0 || sizeZ % SUB_REGION_TOTAL_SIZE != 0) {
        throw std::invalid_argument("all chunk sizes must divide by 32");
    }

    voxelBuffer = new unsigned int[voxelBufferLen = sizeX * sizeY * sizeZ];
    rSizeX = sizeX / SUB_REGION_TOTAL_SIZE;
    rSizeY = sizeY / SUB_REGION_TOTAL_SIZE;
    rSizeZ = sizeZ / SUB_REGION_TOTAL_SIZE;
    renderBuffer = new unsigned int[renderBufferLen = SUB_REGION_BUFFER_SIZE_2 * rSizeX * rSizeY * rSizeZ];
}

VoxelChunk::VoxelChunk(VoxelChunk const& other) : VoxelChunk(other.sizeX, other.sizeY, other.sizeZ) {
    copyFrom(other);
}

bool VoxelChunk::copyFrom(VoxelChunk const& other) {
    if (other.sizeX == sizeX && other.sizeY == sizeY && other.sizeZ == sizeZ) {
        setPos(other.position);
        memcpy(voxelBuffer, other.voxelBuffer, voxelBufferLen * sizeof(unsigned int));
        memcpy(renderBuffer, other.renderBuffer, renderBufferLen * sizeof(unsigned int));
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

unsigned int VoxelChunk::calcNormal(int x, int y, int z) {
    float nx = 0;
    float ny = 0;
    float nz = 0;

    bool required = false;
    for (int xs = -2; xs <= 2; xs++) {
        for (int ys = -2; ys <= 2; ys++) {
            for (int zs = -2; zs <= 2; zs++) {
                int xi = x + xs;
                int yi = y + ys;
                int zi = z + zs;

                unsigned int voxel;
                if (xi < 0 || yi < 0 || zi < 0 || xi >= sizeX || yi >= sizeY || zi >= sizeZ) {
                    required = true;
                    voxel = 0;
                } else {
                    voxel = voxelBuffer[xi + (zi + yi * sizeZ) * sizeX];
                    if (voxel == 0) {
                        required = true;
                    }
                }
            }
        }
    }

    if (!required) {
        return 0xFF00FF;
    }

    for (int xs = -4; xs <= 4; xs++) {
        for (int ys = -4; ys <= 4; ys++) {
            for (int zs = -4; zs <= 4; zs++) {
                int xi = x + xs;
                int yi = y + ys;
                int zi = z + zs;

                unsigned int voxel;
                if (xi < 0 || yi < 0 || zi < 0 || xi >= sizeX || yi >= sizeY || zi >= sizeZ) {
                    voxel = 0;
                } else {
                    voxel = voxelBuffer[xi + (zi + yi * sizeZ) * sizeX];
                }
                if (voxel != 0) {
                    int dx = (xs + 4) / 3 - 1;
                    int dy = (ys + 4) / 3 - 1;
                    int dz = (zs + 4) / 3 - 1;
                    float d = std::max(1.0, sqrt(dx * dx + dy * dy + dz * dz));
                    nx += float(dx) / d; // NOLINT(bugprone-integer-division)
                    ny += float(dy) / d; // NOLINT(bugprone-integer-division)
                    nz += float(dz) / d; // NOLINT(bugprone-integer-division)
                }
            }
        }
    }

    float d = sqrt(nx * nx + ny * ny + nz * nz);
    if (d > 1e-4) {
        nx /= d;
        ny /= d;
        nz /= d;
    }
    unsigned int inx = int((nx + 1) * 127) & 0xFF;
    unsigned int iny = int((ny + 1) * 127) & 0xFF;
    unsigned int inz = int((nz + 1) * 127) & 0xFF;
    return (((inx << 8) | iny) << 8) | inz;
}

bool VoxelChunk::rebuildTier1Region(int rx1, int ry1, int rz1, int r2offset) {
    bool any = false;

    if (r2offset < 0) {
        // calculate tier 2 offset
        int rx2 = rx1 >> SUB_REGION_SIZE_2_BITS; rx1 = rx1 & SUB_REGION_SIZE_2_BITS;
        int ry2 = ry1 >> SUB_REGION_SIZE_2_BITS; ry1 = ry1 & SUB_REGION_SIZE_2_BITS;
        int rz2 = rz1 >> SUB_REGION_SIZE_2_BITS; rz1 = rz1 & SUB_REGION_SIZE_2_BITS;
        r2offset = (rx2 + (rz2 + ry2 * rSizeZ) * rSizeX) * SUB_REGION_BUFFER_SIZE_2;
    }

    // calculate tier 2 region pos from r2offset
    int r2offset0 = r2offset / SUB_REGION_BUFFER_SIZE_2;
    int rx2 = r2offset0 % rSizeX;
    int rz2 = r2offset0 / rSizeX;
    int ry2 = rz2 / rSizeZ;
    rz2 = rz2 % rSizeZ;
    int rx2o = rx2 * SUB_REGION_TOTAL_SIZE + rx1 * SUB_REGION_SIZE_1;
    int ry2o = ry2 * SUB_REGION_TOTAL_SIZE + ry1 * SUB_REGION_SIZE_1;
    int rz2o = rz2 * SUB_REGION_TOTAL_SIZE + rz1 * SUB_REGION_SIZE_1;

    // calculate r1offset
    int r1offset = r2offset + 1 + (rx1 + ((rz1 + (ry1 << SUB_REGION_SIZE_2_BITS)) << SUB_REGION_SIZE_2_BITS)) * SUB_REGION_BUFFER_SIZE_1;

    // iterate over voxels in tier 1 region
    for (int vx = 0, x = rx2o; vx < SUB_REGION_SIZE_1; vx++, x++) {
        for (int vy = 0, y = ry2o; vy < SUB_REGION_SIZE_1; vy++, y++) {
            for (int vz = 0, z = rz2o; vz < SUB_REGION_SIZE_1; vz++, z++) {
                int render_voxel = r1offset + 1 + vx + ((vz + (vy << SUB_REGION_SIZE_1_BITS)) << SUB_REGION_SIZE_1_BITS);
                int volume_voxel = x + (z + y * sizeZ) * sizeX;
                unsigned int voxel_val = voxelBuffer[volume_voxel];
                if (voxel_val != 0) {
                    unsigned int voxel_normal = 0x80FF80;// calcNormal(x, y, z);
                    renderBuffer[render_voxel] = voxel_val | (voxel_normal << 8);
                    any = true;
                }
            }
        }
    }

    renderBuffer[r1offset] = int(any);
    return any;
}

bool VoxelChunk::rebuildTier2Region(int rx2, int ry2, int rz2) {
    bool any = false;

    // calculate r2offset
    int r2offset = (rx2 + (rz2 + ry2 * rSizeZ) * rSizeX) * SUB_REGION_BUFFER_SIZE_2;

    // iterate over tier 1 regions
    for (int rx1 = 0; rx1 < SUB_REGION_SIZE_2; rx1++) {
        for (int ry1 = 0; ry1 < SUB_REGION_SIZE_2; ry1++) {
            for (int rz1 = 0; rz1 < SUB_REGION_SIZE_2; rz1++) {
                any = rebuildTier1Region(rx1, ry1, rz1, r2offset) || any;
            }
        }
    }

    renderBuffer[r2offset] = int(any);
    return any;
}

void VoxelChunk::rebuildRenderBuffer() {
    // iterate over tier 2 regions
    for (int rx2 = 0; rx2 < rSizeX; rx2++) {
        for (int ry2 = 0; ry2 < rSizeY; ry2++) {
            for (int rz2 = 0; rz2 < rSizeZ; rz2++) {
                rebuildTier2Region(rx2, ry2, rz2);
            }
        }
    }
}

void VoxelChunk::attachRenderChunk(RenderChunk* newRenderChunk) {
    if (renderChunk != nullptr) {
        renderChunk->_attach(nullptr);
    }
    renderChunk = newRenderChunk;
    if (renderChunk != nullptr) {
        renderChunk->_attach(this);
    }
}

VoxelChunk::~VoxelChunk() {
    attachRenderChunk(nullptr);
    delete(voxelBuffer);
    delete(renderBuffer);
}
