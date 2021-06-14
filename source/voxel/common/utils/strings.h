#ifndef VOXEL_ENGINE_STRINGS_H
#define VOXEL_ENGINE_STRINGS_H

#include <string>
#include <stdarg.h>

namespace voxel {
namespace utils {

std::string stringFormatVarargs(const std::string fmt_str, va_list args);
std::string stringFormat(const std::string fmt_str, ...);

} // utils
} // voxel

#endif //VOXEL_ENGINE_STRINGS_H
