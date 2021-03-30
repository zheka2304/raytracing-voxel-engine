#include <stdexcept>
#include <iostream>
#include <utility>
#include <math.h>

#include "common/simple-profiler.h"
#include "common/vec.h"
#include "render_chunk.h"



ChunkPos::ChunkPos() : x(0), y(0), z(0) {

}

ChunkPos::ChunkPos(int x, int y, int z) : x(x), y(y), z(z) {

}

ChunkPos::ChunkPos(Vec3 v) :
    x(int(floor(v.x / VoxelChunk::DEFAULT_CHUNK_SIZE))),
    y(int(floor(v.y / VoxelChunk::DEFAULT_CHUNK_SIZE))),
    z(int(floor(v.z / VoxelChunk::DEFAULT_CHUNK_SIZE))) {

}

bool ChunkPos::operator==(const ChunkPos &other) const {
    return x == other.x && y == other.y && z == other.z;
}


VoxelChunk::VoxelChunk() : VoxelChunk(DEFAULT_CHUNK_SIZE, DEFAULT_CHUNK_SIZE, DEFAULT_CHUNK_SIZE) {

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

void VoxelChunk::setPos(const ChunkPos &pos) {
    position = pos;
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
                    unsigned int voxel_normal = calcNormal(x, y, z);
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
    delete(voxelBuffer);
    delete(renderBuffer);
}


RenderChunk::RenderChunk(VoxelRenderEngine* renderEngine) : renderEngine(renderEngine) {

}

void RenderChunk::setPos(int x, int y, int z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

void RenderChunk::setChunkBufferOffset(int offset) {
    chunkBufferOffset = offset;
}

void RenderChunk::_attach(VoxelChunk* newChunk) {
    chunkMutex.lock();
    if (chunk != newChunk) {
        chunk = newChunk;
        if (chunk != nullptr) {
            fullUpdateQueued = true;
        } else {
            fullUpdateQueued = false;
        }
        // at the end we clear per-region update queue
        regionUpdateQueue.clear();
    }
    chunkMutex.unlock();
}

void RenderChunk::runAllUpdates(int maxRegionUpdates) {
    if (fullUpdateQueued) {
        // clear all other queued updates
        chunkMutex.lock();
        regionUpdateQueue.clear();
        chunkMutex.unlock();
        fullUpdateQueued = false;

        // TODO: chunk data must be somehow locked during this update
        glBindBuffer(GL_TEXTURE_BUFFER, renderEngine->getChunkBufferHandle());
        glBufferSubData(GL_TEXTURE_BUFFER,
                        chunkBufferOffset * sizeof(unsigned int),
                        chunk->renderBufferLen * sizeof(unsigned int),
                        chunk->renderBuffer);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    } else {
        glBindBuffer(GL_TEXTURE_BUFFER, renderEngine->getChunkBufferHandle());
        int update_count = 0;
        while(maxRegionUpdates < 0 || update_count < maxRegionUpdates) {
            // lock and pop first element of the map
            chunkMutex.lock();
            auto it = regionUpdateQueue.begin();
            if (it == regionUpdateQueue.end()) {
                chunkMutex.unlock();
                break;
            }
            std::pair<int, int> reg_pair = *it;
            regionUpdateQueue.erase(it);
            chunkMutex.unlock();

            // as the next region is acquired, update it
            // TODO: chunk data must be somehow locked during this update
            glBufferSubData(GL_TEXTURE_BUFFER,
                            (chunkBufferOffset + reg_pair.first) * sizeof(unsigned int),
                            reg_pair.second * sizeof(unsigned int),
                            chunk->renderBuffer + reg_pair.first);
            update_count++;
        }
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }
}


RenderChunk::~RenderChunk() {
    if (this->chunk != nullptr) {
        this->chunk->attachRenderChunk(nullptr);
    }
}


void Camera::setPosition(Vec3 const& pos) {
    position = pos;
}

void Camera::setRotation(float yaw, float pitch) {
    this->yaw = yaw;
    this->pitch = pitch;
}

void Camera::setViewport(float x, float y, float w, float h) {
    viewport[0] = x;
    viewport[1] = y;
    viewport[2] = w;
    viewport[3] = h;
}

void Camera::addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap) {
    visibilityMap.emplace(ChunkPos(0, 0, 0), 2);
    visibilityMap.emplace(ChunkPos(1, 0, 0), 2);
    visibilityMap.emplace(ChunkPos(0, 0, 1), 2);
    visibilityMap.emplace(ChunkPos(1, 0, 1), 2);
}

void Camera::sendParametersToShader(gl::Shader& shader) {
    UNIFORM_HANDLE(viewport_uniform, shader, "VIEWPORT");
    UNIFORM_HANDLE(camera_position_uniform, shader, "CAMERA_POSITION");
    UNIFORM_HANDLE(camera_ray_uniform, shader, "CAMERA_RAY");

    glUniform4f(viewport_uniform, viewport[0], viewport[1], viewport[2], viewport[3]);
    glUniform3f(camera_position_uniform, position.x, position.y, position.z);
    glUniform3f(camera_ray_uniform, cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));
}


