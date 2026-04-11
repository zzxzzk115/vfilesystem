#pragma once

#include "vfilesystem/core/path.hpp"
#include "vfilesystem/interfaces/ifilesystem.hpp"

namespace vfilesystem
{
    // Rooted OS filesystem backend.
    // All paths passed to this backend are treated as relative to `root`.
    class PhysicalFileSystem final : public IFileSystem
    {
    public:
        explicit PhysicalFileSystem(Path root = Path {"."});

        bool exists(vbase::StringView p) const override;
        bool isFile(vbase::StringView p) const override;
        bool isDirectory(vbase::StringView p) const override;

        vbase::Result<void, FsError> createDirectory(vbase::StringView p) override;
        vbase::Result<void, FsError> createDirectories(vbase::StringView p) override;
        vbase::Result<void, FsError> removeFile(vbase::StringView p) override;
        vbase::Result<void, FsError> removeDirectory(vbase::StringView p, bool recursive) override;
        vbase::Result<void, FsError> rename(vbase::StringView from, vbase::StringView to) override;
        vbase::Result<std::vector<DirectoryEntry>, FsError> list(vbase::StringView p) const override;

        vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView p, FileMode mode) override;

        std::string getFullPath(vbase::StringView p) const { return toOSPath(Path {p}); }

    private:
        Path        m_Root;
        std::string toOSPath(const Path& p) const;
    };

} // namespace vfilesystem
