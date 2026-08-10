using System.Runtime.InteropServices;

namespace NIFToolset.Managed;

public enum NifResult
{
    Ok = 0,
    InvalidHandle = 1,
    InvalidArgument = 2,
    OutOfRange = 3,
    InvalidType = 4,
    EngineError = 5,
    OutOfMemory = 6,
    Exception = 7,
    NotSupported = 8,
}

public enum NifNodeType
{
    Unknown = 0,
    Node = 1,
    BspNode = 2,
    BillboardNode = 3,
    SwitchNode = 4,
    LodNode = 5,
    SortAdjustNode = 6,
    Room = 7,
    RoomGroup = 8,
    Terrain = 9,
    TerrainCell = 10,
    TerrainCellNode = 11,
    TerrainCellLeaf = 12,
    TerrainSector = 13,
    Atmosphere = 14,
    Environment = 15,
    Sky = 16,
    SkyDome = 17,
    DecorationField = 18,
    DecorationLayer = 19,
    DecorationPlane = 20,
    OldWall = 21,
}

public enum NifPrimitiveType
{
    Triangles = 0,
    TriangleStrips = 1,
    Lines = 2,
    LineStrips = 3,
    Quads = 4,
    Points = 5,
}

public enum NifDataStreamUsage
{
    VertexIndex = 0,
    Vertex = 1,
    ShaderConstant = 2,
    User = 3,
    DisplayList = 4,
}

[StructLayout(LayoutKind.Sequential)]
public struct NifVec2
{
    public float X;
    public float Y;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifVec3
{
    public float X;
    public float Y;
    public float Z;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifVec4
{
    public float X;
    public float Y;
    public float Z;
    public float W;
}

[StructLayout(LayoutKind.Sequential)]
public unsafe struct NifMat3
{
    public fixed float Values[9];
}

[StructLayout(LayoutKind.Sequential)]
public struct NifTransform
{
    public NifMat3 Rotate;
    public NifVec3 Translate;
    public float Scale;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifRect
{
    public float Left;
    public float Right;
    public float Top;
    public float Bottom;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifFrustum
{
    public float Left;
    public float Right;
    public float Top;
    public float Bottom;
    public float NearPlane;
    public float FarPlane;
    public int IsOrtho;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifBound
{
    public NifVec3 Center;
    public float Radius;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifColor
{
    public float R;
    public float G;
    public float B;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifColorA
{
    public float R;
    public float G;
    public float B;
    public float A;
}

/// <summary>Raw native collision result. Prefer <see cref="CollisionIntersection"/> so owned handles are released safely.</summary>
[StructLayout(LayoutKind.Sequential)]
public struct NativeCollisionIntersection
{
    public nint Root0;
    public nint Root1;
    public nint Object0;
    public nint Object1;
    public float Time;
    public NifVec3 Point;
    public NifVec3 Normal0;
    public NifVec3 Normal1;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifTextKeyDesc
{
    public float Time;
    public nint Text;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifKfmSequenceGroupEntryDesc
{
    public uint SequenceId;
    public int Priority;
    public float Weight;
    public float EaseInTime;
    public float EaseOutTime;
    public uint SynchronizeSequenceId;
    public int Additive;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifKfmBlendPairDesc
{
    public nint StartKey;
    public nint TargetKey;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifKfmChainEntryDesc
{
    public uint SequenceId;
    public float Duration;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifCollisionTriangle
{
    public NifVec3 Point0;
    public NifVec3 Point1;
    public NifVec3 Point2;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifDataStreamRegion
{
    public uint StartIndex;
    public uint Range;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifDataStreamElementDesc
{
    public int Format;
    public uint Offset;
    public uint SizeInBytes;
    public uint ComponentCount;
    public uint ComponentSize;
    public int Type;
    public int IsNormalized;
    public int IsPacked;
}

[StructLayout(LayoutKind.Sequential)]
public struct NifBgfxRendererDesc
{
    public nint NativeWindowHandle;
    public uint Width;
    public uint Height;
    public int VSync;
}
