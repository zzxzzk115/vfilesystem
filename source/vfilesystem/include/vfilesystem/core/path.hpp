#pragma once

#include <vbase/core/string_view.hpp>

#include <string>

namespace vfilesystem
{
    class Path
    {
    public:
        Path() = default;
        explicit Path(vbase::StringView p);

        vbase::StringView str() const noexcept { return m_Path; }

        bool empty() const noexcept { return m_Path.empty(); }
        bool isRoot() const noexcept { return m_Path == "/"; }

        Path parent() const;
        Path filename() const;
        Path extension() const;

        Path join(vbase::StringView rel) const;

        static std::string normalize(vbase::StringView p);

    private:
        std::string m_Path; // normalized UTF-8 path
    };

    inline bool operator==(const Path& a, const Path& b) noexcept { return a.str() == b.str(); }
    inline bool operator!=(const Path& a, const Path& b) noexcept { return !(a == b); }

} // namespace vfilesystem
