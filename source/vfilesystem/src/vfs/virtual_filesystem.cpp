#include "vfilesystem/vfs/virtual_filesystem.hpp"

#include <ranges>

namespace vfilesystem
{
    namespace
    {
        std::string format_virtual_path(const std::string& scheme, vbase::StringView path)
        {
            if (scheme.empty())
                return std::string(path);
            return scheme + "://" + std::string(path);
        }
    } // namespace

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

    vbase::Result<void, FsError> VirtualFileSystem::createDirectory(vbase::StringView uri)
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        return mp->fs->createDirectory(rel.str());
    }

    vbase::Result<void, FsError> VirtualFileSystem::createDirectories(vbase::StringView uri)
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        return mp->fs->createDirectories(rel.str());
    }

    vbase::Result<void, FsError> VirtualFileSystem::removeFile(vbase::StringView uri)
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        return mp->fs->removeFile(rel.str());
    }

    vbase::Result<void, FsError> VirtualFileSystem::removeDirectory(vbase::StringView uri, bool recursive)
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        return mp->fs->removeDirectory(rel.str(), recursive);
    }

    vbase::Result<void, FsError> VirtualFileSystem::rename(vbase::StringView from, vbase::StringView to)
    {
        const Uri fromUri = parse_uri(from);
        const Uri toUri   = parse_uri(to);

        const MountPoint* fromMp = nullptr;
        const MountPoint* toMp   = nullptr;
        Path              fromRel;
        Path              toRel;

        if (!resolve(fromUri, fromMp, fromRel) || !resolve(toUri, toMp, toRel))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        if (fromMp->fs.get() != toMp->fs.get())
            return vbase::Result<void, FsError>::err(FsError::eNotSupported);

        return fromMp->fs->rename(fromRel.str(), toRel.str());
    }

    vbase::Result<std::vector<DirectoryEntry>, FsError> VirtualFileSystem::list(vbase::StringView uri) const
    {
        Uri uriParsed = parse_uri(uri);

        const MountPoint* mp = nullptr;
        Path              rel;

        if (!resolve(uriParsed, mp, rel))
            return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eNotFound);

        auto entries = mp->fs->list(rel.str());
        if (!entries)
            return entries;

        for (auto& entry : entries.value())
            entry.path = format_virtual_path(uriParsed.scheme, entry.path);

        return entries;
    }
} // namespace vfilesystem
