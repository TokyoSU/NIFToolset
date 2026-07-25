# Bindings

- `NIFToolset.Native`: shared C ABI bridge used by P/Invoke.
- `NIFToolset.Managed`: C# interop assembly with two API layers:
  - an object-oriented model that mirrors the native Gamebryo/NIFToolset types;
  - the complete low-level `NativeMethods` P/Invoke surface.

The object model contains 56 disposable managed wrapper classes and follows the available native inheritance relationships, including:

```text
NiPSParticleSystem -> NiMesh -> NiAVObject -> NiObject
NiOldWall         -> NiNode -> NiAVObject -> NiObject
NiRoomGroup        -> NiNode -> NiAVObject -> NiObject
NiLODNode          -> NiSwitchNode -> NiNode -> NiAVObject -> NiObject
NiTerrainCellNode  -> NiTerrainCell -> NiNode -> NiAVObject -> NiObject
NiSkyDome          -> NiSky -> NiNode -> NiAVObject -> NiObject
NiRoom             -> NiNode -> NiAVObject -> NiObject
NiCamera           -> NiAVObject -> NiObject
NiPortal           -> NiAVObject -> NiObject
```

`NIFToolset.Managed/NativeMethods.cs` covers all 758 exported native entry points. `ObjectModel.Generated.cs` exposes 594 generated object-oriented methods/factories, with additional idiomatic properties and helpers in `ObjectModel.Convenience.cs`.

After changing the C ABI, run:

```powershell
python Bindings\NIFToolset.Managed\generate_bindings.py
python Bindings\NIFToolset.Managed\generate_object_model.py
python Bindings\verify_bindings.py
```

The CMake build produces `NIFToolset.Native.dll`. The managed project targets `netstandard2.1` and `net8.0`; the managed application and native DLL must use the same architecture, normally x64.
