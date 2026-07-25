# NIFToolset.Native ownership and C# interop contract

## Calling convention and scalar types

- Exported functions and callbacks use the C calling convention (`cdecl`).
- Native booleans cross the ABI as 32-bit `int`; use `int`, not C# `bool`, in P/Invoke declarations.
- Native `unsigned int` values are 32-bit and map to C# `uint`.
- `size_t` maps to C# `nuint`.
- Structs use sequential layout. The native project contains size assertions; the managed helper validates the same sizes before its high-level interop calls, or explicitly through `NifNative.ValidateAbi()`.

## Handle ownership

Every non-null handle returned by value or through an output parameter is an owned wrapper unless the API explicitly says that it is borrowed. Each owned wrapper must be released exactly once with its matching `*_Destroy` function. The wrapper usually holds a Gamebryo smart pointer, so releasing it decrements a reference and does not necessarily destroy a shared engine object.

Never create two owning managed `SafeHandle` instances from the same wrapper pointer. Conversions such as `Object_AsMesh` and getters such as `Stream_GetObjectAt` allocate a new wrapper and therefore produce independently disposable handles.

`NIF_Mesh_RemoveStreamRef` detaches the referenced stream and invalidates the contents of that wrapper; the wrapper itself must still be passed to `NIF_DataStreamRef_Destroy`.

## Borrowed memory

`NIF_DataStream_Lock` and `NIF_DataStream_LockRegion` return borrowed pointers. They remain valid only until the matching unlock and must never be freed by C#.

## Collision results

A successful `NIF_Collision_Data_FindABVIntersect` allocates every non-null handle inside `NIF_CollisionIntersectDesc`. Call `NIF_CollisionIntersectDesc_Release` exactly once, preferably from a `finally` block. The release function is tolerant of duplicate fields from older bridge versions and clears the entire structure.

## Render-step callbacks

The managed delegate must remain rooted for as long as the native callback is installed. Clear callbacks before releasing callback state. `NIF_RenderStep_Destroy` also unregisters callbacks owned by that wrapper, but it cannot root a C# delegate; use the `RenderStepCallbacks` helper in `NIFToolset.Managed`.

Managed exceptions must be caught inside the callback thunk and must never cross into native code. Callback installation, replacement, clearing, and wrapper disposal must be performed while the render step is not concurrently executing on another thread; the underlying Gamebryo callback setters do not provide a cross-thread quiescence guarantee.

## Strings

String input is passed as null-terminated UTF-8 bytes by the managed helper. Returned `const char*` values are borrowed and can be invalidated by native object mutation. Copy them immediately. Prefer the dedicated copy functions or `NIF_CopyString` rather than retaining pointers.

## Errors

The common error state is thread-local. Read `NIF_GetLastErrorCode` and copy `NIF_GetLastErrorMessage` on the same thread immediately after a failed call. Successful calls do not guarantee that an older error is cleared; use function return values as the primary success signal.
