#include "../common.hpp"

#include <vfilesystem/backends/physical_filesystem.hpp>

using namespace vfilesystem;

int main()
{
    print_title("PhysicalFileSystem read_all");

    PhysicalFileSystem fs(Path {"."});
    auto               data = fs.readAll("README.md");
    if (!data)
    {
        std::cout << "read failed\n";
        return 1;
    }
    std::cout << "README.md bytes: " << data.value().size() << "\n";

    return 0;
}
