using Microsoft.Win32.SafeHandles;

namespace NIFToolset.Managed;

public abstract class NifSafeHandle : SafeHandleZeroOrMinusOneIsInvalid
{
    protected NifSafeHandle() : base(ownsHandle: true) { }

    internal nint DangerousValue => DangerousGetHandle();

    protected bool TryRelease(Action<nint> release)
    {
        try
        {
            release(handle);
            return true;
        }
        catch
        {
            // SafeHandle finalization must never throw during process shutdown.
            return false;
        }
    }
}

public sealed class SafeStreamHandle : NifSafeHandle
{
    internal SafeStreamHandle() { }
    internal SafeStreamHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Stream_Destroy);
}

public sealed class SafeObjectHandle : NifSafeHandle
{
    internal SafeObjectHandle() { }
    internal SafeObjectHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Object_Destroy);
}

public sealed class SafeAVObjectHandle : NifSafeHandle
{
    internal SafeAVObjectHandle() { }
    internal SafeAVObjectHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_AVObject_Destroy);
}

public sealed class SafeMeshHandle : NifSafeHandle
{
    internal SafeMeshHandle() { }
    internal SafeMeshHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Mesh_Destroy);
}

public sealed class SafeNodeHandle : NifSafeHandle
{
    internal SafeNodeHandle() { }
    internal SafeNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Node_Destroy);
}

public sealed class SafeBSPNodeHandle : NifSafeHandle
{
    internal SafeBSPNodeHandle() { }
    internal SafeBSPNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_BSPNode_Destroy);
}

public sealed class SafeBillboardNodeHandle : NifSafeHandle
{
    internal SafeBillboardNodeHandle() { }
    internal SafeBillboardNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_BillboardNode_Destroy);
}

public sealed class SafeSwitchNodeHandle : NifSafeHandle
{
    internal SafeSwitchNodeHandle() { }
    internal SafeSwitchNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_SwitchNode_Destroy);
}

public sealed class SafeLODNodeHandle : NifSafeHandle
{
    internal SafeLODNodeHandle() { }
    internal SafeLODNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_LODNode_Destroy);
}

public sealed class SafeSortAdjustNodeHandle : NifSafeHandle
{
    internal SafeSortAdjustNodeHandle() { }
    internal SafeSortAdjustNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_SortAdjustNode_Destroy);
}

public sealed class SafeTerrainHandle : NifSafeHandle
{
    internal SafeTerrainHandle() { }
    internal SafeTerrainHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Terrain_Destroy);
}

public sealed class SafeTerrainCellHandle : NifSafeHandle
{
    internal SafeTerrainCellHandle() { }
    internal SafeTerrainCellHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_TerrainCell_Destroy);
}

public sealed class SafeTerrainCellNodeHandle : NifSafeHandle
{
    internal SafeTerrainCellNodeHandle() { }
    internal SafeTerrainCellNodeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_TerrainCellNode_Destroy);
}

public sealed class SafeTerrainCellLeafHandle : NifSafeHandle
{
    internal SafeTerrainCellLeafHandle() { }
    internal SafeTerrainCellLeafHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_TerrainCellLeaf_Destroy);
}

public sealed class SafeTerrainSectorHandle : NifSafeHandle
{
    internal SafeTerrainSectorHandle() { }
    internal SafeTerrainSectorHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_TerrainSector_Destroy);
}

public sealed class SafeAtmosphereHandle : NifSafeHandle
{
    internal SafeAtmosphereHandle() { }
    internal SafeAtmosphereHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Atmosphere_Destroy);
}

public sealed class SafeEnvironmentHandle : NifSafeHandle
{
    internal SafeEnvironmentHandle() { }
    internal SafeEnvironmentHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Environment_Destroy);
}

public sealed class SafeSkyHandle : NifSafeHandle
{
    internal SafeSkyHandle() { }
    internal SafeSkyHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Sky_Destroy);
}

