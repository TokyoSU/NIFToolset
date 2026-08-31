# Binding safety and C# interop changes

This revision hardens the C ABI and adds an ownership-aware managed layer.

## Native bridge

- Replaced generic `void*` handles with distinct opaque pointer types while preserving pointer ABI compatibility.
- Added matching destroy functions for culling processes, render buffers, and depth/stencil buffers.
- Corrected `NiNew`/`NiDelete` allocator pairing and allocation-failure cleanup.
- Removed the incompatible particle-system-to-mesh wrapper cast.
- Added typed `cdecl` callback declarations, callback clearing, and wrapper destruction cleanup.
- Added collision-result release with independently owned result handles.
- Added thread-local result codes and copy-based error/string helpers.
- Added runtime type inspection and safe object-to-mesh/particle downcasts.
- Added stream save, insert, remove, version, and endianness functions.
- Added bounds and argument checks in high-risk mesh/render-target paths.
- Added ABI size assertions for structs shared with C#.
- Made `NIFToolset.Native` an explicit shared library and set the MSVC calling convention to `cdecl`.

## Managed bridge

- Added `SafeHandle` classes for every owned native wrapper type.
- Added UTF-8/error helpers, ABI validation, stream wrappers, RTTI/downcast helpers, and collision-result ownership.
- Added rooted render-step callback management that prevents managed exceptions from crossing native code.
- Targets `netstandard2.1` and `net8.0`.

## Validation

Run `python Bindings/verify_bindings.py` to check native declaration/definition coverage, managed imports, handle release coverage, whitespace, and public C/C++ header compatibility.

A full native build still requires the repository's Windows platform dependencies plus the vcpkg manifest dependencies (including bgfx and SDL3). Callback mutation/disposal must occur while the native render step is not concurrently executing.
