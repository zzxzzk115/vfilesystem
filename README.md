# vfilesystem

**vfilesystem** is a lightweight, composable filesystem abstraction layer for the Vultra ecosystem.

It provides standardised IO primitives (path, file, filesystem interfaces) and a virtual filesystem
composition layer (mount table), built on top of **vbase**.

---

## Scope

vfilesystem provides:

- `Path` (normalized UTF-8 paths using `/`)
- `IFile` and `IFileSystem` interfaces
- Backends:
  - `PhysicalFileSystem` (OS filesystem, rooted)
  - `MemoryFileSystem` (in-memory, for tests/tools)
- `VirtualFileSystem` (VFS mount table that composes multiple filesystems)

vfilesystem does **not** provide:

- Asset loading / caching / reference counting
- Streaming or async IO
- Task scheduling
- Global locking strategy

---

## Threading model

vfilesystem is **thread-compatible**:

- Stateless value types are safe to copy and use across threads.
- Individual `IFile` objects are **not** internally synchronized.
- Sharing an `IFileSystem` instance across threads requires external synchronization unless the backend documents otherwise.

vfilesystem does not impose a locking strategy.

---

## Examples

See `examples/` for usage samples.

---

## License

This project is under the [MIT](./LICENSE) license.