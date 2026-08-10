#!/usr/bin/env python3
from __future__ import annotations
import re
from pathlib import Path

MANAGED = Path(__file__).resolve().parent
ROOT = MANAGED.parent.parent
NATIVE = ROOT / 'Bindings/NIFToolset.Native'

HEADER_ORDER = [
    'NIF.Native.Common.h',
    'NIF.Native.System.h',
    'NIF.Native.Main.Stream.h',
    'NIF.Native.Main.Object.h',
    'NIF.Native.Main.Scene.h',
    'NIF.Native.Nodes.h',
    'NIF.Native.Main.Camera.h',
    'NIF.Native.Mesh.h',
    'NIF.Native.Animation.h',
    'NIF.Native.Particle.h',
    'NIF.Native.Collision.h',
    'NIF.Native.Portal.h',
    'NIF.Native.Renderer.Bgfx.h',
    'NIF.Native.Renderer.Utility.h',
    'NIF.Native.RenderTarget.h',
    'NIF.Native.RenderPipeline.h',
    'NIF.Native.Floodgate.h',
]

HANDLE_MAP = {
    'NIF_StreamHandle': 'SafeStreamHandle',
    'NIF_ObjectHandle': 'SafeObjectHandle',
    'NIF_AVObjectHandle': 'SafeAVObjectHandle',
    'NIF_MeshHandle': 'SafeMeshHandle',
    'NIF_NodeHandle': 'SafeNodeHandle',
    'NIF_BSPNodeHandle': 'SafeBSPNodeHandle',
    'NIF_BillboardNodeHandle': 'SafeBillboardNodeHandle',
    'NIF_SwitchNodeHandle': 'SafeSwitchNodeHandle',
    'NIF_LODNodeHandle': 'SafeLODNodeHandle',
    'NIF_SortAdjustNodeHandle': 'SafeSortAdjustNodeHandle',
    'NIF_TerrainHandle': 'SafeTerrainHandle',
    'NIF_TerrainCellHandle': 'SafeTerrainCellHandle',
    'NIF_TerrainCellNodeHandle': 'SafeTerrainCellNodeHandle',
    'NIF_TerrainCellLeafHandle': 'SafeTerrainCellLeafHandle',
    'NIF_TerrainSectorHandle': 'SafeTerrainSectorHandle',
    'NIF_AtmosphereHandle': 'SafeAtmosphereHandle',
    'NIF_EnvironmentHandle': 'SafeEnvironmentHandle',
    'NIF_SkyHandle': 'SafeSkyHandle',
    'NIF_SkyDomeHandle': 'SafeSkyDomeHandle',
    'NIF_DecorationFieldHandle': 'SafeDecorationFieldHandle',
    'NIF_DecorationLayerHandle': 'SafeDecorationLayerHandle',
    'NIF_DecorationPlaneHandle': 'SafeDecorationPlaneHandle',
    'NIF_CameraHandle': 'SafeCameraHandle',
    'NIF_DataStreamHandle': 'SafeDataStreamHandle',
    'NIF_DataStreamRefHandle': 'SafeDataStreamRefHandle',
    'NIF_ControllerSequenceHandle': 'SafeControllerSequenceHandle',
    'NIF_SequenceDataHandle': 'SafeSequenceDataHandle',
    'NIF_TextKeyExtraDataHandle': 'SafeTextKeyExtraDataHandle',
    'NIF_KFMToolHandle': 'SafeKfmToolHandle',
    'NIF_ActorManagerHandle': 'SafeActorManagerHandle',
    'NIF_CollisionDataHandle': 'SafeCollisionDataHandle',
    'NIF_CollisionGroupHandle': 'SafeCollisionGroupHandle',
    'NIF_PortalHandle': 'SafePortalHandle',
    'NIF_RoomHandle': 'SafeRoomHandle',
    'NIF_OldWallHandle': 'SafeOldWallHandle',
    'NIF_RoomGroupHandle': 'SafeRoomGroupHandle',
    'NIF_RendererHandle': 'SafeRendererHandle',
    'NIF_RenderTargetGroupHandle': 'SafeRenderTargetGroupHandle',
    'NIF_RenderBufferHandle': 'SafeRenderBufferHandle',
    'NIF_DepthStencilBufferHandle': 'SafeDepthStencilBufferHandle',
    'NIF_CullingProcessHandle': 'SafeCullingProcessHandle',
    'NIF_MeshCullingProcessHandle': 'SafeMeshCullingProcessHandle',
    'NIF_AlphaAccumulatorHandle': 'SafeAlphaAccumulatorHandle',
    'NIF_RenderListProcessorHandle': 'SafeRenderListProcessorHandle',
    'NIF_AlphaSortProcessorHandle': 'SafeAlphaSortProcessorHandle',
    'NIF_RenderViewHandle': 'SafeRenderViewHandle',
    'NIF_RenderView3DHandle': 'SafeRenderView3DHandle',
    'NIF_RenderClickHandle': 'SafeRenderClickHandle',
    'NIF_ViewRenderClickHandle': 'SafeViewRenderClickHandle',
    'NIF_RenderStepHandle': 'SafeRenderStepHandle',
    'NIF_DefaultClickRenderStepHandle': 'SafeDefaultClickRenderStepHandle',
    'NIF_ParticleSystemHandle': 'SafeParticleSystemHandle',
    'NIF_PSEmitterHandle': 'SafeParticleEmitterHandle',
    'NIF_FloodgateTaskHandle': 'SafeFloodgateTaskHandle',
    'NIF_FloodgateWorkflowHandle': 'SafeFloodgateWorkflowHandle',
    'NIF_FloodgateStreamHandle': 'SafeFloodgateStreamHandle',
}

