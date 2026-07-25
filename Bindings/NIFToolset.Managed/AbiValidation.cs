using System.Runtime.InteropServices;

namespace NIFToolset.Managed;

internal static class AbiValidation
{
    internal static void Validate()
    {
        ValidateSize<NifVec2>(8);
        ValidateSize<NifVec3>(12);
        ValidateSize<NifVec4>(16);
        ValidateSize<NifMat3>(36);
        ValidateSize<NifTransform>(52);
        ValidateSize<NifRect>(16);
        ValidateSize<NifFrustum>(28);
        ValidateSize<NifBound>(16);
        ValidateSize<NifColor>(12);
        ValidateSize<NifColorA>(16);
        ValidateSize<NativeCollisionIntersection>(IntPtr.Size == 8 ? 72 : 56);
        ValidateSize<NifTextKeyDesc>(IntPtr.Size == 8 ? 16 : 8);
        ValidateSize<NifKfmSequenceGroupEntryDesc>(28);
        ValidateSize<NifKfmBlendPairDesc>(IntPtr.Size * 2);
        ValidateSize<NifKfmChainEntryDesc>(8);
        ValidateSize<NifCollisionTriangle>(36);
        ValidateSize<NifDataStreamRegion>(8);
        ValidateSize<NifDataStreamElementDesc>(32);
        ValidateSize<NifDx11RendererDesc>(IntPtr.Size == 8 ? 96 : 88);
    }

    private static void ValidateSize<T>(int expected) where T : struct
    {
        int actual = Marshal.SizeOf<T>();
        if (actual != expected)
        {
            throw new TypeLoadException($"Native ABI mismatch for {typeof(T).Name}: expected {expected} bytes, got {actual}.");
        }
    }
}
