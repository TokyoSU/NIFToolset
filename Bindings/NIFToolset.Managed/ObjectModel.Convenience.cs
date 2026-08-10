namespace NIFToolset.Managed;

public unsafe partial class NiStream
{
    public uint ObjectCount => GetObjectCount();
    public uint FileVersion => GetFileVersion();
    public uint UserVersion => GetUserVersion();
    public bool SourceIsLittleEndian => GetSourceIsLittleEndian() != 0;

    public bool SaveAsLittleEndian
    {
        get => GetSaveAsLittleEndian() != 0;
        set => SetSaveAsLittleEndian(NifNative.ToNativeBool(value));
    }

    /// <summary>
    /// Returns a new owned managed reference to the object at <paramref name="index"/>.
    /// Dispose the returned wrapper when it is no longer needed.
    /// </summary>
    public NiObject this[uint index] =>
        GetObjectAt(index) ?? throw new IndexOutOfRangeException($"No object exists at index {index}.");

    public void Load(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            throw new ArgumentException("Path cannot be null or whitespace.", nameof(path));
        }
        if (LoadFromFile(path) == 0)
        {
            NifNative.ThrowLastError(nameof(Load));
        }
    }

    public void Save(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            throw new ArgumentException("Path cannot be null or whitespace.", nameof(path));
        }
        if (SaveToFile(path) == 0)
        {
            NifNative.ThrowLastError(nameof(Save));
        }
    }

    public void Insert(NiObject obj)
    {
        if (InsertObject(obj) == 0)
        {
            NifNative.ThrowLastError(nameof(Insert));
        }
    }

    public void Remove(NiObject obj)
    {
        if (RemoveObject(obj) == 0)
        {
            NifNative.ThrowLastError(nameof(Remove));
        }
    }
}

public unsafe partial class NiObject
{
    public string Name
    {
        get => GetName();
        set => SetName(value);
    }

    public string NativeTypeName => GetTypeName();

    /// <summary>Exact known scene-graph node type, or Unknown for non-nodes and unlisted custom nodes.</summary>
    public NifNodeType NodeType => GetNodeType();

    public string NodeTypeName => NifNative.Utf8String(NativeMethods.NIF_NodeType_GetName(NodeType));
}

public unsafe partial class NiAVObject
{
    public NifVec3 Translation
    {
        get => GetTranslate();
        set => SetTranslate(value);
    }

    public NifVec3 WorldTranslation => GetWorldTranslate();

    public NifMat3 Rotation
    {
        get => GetRotate();
        set => SetRotate(value);
    }

    public NifMat3 WorldRotation => GetWorldRotate();

    public float Scale
    {
        get => GetScale();
        set => SetScale(value);
    }

    public float WorldScale => GetWorldScale();

    /// <summary>
    /// Gets a new owned managed reference to the attached collision data, or null.
    /// Dispose a non-null returned wrapper when it is no longer needed.
    /// Assigning a value shares the underlying native object; assigning null clears it.
    /// </summary>
    public NiCollisionData? CollisionData
    {
        get
        {
            TryGetCollisionData(out NiCollisionData? data);
            return data;
        }
        set
        {
            if (value is null)
            {
                ClearCollisionData();
            }
            else if (SetCollisionData(value) == 0)
            {
                NifNative.ThrowLastError(nameof(CollisionData));
            }
        }
    }
}

public unsafe partial class NiNode
{
    public uint ChildCount => GetChildCount();

    /// <summary>
    /// Returns a new owned managed reference to the child at <paramref name="index"/>.
    /// Dispose the returned wrapper when it is no longer needed.
    /// </summary>
    public NiAVObject this[uint index] =>
        GetChildAt(index) ?? throw new IndexOutOfRangeException($"No child exists at index {index}.");
}

public unsafe partial class NiCamera
{
    public NifVec3 WorldLocation => GetWorldLocation();
    public NifVec3 WorldDirection => GetWorldDirection();
    public NifVec3 WorldUp => GetWorldUp();
    public NifVec3 WorldRight => GetWorldRight();

    public NifFrustum Frustum
    {
        get => GetFrustum();
        set => SetFrustum(value);
    }

    public NifRect Viewport
    {
        get => GetViewPort();
        set => SetViewPort(value);
    }

    public float MinimumNearPlaneDistance
    {
        get => GetMinNearPlaneDist();
        set => SetMinNearPlaneDist(value);
    }