TYPE_MAP = {
    'void': 'void',
    'int': 'int',
    'unsigned int': 'uint',
    'unsigned short': 'ushort',
    'float': 'float',
    'size_t': 'nuint',
    'NIF_Result': 'NifResult',
    'NIF_NodeType': 'NifNodeType',
    'NIF_Vec2': 'NifVec2',
    'NIF_Vec3': 'NifVec3',
    'NIF_Vec4': 'NifVec4',
    'NIF_Mat3': 'NifMat3',
    'NIF_Transform': 'NifTransform',
    'NIF_Rect': 'NifRect',
    'NIF_Frustum': 'NifFrustum',
    'NIF_Bound': 'NifBound',
    'NIF_Color': 'NifColor',
    'NIF_ColorA': 'NifColorA',
    'NIF_TextKeyDesc': 'NifTextKeyDesc',
    'NIF_KFMSequenceGroupEntryDesc': 'NifKfmSequenceGroupEntryDesc',
    'NIF_KFMBlendPairDesc': 'NifKfmBlendPairDesc',
    'NIF_KFMChainEntryDesc': 'NifKfmChainEntryDesc',
    'NIF_CollisionIntersectDesc': 'NativeCollisionIntersection',
    'NIF_CollisionTriangleDesc': 'NifCollisionTriangle',
    'NIF_DataStreamRegion': 'NifDataStreamRegion',
    'NIF_DataStreamElementDesc': 'NifDataStreamElementDesc',
    'NIF_BgfxRendererDesc': 'NifBgfxRendererDesc',
    'NIF_RenderStepCallback': 'RenderStepCallback?',
}

