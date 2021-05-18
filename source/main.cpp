/**
 * This is a terrible mess of some tests & dirty initialization, most of that will be
 * moved to VoxelEngine class in the future.
 */

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <sstream>

#include "common/simple-profiler.h"
#include "engine/voxel_chunk.h"
#include "renderer/render_engine.h"
#include "renderer/context.h"


long long get_time_milliseconds() {
    using namespace std::chrono;
    milliseconds ms = duration_cast< milliseconds >(
            system_clock::now().time_since_epoch()
    );
    return ms.count();
}

float get_time_since_start() {
    static long long start = 0;
    if (start == 0) {
        start = get_time_milliseconds();
    }
    return float(get_time_milliseconds() - start) / 1000.0f;
}


unsigned int int_hash(unsigned int i) {
    i ^= 2747636419u;
    i *= 2654435769u;
    i ^= i >> 16;
    i *= 2654435769u;
    i ^= i >> 16;
    i *= 2654435769u;
    return i;
}

float pos_rand(int x, int z) {
    return float(int_hash(x) ^ int_hash(int_hash(z))) / float(0xFFFFFFFFu);
}

float smooth_rand(float x, float z) {
    float flx = floor(x);
    float flz = floor(z);
    int ix = int(flx);
    int iz = int(flz);
    float r00 = pos_rand(ix, iz);
    float r01 = pos_rand(ix, iz + 1);
    float r10 = pos_rand(ix + 1, iz);
    float r11 = pos_rand(ix + 1, iz + 1);
    float fx = x - flx;
    float fz = z - flz;
    return (r00 * (1 - fz) + r01 * fz) * (1 - fx) + (r10 * (1 - fz) + r11 * fz) * fx;
}



class DebugChunkHandler : public ChunkHandler {
public:
    bool isValidPosition(ChunkPos const& pos) override {
        return pos.y == 0;
    }

    bool requestChunk(VoxelChunk& chunk) override {
        for (int x = 0; x < 128; x++) {
            for (int z = 0; z < 128; z++) {
                float fx = (x + chunk.position.x * ChunkPos::CHUNK_SIZE) / 8.0f;
                float fz = (z + chunk.position.z * ChunkPos::CHUNK_SIZE) / 8.0f;
                float r = smooth_rand(fx, fz) * 0.125 +
                        smooth_rand(fx / 2.0f, fz / 2.0f) * 0.25 +
                        smooth_rand(fx / 4.0f, fz / 4.0f) * 0.5;
                r = float(abs(x / 4 - 16) < 6 && abs(z / 4 - 16) < 6 && (chunk.position.x + chunk.position.z) % 2);
                // r = 1.0 - (std::max(abs(x - 64), abs(z - 64))) / 64.0f;
                float h = 12 + 48 * r;
                for (int y = 0; y < 128; y++) {
                    int dx = x - 64;
                    int dy = y - 64;
                    int dz = z - 64;
                    // int d = dx * dx + dy * dy + dz * dz;
                    // int c = (y == 0 || y == 127) + (x == 0 || x == 127) + (z == 0 || z == 127);
                     chunk.voxelBuffer[x + (z + y * 128) * 128] = y < h; //(dx * dx + dy * dy + dz * dz) < 32 * 32;
                }
            }
        }
        return true;
    }

    void bakeChunk(VoxelChunk& chunk) override {
        // PROFILER_BEGIN0(bake_chunk)
        // chunk.rebuildRenderBuffer();
    }

    Vec2i getBakeLockRadius() override {
        return Vec2i(0, 0);
    }

    void unloadChunk(VoxelChunk& chunk) override {
        std::cout << "unload chunk " << chunk.position.x << " " << chunk.position.y << " " << chunk.position.z << "\n";
    }
};