void OrthographicCamera::addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap) {
    Vec3 forward(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));
    Vec3 right = VecMath::normalize(Vec3(forward.z, 0, -forward.x));
    Vec3 up = VecMath::cross(forward, right);

    float w = viewport[2], h = viewport[3];
    int width = (int) ceil(w / VoxelChunk::DEFAULT_CHUNK_SIZE);
    int height = (int) ceil(h / VoxelChunk::DEFAULT_CHUNK_SIZE);
    for (int y = 0; y <= height; y++) {
        for (int x = 0; x <= width; x++) {
            Vec3 ray_start = position + forward * -128 +
                    right * ((float(x) - float(width) / 2) * VoxelChunk::DEFAULT_CHUNK_SIZE) +
                    up * ((float(y) - float(height) / 2) * VoxelChunk::DEFAULT_CHUNK_SIZE);

            Vec3 position_f = ray_start / VoxelChunk::DEFAULT_CHUNK_SIZE;
            Vec3i position = VecMath::floor_to_int(position_f);

            Vec3i step(
                    forward.x > 0 ? 1 : -1,
                    forward.y > 0 ? 1 : -1,
                    forward.z > 0 ? 1 : -1
            );

            Vec3 rSqr = forward * forward;
            if (rSqr.x == 0) rSqr.x = 1e-8;
            if (rSqr.y == 0) rSqr.y = 1e-8;
            if (rSqr.z == 0) rSqr.z = 1e-8;

            Vec3 rayS(
                    sqrt(1.0 + rSqr.y / rSqr.x + rSqr.z / rSqr.x),
                    sqrt(1.0 + rSqr.x / rSqr.y + rSqr.z / rSqr.y),
                    sqrt(1.0 + rSqr.x / rSqr.z + rSqr.y / rSqr.z)
            );

            Vec3 rayL(
                    (forward.x > 0 ? (float(position.x) + 1 - position_f.x) : (position_f.x - float(position.x))) * rayS.x,
                    (forward.y > 0 ? (float(position.y) + 1 - position_f.y) : (position_f.y - float(position.y))) * rayS.y,
                    (forward.z > 0 ? (float(position.z) + 1 - position_f.z) : (position_f.z - float(position.z))) * rayS.z
            );

            float distance = 0;
            while (distance < 5) {
                if (position.y == 0)
                    visibilityMap.emplace(ChunkPos(position.x, position.y, position.z), 2);

                if (rayL.x < rayL.y) {
                    if (rayL.x < rayL.z) {
                        position.x += step.x;
                        distance = rayL.x;
                        rayL.x += rayS.x;
                    } else {
                        position.z += step.z;
                        distance = rayL.z;
                        rayL.z += rayS.z;
                    }
                } else {
                    if (rayL.y < rayL.z) {
                        position.y += step.y;
                        distance = rayL.y;
                        rayL.y += rayS.y;
                    } else {
                        position.z += step.z;
                        distance = rayL.z;
                        rayL.z += rayS.z;
                    }
                }
            }
        }
    }
    /*
    for (int z = -5; z <= 5; z++) {
        for (int x = -5; x <= 5; x++) {
            ChunkPos p(x, 0, z);
            if (visibilityMap.find(p) != visibilityMap.end()) {
                std::cout << (visibilityMap[p] == 2 ? "#" : ".");
            } else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n"; */
}


