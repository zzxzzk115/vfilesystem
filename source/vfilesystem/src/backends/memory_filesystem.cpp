#include "vfilesystem/backends/memory_filesystem.hpp"
#include "vfilesystem/core/path.hpp"
#include "vfilesystem/interfaces/ifile.hpp"

#include <algorithm>
#include <cstring>

namespace vfilesystem
{
    class MemoryFile final : public IFile
    {
    public:
        MemoryFile(std::vector<std::byte>& buf, FileMode mode) : m_Buf(buf), m_Mode(mode)
        {
            if (mode == FileMode::eAppend)
                m_Cursor = m_Buf.size();
        }

        size_t read(void* dst, size_t size) override
        {
            if (m_Mode == FileMode::eWrite || m_Mode == FileMode::eAppend)
                return 0;
            const size_t avail = (m_Cursor < m_Buf.size()) ? (m_Buf.size() - m_Cursor) : 0;
            const size_t n     = std::min(avail, size);
            if (n)
            {
                std::memcpy(dst, m_Buf.data() + m_Cursor, n);
                m_Cursor += n;
            }
            return n;
        }

        size_t write(const void* src, size_t size) override
        {
            if (m_Mode == FileMode::eRead)
                return 0;
            const size_t end = m_Cursor + size;
            if (end > m_Buf.size())
                m_Buf.resize(end);
            std::memcpy(m_Buf.data() + m_Cursor, src, size);
            m_Cursor += size;
            return size;
        }

        bool seek(uint64_t offset) override
        {
            m_Cursor = static_cast<size_t>(offset);
            return true;
        }
        uint64_t tell() const override { return static_cast<uint64_t>(m_Cursor); }
        uint64_t size() const override { return static_cast<uint64_t>(m_Buf.size()); }

        std::vector<std::byte> readAllBytes() override
        {
            std::vector<std::byte> out;
            if (m_Mode == FileMode::eWrite || m_Mode == FileMode::eAppend)
                return out;
            if (m_Cursor < m_Buf.size())
            {
                out.resize(m_Buf.size() - m_Cursor);
                std::memcpy(out.data(), m_Buf.data() + m_Cursor, out.size());
                m_Cursor += out.size();
            }
            return out;
        }

    private:
        std::vector<std::byte>& m_Buf;
        FileMode                m_Mode {FileMode::eRead};
        size_t                  m_Cursor {0};
    };

    static std::string key_of(vbase::StringView p)
    {
        Path path(p);
        auto sv = path.str();
        if (!sv.empty() && sv.front() == '/')
            sv = vbase::StringView {sv.data() + 1, sv.size() - 1};
        return std::string(sv.data(), sv.size());
    }

    bool MemoryFileSystem::exists(vbase::StringView p) const { return m_Files.find(key_of(p)) != m_Files.end(); }
    bool MemoryFileSystem::isFile(vbase::StringView p) const { return exists(p); }
    bool MemoryFileSystem::isDirectory(vbase::StringView) const { return false; }

    vbase::Result<std::unique_ptr<IFile>, FsError> MemoryFileSystem::open(vbase::StringView p, FileMode mode)
    {
        const std::string k = key_of(p);

        if (mode == FileMode::eRead)
        {
            auto it = m_Files.find(k);
            if (it == m_Files.end())
                return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);
            return vbase::Result<std::unique_ptr<IFile>, FsError>::ok(
                std::make_unique<MemoryFile>(it->second.data, mode));
        }

        auto& entry = m_Files[k]; // create if missing
        if (mode == FileMode::eWrite)
            entry.data.clear();
        return vbase::Result<std::unique_ptr<IFile>, FsError>::ok(std::make_unique<MemoryFile>(entry.data, mode));
    }

    void MemoryFileSystem::put(vbase::StringView p, std::vector<std::byte> data)
    {
        m_Files[key_of(p)] = Entry {std::move(data)};
    }

    vbase::Result<std::vector<std::byte>, FsError> MemoryFileSystem::get(vbase::StringView p) const
    {
        auto it = m_Files.find(key_of(p));
        if (it == m_Files.end())
            return vbase::Result<std::vector<std::byte>, FsError>::err(FsError::eNotFound);
        return vbase::Result<std::vector<std::byte>, FsError>::ok(it->second.data);
    }
} // namespace vfilesystem
