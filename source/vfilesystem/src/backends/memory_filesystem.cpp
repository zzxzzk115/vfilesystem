#include "vfilesystem/backends/memory_filesystem.hpp"
#include "vfilesystem/core/path.hpp"
#include "vfilesystem/interfaces/ifile.hpp"

#include <algorithm>
#include <cstring>
#include <map>

namespace vfilesystem
{
    namespace
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

        std::string key_of(vbase::StringView p)
        {
            Path path(p);
            auto sv = path.str();
            if (sv == "/")
                return {};
            if (!sv.empty() && sv.front() == '/')
                sv = vbase::StringView {sv.data() + 1, sv.size() - 1};
            return std::string(sv.data(), sv.size());
        }

        std::string parent_key_of(const std::string& key)
        {
            if (key.empty())
                return {};

            const auto pos = key.find_last_of('/');
            if (pos == std::string::npos)
                return {};
            return key.substr(0, pos);
        }

        bool is_descendant_of(const std::string& key, const std::string& parent)
        {
            if (parent.empty())
                return !key.empty();
            return key.size() > parent.size() && key.compare(0, parent.size(), parent) == 0 && key[parent.size()] == '/';
        }

        bool exists_path(
            const std::unordered_map<std::string, MemoryFileSystem::Entry>& files,
            const std::unordered_set<std::string>&                         directories,
            const std::string&                                            key)
        {
            if (files.find(key) != files.end())
                return true;
            if (key.empty())
                return true;
            if (directories.find(key) != directories.end())
                return true;

            for (const auto& [fileKey, entry] : files)
            {
                (void)entry;
                if (is_descendant_of(fileKey, key))
                    return true;
            }

            for (const auto& dirKey : directories)
                if (is_descendant_of(dirKey, key))
                    return true;

            return false;
        }

        bool is_directory_path(
            const std::unordered_map<std::string, MemoryFileSystem::Entry>& files,
            const std::unordered_set<std::string>&                         directories,
            const std::string&                                            key)
        {
            if (key.empty())
                return true;
            if (directories.find(key) != directories.end())
                return true;

            for (const auto& [fileKey, entry] : files)
            {
                (void)entry;
                if (is_descendant_of(fileKey, key))
                    return true;
            }

            for (const auto& dirKey : directories)
                if (is_descendant_of(dirKey, key))
                    return true;

            return false;
        }

        bool has_children(
            const std::unordered_map<std::string, MemoryFileSystem::Entry>& files,
            const std::unordered_set<std::string>&                         directories,
            const std::string&                                            key)
        {
            for (const auto& [fileKey, entry] : files)
            {
                (void)entry;
                if (is_descendant_of(fileKey, key))
                    return true;
            }

            for (const auto& dirKey : directories)
                if (is_descendant_of(dirKey, key))
                    return true;

            return false;
        }

        std::string join_key(const std::string& base, const std::string& child)
        {
            if (base.empty())
                return child;
            return base + "/" + child;
        }

