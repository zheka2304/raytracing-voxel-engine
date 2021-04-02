
#ifndef VOXEL_ENGINE_CAMERA_H
#define VOXEL_ENGINE_CAMERA_H

#include <unordered_map>
#include <memory>

#include "common/vec.h"
#include "common/chunk_pos.h"
#include "util.h"


class ChunkSource;

class Camera {
public:
    Vec3 position = Vec3(0, 0, 0);
    float yaw = 0, pitch = 0;
    float near = -64, far = 256;
    float viewport[4] = { 0 };

    void setPosition(Vec3 const& position);
    void setRotation(float yaw, float pitch);
    void setViewport(float x, float y, float w, float h);
    void setNearAndFarPlane(float near, float far);

    // iterates over all visible chunk positions
    virtual void addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap);
    virtual void sendParametersToShader(gl::Shader& shader);

    // checks current visibility and requests chunks from source
    virtual void requestChunksFromSource(std::shared_ptr<ChunkSource> chunkSource);
};

class OrthographicCamera : public Camera {
private:
    void _addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap, int level, float viewExpand, bool useEmplace);
public:
    void addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap) override;

};


#endif //VOXEL_ENGINE_CAMERA_H
