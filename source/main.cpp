#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <stdio.h>
#include <iostream>
#include <chrono>

#include "common/simple-profiler.h"
#include "engine/voxel_chunk.h"
#include "renderer/render_engine.h"


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


void generatorThread() {

}


int main(int argc, char* argv[]) {

    VoxelChunk baseChunk;
    std::cout << "building chunk\n";
    for (int x = 0; x < 128; x++) {
        for (int z = 0; z < 128; z++) {
            float r00 = pos_rand(x / 16, z / 16);
            float r01 = pos_rand(x / 16, z / 16 + 1);
            float r10 = pos_rand(x / 16 + 1, z / 16);
            float r11 = pos_rand(x / 16 + 1, z / 16 + 1);
            float fx = float(x % 16) / 16;
            float fz = float(z % 16) / 16;
            float r = (r00 * (1 - fz) + r01 * fz) * (1 - fx) + (r10 * (1 - fz) + r11 * fz) * fx;
            float h =  r * 32 + 8;
            for (int y = 0; y < 128; y++) {
                int dx = x - 64;
                int dy = y - 64;
                int dz = z - 64;
                int d = dx * dx + dy * dy + dz * dz;
                int c = (y == 0 || y == 127) + (x == 0 || x == 127) + (z == 0 || z == 127);
                int v = baseChunk.voxelBuffer[x + (z + y * 128) * 128] = y < h;
            }
        }
    }
    baseChunk.rebuildRenderBuffer();
    std::cout << "complete\n";

    std::shared_ptr<ChunkSource> chunkSource = std::make_shared<DebugChunkSource>([&] (ChunkPos pos) -> VoxelChunk* {
        if (pos.y == 0) {
            return new VoxelChunk(baseChunk);
        }
        return nullptr;
    });

    /*
    std::shared_ptr<ChunkSource> chunkSource = std::make_shared<ThreadedChunkSource>([&] (VoxelChunk* chunk) -> bool {
        if (chunk->position.y == 0) {
            chunk->copyFrom(baseChunk);
            return true;
        }
        return true;
    }, 4);*/

    std::shared_ptr<Camera> camera = std::make_shared<OrthographicCamera>();


    // ---- open window & init gl ----

    if(!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwDefaultWindowHints();

    GLFWwindow* window = glfwCreateWindow(480 * 3, 270 * 3, "Fuck",
                                          nullptr, nullptr);
    if(window == nullptr) {
        std::cerr << "Failed to open GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    int gladInitRes = gladLoadGL();
    if (!gladInitRes) {
        fprintf(stderr, "Unable to initialize glad\n");
        glfwDestroyWindow(window);
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
    std::cout << "bytes per chunk " << (VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE + ChunkPos::CHUNK_SIZE * ChunkPos::CHUNK_SIZE * ChunkPos::CHUNK_SIZE) * sizeof(unsigned int) << "\n";

    // init shader and uniforms
    gl::Shader raytraceShader("raytrace", std::vector<std::string>({"RAYTRACE_DEPTH" }));
    gl::Shader textureShader("texture.vertex", "process_soft_shadow.fragment", { "SOFT_SHADOWS", "HIGH_QUALITY_SHADOWS" });

    VoxelRenderEngine renderEngine(chunkSource, camera);

    gl::RenderToTexture renderToTexture(480 * 1, 270 * 1, 3);

    // start
    float posX = 0, posY = 64, posZ = 0, cameraYaw = 3.1415 / 4;

    int f = 5;
    int frame = 0;
    float last_fps_time = get_time_since_start();
    while(!glfwWindowShouldClose(window)) {
        glClearColor(1.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        raytraceShader.use();

        camera->setPosition(Vec3(posX, posY, posZ));
        camera->setRotation(cameraYaw, -3.1415f / 4);
        camera->setViewport(0, 0, 480 / 2, 270 / 2);

        if (f > 0) {
            renderEngine.updateVisibleChunks();
            renderEngine.prepareForRender(raytraceShader);
            if (frame % 10 == 0) {
                camera->requestChunksFromSource(chunkSource);
            }

        }

        camera->sendParametersToShader(raytraceShader);
        glUniform1f(raytraceShader.getUniform("TIME"), get_time_since_start());
        glUniform4f(raytraceShader.getUniform("DIRECT_LIGHT_COLOR"), 0.7, 0.7, 1.0, 1.0);
        glUniform4f(raytraceShader.getUniform("AMBIENT_LIGHT_COLOR"), 0.1, 0.0, 0.0, 0.2);
        glUniform3f(raytraceShader.getUniform("DIRECT_LIGHT_RAY"), 1.0, -0.4, 1.0);

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

        renderToTexture.startRenderToTexture();
        renderToTexture.drawFullScreenQuad();
        renderToTexture.endRenderToTexture();
        textureShader.use();

        glViewport(0, 0, 480 * 3, 270 * 3);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderToTexture.outputTextures[0].handle);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, renderToTexture.outputTextures[1].handle);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, renderToTexture.outputTextures[2].handle);
        glUniform1i(textureShader.getUniform("TEXTURE_0"), 0);
        glUniform1i(textureShader.getUniform("TEXTURE_1"), 1);
        glUniform1i(textureShader.getUniform("TEXTURE_2"), 2);
        glUniform2f(textureShader.getUniform("BLEND_RADIUS"), 2 / 480.0f, 2 / 270.0f);
        renderToTexture.drawFullScreenQuad();


        glfwSwapBuffers(window);
        glfwPollEvents();

        frame++;
        if (frame % 100 == 0) {
            float time = get_time_since_start();
            std::cout << "fps: " << (100 / (time - last_fps_time)) << "\n";
            last_fps_time = time;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