DebugChunkSource::DebugChunkSource(std::function<VoxelChunk *(ChunkPos)> providerFunc) : chunkProviderFunc(std::move(providerFunc)) {

}

VoxelChunk* DebugChunkSource::getChunkAt(ChunkPos pos) {
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        return it->second;
    }
    VoxelChunk* chunk = chunkProviderFunc(pos);
    if (chunk != nullptr) {
        chunkMap.emplace(pos, chunk);
    }
    return chunk;
}

DebugChunkSource::~DebugChunkSource() {
    for (auto posAndChunk : chunkMap) {
        delete(posAndChunk.second);
    }
}



VoxelRenderEngine::VoxelRenderEngine(std::shared_ptr<ChunkSource> chunkSource, std::shared_ptr<Camera> camera) : chunkSource(std::move(chunkSource)), camera(std::move(camera)) {
    int maxBufferTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxBufferTextureSize);
    int bufferSize = std::min(maxBufferTextureSize, VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE * MAX_RENDER_CHUNK_INSTANCES);

    chunkBuffer = new gl::BufferTexture(bufferSize * sizeof(unsigned int), nullptr);
    chunkBufferSize = bufferSize / VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE;


    std::cout << "initialized voxel render engine, max_render_chunks = " << chunkBufferSize << " \n";
}

RenderChunk* VoxelRenderEngine::getNewRenderChunk(int reuseVisibilityLevel) {
    if (!renderChunkPool.empty()) {
        RenderChunk* result = *renderChunkPool.begin();
        renderChunkPool.pop_front();
        return result;
    } else {
        if (totalRenderChunkInstances < chunkBufferSize) {
            totalRenderChunkInstances++;
            RenderChunk* renderChunk = new RenderChunk(this);
            for (int i = 0; i < chunkBufferSize; i++) {
                if (!chunkBufferUsage[i]) {
                    chunkBufferUsage[i] = true;
                    renderChunk->setChunkBufferOffset(i * VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE);
                    break;
                }
            }
            return renderChunk;
        } else {
            for (auto it = renderChunkMap.begin(); it != renderChunkMap.end(); it++) {
                if (it->second->visibilityLevel <= reuseVisibilityLevel) {
                    RenderChunk* renderChunk = it->second;
                    renderChunkMap.erase(it);
                    return renderChunk;
                }
            }
            return nullptr;
        }
    }

}

GLuint VoxelRenderEngine::getChunkBufferHandle() {
    return chunkBuffer->buffer_handle;
}

void VoxelRenderEngine::updateVisibleChunks() {
    PROFILER_BEGIN(VoxelRenderEngine_updateVisibleChunks);

    std::unordered_map<ChunkPos, int> visibilityUpdatesMap;
    camera->addAllVisiblePositions(visibilityUpdatesMap);
    std::list<std::pair<ChunkPos, int>> newVisibleChunks;
    for (auto const& posAndChunk : renderChunkMap) {
        posAndChunk.second->visibilityLevel = RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE;
    }
    for (auto const& posAndLevel : visibilityUpdatesMap) {
        auto it = renderChunkMap.find(posAndLevel.first);
        if (it != renderChunkMap.end()) {
            it->second->visibilityLevel = posAndLevel.second;
            if (posAndLevel.second == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
                it->second->runAllUpdates();
            }
        } else {
            newVisibleChunks.emplace_back(posAndLevel);
        }
    }
    for (auto const &posAndLevel : newVisibleChunks) {
        if (posAndLevel.second == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
            VoxelChunk* chunk = chunkSource->getChunkAt(posAndLevel.first);
            if (chunk != nullptr) {
                RenderChunk* renderChunk = getNewRenderChunk();
                if (renderChunk != nullptr) {
                    renderChunkMap.emplace(posAndLevel.first, renderChunk);
                    renderChunk->setPos(posAndLevel.first.x, posAndLevel.first.y, posAndLevel.first.z);
                    chunk->attachRenderChunk(renderChunk);
                    renderChunk->runAllUpdates();
                }
            }
        }
    }

    /*
    int visible_count = 0;
    int near_visible_count = 0;
    int not_visible_count = 0;
    int pooled_count = renderChunkPool.size();

    for (auto const& posAndChunk : renderChunkMap) {
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
            visible_count++;
        }
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_NEAR_VIEW) {
            near_visible_count++;
        }
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE) {
            not_visible_count++;
        }
    }

    std::cout << "visible=" << visible_count << " near_visible=" << near_visible_count << " not_visible=" << not_visible_count << " pooled=" << pooled_count << "\n";
    */

}