int main(int argc, char* argv[]) {

    {
        WorkerThread* testBackgroundWorkingThread = new WorkerThread();

        std::shared_ptr<ChunkSource> chunkSource = std::make_shared<ThreadedChunkSource>(
                std::make_shared<DebugChunkHandler>(), 3, 5000);

        std::shared_ptr<Camera> camera = std::make_shared<OrthographicCamera>();

        // task, that will loop in background
        int workerTaskFrame = 0;
        std::function<void()> workerTask = [&workerTaskFrame, &workerTask, &testBackgroundWorkingThread, &chunkSource] () -> void {
            VoxelChunk* chunk = chunkSource->getChunkAt(ChunkPos(0, 0, 0));
            if (chunk != 0) {
                std::unique_lock<std::timed_mutex> contentLock(chunk->getContentMutex());
                int v = workerTaskFrame % 2;
                for (int x = 64; x < 80; x++) {
                    for (int y = 64; y < 80; y++) {
                        for (int z = 64; z < 80; z++) {
                            chunk->voxelBuffer[x + (z + y * 128) * 128] = v;
                        }
                    }
                }
                contentLock.unlock();
                chunk->queueRegionUpdate(4, 4, 4);
            }
            // std::this_thread::sleep_for(std::chrono::milliseconds(50));
            workerTaskFrame++;
            testBackgroundWorkingThread->queue(workerTask);
        };
        testBackgroundWorkingThread->queue(workerTask);

        // ---- open window & init gl ----

        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return -1;
        }


        std::shared_ptr<VoxelEngine> voxelEngine = std::make_shared<VoxelEngine>(WindowParams({ "Fuck", 480 * 3, 270 * 3 }));
        GLFWwindow* window = voxelEngine->getWindow();
        voxelEngine->windowContext.makeCurrent();

        int gladInitRes = gladLoadGL();
        if (!gladInitRes) {
            fprintf(stderr, "Unable to initialize glad\n");
            glfwTerminate();
            return 0;
        }

        glfwSwapInterval(1);
        glfwShowWindow(window);

        gl::Mesh mesh;
        float r = 1920 / 1080.0;
        mesh.vertices.push_back({-1, -1, -1, 1, 0, 0, 1, 0, 0});
        mesh.vertices.push_back({1, -1, -1, 1, 1, 0, 1, 1, 0});
        mesh.vertices.push_back({1, 1, -1, 1, 0, 1, 1, 1, 1});
        mesh.vertices.push_back({-1, -1, -1, 1, 0, 0, 1, 0, 0});
        mesh.vertices.push_back({1, 1, -1, 1, 0, 1, 1, 1, 1});
        mesh.vertices.push_back({-1, 1, -1, 1, 1, 0, 1, 0, 1});
        mesh.rebuild();

        std::cout << "chunk region tier 1 size " << VoxelChunk::SUB_REGION_BUFFER_SIZE_1 << "\n";
        std::cout << "chunk region tier 2 size " << VoxelChunk::SUB_REGION_BUFFER_SIZE_2 << "\n";
        std::cout << "chunk total region size " << VoxelChunk::SUB_REGION_TOTAL_SIZE << "\n";
        std::cout << "bytes per chunk " << (VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE +
                                            ChunkPos::CHUNK_SIZE * ChunkPos::CHUNK_SIZE * ChunkPos::CHUNK_SIZE) *
                                           sizeof(unsigned int) << "\n";

        // init shader and uniforms
        gl::Shader textureShader("texture.vert", "process_soft_shadow.frag",
                                 {"HIGH_QUALITY_SHADOWS0", "SOFT_SHADOWS0"});

        VoxelRenderEngine renderEngine(voxelEngine, chunkSource, camera, {480 * 2, 270 * 2, 3});

        // start
        float posX = 0, posY = 64, posZ = 0, cameraYaw = 3.1415 / 4;

        int frame = 0;
        float last_fps_time = get_time_since_start();
        while (!glfwWindowShouldClose(window)) {
            glClearColor(1.0, 0.0, 1.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);

            camera->setPosition(Vec3(posX, posY, posZ));
            camera->setRotation(cameraYaw, -3.1415f / 4);
            camera->setViewport(0, 0, 480 / 2, 270 / 2);

            renderEngine.updateVisibleChunks();
            renderEngine.render();
            // glMemoryBarrier(GL_ALL_BARRIER_BITS);
            renderEngine.runQueuedRenderChunkUpdates(16);
            if (frame % 10 == 0) {
                camera->requestChunksFromSource(chunkSource);
            }
            if (frame % 30 == 0) {
                chunkSource->startUnload();
            }

            float unitMove = 10.0, unitRotation = -0.1;
            if (glfwGetKey(window, GLFW_KEY_Q)) {
                cameraYaw -= unitRotation;
            }
            if (glfwGetKey(window, GLFW_KEY_E)) {
                cameraYaw += unitRotation;
            }
            if (glfwGetKey(window, GLFW_KEY_D)) {
                posX += unitMove * sin(cameraYaw);
                posZ -= unitMove * cos(cameraYaw);
            }
            if (glfwGetKey(window, GLFW_KEY_A)) {
                posX -= unitMove * sin(cameraYaw);
                posZ += unitMove * cos(cameraYaw);
            }
            if (glfwGetKey(window, GLFW_KEY_W)) {
                posX += unitMove * cos(cameraYaw);
                posZ += unitMove * sin(cameraYaw);
            }
            if (glfwGetKey(window, GLFW_KEY_S)) {
                posX -= unitMove * cos(cameraYaw);
                posZ -= unitMove * sin(cameraYaw);
            }

            textureShader.use();
            glEnable(GL_TEXTURE_2D);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, renderEngine.o_ColorTexture.handle);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, renderEngine.o_LightTexture.handle);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, renderEngine.o_DepthTexture.handle);
            glUniform1i(textureShader.getUniform("TEXTURE_0"), 0);
            glUniform1i(textureShader.getUniform("TEXTURE_1"), 1);
            glUniform1i(textureShader.getUniform("TEXTURE_2"), 2);
            glUniform2f(textureShader.getUniform("BLEND_RADIUS"), 0.5f / 480.0f, 0.5f / 270.0f);
            mesh.render();

            glfwSwapBuffers(window);
            glfwPollEvents();

            frame++;
            if (frame % 50 == 0) {
                float time = get_time_since_start();
                std::stringstream ss;
                ss << "Fps: " << (50 / (time - last_fps_time)) << "\n";
                last_fps_time = time;
                glfwSetWindowTitle(window, ss.str().c_str());
            }
        }

        delete(testBackgroundWorkingThread);
    }

    glfwTerminate();

    return 0;
}
