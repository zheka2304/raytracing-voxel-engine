#include "strings.h"

#include <memory>
#include <cstring>


namespace voxel {
namespace utils {

std::string stringFormatVarargs(const std::string fmt_str, va_list args) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_copy(ap, args);
        final_n = vsnprintf(&formatted[0], (size_t) n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

std::string stringFormat(const std::string fmt_str, ...) {
    va_list va;
    va_start(va, fmt_str);
    std::string result = stringFormatVarargs(fmt_str, va);
    va_end(va);
    return result;
}

} // utils
} // voxel
