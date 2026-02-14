#pragma once

#include "vfilesystem/interfaces/ifilesystem.hpp"

#include <unordered_map>
#include <vector>

namespace vfilesystem
{
    // In-memory filesystem backend (primarily for tests/tools).
    class MemoryFileSystem final : public IFileSystem
    {
    public:
        MemoryFileSystem() = default;

        bool exists(vbase::StringView p) const override;
        bool isFile(vbase::StringView p) const override;
        bool isDirectory(vbase::StringView p) const override;

        vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView p, FileMode mode) override;

        // Helper utilities
        void                                           put(vbase::StringView p, std::vector<std::byte> data);
        vbase::Result<std::vector<std::byte>, FsError> get(vbase::StringView p) const;

    private:
        struct Entry
        {
            std::vector<std::byte> data;
        };
        std::unordered_map<std::string, Entry> m_Files;
    };
} // namespace vfilesystem
