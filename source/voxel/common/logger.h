#ifndef VOXEL_ENGINE_LOGGER_H
#define VOXEL_ENGINE_LOGGER_H

#include <string>
#include <stdarg.h>
#include "voxel/common/base.h"


namespace voxel {

class Logger {

public:
    enum Flag : u32 {
        flag_debug = 1 << 0,
        flag_info = 1 << 1,
        flag_warning = 1 << 2,
        flag_error = 1 << 3,
        flag_critical = 1 << 4,
    };

    Logger();
    Logger(Logger const& other);
    Logger(Logger&& other);

    void begin(const std::string& section);
    void message(u32 flags, std::string tag, std::string message, ...);
    void end();

};

} // voxel

#endif //VOXEL_ENGINE_LOGGER_H
