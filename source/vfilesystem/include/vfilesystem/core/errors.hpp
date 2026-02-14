#pragma once

#include <cstdint>

namespace vfilesystem
{
    enum class FsError : uint8_t
    {
        eOk = 0,
        eNotFound,
        ePermissionDenied,
        eInvalidPath,
        eIOError,
        eAlreadyExists,
        eNotSupported,
    };
} // namespace vfilesystem
