#pragma once

#include "vfilesystem/core/errors.hpp"
#include "vfilesystem/core/file_mode.hpp"
#include "vfilesystem/interfaces/ifile.hpp"

#include <vbase/core/result.hpp>
#include <vbase/core/span.hpp>
#include <vbase/core/string_view.hpp>

#include <memory>
#include <vector>

namespace vfilesystem
{
    // Thread-compatible: no internal locking policy.
    class IFileSystem
    {
    public:
        virtual ~IFileSystem() = default;

        virtual bool exists(vbase::StringView p) const      = 0;
        virtual bool isFile(vbase::StringView p) const      = 0;
        virtual bool isDirectory(vbase::StringView p) const = 0;

        virtual vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView p, FileMode mode) = 0;

        vbase::Result<std::vector<std::byte>, FsError> readAll(vbase::StringView p);
        vbase::Result<void, FsError>                   writeAll(vbase::StringView p, vbase::ConstByteSpan bytes);
    };
} // namespace vfilesystem
