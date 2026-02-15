#include "vfilesystem/vfs/virtual_filesystem.hpp"

#include <ranges>

namespace vfilesystem
{
    static bool prefix_match(vbase::StringView path, vbase::StringView prefix)
    {
        if (prefix.empty())
            return true;
        if (path.size() < prefix.size())
            return false;
        if (path.substr(0, prefix.size()) != prefix)
            return false;
        if (path.size() == prefix.size())
            return true;
        return path[prefix.size()] == '/';
    }

    void VirtualFileSystem::mount(std::shared_ptr<IFileSystem> backend, std::string scheme)
    {
        MountPoint mp;
        mp.scheme = std::move(scheme);
        mp.fs     = std::move(backend);
        m_Mounts.push_back(std::move(mp));
    }

    bool VirtualFileSystem::resolve(const Uri& uri, const MountPoint*& outMp, Path& outRel) const
    {
        const MountPoint* best    = nullptr;
        size_t            bestLen = 0;

        for (const auto& mountPoint : std::ranges::reverse_view(m_Mounts))
        {
            if (mountPoint.scheme != uri.scheme)
                continue;

            best = &mountPoint;
            break;
        }

        if (!best)
            return false;

        outMp  = best;
        outRel = uri.path;
        return true;
    }

    vbase::Result<std::unique_ptr<IFile>, FsError> VirtualFileSystem::open(vbase::StringView uri, FileMode mode)
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);

        return mp->fs->open(rel.str(), mode);
    }

    bool VirtualFileSystem::exists(vbase::StringView uri) const
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return false;

        return mp->fs->exists(rel.str());
    }

    bool VirtualFileSystem::isFile(vbase::StringView uri) const
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return false;

        return mp->fs->isFile(rel.str());
    }

    bool VirtualFileSystem::isDirectory(vbase::StringView uri) const
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return false;

        return mp->fs->isDirectory(rel.str());
    }
} // namespace vfilesystem
