#include "vfilesystem/backends/physical_filesystem.hpp"
#include "vfilesystem/interfaces/ifile.hpp"

#include <filesystem>
#include <fstream>

namespace vfilesystem
{
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

    vbase::Result<std::unique_ptr<IFile>, FsError> PhysicalFileSystem::open(vbase::StringView p, FileMode mode)
    {
        std::string os = toOSPath(Path(p));

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
