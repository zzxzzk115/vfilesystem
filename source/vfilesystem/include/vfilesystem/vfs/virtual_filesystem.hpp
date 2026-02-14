#pragma once

#include "vfilesystem/core/uri.hpp"
#include "vfilesystem/interfaces/ifilesystem.hpp"
#include "vfilesystem/vfs/mount_point.hpp"

#include <memory>
#include <string>
#include <vector>

namespace vfilesystem
{
    class VirtualFileSystem final : public IFileSystem
    {
    public:
        VirtualFileSystem() = default;

        // Mount with optional scheme (empty = default)
        void mount(std::shared_ptr<IFileSystem> backend, std::string scheme = "vfs");

        bool exists(vbase::StringView p) const override;
        bool isFile(vbase::StringView p) const override;
        bool isDirectory(vbase::StringView p) const override;

        // URI-based open (vfs://, mem://, disk://)
        vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView uri, FileMode mode) override;

    private:
        std::vector<MountPoint> m_Mounts;

        bool resolve(const Uri& uri, const MountPoint*& outMp, Path& outRel) const;
    };
} // namespace vfilesystem
