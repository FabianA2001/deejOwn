#include "util/PathUtils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace winaudiomixer {

    std::filesystem::path getExecutableDir()
    {
#ifdef _WIN32
        wchar_t buffer[MAX_PATH];
        const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        if (length == 0 || length == MAX_PATH) {
            return std::filesystem::current_path();
        }
        return std::filesystem::path(buffer).parent_path();
#else
        char buffer[PATH_MAX];
        const ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
        if (length <= 0) {
            return std::filesystem::current_path();
        }
        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
#endif
    }

} // namespace winaudiomixer