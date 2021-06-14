#include "logger.h"

#include <iostream>
#include <stdarg.h>

#include "voxel/common/utils/strings.h"


namespace voxel {
    Logger::Logger() {

    }

    Logger::Logger(const Logger& other) {
        // TODO: implement shared log storage and passing logger by value
    }

    Logger::Logger(Logger&& other) {

    }

    void Logger::begin(const std::string& section) {
        // TODO: implement log sections
    }

    void Logger::end() {
        // TODO: implement log sections
    }

    void Logger::message(uint32_t flags, std::string tag, std::string message, ...) {
        va_list va;
        va_start(va, message);
        std::string formatted_message = voxel::utils::stringFormatVarargs(message, va);
        va_end(va);

        // TODO: temporary solution, implement log messages
        if (flags & flag_error) {
            std::cerr << '[' << tag << "] " << formatted_message << "\n";
        } else {
            std::cout << '[' << tag << "] " << formatted_message << "\n";
        }
    }

} // voxel