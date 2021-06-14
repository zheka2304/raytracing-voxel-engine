#include "voxel/common/logger.h"

int main() {
    voxel::Logger logger;
    logger.message(voxel::Logger::flag_debug, "MAIN", "hello %s %i", "world", 12345);
    return 0;
}