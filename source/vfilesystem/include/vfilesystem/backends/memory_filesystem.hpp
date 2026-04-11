#pragma once

#include "vfilesystem/interfaces/ifilesystem.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vfilesystem
{
    // In-memory filesystem backend (primarily for tests/tools).
    class MemoryFileSystem final : public IFileSystem
    {
    public:
        MemoryFileSystem() = default;

        struct Entry
        {
            std::vector<std::byte> data;
        };

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

        // Helper utilities
        void                                           put(vbase::StringView p, std::vector<std::byte> data);
        vbase::Result<std::vector<std::byte>, FsError> get(vbase::StringView p) const;

    private:
        std::unordered_map<std::string, Entry> m_Files;
        std::unordered_set<std::string>        m_Directories;
    };
} // namespace vfilesystem
