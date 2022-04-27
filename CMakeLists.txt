# Project definition
cmake_minimum_required(VERSION 3.1)
project(voxel_engine)

# flags
SET(CMAKE_CXX_FLAGS, "${CMAKE_CXX_FLAGS} -Wno-register")


# Source files
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/source")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libraries")

file(GLOB_RECURSE SOURCES
        "${SRC_DIR}/**.h"
        "${SRC_DIR}/**.cc"
        )

# Executable definition and properties
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "${SRC_DIR}")
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# GLFW
set(GLFW_DIR "${LIB_DIR}/glfw")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")
add_subdirectory("${GLFW_DIR}")
target_link_libraries(${PROJECT_NAME} "glfw" "${GLFW_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

# glad
set(GLAD_DIR "${LIB_DIR}/glad")
add_library("glad" "${GLAD_DIR}/src/glad.c")
target_include_directories("glad" PRIVATE "${GLAD_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLAD_DIR}/include")
target_link_libraries(${PROJECT_NAME} "glad" "${CMAKE_DL_LIBS}")

# glfw
set(GLM_DIR "${LIB_DIR}/glm")
# add_subdirectory("${GLM_DIR}")
# target_link_libraries(${PROJECT_NAME} "glm")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLM_DIR}")

# json cpp
set(JSON_CPP_DIR "${LIB_DIR}/jsoncpp")
add_subdirectory("${JSON_CPP_DIR}")
target_link_libraries(${PROJECT_NAME} "jsoncpp_static")
target_include_directories(${PROJECT_NAME} PRIVATE "${JSONCPP_INCLUDE_DIR}")

# parallel hashmap
target_include_directories(${PROJECT_NAME} PRIVATE "${LIB_DIR}/phmap")
