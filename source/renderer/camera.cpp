#include <iostream>
#include "camera.h"
#include "render_chunk.h"
#include "engine/chunk_source.h"


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
    // add debug chunks
    visibilityMap.emplace(ChunkPos(0, 0, 0), 2);
    visibilityMap.emplace(ChunkPos(1, 0, 0), 2);
    visibilityMap.emplace(ChunkPos(0, 0, 1), 2);
    visibilityMap.emplace(ChunkPos(1, 0, 1), 2);
}

void Camera::sendParametersToShader(UniformData& uniformData) {
    uniformData = {
            0,
            { viewport[0], viewport[1], viewport[2], viewport[3] },
            { position.x, position.y, position.z },
            { cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch) },
            near, far
    };
}

void Camera::setNearAndFarPlane(float near, float far) {
    this->near = near;
    this->far = far;
}

void Camera::requestChunksFromSource(std::shared_ptr<ChunkSource> chunkSource) {
    std::unordered_map<ChunkPos, int> visibilityMap;
    addAllVisiblePositions(visibilityMap);
    for (auto const& posAndVisibility : visibilityMap) {
        chunkSource->requestChunk(posAndVisibility.first);
    }
}


void OrthographicCamera::_addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap, int level, float viewExpand, bool useEmplace) {
    Vec3 forward(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));
    Vec3 right = VecMath::normalize(Vec3(forward.z, 0, -forward.x));
    Vec3 up = VecMath::cross(forward, right);

    float w = viewport[2], h = viewport[3];
    int width = (int) ceil(w / ChunkPos::CHUNK_SIZE + viewExpand);
    int height = (int) ceil(h / ChunkPos::CHUNK_SIZE + viewExpand);

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

    for (int y = 0; y <= height; y++) {
        for (int x = 0; x <= width; x++) {
            Vec3 ray_start = position + forward * (near - ChunkPos::CHUNK_SIZE) +
                             right * ((float(x) - float(width) / 2) * ChunkPos::CHUNK_SIZE) +
                             up * ((float(y) - float(height) / 2) * ChunkPos::CHUNK_SIZE);

            Vec3 position_f = ray_start / ChunkPos::CHUNK_SIZE;
            Vec3i position = VecMath::floor_to_int(position_f);

            Vec3 rayL(
                    (forward.x > 0 ? (float(position.x) + 1 - position_f.x) : (position_f.x - float(position.x))) * rayS.x,
                    (forward.y > 0 ? (float(position.y) + 1 - position_f.y) : (position_f.y - float(position.y))) * rayS.y,
                    (forward.z > 0 ? (float(position.z) + 1 - position_f.z) : (position_f.z - float(position.z))) * rayS.z
            );

            float distance = 0;
            float maxDistance = (far - near) / ChunkPos::CHUNK_SIZE;
            while (distance < maxDistance) {
                if (useEmplace) {
                    visibilityMap.emplace(ChunkPos(position.x, position.y, position.z), level);
                } else {
                    visibilityMap[ChunkPos(position.x, position.y, position.z)] = level;
                }
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
}

void OrthographicCamera::addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap) {
    _addAllVisiblePositions(visibilityMap, RenderChunk::VISIBILITY_LEVEL_NEAR_VIEW, 3, true);
    _addAllVisiblePositions(visibilityMap, RenderChunk::VISIBILITY_LEVEL_VISIBLE, 0, false);

    /*
    std::cout << "visibility debug (y = 0, -5 <= x,z <= 5): \n";
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