public sealed class SafeSkyDomeHandle : NifSafeHandle
{
    internal SafeSkyDomeHandle() { }
    internal SafeSkyDomeHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_SkyDome_Destroy);
}

public sealed class SafeDecorationFieldHandle : NifSafeHandle
{
    internal SafeDecorationFieldHandle() { }
    internal SafeDecorationFieldHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DecorationField_Destroy);
}

public sealed class SafeDecorationLayerHandle : NifSafeHandle
{
    internal SafeDecorationLayerHandle() { }
    internal SafeDecorationLayerHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DecorationLayer_Destroy);
}

public sealed class SafeDecorationPlaneHandle : NifSafeHandle
{
    internal SafeDecorationPlaneHandle() { }
    internal SafeDecorationPlaneHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DecorationPlane_Destroy);
}

public sealed class SafeCameraHandle : NifSafeHandle
{
    internal SafeCameraHandle() { }
    internal SafeCameraHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Camera_Destroy);
}

public sealed class SafeDataStreamHandle : NifSafeHandle
{
    internal SafeDataStreamHandle() { }
    internal SafeDataStreamHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DataStream_Destroy);
}

public sealed class SafeDataStreamRefHandle : NifSafeHandle
{
    internal SafeDataStreamRefHandle() { }
    internal SafeDataStreamRefHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DataStreamRef_Destroy);
}

public sealed class SafeControllerSequenceHandle : NifSafeHandle
{
    internal SafeControllerSequenceHandle() { }
    internal SafeControllerSequenceHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Animation_Sequence_Destroy);
}

public sealed class SafeSequenceDataHandle : NifSafeHandle
{
    internal SafeSequenceDataHandle() { }
    internal SafeSequenceDataHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Animation_SequenceData_Destroy);
}

public sealed class SafeTextKeyExtraDataHandle : NifSafeHandle
{
    internal SafeTextKeyExtraDataHandle() { }
    internal SafeTextKeyExtraDataHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Animation_TextKeys_Destroy);
}

public sealed class SafeKfmToolHandle : NifSafeHandle
{
    internal SafeKfmToolHandle() { }
    internal SafeKfmToolHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Animation_KFM_Destroy);
}

public sealed class SafeActorManagerHandle : NifSafeHandle
{
    internal SafeActorManagerHandle() { }
    internal SafeActorManagerHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Animation_ActorManager_Destroy);
}

public sealed class SafeCollisionDataHandle : NifSafeHandle
{
    internal SafeCollisionDataHandle() { }
    internal SafeCollisionDataHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Collision_Data_Destroy);
}

public sealed class SafeCollisionGroupHandle : NifSafeHandle
{
    internal SafeCollisionGroupHandle() { }
    internal SafeCollisionGroupHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Collision_Group_Destroy);
}

public sealed class SafePortalHandle : NifSafeHandle
{
    internal SafePortalHandle() { }
    internal SafePortalHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Portal_Destroy);
}

public sealed class SafeRoomHandle : NifSafeHandle
{
    internal SafeRoomHandle() { }
    internal SafeRoomHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Room_Destroy);
}

public sealed class SafeOldWallHandle : NifSafeHandle
{
    internal SafeOldWallHandle() { }
    internal SafeOldWallHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_OldWall_Destroy);
}

public sealed class SafeRoomGroupHandle : NifSafeHandle
{
    internal SafeRoomGroupHandle() { }
    internal SafeRoomGroupHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RoomGroup_Destroy);
}

public sealed class SafeRendererHandle : NifSafeHandle
{
    internal SafeRendererHandle() { }
    internal SafeRendererHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Renderer_Destroy);
}

public sealed class SafeRenderTargetGroupHandle : NifSafeHandle
{
    internal SafeRenderTargetGroupHandle() { }
    internal SafeRenderTargetGroupHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderTargetGroup_Destroy);
}

