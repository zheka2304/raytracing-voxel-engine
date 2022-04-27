# Voxel Raytracer

SVO voxel raytracer, built with OpenGL and C++, focused mostly on optimizing lighting right now. 
Operates on chunked worlds, generated or loaded in real time, so it supports both loading voxel models and procedural endless world generation.

# Data structure

World consists of cubic chunks (each considered to be unit size), each is a root of [sparse voxel octree](https://en.wikipedia.org/wiki/Sparse_voxel_octree). 
Chunks are stored in map, and pointers to visible chunks are stored in dense grid in GPU memory.

# Lighting

Lighting supports soft shadows and colored light from multiple light sources, using only one random ray casted each frame per pixel. 
Optimizing lighting is one of the main goals of this project. In a nutshell, this is achieved by tracking movement of pixels on screen and accumulating light value each frame, 
then applying additional filtering to the result. Pixel tracking done by creating spatial buffer, that stores all pixel positions in world space.

# Build & Requirements

Built with CMake, requires C++17. OpenGL version must be at least 4.2. For the executable to load everything and run correctly, the working directory must be set to the root of the project.

# Screenshots

![figure 1](https://github.com/zheka2304/raytracing-voxel-engine/blob/master/assets/screenshots/2.png?raw=true)

![figure 2](https://github.com/zheka2304/raytracing-voxel-engine/blob/master/assets/screenshots/3.png?raw=true)