# Functions used by SafeHandle finalizers stay internal to discourage manual double-destruction.
DESTROY_NAMES = {
    'NIF_Stream_Destroy', 'NIF_Object_Destroy', 'NIF_AVObject_Destroy', 'NIF_Mesh_Destroy',
    'NIF_Node_Destroy', 'NIF_BSPNode_Destroy', 'NIF_BillboardNode_Destroy', 'NIF_SwitchNode_Destroy', 'NIF_LODNode_Destroy', 'NIF_SortAdjustNode_Destroy', 'NIF_Terrain_Destroy', 'NIF_TerrainCell_Destroy', 'NIF_TerrainCellNode_Destroy', 'NIF_TerrainCellLeaf_Destroy', 'NIF_TerrainSector_Destroy', 'NIF_Atmosphere_Destroy', 'NIF_Environment_Destroy', 'NIF_Sky_Destroy', 'NIF_SkyDome_Destroy', 'NIF_DecorationField_Destroy', 'NIF_DecorationLayer_Destroy', 'NIF_DecorationPlane_Destroy', 'NIF_Camera_Destroy', 'NIF_DataStream_Destroy', 'NIF_DataStreamRef_Destroy',
    'NIF_Animation_Sequence_Destroy', 'NIF_Animation_SequenceData_Destroy', 'NIF_Animation_TextKeys_Destroy',
    'NIF_Animation_KFM_Destroy', 'NIF_Animation_ActorManager_Destroy', 'NIF_Collision_Data_Destroy',
    'NIF_Collision_Group_Destroy', 'NIF_Portal_Destroy', 'NIF_Room_Destroy', 'NIF_OldWall_Destroy', 'NIF_RoomGroup_Destroy',
    'NIF_Renderer_Destroy', 'NIF_RenderTargetGroup_Destroy', 'NIF_RenderBuffer_Destroy',
    'NIF_DepthStencilBuffer_Destroy', 'NIF_CullingProcess_Destroy', 'NIF_MeshCullingProcess_Destroy',
    'NIF_AlphaAccumulator_Destroy', 'NIF_RenderListProcessor_Destroy', 'NIF_AlphaSortProcessor_Destroy',
    'NIF_RenderView_Destroy', 'NIF_RenderView3D_Destroy', 'NIF_RenderClick_Destroy',
    'NIF_ViewRenderClick_Destroy', 'NIF_RenderStep_Destroy', 'NIF_DefaultClickRenderStep_Destroy',
    'NIF_Particle_System_Destroy', 'NIF_Particle_Emitter_Destroy', 'NIF_Floodgate_Task_Destroy',
    'NIF_Floodgate_Workflow_Destroy', 'NIF_Floodgate_Stream_Destroy',
}

FUNC_RE = re.compile(r'NIFTOOLSET_NATIVE_ENTRY\s+([\w\s\*]+?)\s+(NIF_\w+)\s*\((.*?)\)\s*;', re.S)


def normalize(s: str) -> str:
    return ' '.join(s.replace('\r', ' ').replace('\n', ' ').split())


def parse_arg(arg: str) -> tuple[str, str, int, bool]:
    arg = normalize(arg)
    is_const = arg.startswith('const ')
    if is_const:
        arg = arg[6:].strip()
    # Capture the last identifier as parameter name; stars may be attached to either side.
    m = re.match(r'(.+?)([A-Za-z_]\w*)$', arg)
    if not m:
        raise ValueError(f'Cannot parse argument: {arg!r}')
    type_part = m.group(1).strip()
    name = m.group(2)
    stars = type_part.count('*')
    base = normalize(type_part.replace('*', ' '))
    return base, name, stars, is_const


def map_return(ret: str, func: str) -> str:
    ret = normalize(ret)
    is_const = ret.startswith('const ')
    if is_const:
        ret = ret[6:].strip()
    stars = ret.count('*')
    base = normalize(ret.replace('*', ' '))
    if base in HANDLE_MAP and stars == 0:
        return HANDLE_MAP[base]
    if base == 'char' and stars == 1:
        return 'nint'
    if base == 'void' and stars == 1:
        return 'nint'
    if stars:
        mapped = TYPE_MAP.get(base)
        if not mapped:
            raise KeyError(f'Unknown return pointer type {ret} in {func}')
        return mapped.rstrip('?') + ('*' * stars)
    if base in TYPE_MAP:
        return TYPE_MAP[base]
    raise KeyError(f'Unknown return type {ret} in {func}')


CSHARP_KEYWORDS = {
    'abstract', 'as', 'base', 'bool', 'break', 'byte', 'case', 'catch', 'char', 'checked',
    'class', 'const', 'continue', 'decimal', 'default', 'delegate', 'do', 'double', 'else',
    'enum', 'event', 'explicit', 'extern', 'false', 'finally', 'fixed', 'float', 'for',
    'foreach', 'goto', 'if', 'implicit', 'in', 'int', 'interface', 'internal', 'is',
    'lock', 'long', 'namespace', 'new', 'null', 'object', 'operator', 'out', 'override',
    'params', 'private', 'protected', 'public', 'readonly', 'ref', 'return', 'sbyte',
    'sealed', 'short', 'sizeof', 'stackalloc', 'static', 'string', 'struct', 'switch',
    'this', 'throw', 'true', 'try', 'typeof', 'uint', 'ulong', 'unchecked', 'unsafe',
    'ushort', 'using', 'virtual', 'void', 'volatile', 'while',
}

