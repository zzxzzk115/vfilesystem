#include "vfilesystem/interfaces/ifilesystem.hpp"

namespace vfilesystem
{
    vbase::Result<std::vector<std::byte>, FsError> IFileSystem::readAll(vbase::StringView p)
    {
        auto f = open(p, FileMode::eRead);
        if (!f)
            return vbase::Result<std::vector<std::byte>, FsError>::err(f.error());

        auto&          file = *f.value();
        const uint64_t sz   = file.size();

        std::vector<std::byte> buf;
        buf.resize(static_cast<size_t>(sz));

        if (sz > 0)
        {
            size_t r = file.read(buf.data(), static_cast<size_t>(sz));
            if (r != static_cast<size_t>(sz))
                return vbase::Result<std::vector<std::byte>, FsError>::err(FsError::eIOError);
        }

        return vbase::Result<std::vector<std::byte>, FsError>::ok(std::move(buf));
    }

    vbase::Result<void, FsError> IFileSystem::writeAll(vbase::StringView p, vbase::ConstByteSpan bytes)
    {
        auto f = open(p, FileMode::eWrite);
        if (!f)
            return vbase::Result<void, FsError>::err(f.error());

        auto& file = *f.value();
        if (!bytes.empty())
        {
            size_t w = file.write(bytes.data(), bytes.size());
            if (w != bytes.size())
                return vbase::Result<void, FsError>::err(FsError::eIOError);
        }

        return vbase::Result<void, FsError>::ok();
    }
} // namespace vfilesystem
