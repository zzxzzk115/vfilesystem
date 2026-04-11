# vfilesystem

<p align="center">
    <a href="https://github.com/zzxzzk115/vfilesystem/actions" alt="Build-Windows">
        <img src="https://img.shields.io/github/actions/workflow/status/zzxzzk115/vfilesystem/build_windows.yaml?branch=master&label=Build-Windows&logo=github" /></a>
    <a href="https://github.com/zzxzzk115/vfilesystem/actions" alt="Build-Linux">
        <img src="https://img.shields.io/github/actions/workflow/status/zzxzzk115/vfilesystem/build_linux.yaml?branch=master&label=Build-Linux&logo=github" /></a>
    <a href="https://github.com/zzxzzk115/vfilesystem/actions" alt="Build-macOS">
        <img src="https://img.shields.io/github/actions/workflow/status/zzxzzk115/vfilesystem/build_macos.yaml?branch=master&label=Build-macOS&logo=github" /></a>
    <a href="https://github.com/zzxzzk115/vfilesystem/actions" alt="Build-Android">
        <img src="https://img.shields.io/github/actions/workflow/status/zzxzzk115/vfilesystem/build_android.yaml?branch=master&label=Build-Android&logo=github" /></a>
    <a href="https://github.com/zzxzzk115/vfilesystem/actions" alt="Build-WASM">
        <img src="https://img.shields.io/github/actions/workflow/status/zzxzzk115/vfilesystem/build_wasm.yaml?branch=master&label=Build-WASM&logo=github" /></a>
</p>

**vfilesystem** is a lightweight, composable filesystem abstraction layer for the Vultra ecosystem.

It provides standardised IO primitives (path, file, filesystem interfaces) and a virtual filesystem
composition layer (mount table), built on top of **vbase**.

---

## Scope

vfilesystem provides:

- `Path` (normalized UTF-8 paths using `/`)
- `IFile` and `IFileSystem` interfaces
  - common filesystem operations: `open`, `readAll`, `writeAll`, `createDirectory`, `createDirectories`,
    `removeFile`, `removeDirectory`, `rename`, `list`
- Backends:
  - `PhysicalFileSystem` (OS filesystem, rooted)
  - `MemoryFileSystem` (in-memory, for tests/tools)
  - platform-aware filesystem factory (`makePlatformFileSystem()` for `assets` / `writable` semantics)
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

## Platform-aware dispatch

`makePlatformFileSystem()` provides a general entry point that dispatches to platform-specific filesystem handling.

- `PlatformFileSystemKind::eAssets`
  - Desktop platforms: rooted `PhysicalFileSystem(".")`
  - `WASM` / Emscripten: rooted `PhysicalFileSystem("/")`
  - `Android`: uses `AAssetManager*` when provided, otherwise requires `rootOverride`
- `PlatformFileSystemKind::eWritable`
  - Desktop platforms: rooted `PhysicalFileSystem(".")`
  - `WASM` / Emscripten: rooted `PhysicalFileSystem("/persistent")`
  - `Android`: requires `rootOverride` because the writable app sandbox path must come from the host app

Notes:

- `rootOverride` forces a rooted `PhysicalFileSystem` on desktop / `WASM`, and on Android when no `AAssetManager*` is supplied.
- When `AAssetManager*` is provided on Android, `rootOverride` is treated as an asset subdirectory inside the packaged asset tree.
- Android asset access is read-only when driven by `AAssetManager`, so mutating operations return `FsError::eNotSupported`.
- The library does not mount `IDBFS` for `WASM`; it only chooses the conventional writable root. Mounting/syncing remains the host application's responsibility.

---

## License

This project is under the [MIT](./LICENSE) license.