void VoxelRenderEngine::prepareForRender(gl::Shader& shader) {
    PROFILER_BEGIN(VoxelRenderEngine_prepareForRender);

    int minX = 0x7FFFFFFF, minY = 0x7FFFFFFF, minZ = 0x7FFFFFFF;
    int maxX = -0x7FFFFFFF, maxY = -0x7FFFFFFF, maxZ = -0x7FFFFFFF;

    int chunksInView = 0;
    for (auto const &posAndChunk : renderChunkMap) {
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
            ChunkPos pos = posAndChunk.first;
            if (minX > pos.x) {
                minX = pos.x;
            }
            if (minY > pos.y) {
                minY = pos.y;
            }
            if (minZ > pos.z) {
                minZ = pos.z;
            }
            if (maxX < pos.x) {
                maxX = pos.x;
            }
            if (maxY < pos.y) {
                maxY = pos.y;
            }
            if (maxZ < pos.z) {
                maxZ = pos.z;
            }
            chunksInView++;
        }
    }

    if (chunksInView > 0) {
        int count[3] = {maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1};
        while (count[0] * count[1] * count[2] > chunkBufferSize) {
            // while visible chunk count is too big subtract 1 from largest axis count
            if (count[0] > count[1]) {
                if (count[2] > count[0]) {
                    count[2]--;
                } else {
                    count[0]--;
                }
            } else {
                if (count[2] > count[1]) {
                    count[2]--;
                } else {
                    count[1]--;
                }
            }
        }

        int offset[3] = {minX + (maxX - minX - count[0]) / 2, minY + (maxY - minY - count[1]) / 2,
                         minZ + (maxZ - minZ - count[2]) / 2};

        GLint chunkTextureOffsets[MAX_RENDER_CHUNK_INSTANCES] { 1 };
        for (auto const& posAndChunk : renderChunkMap) {
            if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
                ChunkPos pos = posAndChunk.first;
                if (pos.x >= offset[0] && pos.y >= offset[1] && pos.z >= offset[2] &&
                    pos.x < offset[0] + count[0] && pos.y < offset[1] + count[1] && pos.z < offset[2] + count[2]) {
                    chunkTextureOffsets[(pos.x - offset[0]) + ((pos.z - offset[2]) + (pos.y - offset[1]) * count[2]) *
                                                              count[0]] = posAndChunk.second->chunkBufferOffset;
                }
            }
        }

        glActiveTexture(GL_TEXTURE0);
        chunkBuffer->bind();

        shader.use();
        glUniform3i(shader.getUniform("CHUNK_OFFSET"), offset[0], offset[1], offset[2]);
        glUniform3i(shader.getUniform("CHUNK_COUNT"), count[0], count[1], count[2]);
        glUniform1ui(shader.getUniform("CHUNK_BUFFER"), 1);
        glUniform1iv(shader.getUniform("CHUNK_DATA_OFFSETS_IN_BUFFER"), MAX_RENDER_CHUNK_INSTANCES, chunkTextureOffsets);
    } else {
        // no chunks in view
        glUniform3i(shader.getUniform("CHUNK_COUNT"), 0, 0, 0);
    }

}

