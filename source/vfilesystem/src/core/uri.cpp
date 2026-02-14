#include "vfilesystem/core/uri.hpp"

namespace vfilesystem
{
    Uri parse_uri(vbase::StringView input)
    {
        Uri  u;
        auto pos = input.find("://");
        if (pos != std::string::npos)
        {
            u.scheme         = input.substr(0, pos);
            std::string rest = input.substr(pos + 3).data();
            if (rest.empty() || rest[0] != '/')
                rest = "/" + rest;
            u.path = Path {rest};
        }
        else
        {
            u.path = Path {input};
        }
        return u;
    }
} // namespace vfilesystem
