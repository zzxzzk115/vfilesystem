#pragma once

#include "vfilesystem/core/path.hpp"

#include <string>

namespace vfilesystem
{
    struct Uri
    {
        std::string scheme; // empty if none
        Path        path;   // normalized logical path
    };

    Uri parse_uri(vbase::StringView input);
} // namespace vfilesystem
