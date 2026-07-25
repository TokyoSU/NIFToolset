# NIFToolset.Managed

`NIFToolset.Managed` is the C# interop assembly for `NIFToolset.Native.dll`.
It provides an object-oriented layer for normal C# use and retains the full raw P/Invoke API for advanced interop.

## Recommended object-oriented API

The managed hierarchy mirrors the native types supported by the C ABI:

```text
NiNativeObject
├─ NiStream
├─ NiObject
│  ├─ NiAVObject
│  │  ├─ NiNode
│  │  │  ├─ NiBSPNode
│  │  │  ├─ NiBillboardNode
│  │  │  ├─ NiSwitchNode
│  │  │  │  └─ NiLODNode
│  │  │  ├─ NiSortAdjustNode
│  │  │  ├─ NiRoom
│  │  │  ├─ NiOldWall
│  │  │  ├─ NiRoomGroup
│  │  │  ├─ NiTerrain
│  │  │  ├─ NiTerrainCell
│  │  │  │  ├─ NiTerrainCellNode
│  │  │  │  └─ NiTerrainCellLeaf
│  │  │  ├─ NiTerrainSector
│  │  │  ├─ NiAtmosphere
│  │  │  ├─ NiEnvironment
│  │  │  ├─ NiSky
│  │  │  │  └─ NiSkyDome
│  │  │  ├─ NiDecorationField
│  │  │  ├─ NiDecorationLayer
│  │  │  └─ NiDecorationPlane
│  │  ├─ NiCamera
│  │  ├─ NiMesh
│  │  │  └─ NiPSParticleSystem
│  │  └─ NiPortal
│  ├─ NiDataStream
│  ├─ NiControllerSequence
│  ├─ NiSequenceData
│  ├─ NiTextKeyExtraData
│  ├─ NiPSEmitter
│  ├─ NiCollisionData
│  └─ NiRenderer
├─ NiDataStreamRef
├─ NiKFMTool
├─ NiActorManager
├─ NiCollisionGroup
├─ NiRenderTargetGroup
├─ NiRenderBuffer
├─ NiDepthStencilBuffer
├─ NiCullingProcess
│  └─ NiMeshCullingProcess
├─ NiAlphaAccumulator
├─ NiRenderListProcessor
│  └─ NiAlphaSortProcessor
├─ NiRenderView
│  └─ NiRenderView3D
├─ NiRenderClick
│  └─ NiViewRenderClick
├─ NiRenderStep
│  └─ NiDefaultClickRenderStep
├─ NiSPTask
├─ NiSPWorkflow
└─ NiSPStream
```

Value types such as `NifVec3`, `NifMat3`, `NifTransform`, `NifBound`, renderer descriptions, collision descriptions, and enums remain blittable C# structs/enums.

### Load and inspect a NIF

```csharp
using NIFToolset.Managed;

using NifRuntime runtime = NifRuntime.Initialize();
using NiStream stream = NiStream.Create();

stream.Load(@"C:\Game\model.nif");

for (uint index = 0; index < stream.ObjectCount; index++)
{
    using NiObject obj = stream[index];
    Console.WriteLine($"{obj.NativeTypeName}: {obj.Name}");

    if (obj is NiMesh mesh)
    {
        Console.WriteLine(
            $"Vertices={mesh.VertexCount}, Submeshes={mesh.SubmeshCount}");
    }
}
```

`NiObjectFactory` automatically chooses the most specific supported wrapper when an object is loaded from a stream or retrieved through a generic object handle.

Node RTTI is exposed without string comparisons through `NifNodeType`:

```csharp
using NiObject obj = stream[0];
Console.WriteLine(obj.NodeType);

if (obj is NiLODNode lodNode)
{
    // NiLODNode inherits NiSwitchNode, NiNode, NiAVObject, and NiObject members.
    Console.WriteLine(lodNode.ChildCount);
}
```

### Create and edit a mesh

```csharp
using NifRuntime runtime = NifRuntime.Initialize();
using NiMesh mesh = NiMesh.Create();

mesh.Name = "TerrainChunk";
mesh.PrimitiveType = NifPrimitiveType.Triangles;
mesh.SubmeshCount = 4;
mesh.Translation = new NifVec3 { X = 10, Y = 20, Z = 5 };
mesh.RecomputeBounds();
```

The C++ inheritance relationship is also present in C#. A `NiMesh` can directly use `NiAVObject` and `NiObject` members such as `Translation`, `Rotation`, `Scale`, `Update`, `Name`, and `IsKindOf`.

## Ownership and disposal

Every wrapper returned from a native getter owns a native wrapper reference and implements `IDisposable`.

```csharp
using NiNode root = NiNode.Create();
using NiAVObject child = root[0];
using NiCollisionData? collision = child.CollisionData;
```

Important rules:

- Dispose every returned `Ni*` wrapper, normally with `using`.
- Keep `NifRuntime` alive longer than all wrappers.
- Disposing a wrapper releases its native reference; it does not necessarily destroy a shared Gamebryo object immediately.
- Do not create two managed owners around the same raw native wrapper pointer.
- Getters such as `NiStream[index]`, `NiNode[index]`, `GetModifierAt`, `GetDataStream`, `CollisionData`, and `Adjoiner` return new owned references.

## Raw P/Invoke API

`NativeMethods` contains all 758 native exports. Use it when a high-level wrapper is inconvenient or when working directly with pointer/array functions:

```csharp
using SafeMeshHandle mesh = NativeMethods.NIF_Mesh_Create();
NativeMethods.NIF_Mesh_SetName(mesh, "TerrainChunk");
uint count = NativeMethods.NIF_Mesh_GetSubmeshCount(mesh);
```

The 56 destroy imports are internal because their matching `SafeHandle` classes release them automatically.

## Pointer and array functions

Functions involving arbitrary buffers intentionally remain `unsafe`. Pin managed memory or use stack allocation, and obey the native count and lifetime requirements:

```csharp
unsafe
{
    NifVec3[] vertices =
    {
        new() { X = -1, Y = 0, Z = 0 },
        new() { X =  1, Y = 0, Z = 0 },
        new() { X =  0, Y = 1, Z = 0 },
    };

    fixed (NifVec3* data = vertices)
    {
        using NiPortal portal = NiPortal.Create(
            (uint)vertices.Length,
            data,
            null!,
            NifNative.ToNativeBool(true));
    }
}
```

Pointers returned by `NiDataStream.Lock`, `NiDataStream.LockRegion`, or `NiSPStream.GetData` are borrowed. Never free them and never use them beyond the corresponding native lock/data lifetime.

## Add the project to a solution

```powershell
dotnet sln MySolution.sln add path\to\NIFToolset.Managed\NIFToolset.Managed.csproj
dotnet add path\to\MyApplication.csproj reference path\to\NIFToolset.Managed\NIFToolset.Managed.csproj
```

Build the application as x64 and place `NIFToolset.Native.dll` and its native dependencies beside the application executable.

## Regeneration

After changing a public native binding header:

```powershell
python Bindings\NIFToolset.Managed\generate_bindings.py
python Bindings\NIFToolset.Managed\generate_object_model.py
python Bindings\NIFToolset.Managed\verify_object_model.py
python Bindings\verify_bindings.py
```

`ObjectModel.Generated.cs` should not be edited manually. Put handwritten properties and higher-level behavior in partial-class files such as `ObjectModel.Convenience.cs`.