        std::string direct_child_name(const std::string& key, const std::string& directory)
        {
            if (directory.empty())
            {
                const auto slash = key.find('/');
                return key.substr(0, slash);
            }

            if (!is_descendant_of(key, directory))
                return {};

            const std::string_view remainder(key.data() + directory.size() + 1, key.size() - directory.size() - 1);
            const auto             slash = remainder.find('/');
            return std::string(remainder.substr(0, slash));
        }
    } // namespace

    bool MemoryFileSystem::exists(vbase::StringView p) const { return exists_path(m_Files, m_Directories, key_of(p)); }

    bool MemoryFileSystem::isFile(vbase::StringView p) const { return m_Files.find(key_of(p)) != m_Files.end(); }

    bool MemoryFileSystem::isDirectory(vbase::StringView p) const
    {
        const std::string key = key_of(p);
        if (m_Files.find(key) != m_Files.end())
            return false;
        return is_directory_path(m_Files, m_Directories, key);
    }

    vbase::Result<void, FsError> MemoryFileSystem::createDirectory(vbase::StringView p)
    {
        const std::string key = key_of(p);
        if (key.empty())
            return vbase::Result<void, FsError>::ok();
        if (m_Files.find(key) != m_Files.end())
            return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);
        if (is_directory_path(m_Files, m_Directories, key))
            return vbase::Result<void, FsError>::ok();

        const std::string parent = parent_key_of(key);
        if (!is_directory_path(m_Files, m_Directories, parent))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        m_Directories.insert(key);
        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> MemoryFileSystem::createDirectories(vbase::StringView p)
    {
        const std::string key = key_of(p);
        if (key.empty())
            return vbase::Result<void, FsError>::ok();
        if (m_Files.find(key) != m_Files.end())
            return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);

        size_t offset = 0;
        while (offset < key.size())
        {
            const auto slash = key.find('/', offset);
            const auto len   = (slash == std::string::npos) ? key.size() : slash;
            const auto part  = key.substr(0, len);

            if (m_Files.find(part) != m_Files.end())
                return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);

            m_Directories.insert(part);
            if (slash == std::string::npos)
                break;
            offset = slash + 1;
        }

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> MemoryFileSystem::removeFile(vbase::StringView p)
    {
        const std::string key = key_of(p);
        auto              it  = m_Files.find(key);
        if (it == m_Files.end())
        {
            if (is_directory_path(m_Files, m_Directories, key))
                return vbase::Result<void, FsError>::err(FsError::eInvalidPath);
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        }

        m_Files.erase(it);
        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> MemoryFileSystem::removeDirectory(vbase::StringView p, bool recursive)
    {
        const std::string key = key_of(p);
        if (key.empty())
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);
        if (m_Files.find(key) != m_Files.end())
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);
        if (!is_directory_path(m_Files, m_Directories, key))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        if (!recursive && has_children(m_Files, m_Directories, key))
            return vbase::Result<void, FsError>::err(FsError::eDirectoryNotEmpty);

        for (auto it = m_Files.begin(); it != m_Files.end();)
        {
            if (it->first == key || is_descendant_of(it->first, key))
                it = m_Files.erase(it);
            else
                ++it;
        }

        for (auto it = m_Directories.begin(); it != m_Directories.end();)
        {
            if (*it == key || is_descendant_of(*it, key))
                it = m_Directories.erase(it);
            else
                ++it;
        }

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<void, FsError> MemoryFileSystem::rename(vbase::StringView from, vbase::StringView to)
    {
        const std::string fromKey = key_of(from);
        const std::string toKey   = key_of(to);

        if (fromKey.empty() || toKey.empty())
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);
        if (fromKey == toKey)
            return vbase::Result<void, FsError>::ok();
        if (exists_path(m_Files, m_Directories, toKey))
            return vbase::Result<void, FsError>::err(FsError::eAlreadyExists);
        if (!is_directory_path(m_Files, m_Directories, parent_key_of(toKey)))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);

        auto fileIt = m_Files.find(fromKey);
        if (fileIt != m_Files.end())
        {
            auto node = m_Files.extract(fileIt);
            node.key() = toKey;
            m_Files.insert(std::move(node));
            return vbase::Result<void, FsError>::ok();
        }

        if (!is_directory_path(m_Files, m_Directories, fromKey))
            return vbase::Result<void, FsError>::err(FsError::eNotFound);
        if (is_descendant_of(toKey, fromKey))
            return vbase::Result<void, FsError>::err(FsError::eInvalidPath);

        std::vector<std::pair<std::string, Entry>> movedFiles;
        movedFiles.reserve(m_Files.size());
        for (auto it = m_Files.begin(); it != m_Files.end();)
        {
            if (is_descendant_of(it->first, fromKey))
            {
                const std::string suffix = it->first.substr(fromKey.size());
                movedFiles.push_back({toKey + suffix, std::move(it->second)});
                it = m_Files.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::vector<std::string> movedDirectories;
        if (m_Directories.find(fromKey) != m_Directories.end())
            movedDirectories.push_back(toKey);

        for (auto it = m_Directories.begin(); it != m_Directories.end();)
        {
            if (it->empty())
            {
                ++it;
                continue;
            }

            if (*it == fromKey || is_descendant_of(*it, fromKey))
            {
                const std::string oldValue = *it;
                const std::string suffix   = oldValue.substr(fromKey.size());
                movedDirectories.push_back(toKey + suffix);
                it = m_Directories.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (auto& [newKey, entry] : movedFiles)
            m_Files.emplace(std::move(newKey), std::move(entry));
        for (auto& dir : movedDirectories)
            m_Directories.insert(std::move(dir));

        return vbase::Result<void, FsError>::ok();
    }

    vbase::Result<std::vector<DirectoryEntry>, FsError> MemoryFileSystem::list(vbase::StringView p) const
    {
        const std::string key = key_of(p);
        if (m_Files.find(key) != m_Files.end())
            return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eInvalidPath);
        if (!is_directory_path(m_Files, m_Directories, key))
            return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eNotFound);

        std::map<std::string, DirectoryEntry> entries;

        for (const auto& [fileKey, entry] : m_Files)
        {
            const std::string childName = direct_child_name(fileKey, key);
            if (childName.empty())
                continue;

            const bool directFile = join_key(key, childName) == fileKey;
            auto&      out        = entries[childName];
            out.name              = childName;
            out.path              = join_key(key, childName);
            out.isFile            = directFile;
            out.isDirectory       = !directFile;
            if (directFile)
                out.size = static_cast<uint64_t>(entry.data.size());
        }

        for (const auto& dirKey : m_Directories)
        {
            const std::string childName = direct_child_name(dirKey, key);
            if (childName.empty())
                continue;

            auto& out      = entries[childName];
            out.name       = childName;
            out.path       = join_key(key, childName);
            out.isDirectory = true;
        }

        std::vector<DirectoryEntry> out;
        out.reserve(entries.size());
        for (auto& [name, entry] : entries)
        {
            (void)name;
            out.push_back(std::move(entry));
        }

        return vbase::Result<std::vector<DirectoryEntry>, FsError>::ok(std::move(out));
    }

    vbase::Result<std::unique_ptr<IFile>, FsError> MemoryFileSystem::open(vbase::StringView p, FileMode mode)
    {
        const std::string key = key_of(p);

        if (mode == FileMode::eRead)
        {
            auto it = m_Files.find(key);
            if (it == m_Files.end())
                return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);
            return vbase::Result<std::unique_ptr<IFile>, FsError>::ok(std::make_unique<MemoryFile>(it->second.data, mode));
        }

        if (!is_directory_path(m_Files, m_Directories, parent_key_of(key)))
            return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);
        if (m_Directories.find(key) != m_Directories.end())
            return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eAlreadyExists);

        auto& entry = m_Files[key];
        if (mode == FileMode::eWrite)
            entry.data.clear();
        return vbase::Result<std::unique_ptr<IFile>, FsError>::ok(std::make_unique<MemoryFile>(entry.data, mode));
    }

    void MemoryFileSystem::put(vbase::StringView p, std::vector<std::byte> data) { m_Files[key_of(p)] = Entry {std::move(data)}; }

    vbase::Result<std::vector<std::byte>, FsError> MemoryFileSystem::get(vbase::StringView p) const
    {
        auto it = m_Files.find(key_of(p));
        if (it == m_Files.end())
            return vbase::Result<std::vector<std::byte>, FsError>::err(FsError::eNotFound);
        return vbase::Result<std::vector<std::byte>, FsError>::ok(it->second.data);
    }
} // namespace vfilesystem
