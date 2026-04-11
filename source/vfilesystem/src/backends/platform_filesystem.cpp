#include "vfilesystem/backends/platform_filesystem.hpp"

#include "vfilesystem/backends/physical_filesystem.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <vector>

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace vfilesystem
{
    namespace
    {
        std::string to_string(vbase::StringView sv) { return std::string(sv.data(), sv.size()); }

        std::filesystem::path to_std_path(const Path& path) { return std::filesystem::path(to_string(path.str())); }

        bool ensure_directory(const Path& path)
        {
            if (path.empty())
                return true;

            std::error_code ec;
            std::filesystem::create_directories(to_std_path(path), ec);
            return !ec;
        }

        std::shared_ptr<IFileSystem> make_physical_fs(const Path& root) { return std::make_shared<PhysicalFileSystem>(root); }

        vbase::Result<std::shared_ptr<IFileSystem>, FsError> make_writable_physical_fs(const Path& root)
        {
            if (!ensure_directory(root))
                return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::err(FsError::eIOError);
            return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::ok(make_physical_fs(root));
        }

#if defined(__ANDROID__)
        class AndroidAssetFile final : public IFile
        {
        public:
            explicit AndroidAssetFile(std::vector<std::byte> bytes) : m_Bytes(std::move(bytes)) {}

            size_t read(void* dst, size_t size) override
            {
                const size_t remaining = m_Bytes.size() - std::min(m_Pos, m_Bytes.size());
                const size_t count     = std::min(size, remaining);
                if (count == 0)
                    return 0;

                std::memcpy(dst, m_Bytes.data() + m_Pos, count);
                m_Pos += count;
                return count;
            }

            size_t write(const void*, size_t) override { return 0; }

            bool seek(uint64_t offset) override
            {
                if (offset > m_Bytes.size())
                    return false;
                m_Pos = static_cast<size_t>(offset);
                return true;
            }

            uint64_t tell() const override { return static_cast<uint64_t>(m_Pos); }

            uint64_t size() const override { return static_cast<uint64_t>(m_Bytes.size()); }

            std::vector<std::byte> readAllBytes() override { return m_Bytes; }

        private:
            std::vector<std::byte> m_Bytes;
            size_t                 m_Pos {0};
        };

        class AndroidAssetFileSystem final : public IFileSystem
        {
        public:
            AndroidAssetFileSystem(AAssetManager* assetManager, Path root)
                : m_AssetManager(assetManager), m_Root(std::move(root))
            {}

            bool exists(vbase::StringView p) const override { return isFile(p) || isDirectory(p); }

            bool isFile(vbase::StringView p) const override
            {
                std::string assetPath = to_asset_path(Path {p});
                AAsset*     asset     = AAssetManager_open(m_AssetManager, assetPath.c_str(), AASSET_MODE_UNKNOWN);
                if (!asset)
                    return false;

                AAsset_close(asset);
                return true;
            }

            bool isDirectory(vbase::StringView p) const override
            {
                std::string assetPath = to_asset_path(Path {p});
                if (assetPath.empty())
                    return true;

                AAssetDir* dir = AAssetManager_openDir(m_AssetManager, assetPath.c_str());
                if (!dir)
                    return false;

                const char* name   = AAssetDir_getNextFileName(dir);
                const bool  exists = (name != nullptr);
                AAssetDir_close(dir);
                return exists;
            }

            vbase::Result<void, FsError> createDirectory(vbase::StringView) override
            {
                return vbase::Result<void, FsError>::err(FsError::eNotSupported);
            }

            vbase::Result<void, FsError> createDirectories(vbase::StringView) override
            {
                return vbase::Result<void, FsError>::err(FsError::eNotSupported);
            }

            vbase::Result<void, FsError> removeFile(vbase::StringView) override
            {
                return vbase::Result<void, FsError>::err(FsError::eNotSupported);
            }

            vbase::Result<void, FsError> removeDirectory(vbase::StringView, bool) override
            {
                return vbase::Result<void, FsError>::err(FsError::eNotSupported);
            }

            vbase::Result<void, FsError> rename(vbase::StringView, vbase::StringView) override
            {
                return vbase::Result<void, FsError>::err(FsError::eNotSupported);
            }

            vbase::Result<std::vector<DirectoryEntry>, FsError> list(vbase::StringView p) const override
            {
                if (isFile(p))
                    return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eInvalidPath);

                std::string assetPath = to_asset_path(Path {p});
                AAssetDir*  dir       = AAssetManager_openDir(m_AssetManager, assetPath.c_str());
                if (!dir)
                    return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eNotFound);

                std::vector<DirectoryEntry> entries;
                for (const char* name = AAssetDir_getNextFileName(dir); name != nullptr; name = AAssetDir_getNextFileName(dir))
                {
                    DirectoryEntry entry;
                    entry.name = name;

                    const Path logicalPath = Path {p}.join(name);
                    entry.path             = to_string(logicalPath.str());

                    std::string childAssetPath = to_asset_path(logicalPath);
                    if (AAsset* asset = AAssetManager_open(m_AssetManager, childAssetPath.c_str(), AASSET_MODE_UNKNOWN))
                    {
                        entry.isFile = true;
                        entry.size   = static_cast<uint64_t>(AAsset_getLength64(asset));
                        AAsset_close(asset);
                    }
                    else
                    {
                        entry.isDirectory = true;
                    }

                    entries.push_back(std::move(entry));
                }

                AAssetDir_close(dir);

                if (entries.empty() && !assetPath.empty())
                    return vbase::Result<std::vector<DirectoryEntry>, FsError>::err(FsError::eNotFound);

                std::sort(entries.begin(), entries.end(), [](const DirectoryEntry& a, const DirectoryEntry& b) {
                    return a.name < b.name;
                });

                return vbase::Result<std::vector<DirectoryEntry>, FsError>::ok(std::move(entries));
            }

            vbase::Result<std::unique_ptr<IFile>, FsError> open(vbase::StringView p, FileMode mode) override
            {
                if (mode != FileMode::eRead)
                    return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotSupported);

                std::string assetPath = to_asset_path(Path {p});
                AAsset*     asset     = AAssetManager_open(m_AssetManager, assetPath.c_str(), AASSET_MODE_STREAMING);
                if (!asset)
                    return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eNotFound);

                const off64_t length = AAsset_getLength64(asset);
                if (length < 0)
                {
                    AAsset_close(asset);
                    return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eIOError);
                }

                std::vector<std::byte> bytes;
                bytes.resize(static_cast<size_t>(length));

                size_t offset = 0;
                while (offset < bytes.size())
                {
                    const size_t remaining = bytes.size() - offset;
                    const int    chunk     = AAsset_read(asset, bytes.data() + offset, remaining);
                    if (chunk <= 0)
                    {
                        AAsset_close(asset);
                        return vbase::Result<std::unique_ptr<IFile>, FsError>::err(FsError::eIOError);
                    }
                    offset += static_cast<size_t>(chunk);
                }

                AAsset_close(asset);
                return vbase::Result<std::unique_ptr<IFile>, FsError>::ok(
                    std::make_unique<AndroidAssetFile>(std::move(bytes)));
            }

        private:
            std::string to_asset_path(const Path& path) const
            {
                const Path joined = m_Root.join(path.str());
                std::string out   = to_string(joined.str());
                if (!out.empty() && out.front() == '/')
                    out.erase(out.begin());
                return out;
            }

            AAssetManager* m_AssetManager {nullptr};
            Path           m_Root;
        };
