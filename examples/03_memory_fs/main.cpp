#include "../common.hpp"

#include <vfilesystem/backends/memory_filesystem.hpp>

using namespace vfilesystem;

int main()
{
    print_title("MemoryFileSystem common operations");

    MemoryFileSystem mem {};

    if (!mem.createDirectories("cache/runtime"))
        return 1;

    const char msg[] = "hello";
    const auto* ptr  = reinterpret_cast<const std::byte*>(msg);

    auto writeResult = mem.writeAll("cache/runtime/foo.bin", vbase::ConstByteSpan {ptr, 5});
    if (!writeResult)
        return 1;

    auto entries = mem.list("cache");
    if (!entries)
        return 1;

    auto renameResult = mem.rename("cache/runtime/foo.bin", "cache/runtime/bar.bin");
    if (!renameResult)
        return 1;

    auto r = mem.readAll("cache/runtime/bar.bin");
    if (!r)
        return 1;

    auto removeFileResult = mem.removeFile("cache/runtime/bar.bin");
    if (!removeFileResult)
        return 1;

    auto removeDirResult = mem.removeDirectory("cache/runtime", false);
    if (!removeDirResult)
        return 1;

    std::cout << "cache entries: " << entries.value().size() << "\n";
    if (!entries.value().empty())
        std::cout << "first entry: " << entries.value().front().path << "\n";
    std::cout << "bar.bin bytes: " << r.value().size() << "\n";
    std::cout << "msg: " << std::string_view(reinterpret_cast<const char*>(r.value().data()), r.value().size()) << "\n";

    return 0;
}
