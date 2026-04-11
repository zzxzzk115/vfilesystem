#pragma once

#include "vfilesystem/core/path.hpp"
#include "vfilesystem/interfaces/ifilesystem.hpp"

#include <cstdint>
#include <memory>
#include <optional>

namespace vfilesystem
{
    // Semantic roots that dispatch to platform-specific filesystem handling.
    enum class PlatformFileSystemKind : uint8_t
    {
        eAssets = 0,
        eWritable,
    };

    struct PlatformFileSystemOptions
    {
        // Forces a rooted PhysicalFileSystem on every platform.
        std::optional<Path> rootOverride;

        // Android-only: pass the native AAssetManager* as an opaque pointer.
        // Ignored on non-Android builds.
        void* androidAssetManager {nullptr};
    };

    vbase::Result<std::shared_ptr<IFileSystem>, FsError> makePlatformFileSystem(
        PlatformFileSystemKind            kind,
        const PlatformFileSystemOptions& options = {});
} // namespace vfilesystem
