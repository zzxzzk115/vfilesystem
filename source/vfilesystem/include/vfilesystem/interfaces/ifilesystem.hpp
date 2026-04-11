#pragma once

#include "vfilesystem/core/errors.hpp"
#include "vfilesystem/core/file_mode.hpp"
#include "vfilesystem/interfaces/ifile.hpp"

#include <vbase/core/result.hpp>
#include <vbase/core/span.hpp>
#include <vbase/core/string_view.hpp>

#include <memory>
#include <string>
#include <vector>

namespace vfilesystem
{
    struct DirectoryEntry
    {
        std::string name;
        std::string path;
        bool        isFile {false};
        bool        isDirectory {false};
        uint64_t    size {0};
    };

    // Thread-compatible: no internal locking policy.
    class IFileSystem
    {
    public:
        virtual ~IFileSystem() = default;

        virtual bool exists(vbase::StringView p) const      = 0;
        virtual bool isFile(vbase::StringView p) const      = 0;
        virtual bool isDirectory(vbase::StringView p) const = 0;

        virtual vbase::Result<void, FsError> createDirectory(vbase::StringView)
        {
            return vbase::Result<void, FsError>::err(FsError::eNotSupported);
        }

        virtual vbase::Result<void, FsError> createDirectories(vbase::StringView)
        {
            return vbase::Result<void, FsError>::err(FsError::eNotSupported);
        }

        virtual vbase::Result<void, FsError> removeFile(vbase::StringView)
        {
            return vbase::Result<void, FsError>::err(FsError::eNotSupported);
        }

        virtual vbase::Result<void, FsError> removeDirectory(vbase::StringView, bool)
        {
            return vbase::Result<void, FsError>::err(FsError::eNotSupported);
        }

        virtual vbase::Result<void, FsError> rename(vbase::StringView, vbase::StringView)
        {
            return vbase::Result<void, FsError>::err(FsError::eNotSupported);
        }

        virtual vbase::Result<std::vector<DirectoryEntry>, FsError> list(vbase::StringView) const
        {
            return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eNotSupported);
        }

        virtual vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView p, FileMode mode) = 0;

        vbase::Result<std::vector<std::byte>, FsError> readAll(vbase::StringView p);
        vbase::Result<void, FsError>                   writeAll(vbase::StringView p, vbase::ConstByteSpan bytes);
    };
} // namespace vfilesystem
