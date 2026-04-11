#include "../common.hpp"

#include <vfilesystem/backends/platform_filesystem.hpp>

using namespace vfilesystem;

int main()
{
    print_title("Platform filesystem dispatch");

    PlatformFileSystemOptions seedOptions {};
    seedOptions.rootOverride = Path {"build/platform_fs_assets"};

    auto seed = makePlatformFileSystem(PlatformFileSystemKind::eWritable, seedOptions);
    if (!seed)
    {
        std::cout << "seed filesystem create failed\n";
        return 1;
    }

    auto assetDirResult = seed.value()->createDirectories("demo");
    if (!assetDirResult)
    {
        std::cout << "asset directory create failed\n";
        return 1;
    }

    const char assetMessage[] = "hello from seeded asset content";
    const auto* assetBegin    = reinterpret_cast<const std::byte*>(assetMessage);
    auto assetSeedResult =
        seed.value()->writeAll("demo/asset.txt", vbase::ConstByteSpan {assetBegin, sizeof(assetMessage) - 1});
    if (!assetSeedResult)
    {
        std::cout << "asset seed write failed\n";
        return 1;
    }

    PlatformFileSystemOptions assetOptions {};
    assetOptions.rootOverride = Path {"build/platform_fs_assets"};

    auto assets = makePlatformFileSystem(PlatformFileSystemKind::eAssets, assetOptions);
    if (!assets)
    {
        std::cout << "asset filesystem create failed\n";
        return 1;
    }

    auto assetEntries = assets.value()->list("demo");
    if (!assetEntries)
    {
        std::cout << "asset list failed\n";
        return 1;
    }

    auto data = assets.value()->readAll("demo/asset.txt");
    if (!data)
    {
        std::cout << "asset read failed\n";
        return 1;
    }

    std::cout << "demo entries: " << assetEntries.value().size() << "\n";
    std::cout << "demo/asset.txt bytes via PlatformFileSystem: " << data.value().size() << "\n";
    std::cout << "asset content: "
              << std::string_view(reinterpret_cast<const char*>(data.value().data()), data.value().size()) << "\n";

    PlatformFileSystemOptions writableOptions {};
    writableOptions.rootOverride = Path {"build/platform_fs_scratch"};

    auto writable = makePlatformFileSystem(PlatformFileSystemKind::eWritable, writableOptions);
    if (!writable)
    {
        std::cout << "writable filesystem create failed\n";
        return 1;
    }

    auto writableDirResult = writable.value()->createDirectories("nested");
    if (!writableDirResult)
    {
        std::cout << "writable directory create failed\n";
        return 1;
    }

    const char message[] = "hello from PlatformFileSystem";
    const auto* begin    = reinterpret_cast<const std::byte*>(message);
    auto result = writable.value()->writeAll("nested/platform.txt", vbase::ConstByteSpan {begin, sizeof(message) - 1});
    if (!result)
    {
        std::cout << "platform write failed\n";
        return 1;
    }

    auto renameResult = writable.value()->rename("nested/platform.txt", "nested/platform_renamed.txt");
    if (!renameResult)
    {
        std::cout << "platform rename failed\n";
        return 1;
    }

    auto writableEntries = writable.value()->list("nested");
    if (!writableEntries)
    {
        std::cout << "writable list failed\n";
        return 1;
    }

    auto echo = writable.value()->readAll("nested/platform_renamed.txt");
    if (!echo)
    {
        std::cout << "platform read failed\n";
        return 1;
    }

    auto removeFileResult = writable.value()->removeFile("nested/platform_renamed.txt");
    if (!removeFileResult)
    {
        std::cout << "platform remove file failed\n";
        return 1;
    }

    auto removeDirResult = writable.value()->removeDirectory("nested", false);
    if (!removeDirResult)
    {
        std::cout << "platform remove directory failed\n";
        return 1;
    }

    std::cout << "nested entries: " << writableEntries.value().size() << "\n";
    std::cout << "platform_renamed.txt bytes: " << echo.value().size() << "\n";
    std::cout << "content: "
              << std::string_view(reinterpret_cast<const char*>(echo.value().data()), echo.value().size()) << "\n";

    return 0;
}
