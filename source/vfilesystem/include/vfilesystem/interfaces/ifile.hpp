#pragma once

#include <cstddef>
#include <cstdint>

namespace vfilesystem
{
    // Thread-compatible: a single IFile instance is NOT synchronized.
    class IFile
    {
    public:
        virtual ~IFile() = default;

        virtual size_t read(void* dst, size_t size)        = 0;
        virtual size_t write(const void* src, size_t size) = 0;

        virtual bool     seek(uint64_t offset) = 0;
        virtual uint64_t tell() const          = 0;

        virtual uint64_t size() const = 0;
    };
} // namespace vfilesystem