def escape_identifier(name: str) -> str:
    return '@' + name if name in CSHARP_KEYWORDS else name

def map_arg(base: str, name: str, stars: int, is_const: bool, func: str) -> str:
    name = escape_identifier(name)
    if func == 'NIF_CollisionIntersectDesc_Release' and name == 'intersect':
        return 'ref NativeCollisionIntersection intersect'
    if func == 'NIF_Collision_Data_FindABVIntersect' and name == 'intersect':
        return 'out NativeCollisionIntersection intersect'

    # A pointer to an opaque handle is always an owned output wrapper in the current C ABI.
    if base in HANDLE_MAP:
        if stars == 0:
            return f'{HANDLE_MAP[base]} {name}'
        if stars == 1:
            return f'out {HANDLE_MAP[base]} {name}'
        raise KeyError(f'Unsupported handle pointer depth in {func}: {base}{"*"*stars}')

    if base == 'char' and stars == 1:
        # NIF_CopyString accepts a native source pointer; all other const char* arguments are UTF-8 input strings.
        if is_const and not (func == 'NIF_CopyString' and name == 'source'):
            return f'[MarshalAs(UnmanagedType.LPUTF8Str)] string? {name}'
        return f'nint {name}'

    if base == 'void' and stars == 1:
        return f'nint {name}'

    if stars == 0:
        mapped = TYPE_MAP.get(base)
        if not mapped:
            raise KeyError(f'Unknown argument type {base} in {func}')
        return f'{mapped} {name}'

    mapped = TYPE_MAP.get(base)
    if not mapped:
        raise KeyError(f'Unknown pointer argument type {base} in {func}')

    # Preserve pointer semantics for arrays, optional values, and in/out parameters.
    return f'{mapped.rstrip("?")}{"*" * stars} {name}'


def parse_functions(path: Path):
    text = path.read_text(encoding='utf-8')
    for ret, name, args in FUNC_RE.findall(text):
        arg_list = []
        args = normalize(args)
        if args and args != 'void':
            for raw in args.split(','):
                base, arg_name, stars, is_const = parse_arg(raw)
                arg_list.append(map_arg(base, arg_name, stars, is_const, name))
        yield normalize(ret), name, arg_list


lines = [
    '// <auto-generated>',
    '// Generated from Bindings/NIFToolset.Native public C headers.',
    '// Run generate_managed_bindings.py after changing the native C ABI.',
    '// </auto-generated>',
    '',
    'using System.Runtime.InteropServices;',
    '',
    'namespace NIFToolset.Managed;',
    '',
    '/// <summary>',
    '/// Complete low-level P/Invoke surface for NIFToolset.Native.',
    '/// Prefer the higher-level managed wrappers when one is available.',
    '/// Every returned SafeHandle owns exactly one native wrapper handle.',
    '/// </summary>',
    'public static unsafe partial class NativeMethods',
    '{',
    '    public const string LibraryName = "NIFToolset.Native";',
    '    private const CallingConvention CallConvention = CallingConvention.Cdecl;',
    '',
    '    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]',
    '    public delegate int RenderStepCallback(nint renderStep, nint userData);',
]

count = 0
names = set()
for header_name in HEADER_ORDER:
    path = NATIVE / header_name
    funcs = list(parse_functions(path))
    lines += ['', f'    #region {header_name}']
    for ret, name, args in funcs:
        if name in names:
            raise RuntimeError(f'Duplicate function: {name}')
        names.add(name)
        cs_ret = map_return(ret, name)
        access = 'internal' if name in DESTROY_NAMES else 'public'
        # Destroy entry points receive a raw pointer from SafeHandle.ReleaseHandle, never another SafeHandle.
        if name in DESTROY_NAMES:
            args = ['nint handle']
        lines.append('    [DllImport(LibraryName, CallingConvention = CallConvention, ExactSpelling = true)]')
        lines.append(f'    {access} static extern {cs_ret} {name}({", ".join(args)});')
        count += 1
    lines += [f'    #endregion']
lines += ['}', '']

out = MANAGED / 'NativeMethods.cs'
out.write_text('\n'.join(lines), encoding='utf-8', newline='\n')
print(f'Wrote {out} with {count} functions')
