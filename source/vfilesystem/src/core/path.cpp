#include "vfilesystem/core/path.hpp"

#include <vector>

namespace vfilesystem
{
    // Canonical form rules:
    // - Uses '/' as separator
    // - Removes '.' segments
    // - Resolves '..' segments where possible
    // - "." normalizes to ""
    // - Empty string represents current directory
    // - Absolute root is "/"
    // - Canonical form never contains "." segments

    std::string Path::normalize(vbase::StringView p)
    {
        if (p.size() == 1 && p[0] == '.')
            return {}; // "." -> ""

        std::string s(p.data(), p.size());

        // normalize separators
        for (auto& c : s)
            if (c == '\\')
                c = '/';

        if (s.empty())
            return s;

        const bool absolute = s[0] == '/';

        std::vector<std::string> segs;
        segs.reserve(16);

        size_t i = 0;
        while (i < s.size())
        {
            while (i < s.size() && s[i] == '/')
                ++i;

            if (i >= s.size())
                break;

            size_t j = i;
            while (j < s.size() && s[j] != '/')
                ++j;

            std::string part = s.substr(i, j - i);
            i                = j;

            if (part.empty() || part == ".")
                continue;

            if (part == "..")
            {
                if (!segs.empty() && segs.back() != "..")
                {
                    segs.pop_back();
                }
                else if (!absolute)
                {
                    segs.push_back("..");
                }
                continue;
            }

            segs.push_back(std::move(part));
        }

        std::string out;

        if (absolute)
            out.push_back('/');

        for (size_t k = 0; k < segs.size(); ++k)
        {
            if (k)
                out.push_back('/');
            out += segs[k];
        }

        if (absolute && out.empty())
            out = "/";

        return out;
    }

    Path::Path(vbase::StringView p) : m_Path(normalize(p)) {}

    Path Path::parent() const
    {
        if (m_Path.empty())
            return Path {""}; // current dir parent = current

        if (m_Path == "/")
            return Path {"/"}; // root parent = root

        auto pos = m_Path.find_last_of('/');

        if (pos == std::string::npos)
            return Path {""}; // single segment relative path

        if (pos == 0)
            return Path {"/"}; // child of root

        return Path {vbase::StringView {m_Path.data(), pos}};
    }

    Path Path::filename() const
    {
        if (m_Path.empty() || m_Path == "/")
            return Path {""};

        auto pos = m_Path.find_last_of('/');

        if (pos == std::string::npos)
            return *this;

        return Path {vbase::StringView {m_Path.data() + pos + 1, m_Path.size() - (pos + 1)}};
    }

    Path Path::extension() const
    {
        auto f = filename().str();

        if (f.empty())
            return Path {""};

        auto dot = f.rfind('.');

        // no extension
        if (dot == vbase::StringView::npos)
            return Path {""};

        // ".gitignore" style hidden file -> no extension
        if (dot == 0)
            return Path {""};

        return Path {vbase::StringView {f.data() + dot, f.size() - dot}};
    }

    Path Path::join(vbase::StringView rel) const
    {
        if (rel.empty())
            return *this;

        // if rel is absolute -> return rel normalized
        if (!rel.empty() && rel[0] == '/')
            return Path {rel};

        if (m_Path.empty())
            return Path {rel};

        std::string combined = m_Path;

        if (combined.back() != '/')
            combined.push_back('/');

        combined.append(rel.data(), rel.size());

        return Path {combined};
    }

} // namespace vfilesystem