    public float MaximumFarNearRatio
    {
        get => GetMaxFarNearRatio();
        set => SetMaxFarNearRatio(value);
    }

    public float LODAdjust
    {
        get => GetLODAdjust();
        set => SetLODAdjust(value);
    }
}

public unsafe partial class NiMesh
{
    public uint VertexCount => GetVertexCount();
    public uint TotalPrimitiveCount => GetTotalPrimitiveCount();
    public uint StreamReferenceCount => GetStreamRefCount();
    public uint ModifierCount => GetModifierCount();

    public uint SubmeshCount
    {
        get => GetSubmeshCount();
        set => SetSubmeshCount(value);
    }

    public NifPrimitiveType PrimitiveType
    {
        get => (NifPrimitiveType)GetPrimitiveType();
        set => SetPrimitiveType((int)value);
    }

    public string PrimitiveTypeName => GetPrimitiveTypeString();

    public NifBound ModelBound
    {
        get => GetModelBound();
        set => SetModelBound(value);
    }
}

public unsafe partial class NiDataStream
{
    public uint Stride => GetStride();
    public uint SizeInBytes => GetSize();
    public uint TotalCount => GetTotalCount();
    public uint AccessMask => GetAccessMask();
    public NifDataStreamUsage Usage => (NifDataStreamUsage)GetUsage();
    public uint RegionCount => GetRegionCount();
    public uint ElementDescriptionCount => GetElementDescCount();
    public bool IsLocked => GetLocked() != 0;

    public bool Streamable
    {
        get => GetStreamable() != 0;
        set => SetStreamable(NifNative.ToNativeBool(value));
    }
}

public unsafe partial class NiDataStreamRef
{
    public uint Stride => GetStride();
    public uint SizeInBytes => GetSize();
    public uint TotalCount => GetTotalCount();
    public uint AccessMask => GetAccessMask();
    public NifDataStreamUsage Usage => (NifDataStreamUsage)GetUsage();
    public uint ElementDescriptionCount => GetElementDescCount();

    public bool PerInstance
    {
        get => IsPerInstance();
        set => SetPerInstance(NifNative.ToNativeBool(value));
    }
}

public unsafe partial class NiPSParticleSystem
{
    public uint MaximumParticleCount => GetMaxNumParticles();
    public uint ParticleCount => GetNumParticles();
    public float LastSimulationTime => GetLastTime();
    public uint EmitterCount => GetEmitterCount();
    public uint SpawnerCount => GetSpawnerCount();

    public bool WorldSpace
    {
        get => GetWorldSpace() != 0;
        set => SetWorldSpace(NifNative.ToNativeBool(value));
    }
}

public unsafe partial class NiPSEmitter
{
    public float Speed { get => GetSpeed(); set => SetSpeed(value); }
    public float SpeedVariation { get => GetSpeedVar(); set => SetSpeedVar(value); }
    public float SpeedFlipRatio { get => GetSpeedFlipRatio(); set => SetSpeedFlipRatio(value); }
    public float LifeSpan { get => GetLifeSpan(); set => SetLifeSpan(value); }
    public float LifeSpanVariation { get => GetLifeSpanVar(); set => SetLifeSpanVar(value); }
    public float Declination { get => GetDeclination(); set => SetDeclination(value); }
    public float DeclinationVariation { get => GetDeclinationVar(); set => SetDeclinationVar(value); }
    public float PlanarAngle { get => GetPlanarAngle(); set => SetPlanarAngle(value); }
    public float PlanarAngleVariation { get => GetPlanarAngleVar(); set => SetPlanarAngleVar(value); }
    public float Size { get => GetSize(); set => SetSize(value); }
    public float SizeVariation { get => GetSizeVar(); set => SetSizeVar(value); }
    public float RotationAngle { get => GetRotAngle(); set => SetRotAngle(value); }
    public float RotationAngleVariation { get => GetRotAngleVar(); set => SetRotAngleVar(value); }
    public float RotationSpeed { get => GetRotSpeed(); set => SetRotSpeed(value); }
    public float RotationSpeedVariation { get => GetRotSpeedVar(); set => SetRotSpeedVar(value); }
    public NifVec3 RotationAxis { get => GetRotAxis(); set => SetRotAxis(value); }
    public bool RandomRotationSpeedSign { get => GetRandomRotSpeedSign() != 0; set => SetRandomRotSpeedSign(NifNative.ToNativeBool(value)); }
    public bool RandomRotationAxis { get => GetRandomRotAxis() != 0; set => SetRandomRotAxis(NifNative.ToNativeBool(value)); }
}

