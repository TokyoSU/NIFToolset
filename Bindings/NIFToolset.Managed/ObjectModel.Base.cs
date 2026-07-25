namespace NIFToolset.Managed;

/// <summary>
/// Base class for managed wrappers around owned NIFToolset native handles.
/// A wrapper can own several native handle wrappers when it represents a C++
/// inheritance chain. The underlying Gamebryo object remains reference counted.
/// </summary>
public abstract class NiNativeObject : IDisposable
{
    private readonly List<NifSafeHandle> _ownedHandles = new();
    private bool _disposed;

    public bool IsDisposed => _disposed;

    protected T Own<T>(T handle, string operation) where T : NifSafeHandle
    {
        ThrowIfDisposed();
        NiHandleGuard.Require(handle, operation);
        _ownedHandles.Add(handle);
        return handle;
    }

    protected void ThrowIfDisposed()
    {
        if (_disposed)
        {
            throw new ObjectDisposedException(GetType().Name);
        }
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        for (int index = _ownedHandles.Count - 1; index >= 0; index--)
        {
            _ownedHandles[index].Dispose();
        }
        _ownedHandles.Clear();
        GC.SuppressFinalize(this);
    }
}

internal static class NiHandleGuard
{
    internal static T Require<T>(T? handle, string operation) where T : NifSafeHandle
    {
        if (handle is not null && !handle.IsInvalid && !handle.IsClosed)
        {
            return handle;
        }

        handle?.Dispose();
        NifResult result = NifNative.LastErrorCode;
        string message = NifNative.LastErrorMessage;
        if (result != NifResult.Ok || !string.IsNullOrWhiteSpace(message))
        {
            NifNative.ThrowLastError(operation);
        }

        throw new NifNativeException(
            NifResult.InvalidHandle,
            $"{operation} returned an invalid native handle.");
    }

    internal static T RequireConverted<T>(int result, T? handle, string operation)
        where T : NifSafeHandle
    {
        if (result != 0)
        {
            return Require(handle, operation);
        }

        handle?.Dispose();
        NifResult error = NifNative.LastErrorCode;
        if (error != NifResult.Ok)
        {
            NifNative.ThrowLastError(operation);
        }

        throw new InvalidCastException($"{operation} could not convert the native object.");
    }

    internal static THandle Get<TObject, THandle>(
        TObject? value,
        Func<TObject, THandle> selector,
        string parameterName)
        where TObject : NiNativeObject
        where THandle : NifSafeHandle
    {
        if (value is null)
        {
            throw new ArgumentNullException(parameterName);
        }
        value.ThrowIfDisposedForInterop();
        return selector(value);
    }

    internal static THandle GetOptional<TObject, THandle>(
        TObject? value,
        Func<TObject, THandle> selector,
        string parameterName)
        where TObject : NiNativeObject
        where THandle : NifSafeHandle
    {
        if (value is null)
        {
            return null!;
        }
        value.ThrowIfDisposedForInterop();
        return selector(value);
    }

    internal static bool IsUsable(NifSafeHandle? handle) =>
        handle is not null && !handle.IsInvalid && !handle.IsClosed;

    internal static TWrapper? WrapNullable<THandle, TWrapper>(
        THandle? handle,
        Func<THandle, TWrapper> factory)
        where THandle : NifSafeHandle
        where TWrapper : NiNativeObject
    {
        if (!IsUsable(handle))
        {
            handle?.Dispose();
            return null;
        }

        try
        {
            return factory(handle!);
        }
        catch
        {
            handle!.Dispose();
            throw;
        }
    }

    private static void ThrowIfDisposedForInterop(this NiNativeObject value)
    {
        if (value.IsDisposed)
        {
            throw new ObjectDisposedException(value.GetType().Name);
        }
    }
}

