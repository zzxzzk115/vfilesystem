#pragma once

#include <cstdint>

namespace vfilesystem
{
    enum class FileMode : uint8_t
    {
        eRead,
        eWrite,
        eAppend,
        eReadWrite,
    };
} // namespace vfilesystem