public unsafe partial class NiControllerSequence
{
    public uint ActivationId => GetActivationId();
    public int State => GetState();
    public bool AdditiveBlend => IsAdditiveBlend();
    public int Priority => GetPriority();
    public float Duration => GetDuration();
    public float DurationDividedByFrequency => GetDurationDivFreq();

    public float Offset { get => GetOffset(); set => SetOffset(value); }
    public float Weight { get => GetWeight(); set => SetWeight(value); }
    public float Frequency { get => GetFrequency(); set => SetFrequency(value); }
}

public unsafe partial class NiSequenceData
{
    public float Duration => GetDuration();
    public float DurationDividedByFrequency => GetDurationDivFreq();
    public float Frequency { get => GetFrequency(); set => SetFrequency(value); }
    public int CycleType { get => GetCycleType(); set => SetCycleType(value); }
}

public unsafe partial class NiPortal
{
    public bool Active
    {
        get => GetActive() != 0;
        set => SetActive(NifNative.ToNativeBool(value));
    }

    public uint VertexCount => GetVertexCount();

    /// <summary>
    /// Gets a new owned managed reference to the object adjoining this portal.
    /// Dispose a non-null returned wrapper when it is no longer needed.
    /// Assigning null clears the native portal link.
    /// </summary>
    public NiAVObject? Adjoiner
    {
        get => GetAdjoiner();
        set => SetAdjoiner(value);
    }

    public NifBound PortalModelBound { get => GetModelBound(); set => SetModelBound(value); }
}

public unsafe partial class NiRenderView
{
    public string Name { get => GetName(); set => SetName(value); }
}

public unsafe partial class NiRenderClick
{
    public string Name { get => GetName(); set => SetName(value); }
}

public unsafe partial class NiRenderStep
{
    public string Name { get => GetName(); set => SetName(value); }

    public RenderStepCallbacks CreateCallbacks() => new(NativeRenderStepHandle);
}

public unsafe partial class NiCollisionData
{
    public static int GetCollisionTestType(NiAVObject object0, NiAVObject object1) =>
        NativeMethods.NIF_Collision_Data_GetCollisionTestType(
            NiHandleGuard.Get(object0, value => value.NativeAVObjectHandle, nameof(object0)),
            NiHandleGuard.Get(object1, value => value.NativeAVObjectHandle, nameof(object1)));

    public static bool ValidateForCollision(NiAVObject obj, int collisionMode) =>
        NativeMethods.NIF_Collision_Data_ValidateForCollision(
            NiHandleGuard.Get(obj, value => value.NativeAVObjectHandle, nameof(obj)), collisionMode) != 0;

    public static bool VelocityEnabled
    {
        get => NativeMethods.NIF_Collision_Data_GetEnableVelocity() != 0;
        set => NativeMethods.NIF_Collision_Data_SetEnableVelocity(NifNative.ToNativeBool(value));
    }
}

public unsafe partial class NiRoom
{
    public static uint CurrentTimestamp
    {
        get => NativeMethods.NIF_Room_GetCurrentTimestamp();
        set => NativeMethods.NIF_Room_SetCurrentTimestamp(value);
    }
}

public unsafe partial class NiRoomGroup
{
    public static bool PortallingDisabled
    {
        get => NativeMethods.NIF_RoomGroup_GetPortallingDisabled() != 0;
        set => NativeMethods.NIF_RoomGroup_SetPortallingDisabled(NifNative.ToNativeBool(value));
    }
}

public unsafe partial class NiRenderer
{
    public string DriverInfo => GetDriverInfo();
    public uint RendererId => GetRendererID();
    public uint FrameId => GetFrameID();
    public uint FrameState => GetFrameState();

    public NifColorA BackgroundColor
    {
        get => GetBackgroundColor();
        set => SetBackgroundColor(value);
    }

    public float DepthClear
    {
        get => GetDepthClear();
        set => SetDepthClear(value);
    }

    public uint StencilClear
    {
        get => GetStencilClear();
        set => SetStencilClear(value);
    }

    public bool VSync => BgfxGetVSync() != 0;

    public bool Resize(uint width, uint height, bool vsync = true) =>
        BgfxResize(width, height, vsync ? 1 : 0) != 0;

