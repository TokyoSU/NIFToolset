namespace NIFToolset.Managed;

/// <summary>
/// Owns the four native AVObject wrappers returned by an intersection query.
/// The handle values are borrowed from this object and are valid only until Dispose.
/// </summary>
public sealed class CollisionIntersection : IDisposable
{
    private NativeCollisionIntersection _native;
    private bool _disposed;

    private CollisionIntersection(NativeCollisionIntersection native)
    {
        _native = native;
    }

    public nint Root0 => GetHandle(_native.Root0);
    public nint Root1 => GetHandle(_native.Root1);
    public nint Object0 => GetHandle(_native.Object0);
    public nint Object1 => GetHandle(_native.Object1);
    public float Time => GetValue(_native.Time);
    public NifVec3 Point => GetValue(_native.Point);
    public NifVec3 Normal0 => GetValue(_native.Normal0);
    public NifVec3 Normal1 => GetValue(_native.Normal1);

    public static CollisionIntersection? Find(
        SafeCollisionDataHandle first,
        SafeCollisionDataHandle second,
        float deltaTime,
        bool calculateNormals = true)
    {
        NifNative.EnsureInitialized();
        if (first is null) throw new ArgumentNullException(nameof(first));
        if (second is null) throw new ArgumentNullException(nameof(second));

        NifNative.ClearLastError();
        int found = NativeMethods.NIF_Collision_Data_FindABVIntersect(
            first,
            second,
            deltaTime,
            calculateNormals ? 1 : 0,
            out NativeCollisionIntersection native);

        if (found != 0)
        {
            return new CollisionIntersection(native);
        }
        if (NifNative.LastErrorCode != NifResult.Ok)
        {
            NifNative.ThrowLastError(nameof(Find));
        }
        return null;
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        try
        {
            NativeMethods.NIF_CollisionIntersectDesc_Release(ref _native);
        }
        finally
        {
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }

    ~CollisionIntersection()
    {
        try
        {
            Dispose();
        }
        catch
        {
            // Never let process-shutdown interop failures escape a finalizer.
        }
    }

    private nint GetHandle(nint value)
    {
        if (_disposed) throw new ObjectDisposedException(nameof(CollisionIntersection));
        return value;
    }

    private T GetValue<T>(T value)
    {
        if (_disposed) throw new ObjectDisposedException(nameof(CollisionIntersection));
        return value;
    }
}
