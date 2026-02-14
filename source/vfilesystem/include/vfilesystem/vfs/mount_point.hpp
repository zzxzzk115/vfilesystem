#pragma once

#include <memory>
#include <string>

namespace vfilesystem
{
    class IFileSystem;

    struct MountPoint
    {
        std::string                  scheme;
        std::shared_ptr<IFileSystem> fs;
    };
} // namespace vfilesystem