#endif

        vbase::Result<std::shared_ptr<IFileSystem>, FsError> make_asset_filesystem(
            const PlatformFileSystemOptions& options)
        {
#if defined(__ANDROID__)
            if (options.androidAssetManager != nullptr)
            {
                auto* manager = static_cast<AAssetManager*>(options.androidAssetManager);
                return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::ok(
                    std::make_shared<AndroidAssetFileSystem>(manager, options.rootOverride.value_or(Path {})));
            }

            if (options.rootOverride)
                return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::ok(make_physical_fs(*options.rootOverride));

            return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::err(FsError::eNotSupported);
#else
            if (options.rootOverride)
                return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::ok(make_physical_fs(*options.rootOverride));
#if defined(__EMSCRIPTEN__)
            return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::ok(make_physical_fs(Path {"/"}));
#else
            return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::ok(make_physical_fs(Path {"."}));
#endif
#endif
        }

        vbase::Result<std::shared_ptr<IFileSystem>, FsError> make_writable_filesystem(
            const PlatformFileSystemOptions& options)
        {
            if (options.rootOverride)
                return make_writable_physical_fs(*options.rootOverride);

#if defined(__ANDROID__)
            return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::err(FsError::eNotSupported);
#elif defined(__EMSCRIPTEN__)
            return make_writable_physical_fs(Path {"/persistent"});
#else
            return make_writable_physical_fs(Path {"."});
#endif
        }
    } // namespace

    vbase::Result<std::shared_ptr<IFileSystem>, FsError> makePlatformFileSystem(
        PlatformFileSystemKind            kind,
        const PlatformFileSystemOptions& options)
    {
        switch (kind)
        {
            case PlatformFileSystemKind::eAssets:
                return make_asset_filesystem(options);
            case PlatformFileSystemKind::eWritable:
                return make_writable_filesystem(options);
            default:
                return vbase::Result<std::shared_ptr<IFileSystem>, FsError>::err(FsError::eNotSupported);
        }
    }
} // namespace vfilesystem