/// <summary>Creates the most specific managed wrapper supported by the native C ABI.</summary>
public static class NiObjectFactory
{
    public static NiObject WrapOwned(SafeObjectHandle handle)
    {
        NiHandleGuard.Require(handle, nameof(WrapOwned));
        try
        {
            if (IsKindOf(handle, "NiPSParticleSystem") &&
                NativeMethods.NIF_Object_AsParticleSystem(handle, out SafeParticleSystemHandle particle) != 0)
            {
                handle.Dispose();
                return NiPSParticleSystem.FromOwnedHandle(particle);
            }

            switch (NativeMethods.NIF_Object_GetNodeType(handle))
            {
                case NifNodeType.LodNode:
                    if (NativeMethods.NIF_Object_AsLODNode(handle, out SafeLODNodeHandle lODNode) != 0)
                    {
                        handle.Dispose();
                        return NiLODNode.FromOwnedHandle(lODNode);
                    }
                    break;

                case NifNodeType.SwitchNode:
                    if (NativeMethods.NIF_Object_AsSwitchNode(handle, out SafeSwitchNodeHandle switchNode) != 0)
                    {
                        handle.Dispose();
                        return NiSwitchNode.FromOwnedHandle(switchNode);
                    }
                    break;

                case NifNodeType.BspNode:
                    if (NativeMethods.NIF_Object_AsBSPNode(handle, out SafeBSPNodeHandle bSPNode) != 0)
                    {
                        handle.Dispose();
                        return NiBSPNode.FromOwnedHandle(bSPNode);
                    }
                    break;

                case NifNodeType.BillboardNode:
                    if (NativeMethods.NIF_Object_AsBillboardNode(handle, out SafeBillboardNodeHandle billboardNode) != 0)
                    {
                        handle.Dispose();
                        return NiBillboardNode.FromOwnedHandle(billboardNode);
                    }
                    break;

                case NifNodeType.SortAdjustNode:
                    if (NativeMethods.NIF_Object_AsSortAdjustNode(handle, out SafeSortAdjustNodeHandle sortAdjustNode) != 0)
                    {
                        handle.Dispose();
                        return NiSortAdjustNode.FromOwnedHandle(sortAdjustNode);
                    }
                    break;

                case NifNodeType.TerrainCellNode:
                    if (NativeMethods.NIF_Object_AsTerrainCellNode(handle, out SafeTerrainCellNodeHandle terrainCellNode) != 0)
                    {
                        handle.Dispose();
                        return NiTerrainCellNode.FromOwnedHandle(terrainCellNode);
                    }
                    break;

                case NifNodeType.TerrainCellLeaf:
                    if (NativeMethods.NIF_Object_AsTerrainCellLeaf(handle, out SafeTerrainCellLeafHandle terrainCellLeaf) != 0)
                    {
                        handle.Dispose();
                        return NiTerrainCellLeaf.FromOwnedHandle(terrainCellLeaf);
                    }
                    break;

                case NifNodeType.TerrainCell:
                    if (NativeMethods.NIF_Object_AsTerrainCell(handle, out SafeTerrainCellHandle terrainCell) != 0)
                    {
                        handle.Dispose();
                        return NiTerrainCell.FromOwnedHandle(terrainCell);
                    }
                    break;

                case NifNodeType.TerrainSector:
                    if (NativeMethods.NIF_Object_AsTerrainSector(handle, out SafeTerrainSectorHandle terrainSector) != 0)
                    {
                        handle.Dispose();
                        return NiTerrainSector.FromOwnedHandle(terrainSector);
                    }
                    break;

                case NifNodeType.Terrain:
                    if (NativeMethods.NIF_Object_AsTerrain(handle, out SafeTerrainHandle terrain) != 0)
                    {
                        handle.Dispose();
                        return NiTerrain.FromOwnedHandle(terrain);
                    }
                    break;

                case NifNodeType.SkyDome:
                    if (NativeMethods.NIF_Object_AsSkyDome(handle, out SafeSkyDomeHandle skyDome) != 0)
                    {
                        handle.Dispose();
                        return NiSkyDome.FromOwnedHandle(skyDome);
                    }
                    break;

                case NifNodeType.Sky:
                    if (NativeMethods.NIF_Object_AsSky(handle, out SafeSkyHandle sky) != 0)
                    {
                        handle.Dispose();
                        return NiSky.FromOwnedHandle(sky);
                    }
                    break;

                case NifNodeType.Atmosphere:
                    if (NativeMethods.NIF_Object_AsAtmosphere(handle, out SafeAtmosphereHandle atmosphere) != 0)
                    {
                        handle.Dispose();
                        return NiAtmosphere.FromOwnedHandle(atmosphere);
                    }
                    break;

                case NifNodeType.Environment:
                    if (NativeMethods.NIF_Object_AsEnvironment(handle, out SafeEnvironmentHandle environment) != 0)
                    {
                        handle.Dispose();
                        return NiEnvironment.FromOwnedHandle(environment);
                    }
                    break;

                case NifNodeType.DecorationField:
                    if (NativeMethods.NIF_Object_AsDecorationField(handle, out SafeDecorationFieldHandle decorationField) != 0)
                    {
                        handle.Dispose();
                        return NiDecorationField.FromOwnedHandle(decorationField);
                    }
                    break;

                case NifNodeType.DecorationLayer:
                    if (NativeMethods.NIF_Object_AsDecorationLayer(handle, out SafeDecorationLayerHandle decorationLayer) != 0)
                    {
                        handle.Dispose();
                        return NiDecorationLayer.FromOwnedHandle(decorationLayer);
                    }
                    break;

                case NifNodeType.DecorationPlane:
                    if (NativeMethods.NIF_Object_AsDecorationPlane(handle, out SafeDecorationPlaneHandle decorationPlane) != 0)
                    {
                        handle.Dispose();
                        return NiDecorationPlane.FromOwnedHandle(decorationPlane);
                    }
                    break;

                case NifNodeType.OldWall:
                    if (NativeMethods.NIF_Object_AsOldWall(handle, out SafeOldWallHandle oldWall) != 0)
                    {
                        handle.Dispose();
                        return NiOldWall.FromOwnedHandle(oldWall);
                    }
                    break;

                case NifNodeType.RoomGroup:
                    if (NativeMethods.NIF_Object_AsRoomGroup(handle, out SafeRoomGroupHandle roomGroup) != 0)
                    {
                        handle.Dispose();
                        return NiRoomGroup.FromOwnedHandle(roomGroup);
                    }
                    break;

                case NifNodeType.Room:
                    if (NativeMethods.NIF_Object_AsRoom(handle, out SafeRoomHandle room) != 0)
                    {
                        handle.Dispose();
                        return NiRoom.FromOwnedHandle(room);
                    }
                    break;

                case NifNodeType.Node:
                    if (NativeMethods.NIF_Object_AsNode(handle, out SafeNodeHandle node) != 0)
                    {
                        handle.Dispose();
                        return NiNode.FromOwnedHandle(node);
                    }
                    break;
            }

            // Unknown NiNode-derived RTTI values still receive the generic NiNode wrapper.
            if (IsKindOf(handle, "NiNode") &&
                NativeMethods.NIF_Object_AsNode(handle, out SafeNodeHandle genericNode) != 0)
            {
                handle.Dispose();
                return NiNode.FromOwnedHandle(genericNode);
            }

            if (IsKindOf(handle, "NiMesh") &&
                NativeMethods.NIF_Object_AsMesh(handle, out SafeMeshHandle mesh) != 0)
            {
                handle.Dispose();
                return NiMesh.FromOwnedHandle(mesh);
            }

            if (IsKindOf(handle, "NiDataStream") &&
                NativeMethods.NIF_Object_AsDataStream(handle, out SafeDataStreamHandle dataStream) != 0)
            {
                handle.Dispose();
                return NiDataStream.FromOwnedHandle(dataStream);
            }

            if (IsKindOf(handle, "NiControllerSequence") &&
                NativeMethods.NIF_Object_AsControllerSequence(handle, out SafeControllerSequenceHandle sequence) != 0)
            {
                handle.Dispose();
                return NiControllerSequence.FromOwnedHandle(sequence);
            }

            if (IsKindOf(handle, "NiSequenceData") &&
                NativeMethods.NIF_Object_AsSequenceData(handle, out SafeSequenceDataHandle sequenceData) != 0)
            {
                handle.Dispose();
                return NiSequenceData.FromOwnedHandle(sequenceData);
            }

            if (IsKindOf(handle, "NiTextKeyExtraData") &&
                NativeMethods.NIF_Object_AsTextKeyExtraData(handle, out SafeTextKeyExtraDataHandle textKeys) != 0)
            {
                handle.Dispose();
                return NiTextKeyExtraData.FromOwnedHandle(textKeys);
            }

            if (IsKindOf(handle, "NiCollisionData") &&
                NativeMethods.NIF_Object_AsCollisionData(handle, out SafeCollisionDataHandle collisionData) != 0)
            {
                handle.Dispose();
                return NiCollisionData.FromOwnedHandle(collisionData);
            }

            if (IsKindOf(handle, "NiPSEmitter") &&
                NativeMethods.NIF_Object_AsParticleEmitter(handle, out SafeParticleEmitterHandle emitter) != 0)
            {
                handle.Dispose();
                return NiPSEmitter.FromOwnedHandle(emitter);
            }

            if (NativeMethods.NIF_Object_AsAVObject(handle, out SafeAVObjectHandle avObject) != 0)
            {
                handle.Dispose();
                return NiAVObject.FromOwnedHandle(avObject);
            }

            return NiObject.FromOwnedHandle(handle);
        }
        catch
        {
            if (!handle.IsClosed)
            {
                handle.Dispose();
            }
            throw;
        }
    }

    internal static NiAVObject WrapAVOwned(SafeAVObjectHandle handle)
    {
        NiHandleGuard.Require(handle, nameof(WrapAVOwned));
        SafeObjectHandle objectHandle = NativeMethods.NIF_AVObject_AsObject(handle);
        try
        {
            NiObject wrapped = WrapOwned(objectHandle);
            handle.Dispose();
            return wrapped as NiAVObject
                ?? throw new InvalidCastException("The native AV object did not produce a NiAVObject wrapper.");
        }
        catch
        {
            handle.Dispose();
            throw;
        }
    }

    private static bool IsKindOf(SafeObjectHandle handle, string nativeTypeName) =>
        NativeMethods.NIF_Object_IsKindOf(handle, nativeTypeName) != 0;
}
