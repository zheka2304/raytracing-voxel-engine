
#ifndef VOXEL_ENGINE_CAMERA_H
#define VOXEL_ENGINE_CAMERA_H

#include <unordered_map>

#include "common/vec.h"
#include "common/chunk_pos.h"
#include "util.h"


class Camera {
public:
    Vec3 position = Vec3(0, 0, 0);
    float yaw = 0, pitch = 0;
    float viewport[4] = { 0 };

    void setPosition(Vec3 const& position);
    void setRotation(float yaw, float pitch);
    void setViewport(float x, float y, float w, float h);

    // iterates over all visible chunk positions
    virtual void addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap);
    virtual void sendParametersToShader(gl::Shader& shader);

};

class OrthographicCamera : public Camera {
public:
    float near = -128, far = 384;

    void setNearAndFarPlane(float near, float far);

private:
    void _addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap, int level, float viewExpand, bool useEmplace);
public:
    void addAllVisiblePositions(std::unordered_map<ChunkPos, int>& visibilityMap) override;

};


#endif //VOXEL_ENGINE_CAMERA_H