public sealed class SafeRenderBufferHandle : NifSafeHandle
{
    internal SafeRenderBufferHandle() { }
    internal SafeRenderBufferHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderBuffer_Destroy);
}

public sealed class SafeDepthStencilBufferHandle : NifSafeHandle
{
    internal SafeDepthStencilBufferHandle() { }
    internal SafeDepthStencilBufferHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DepthStencilBuffer_Destroy);
}

public sealed class SafeCullingProcessHandle : NifSafeHandle
{
    internal SafeCullingProcessHandle() { }
    internal SafeCullingProcessHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_CullingProcess_Destroy);
}

public sealed class SafeMeshCullingProcessHandle : NifSafeHandle
{
    internal SafeMeshCullingProcessHandle() { }
    internal SafeMeshCullingProcessHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_MeshCullingProcess_Destroy);
}

public sealed class SafeAlphaAccumulatorHandle : NifSafeHandle
{
    internal SafeAlphaAccumulatorHandle() { }
    internal SafeAlphaAccumulatorHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_AlphaAccumulator_Destroy);
}

public sealed class SafeRenderListProcessorHandle : NifSafeHandle
{
    internal SafeRenderListProcessorHandle() { }
    internal SafeRenderListProcessorHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderListProcessor_Destroy);
}

public sealed class SafeAlphaSortProcessorHandle : NifSafeHandle
{
    internal SafeAlphaSortProcessorHandle() { }
    internal SafeAlphaSortProcessorHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_AlphaSortProcessor_Destroy);
}

public sealed class SafeRenderViewHandle : NifSafeHandle
{
    internal SafeRenderViewHandle() { }
    internal SafeRenderViewHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderView_Destroy);
}

public sealed class SafeRenderView3DHandle : NifSafeHandle
{
    internal SafeRenderView3DHandle() { }
    internal SafeRenderView3DHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderView3D_Destroy);
}

public sealed class SafeRenderClickHandle : NifSafeHandle
{
    internal SafeRenderClickHandle() { }
    internal SafeRenderClickHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderClick_Destroy);
}

public sealed class SafeViewRenderClickHandle : NifSafeHandle
{
    internal SafeViewRenderClickHandle() { }
    internal SafeViewRenderClickHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_ViewRenderClick_Destroy);
}

public sealed class SafeRenderStepHandle : NifSafeHandle
{
    internal SafeRenderStepHandle() { }
    internal SafeRenderStepHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_RenderStep_Destroy);
}

public sealed class SafeDefaultClickRenderStepHandle : NifSafeHandle
{
    internal SafeDefaultClickRenderStepHandle() { }
    internal SafeDefaultClickRenderStepHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_DefaultClickRenderStep_Destroy);
}

public sealed class SafeParticleSystemHandle : NifSafeHandle
{
    internal SafeParticleSystemHandle() { }
    internal SafeParticleSystemHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Particle_System_Destroy);
}

public sealed class SafeParticleEmitterHandle : NifSafeHandle
{
    internal SafeParticleEmitterHandle() { }
    internal SafeParticleEmitterHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Particle_Emitter_Destroy);
}

public sealed class SafeFloodgateTaskHandle : NifSafeHandle
{
    internal SafeFloodgateTaskHandle() { }
    internal SafeFloodgateTaskHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Floodgate_Task_Destroy);
}

public sealed class SafeFloodgateWorkflowHandle : NifSafeHandle
{
    internal SafeFloodgateWorkflowHandle() { }
    internal SafeFloodgateWorkflowHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Floodgate_Workflow_Destroy);
}

public sealed class SafeFloodgateStreamHandle : NifSafeHandle
{
    internal SafeFloodgateStreamHandle() { }
    internal SafeFloodgateStreamHandle(nint ownedHandle) => SetHandle(ownedHandle);

    protected override bool ReleaseHandle() => TryRelease(NativeMethods.NIF_Floodgate_Stream_Destroy);
}
