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

        vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView p, FileMode mode) override;

    private:
        Path        m_Root;
        std::string toOSPath(const Path& p) const;
    };

} // namespace vfilesystem
