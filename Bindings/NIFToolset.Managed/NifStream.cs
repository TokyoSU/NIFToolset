namespace NIFToolset.Managed;

public sealed class NifStream : IDisposable
{
    private NifStream(SafeStreamHandle handle)
    {
        Handle = handle;
    }

    public SafeStreamHandle Handle { get; }

    public uint ObjectCount => NativeMethods.NIF_Stream_GetObjectCount(Handle);
    public uint FileVersion => NativeMethods.NIF_Stream_GetFileVersion(Handle);
    public uint UserVersion => NativeMethods.NIF_Stream_GetUserVersion(Handle);
    public bool SourceIsLittleEndian => NativeMethods.NIF_Stream_GetSourceIsLittleEndian(Handle) != 0;

    public bool SaveAsLittleEndian
    {
        get => NativeMethods.NIF_Stream_GetSaveAsLittleEndian(Handle) != 0;
        set => NativeMethods.NIF_Stream_SetSaveAsLittleEndian(Handle, value ? 1 : 0);
    }

    public static NifStream Create()
    {
        NifNative.EnsureInitialized();
        SafeStreamHandle handle = NativeMethods.NIF_Stream_Create();
        if (handle.IsInvalid)
        {
            handle.Dispose();
            NifNative.ThrowLastError(nameof(Create));
        }

        return new NifStream(handle);
    }

    public void Clear() => NativeMethods.NIF_Stream_Clear(Handle);

    public void Load(string path)
    {
        ValidatePath(path, nameof(path));
        if (NativeMethods.NIF_Stream_LoadFromFile(Handle, path) == 0)
        {
            NifNative.ThrowLastError(nameof(Load));
        }
    }

    public void Save(string path)
    {
        ValidatePath(path, nameof(path));
        if (NativeMethods.NIF_Stream_SaveToFile(Handle, path) == 0)
        {
            NifNative.ThrowLastError(nameof(Save));
        }
    }

    public void Insert(SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        if (NativeMethods.NIF_Stream_InsertObject(Handle, obj) == 0)
        {
            NifNative.ThrowLastError(nameof(Insert));
        }
    }

    public void Remove(SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        if (NativeMethods.NIF_Stream_RemoveObject(Handle, obj) == 0)
        {
            NifNative.ThrowLastError(nameof(Remove));
        }
    }

    public SafeObjectHandle GetObject(uint index)
    {
        SafeObjectHandle handle = NativeMethods.NIF_Stream_GetObjectAt(Handle, index);
        if (handle.IsInvalid)
        {
            handle.Dispose();
            NifNative.ThrowLastError(nameof(GetObject));
        }

        return handle;
    }

    private static void ValidatePath(string path, string parameterName)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            throw new ArgumentException("Path cannot be null or whitespace.", parameterName);
        }
    }

    private static void ThrowIfNull(object? value, string parameterName)
    {
        if (value is null)
        {
            throw new ArgumentNullException(parameterName);
        }
    }

    public void Dispose() => Handle.Dispose();
}

public static class NifObjectExtensions
{
    private static void ThrowIfNull(object? value, string parameterName)
    {
        if (value is null)
        {
            throw new ArgumentNullException(parameterName);
        }
    }

    public static string GetName(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        return NifNative.CopyUtf8String((destination, size) =>
            NativeMethods.NIF_Object_CopyName(obj, destination, size));
    }

    public static string GetNativeTypeName(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        return NifNative.CopyUtf8String((destination, size) =>
            NativeMethods.NIF_Object_CopyTypeName(obj, destination, size));
    }

    public static bool IsKindOf(this SafeObjectHandle obj, string nativeTypeName)
    {
        ThrowIfNull(obj, nameof(obj));
        if (string.IsNullOrWhiteSpace(nativeTypeName)) throw new ArgumentException("Value cannot be null or whitespace.", nameof(nativeTypeName));
        return NativeMethods.NIF_Object_IsKindOf(obj, nativeTypeName) != 0;
    }

    public static SafeAVObjectHandle? AsAVObject(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        int converted = NativeMethods.NIF_Object_AsAVObject(obj, out SafeAVObjectHandle result);
        if (converted != 0 && !result.IsInvalid)
        {
            return result;
        }

        result.Dispose();
        return null;
    }

    public static SafeNodeHandle? AsNode(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        int converted = NativeMethods.NIF_Object_AsNode(obj, out SafeNodeHandle result);
        if (converted != 0 && !result.IsInvalid)
        {
            return result;
        }

        result.Dispose();
        return null;
    }

    public static SafeCameraHandle? AsCamera(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        int converted = NativeMethods.NIF_Object_AsCamera(obj, out SafeCameraHandle result);
        if (converted != 0 && !result.IsInvalid)
        {
            return result;
        }

        result.Dispose();
        return null;
    }

    public static SafeMeshHandle? AsMesh(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        int converted = NativeMethods.NIF_Object_AsMesh(obj, out SafeMeshHandle mesh);
        if (converted != 0 && !mesh.IsInvalid)
        {
            return mesh;
        }

        mesh.Dispose();
        return null;
    }

    public static SafeParticleSystemHandle? AsParticleSystem(this SafeObjectHandle obj)
    {
        ThrowIfNull(obj, nameof(obj));
        int converted = NativeMethods.NIF_Object_AsParticleSystem(obj, out SafeParticleSystemHandle particleSystem);
        if (converted != 0 && !particleSystem.IsInvalid)
        {
            return particleSystem;
        }

        particleSystem.Dispose();
        return null;
    }
}
