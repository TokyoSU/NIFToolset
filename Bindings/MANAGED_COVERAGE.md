# Managed API coverage

The generated `NativeMethods.cs` covers every public function declared by the native C headers.

| Native header | Managed declarations |
|---|---:|
| `NIF.Native.Common.h` | 11 |
| `NIF.Native.System.h` | 5 |
| `NIF.Native.Main.Stream.h` | 17 |
| `NIF.Native.Main.Object.h` | 25 |
| `NIF.Native.Main.Scene.h` | 23 |
| `NIF.Native.Main.Camera.h` | 27 |
| `NIF.Native.Mesh.h` | 73 |
| `NIF.Native.Animation.h` | 144 |
| `NIF.Native.Particle.h` | 70 |
| `NIF.Native.Collision.h` | 50 |
| `NIF.Native.Portal.h` | 47 |
| `NIF.Native.Renderer.DX11.h` | 22 |
| `NIF.Native.Renderer.Utility.h` | 14 |
| `NIF.Native.RenderTarget.h` | 16 |
| `NIF.Native.RenderPipeline.h` | 102 |
| `NIF.Native.Floodgate.h` | 56 |

**Total: 758 declarations.**

- 664 are public low-level calls.
- 56 destroy functions remain internal and are invoked by the corresponding managed `SafeHandle`.
- 38 object-oriented managed classes wrap all owned native handle types.
- 594 methods/factories are generated in the object-oriented layer, before convenience properties and helpers.
- Pointer/array APIs intentionally remain unsafe.
- UTF-8 input strings use `LPUTF8Str`; returned native string pointers are copied with `NifNative.Utf8String`.

The verifier checks native declarations, C++ implementations, P/Invoke declarations, owned handles, wrapper classes, native call names, and wrapper argument counts.
