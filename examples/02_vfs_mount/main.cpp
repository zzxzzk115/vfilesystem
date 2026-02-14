#include "../common.hpp"

#include <vfilesystem/backends/memory_filesystem.hpp>
#include <vfilesystem/backends/physical_filesystem.hpp>
#include <vfilesystem/vfs/virtual_filesystem.hpp>

using namespace vfilesystem;

inline std::vector<std::byte> to_bytes(std::string_view s)
{
    const auto* ptr = reinterpret_cast<const std::byte*>(s.data());
    return std::vector<std::byte>(ptr, ptr + s.size());
}

int main()
{
    print_title("VirtualFileSystem mount (path and URI)");

    auto disk = std::make_shared<PhysicalFileSystem>();
    auto mem  = std::make_shared<MemoryFileSystem>();

    std::string msg = "hello from memory (memory filesystem)!";

    mem->put("/data/hello.txt", to_bytes(msg));

    VirtualFileSystem vfs {};
    vfs.mount(disk);
    vfs.mount(mem); // override (later mount wins)

    auto bytes = vfs.readAll("vfs://data/hello.txt"); // default scheme = "vfs"
    if (!bytes)
    {
        std::cout << "not found (vfs URI)\n";
        return 1;
    }

    vfs.mount(mem, "mem"); // with scheme

    auto bytes2 = vfs.readAll("mem://data/hello.txt");
    if (!bytes2)
    {
        std::cout << "not found (mem URI)\n";
        return 1;
    }

    vfs.mount(disk, "disk"); // remount with scheme

    auto bytes3 = vfs.readAll("disk://examples/02_vfs_mount/hello.txt");
    if (!bytes3)
    {
        std::cout << "not found (disk URI)\n";
        return 1;
    }

    std::cout << "vfs://data/hello.txt size: " << bytes.value().size() << "\n";
    std::cout << "content: "
              << std::string_view(reinterpret_cast<const char*>(bytes.value().data()), bytes.value().size()) << "\n\n";

    std::cout << "mem://data/hello.txt size: " << bytes2.value().size() << "\n";
    std::cout << "content: "
              << std::string_view(reinterpret_cast<const char*>(bytes2.value().data()), bytes2.value().size())
              << "\n\n";

    std::cout << "disk://examples/02_vfs_mount/hello.txt size: " << bytes3.value().size() << "\n";
    std::cout << "content: "
              << std::string_view(reinterpret_cast<const char*>(bytes3.value().data()), bytes3.value().size()) << "\n";

    return 0;
}
