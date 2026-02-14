#include "../common.hpp"

#include <vfilesystem/backends/memory_filesystem.hpp>

using namespace vfilesystem;

int main()
{
    print_title("MemoryFileSystem write/read");

    MemoryFileSystem mem {};

    const char msg[] = "hello";
    auto       w     = mem.open("foo.bin", FileMode::eWrite);
    if (!w)
        return 1;
    w.value()->write(msg, 5);

    auto r = mem.readAll("foo.bin");
    if (!r)
        return 1;

    std::cout << "foo.bin bytes: " << r.value().size() << "\n";
    std::cout << "msg: " << std::string_view(reinterpret_cast<const char*>(r.value().data()), r.value().size()) << "\n";

    return 0;
}
