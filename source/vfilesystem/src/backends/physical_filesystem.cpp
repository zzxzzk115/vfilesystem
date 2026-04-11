#include "vfilesystem/backends/physical_filesystem.hpp"
#include "vfilesystem/interfaces/ifile.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace vfilesystem
{
    namespace
    {
        FsError map_fs_error(const std::error_code& ec)
        {
            using std::errc;

            if (ec == errc::no_such_file_or_directory)
                return FsError::eNotFound;
            if (ec == errc::permission_denied)
                return FsError::ePermissionDenied;
            if (ec == errc::file_exists)
                return FsError::eAlreadyExists;
            if (ec == errc::directory_not_empty)
                return FsError::eDirectoryNotEmpty;
            return FsError::eIOError;
        }

        bool is_write_mode(FileMode mode)
        {
            return mode == FileMode::eWrite || mode == FileMode::eAppend || mode == FileMode::eReadWrite;
        }
    } // namespace

    class PhysicalFile final : public IFile
    {
    public:
        explicit PhysicalFile(std::fstream f) : m_F(std::move(f)) {}

        size_t read(void* dst, size_t size) override
        {
            m_F.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(size));
            return static_cast<size_t>(m_F.gcount());
        }

        size_t write(const void* src, size_t size) override
        {
            m_F.write(reinterpret_cast<const char*>(src), static_cast<std::streamsize>(size));
            return m_F ? size : 0;
        }

        bool seek(uint64_t offset) override
        {
            m_F.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
            m_F.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
            return static_cast<bool>(m_F);
        }

        uint64_t tell() const override
        {
            auto p = m_F.tellg();
            if (p < 0)
                return 0;
            return static_cast<uint64_t>(p);
        }

        uint64_t size() const override
        {
            auto cur = m_F.tellg();
            m_F.seekg(0, std::ios::end);
            auto end = m_F.tellg();
            m_F.seekg(cur, std::ios::beg);
            if (end < 0)
                return 0;
            return static_cast<uint64_t>(end);
        }

        std::vector<std::byte> readAllBytes() override
        {
            auto                   sz = size();
            std::vector<std::byte> out;
            out.resize(static_cast<size_t>(sz));
            if (sz > 0)
            {
                m_F.seekg(0, std::ios::beg);
                m_F.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(sz));
                if (!m_F)
                    out.clear();
            }
            return out;
        }

    private:
        mutable std::fstream m_F;
    };

    static std::ios::openmode to_mode(FileMode m)
    {
        switch (m)
        {
            case FileMode::eRead:
                return std::ios::in | std::ios::binary;
            case FileMode::eWrite:
                return std::ios::out | std::ios::binary | std::ios::trunc;
            case FileMode::eAppend:
                return std::ios::out | std::ios::binary | std::ios::app;
            case FileMode::eReadWrite:
                return std::ios::in | std::ios::out | std::ios::binary;
            default:
                return std::ios::in | std::ios::binary;
        }
    }

    PhysicalFileSystem::PhysicalFileSystem(Path root) : m_Root(std::move(root)) {}

    std::string PhysicalFileSystem::toOSPath(const Path& p) const
    {
        auto sv = p.str();
        if (!sv.empty() && sv.front() == '/')
            sv = vbase::StringView {sv.data() + 1, sv.size() - 1};
        return std::string(m_Root.join(sv).str().data(), m_Root.join(sv).str().size());
    }

    bool PhysicalFileSystem::exists(vbase::StringView p) const { return std::filesystem::exists(toOSPath(Path(p))); }
    bool PhysicalFileSystem::isFile(vbase::StringView p) const
    {
        return std::filesystem::is_regular_file(toOSPath(Path(p)));
    }
    bool PhysicalFileSystem::isDirectory(vbase::StringView p) const
    {
        return std::filesystem::is_directory(toOSPath(Path(p)));
    }

    vbase::Result<void, FsError> PhysicalFileSystem::createDirectory(vbase::StringView p)
    {
        Path path {p};
        if (path.empty() || path.isRoot())
            return vbase::Result<void, FsError>::ok();

        const std::filesystem::path os = toOSPath(path);
        if (std::filesystem::exists(os))
        {
            if (std::filesystem::is_directory(os))
                return vbase::Result<void, FsError>::ok();
            return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);
        }

        const auto parent = os.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        std::error_code ec;
        std::filesystem::create_directory(os, ec);
        if (ec)
            return vbase::Result<void, FsError>::err(map_fs_error(ec));

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> PhysicalFileSystem::createDirectories(vbase::StringView p)
    {
        Path path {p};
        if (path.empty() || path.isRoot())
            return vbase::Result<void, FsError>::ok();

        const std::filesystem::path os = toOSPath(path);
        if (std::filesystem::exists(os))
        {
            if (std::filesystem::is_directory(os))
                return vbase::Result<void, FsError>::ok();
            return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);
        }

        std::error_code ec;
        std::filesystem::create_directories(os, ec);
        if (ec)
            return vbase::Result<void, FsError>::err(map_fs_error(ec));

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> PhysicalFileSystem::removeFile(vbase::StringView p)
    {
        const std::filesystem::path os = toOSPath(Path {p});
        if (!std::filesystem::exists(os))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        if (std::filesystem::is_directory(os))
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);

        std::error_code ec;
        const bool      removed = std::filesystem::remove(os, ec);
        if (ec)
            return vbase::Result<void, FsError>::err(map_fs_error(ec));
        if (!removed)
            return vbase::Result<void, FsError>::err(FsError::eIOError);

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> PhysicalFileSystem::removeDirectory(vbase::StringView p, bool recursive)
    {
        Path path {p};
        if (path.empty() || path.isRoot())
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);

        const std::filesystem::path os = toOSPath(path);
        if (!std::filesystem::exists(os))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        if (!std::filesystem::is_directory(os))
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);

        std::error_code ec;
        if (recursive)
        {
            std::filesystem::remove_all(os, ec);
            if (ec)
                return vbase::Result<void, FsError>::err(map_fs_error(ec));
            return vbase::Result<void, FsError>::ok();
        }

        const bool removed = std::filesystem::remove(os, ec);
        if (ec)
            return vbase::Result<void, FsError>::err(map_fs_error(ec));
        if (!removed)
            return vbase::Result<void, FsError>::err(FsError::eDirectoryNotEmpty);

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> PhysicalFileSystem::rename(vbase::StringView from, vbase::StringView to)
    {
        Path fromPath {from};
        Path toPath {to};

        if (fromPath.empty() || fromPath.isRoot() || toPath.empty() || toPath.isRoot())
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);

        const std::filesystem::path fromOs = toOSPath(fromPath);
        const std::filesystem::path toOs   = toOSPath(toPath);

        if (!std::filesystem::exists(fromOs))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        if (std::filesystem::exists(toOs))
            return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);

        const auto parent = toOs.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        std::error_code ec;
        std::filesystem::rename(fromOs, toOs, ec);
        if (ec)
            return vbase::Result<void, FsError>::err(map_fs_error(ec));

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<std::vector<DirectoryEntry>, FsError> PhysicalFileSystem::list(vbase::StringView p) const
    {
        Path path {p};
        const std::filesystem::path os = toOSPath(path);
        if (!std::filesystem::exists(os))
            return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eNotFound);
        if (!std::filesystem::is_directory(os))
            return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eInvalidPath);

        std::error_code ec;
        std::vector<DirectoryEntry> entries;

        for (const auto& entry : std::filesystem::directory_iterator(os, ec))
        {
            if (ec)
                return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(map_fs_error(ec));

            const std::string name = entry.path().filename().string();
            DirectoryEntry    out;
            out.name        = name;
            out.path        = path.empty() ? name : std::string(path.join(name).str());
            out.isDirectory = entry.is_directory();
            out.isFile      = entry.is_regular_file();
            if (out.isFile)
            {
                std::error_code sizeEc;
                out.size = static_cast<uint64_t>(entry.file_size(sizeEc));
                if (sizeEc)
                    return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(map_fs_error(sizeEc));
            }

            entries.push_back(std::move(out));
        }

        std::sort(entries.begin(), entries.end(), [](const DirectoryEntry& a, const DirectoryEntry& b) {
            return a.name < b.name;
        });

        return vbase::Result<std::vector<DirectoryEntry>, FsError>::ok(std::move(entries));
    }

    vbase::Result<std::unique_ptr<IFile>, FsError> PhysicalFileSystem::open(vbase::StringView p, FileMode mode)
    {
        std::string os = toOSPath(Path(p));

        if (is_write_mode(mode))
        {
            const auto parent = std::filesystem::path(os).parent_path();
            if (!parent.empty() && !std::filesystem::exists(parent))
                return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);
        }

        std::fstream f;
        f.open(os, to_mode(mode));
        if (!f.is_open())
        {
            if (!std::filesystem::exists(os))
                return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);
            return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eIOError);
        }
        return vbase::Result<std::unique_ptr<IFile>, FsError>::ok(std::make_unique<PhysicalFile>(std::move(f)));
    }
} // namespace vfilesystem