    public static uint DefaultClearMode => NativeMethods.NIF_Renderer_GetDefaultClearMode();

    public static NifBgfxRendererDesc CreateDefaultDescription()
    {
        NifBgfxRendererDesc description = default;
        NativeMethods.NIF_BgfxRenderer_FillDefaultDesc(&description);
        return description;
    }

    public static NifBgfxRendererDesc CreateWindowedDescription(nint windowHandle)
    {
        NifBgfxRendererDesc description = default;
        NativeMethods.NIF_BgfxRenderer_FillWindowedDesc(&description, windowHandle);
        return description;
    }

    public static NiRenderer CreateBgfx(NifBgfxRendererDesc description) =>
        NiRenderer.FromOwnedHandle(
            NiHandleGuard.Require(NativeMethods.NIF_BgfxRenderer_Create(&description), nameof(CreateBgfx)));
}

public static class NiAnimation
{
    public static string StartTextKey => NifNative.Utf8String(NativeMethods.NIF_Animation_GetStartTextKey());
    public static string EndTextKey => NifNative.Utf8String(NativeMethods.NIF_Animation_GetEndTextKey());
    public static string MorphTextKey => NifNative.Utf8String(NativeMethods.NIF_Animation_GetMorphTextKey());

    public static string LookupKfmReturnCode(int returnCode) =>
        NifNative.Utf8String(NativeMethods.NIF_Animation_KFM_LookupReturnCode(returnCode));
}

public static class NiRenderSubsystems
{
    public static void InitializeParticle() => NativeMethods.NIF_RenderSubsystems_InitParticle();
    public static void ShutdownParticle() => NativeMethods.NIF_RenderSubsystems_ShutdownParticle();
    public static void InitializePortal() => NativeMethods.NIF_RenderSubsystems_InitPortal();
    public static void ShutdownPortal() => NativeMethods.NIF_RenderSubsystems_ShutdownPortal();
    public static bool InitializeShadowManager() => NativeMethods.NIF_RenderSubsystems_InitShadowManager() != 0;
    public static void ShutdownShadowManager() => NativeMethods.NIF_RenderSubsystems_ShutdownShadowManager();
    public static void SetShadowManagerActive(bool active) =>
        NativeMethods.NIF_RenderSubsystems_SetShadowManagerActive(NifNative.ToNativeBool(active));
}

public static class NiFloodgateProcessor
{
    public static bool IsAvailable => NativeMethods.NIF_Floodgate_Processor_IsAvailable() != 0;

    public static uint WorkerThreadCount
    {
        get => NativeMethods.NIF_Floodgate_Processor_GetWorkerThreadCount();
        set
        {
            if (NativeMethods.NIF_Floodgate_Processor_SetWorkerThreadCount(value) == 0)
            {
                NifNative.ThrowLastError(nameof(WorkerThreadCount));
            }
        }
    }

    public static bool ParallelExecution
    {
        get => NativeMethods.NIF_Floodgate_Processor_GetParallelExecution() != 0;
        set
        {
            if (NativeMethods.NIF_Floodgate_Processor_SetParallelExecution(NifNative.ToNativeBool(value)) == 0)
            {
                NifNative.ThrowLastError(nameof(ParallelExecution));
            }
        }
    }

    public static bool Submit(NiSPWorkflow workflow, int priority) =>
        NativeMethods.NIF_Floodgate_Processor_Submit(
            NiHandleGuard.Get(workflow, value => value.NativeWorkflowHandle, nameof(workflow)), priority) != 0;

    public static bool Poll(NiSPWorkflow workflow) =>
        NativeMethods.NIF_Floodgate_Processor_Poll(
            NiHandleGuard.Get(workflow, value => value.NativeWorkflowHandle, nameof(workflow))) != 0;

    public static bool Wait(NiSPWorkflow workflow, uint timeoutMilliseconds) =>
        NativeMethods.NIF_Floodgate_Processor_Wait(
            NiHandleGuard.Get(workflow, value => value.NativeWorkflowHandle, nameof(workflow)), timeoutMilliseconds) != 0;

    public static void Clear(NiSPWorkflow workflow) =>
        NativeMethods.NIF_Floodgate_Processor_Clear(
            NiHandleGuard.Get(workflow, value => value.NativeWorkflowHandle, nameof(workflow)));
}
