# C# object model

The managed object model wraps every owned native handle type exposed by the C ABI. It is generated from `NativeMethods.cs`, while handwritten partial classes add idiomatic C# properties and validation.

## Design

- Native handle ownership remains implemented by `SafeHandle`.
- Public `Ni*` classes own one or more safe handles and implement `IDisposable` through `NiNativeObject`.
- Derived wrappers acquire native handles for each available base type, preserving valid base-class calls without unsafe pointer reinterpretation.
- Generic `NiObject` and `NiAVObject` results are upgraded by `NiObjectFactory` to the most specific known managed class.
- Native methods returning another handle create a new owned managed wrapper.
- Blittable mathematical and descriptor types remain structs rather than heap-allocated wrapper classes.

## Type mapping

| Native handle | Managed class | Managed base |
|---|---|---|
| `NIF_StreamHandle` | `NiStream` | `NiNativeObject` |
| `NIF_ObjectHandle` | `NiObject` | `NiNativeObject` |
| `NIF_AVObjectHandle` | `NiAVObject` | `NiObject` |
| `NIF_NodeHandle` | `NiNode` | `NiAVObject` |
| `NIF_BSPNodeHandle` | `NiBSPNode` | `NiNode` |
| `NIF_BillboardNodeHandle` | `NiBillboardNode` | `NiNode` |
| `NIF_SwitchNodeHandle` | `NiSwitchNode` | `NiNode` |
| `NIF_LODNodeHandle` | `NiLODNode` | `NiSwitchNode` |
| `NIF_SortAdjustNodeHandle` | `NiSortAdjustNode` | `NiNode` |
| `NIF_TerrainHandle` | `NiTerrain` | `NiNode` |
| `NIF_TerrainCellHandle` | `NiTerrainCell` | `NiNode` |
| `NIF_TerrainCellNodeHandle` | `NiTerrainCellNode` | `NiTerrainCell` |
| `NIF_TerrainCellLeafHandle` | `NiTerrainCellLeaf` | `NiTerrainCell` |
| `NIF_TerrainSectorHandle` | `NiTerrainSector` | `NiNode` |
| `NIF_AtmosphereHandle` | `NiAtmosphere` | `NiNode` |
| `NIF_EnvironmentHandle` | `NiEnvironment` | `NiNode` |
| `NIF_SkyHandle` | `NiSky` | `NiNode` |
| `NIF_SkyDomeHandle` | `NiSkyDome` | `NiSky` |
| `NIF_DecorationFieldHandle` | `NiDecorationField` | `NiNode` |
| `NIF_DecorationLayerHandle` | `NiDecorationLayer` | `NiNode` |
| `NIF_DecorationPlaneHandle` | `NiDecorationPlane` | `NiNode` |
| `NIF_CameraHandle` | `NiCamera` | `NiAVObject` |
| `NIF_MeshHandle` | `NiMesh` | `NiAVObject` |
| `NIF_ParticleSystemHandle` | `NiPSParticleSystem` | `NiMesh` |
| `NIF_PortalHandle` | `NiPortal` | `NiAVObject` |
| `NIF_RoomHandle` | `NiRoom` | `NiNode` |
| `NIF_OldWallHandle` | `NiOldWall` | `NiNode` |
| `NIF_RoomGroupHandle` | `NiRoomGroup` | `NiNode` |
| `NIF_DataStreamHandle` | `NiDataStream` | `NiObject` |
| `NIF_ControllerSequenceHandle` | `NiControllerSequence` | `NiObject` |
| `NIF_SequenceDataHandle` | `NiSequenceData` | `NiObject` |
| `NIF_TextKeyExtraDataHandle` | `NiTextKeyExtraData` | `NiObject` |
| `NIF_PSEmitterHandle` | `NiPSEmitter` | `NiObject` |
| `NIF_CollisionDataHandle` | `NiCollisionData` | `NiObject` |
| `NIF_RendererHandle` | `NiRenderer` | `NiObject` |
| `NIF_DataStreamRefHandle` | `NiDataStreamRef` | `NiNativeObject` |
| `NIF_KFMToolHandle` | `NiKFMTool` | `NiNativeObject` |
| `NIF_ActorManagerHandle` | `NiActorManager` | `NiNativeObject` |
| `NIF_CollisionGroupHandle` | `NiCollisionGroup` | `NiNativeObject` |
| `NIF_RenderTargetGroupHandle` | `NiRenderTargetGroup` | `NiNativeObject` |
| `NIF_RenderBufferHandle` | `NiRenderBuffer` | `NiNativeObject` |
| `NIF_DepthStencilBufferHandle` | `NiDepthStencilBuffer` | `NiNativeObject` |
| `NIF_CullingProcessHandle` | `NiCullingProcess` | `NiNativeObject` |
| `NIF_MeshCullingProcessHandle` | `NiMeshCullingProcess` | `NiCullingProcess` |
| `NIF_AlphaAccumulatorHandle` | `NiAlphaAccumulator` | `NiNativeObject` |
| `NIF_RenderListProcessorHandle` | `NiRenderListProcessor` | `NiNativeObject` |
| `NIF_AlphaSortProcessorHandle` | `NiAlphaSortProcessor` | `NiRenderListProcessor` |
| `NIF_RenderViewHandle` | `NiRenderView` | `NiNativeObject` |
| `NIF_RenderView3DHandle` | `NiRenderView3D` | `NiRenderView` |
| `NIF_RenderClickHandle` | `NiRenderClick` | `NiNativeObject` |
| `NIF_ViewRenderClickHandle` | `NiViewRenderClick` | `NiRenderClick` |
| `NIF_RenderStepHandle` | `NiRenderStep` | `NiNativeObject` |
| `NIF_DefaultClickRenderStepHandle` | `NiDefaultClickRenderStep` | `NiRenderStep` |
| `NIF_FloodgateTaskHandle` | `NiSPTask` | `NiNativeObject` |
| `NIF_FloodgateWorkflowHandle` | `NiSPWorkflow` | `NiNativeObject` |
| `NIF_FloodgateStreamHandle` | `NiSPStream` | `NiNativeObject` |

## Extending a wrapper

Use another partial declaration instead of editing the generated source:

```csharp
namespace NIFToolset.Managed;

public unsafe partial class NiMesh
{
    public bool HasGeometry => VertexCount != 0 && TotalPrimitiveCount != 0;
}
```

When a new native handle type is added, update `CONFIGS` in `generate_object_model.py`, add any required safe native base conversion, regenerate both layers, and run the verifier.

## Scene node coverage

The node wrapper list is verified against `NiTypeMask` in `NiRTTI.h` and the actual C++ inheritance declarations. All 21 RTTI types that are `NiNode` or derive from `NiNode` are represented. Entries such as `NiMaterialNode` are not scene-graph nodes; they derive from `NiRefObject` and are intentionally not treated as `NiNode` wrappers.
