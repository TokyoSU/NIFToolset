#!/usr/bin/env python3
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

MANAGED = Path(__file__).resolve().parent
NATIVE_METHODS = MANAGED / "NativeMethods.cs"
OUTPUT = MANAGED / "ObjectModel.Generated.cs"

@dataclass(frozen=True)
class Config:
    handle: str
    cls: str
    field: str
    prefixes: tuple[str, ...]
    parent: str | None = None
    conversion_kind: str | None = None  # direct or out
    conversion_function: str | None = None

CONFIGS = [
    Config("SafeStreamHandle", "NiStream", "StreamHandle", ("NIF_Stream_",)),
    Config("SafeObjectHandle", "NiObject", "ObjectHandle", ("NIF_Object_",)),
    Config("SafeAVObjectHandle", "NiAVObject", "AVObjectHandle", ("NIF_AVObject_",), "NiObject", "direct", "NIF_AVObject_AsObject"),
    Config("SafeNodeHandle", "NiNode", "NodeHandle", ("NIF_Node_",), "NiAVObject", "direct", "NIF_Node_AsAVObject"),
    Config("SafeBSPNodeHandle", "NiBSPNode", "BspNodeHandle", ("NIF_BSPNode_",), "NiNode", "direct", "NIF_BSPNode_AsNode"),
    Config("SafeBillboardNodeHandle", "NiBillboardNode", "BillboardNodeHandle", ("NIF_BillboardNode_",), "NiNode", "direct", "NIF_BillboardNode_AsNode"),
    Config("SafeSwitchNodeHandle", "NiSwitchNode", "SwitchNodeHandle", ("NIF_SwitchNode_",), "NiNode", "direct", "NIF_SwitchNode_AsNode"),
    Config("SafeLODNodeHandle", "NiLODNode", "LodNodeHandle", ("NIF_LODNode_",), "NiSwitchNode", "direct", "NIF_LODNode_AsSwitchNode"),
    Config("SafeSortAdjustNodeHandle", "NiSortAdjustNode", "SortAdjustNodeHandle", ("NIF_SortAdjustNode_",), "NiNode", "direct", "NIF_SortAdjustNode_AsNode"),
    Config("SafeTerrainHandle", "NiTerrain", "TerrainHandle", ("NIF_Terrain_",), "NiNode", "direct", "NIF_Terrain_AsNode"),
    Config("SafeTerrainCellHandle", "NiTerrainCell", "TerrainCellHandle", ("NIF_TerrainCell_",), "NiNode", "direct", "NIF_TerrainCell_AsNode"),
    Config("SafeTerrainCellNodeHandle", "NiTerrainCellNode", "TerrainCellNodeHandle", ("NIF_TerrainCellNode_",), "NiTerrainCell", "direct", "NIF_TerrainCellNode_AsTerrainCell"),
    Config("SafeTerrainCellLeafHandle", "NiTerrainCellLeaf", "TerrainCellLeafHandle", ("NIF_TerrainCellLeaf_",), "NiTerrainCell", "direct", "NIF_TerrainCellLeaf_AsTerrainCell"),
    Config("SafeTerrainSectorHandle", "NiTerrainSector", "TerrainSectorHandle", ("NIF_TerrainSector_",), "NiNode", "direct", "NIF_TerrainSector_AsNode"),
    Config("SafeAtmosphereHandle", "NiAtmosphere", "AtmosphereHandle", ("NIF_Atmosphere_",), "NiNode", "direct", "NIF_Atmosphere_AsNode"),
    Config("SafeEnvironmentHandle", "NiEnvironment", "EnvironmentHandle", ("NIF_Environment_",), "NiNode", "direct", "NIF_Environment_AsNode"),
    Config("SafeSkyHandle", "NiSky", "SkyHandle", ("NIF_Sky_",), "NiNode", "direct", "NIF_Sky_AsNode"),
    Config("SafeSkyDomeHandle", "NiSkyDome", "SkyDomeHandle", ("NIF_SkyDome_",), "NiSky", "direct", "NIF_SkyDome_AsSky"),
    Config("SafeDecorationFieldHandle", "NiDecorationField", "DecorationFieldHandle", ("NIF_DecorationField_",), "NiNode", "direct", "NIF_DecorationField_AsNode"),
    Config("SafeDecorationLayerHandle", "NiDecorationLayer", "DecorationLayerHandle", ("NIF_DecorationLayer_",), "NiNode", "direct", "NIF_DecorationLayer_AsNode"),
    Config("SafeDecorationPlaneHandle", "NiDecorationPlane", "DecorationPlaneHandle", ("NIF_DecorationPlane_",), "NiNode", "direct", "NIF_DecorationPlane_AsNode"),
    Config("SafeCameraHandle", "NiCamera", "CameraHandle", ("NIF_Camera_",), "NiAVObject", "direct", "NIF_Camera_AsAVObject"),
    Config("SafeMeshHandle", "NiMesh", "MeshHandle", ("NIF_Mesh_",), "NiAVObject", "direct", "NIF_Mesh_AsAVObject"),
    Config("SafeParticleSystemHandle", "NiPSParticleSystem", "ParticleSystemHandle", ("NIF_Particle_System_",), "NiMesh", "direct", "NIF_Particle_System_AsMesh"),
    Config("SafePortalHandle", "NiPortal", "PortalHandle", ("NIF_Portal_",), "NiAVObject", "out", "NIF_Portal_AsAVObject"),
    Config("SafeRoomHandle", "NiRoom", "RoomHandle", ("NIF_Room_",), "NiNode", "out", "NIF_Room_AsNode"),
    Config("SafeOldWallHandle", "NiOldWall", "OldWallHandle", ("NIF_OldWall_",), "NiNode", "direct", "NIF_OldWall_AsNode"),
    Config("SafeRoomGroupHandle", "NiRoomGroup", "RoomGroupHandle", ("NIF_RoomGroup_",), "NiNode", "out", "NIF_RoomGroup_AsNode"),
    Config("SafeDataStreamHandle", "NiDataStream", "DataStreamHandle", ("NIF_DataStream_",), "NiObject", "direct", "NIF_DataStream_AsObject"),
    Config("SafeControllerSequenceHandle", "NiControllerSequence", "ControllerSequenceHandle", ("NIF_Animation_Sequence_",), "NiObject", "direct", "NIF_Animation_Sequence_AsObject"),
    Config("SafeSequenceDataHandle", "NiSequenceData", "SequenceDataHandle", ("NIF_Animation_SequenceData_",), "NiObject", "direct", "NIF_Animation_SequenceData_AsObject"),
    Config("SafeTextKeyExtraDataHandle", "NiTextKeyExtraData", "TextKeyExtraDataHandle", ("NIF_Animation_TextKeys_",), "NiObject", "direct", "NIF_Animation_TextKeys_AsObject"),
    Config("SafeParticleEmitterHandle", "NiPSEmitter", "ParticleEmitterHandle", ("NIF_Particle_Emitter_",), "NiObject", "direct", "NIF_Particle_Emitter_AsObject"),
    Config("SafeCollisionDataHandle", "NiCollisionData", "CollisionDataHandle", ("NIF_Collision_Data_",), "NiObject", "direct", "NIF_Collision_Data_AsObject"),
    Config("SafeRendererHandle", "NiRenderer", "RendererHandle", ("NIF_BgfxRenderer_", "NIF_Renderer_"), "NiObject", "out", "NIF_Renderer_AsObject"),
    Config("SafeDataStreamRefHandle", "NiDataStreamRef", "DataStreamRefHandle", ("NIF_DataStreamRef_",)),
    Config("SafeKfmToolHandle", "NiKFMTool", "KFMToolHandle", ("NIF_Animation_KFM_",)),
    Config("SafeActorManagerHandle", "NiActorManager", "ActorManagerHandle", ("NIF_Animation_ActorManager_",)),
    Config("SafeCollisionGroupHandle", "NiCollisionGroup", "CollisionGroupHandle", ("NIF_Collision_Group_",)),
    Config("SafeRenderTargetGroupHandle", "NiRenderTargetGroup", "RenderTargetGroupHandle", ("NIF_RenderTargetGroup_",)),
    Config("SafeRenderBufferHandle", "NiRenderBuffer", "RenderBufferHandle", ("NIF_RenderBuffer_",)),
    Config("SafeDepthStencilBufferHandle", "NiDepthStencilBuffer", "DepthStencilBufferHandle", ("NIF_DepthStencilBuffer_",)),
    Config("SafeCullingProcessHandle", "NiCullingProcess", "CullingProcessHandle", ("NIF_CullingProcess_",)),
    Config("SafeMeshCullingProcessHandle", "NiMeshCullingProcess", "MeshCullingProcessHandle", ("NIF_MeshCullingProcess_",), "NiCullingProcess", "out", "NIF_MeshCullingProcess_AsCullingProcess"),
    Config("SafeAlphaAccumulatorHandle", "NiAlphaAccumulator", "AlphaAccumulatorHandle", ("NIF_AlphaAccumulator_",)),
    Config("SafeRenderListProcessorHandle", "NiRenderListProcessor", "RenderListProcessorHandle", ("NIF_RenderListProcessor_",)),
    Config("SafeAlphaSortProcessorHandle", "NiAlphaSortProcessor", "AlphaSortProcessorHandle", ("NIF_AlphaSortProcessor_",), "NiRenderListProcessor", "out", "NIF_AlphaSortProcessor_AsRenderListProcessor"),
    Config("SafeRenderViewHandle", "NiRenderView", "RenderViewHandle", ("NIF_RenderView_",)),
    Config("SafeRenderView3DHandle", "NiRenderView3D", "RenderView3DHandle", ("NIF_RenderView3D_",), "NiRenderView", "out", "NIF_RenderView3D_AsRenderView"),
    Config("SafeRenderClickHandle", "NiRenderClick", "RenderClickHandle", ("NIF_RenderClick_",)),
    Config("SafeViewRenderClickHandle", "NiViewRenderClick", "ViewRenderClickHandle", ("NIF_ViewRenderClick_",), "NiRenderClick", "out", "NIF_ViewRenderClick_AsRenderClick"),
    Config("SafeRenderStepHandle", "NiRenderStep", "RenderStepHandle", ("NIF_RenderStep_",)),
    Config("SafeDefaultClickRenderStepHandle", "NiDefaultClickRenderStep", "DefaultClickRenderStepHandle", ("NIF_DefaultClickRenderStep_",), "NiRenderStep", "out", "NIF_DefaultClickRenderStep_AsRenderStep"),
    Config("SafeFloodgateTaskHandle", "NiSPTask", "TaskHandle", ("NIF_Floodgate_Task_",)),
    Config("SafeFloodgateWorkflowHandle", "NiSPWorkflow", "WorkflowHandle", ("NIF_Floodgate_Workflow_",)),
    Config("SafeFloodgateStreamHandle", "NiSPStream", "SPStreamHandle", ("NIF_Floodgate_Stream_",)),
]

BY_HANDLE = {config.handle: config for config in CONFIGS}
BY_CLASS = {config.cls: config for config in CONFIGS}

# Methods already represented by the managed inheritance chain.
SKIP_ACTIONS = {
    "NiAVObject": {"AsObject", "GetName", "SetName"},
    "NiNode": {"AsAVObject"},
    "NiBSPNode": {"AsNode"},
    "NiBillboardNode": {"AsNode"},
    "NiSwitchNode": {"AsNode"},
    "NiLODNode": {"AsSwitchNode"},
    "NiSortAdjustNode": {"AsNode"},
    "NiTerrain": {"AsNode"},
    "NiTerrainCell": {"AsNode"},
    "NiTerrainCellNode": {"AsTerrainCell"},
    "NiTerrainCellLeaf": {"AsTerrainCell"},
    "NiTerrainSector": {"AsNode"},
    "NiAtmosphere": {"AsNode"},
    "NiEnvironment": {"AsNode"},
    "NiSky": {"AsNode"},
    "NiSkyDome": {"AsSky"},
    "NiDecorationField": {"AsNode"},
    "NiDecorationLayer": {"AsNode"},
    "NiDecorationPlane": {"AsNode"},
    "NiCamera": {"AsAVObject", "GetName", "SetName", "GetTranslate", "SetTranslate", "GetRotate", "SetRotate", "GetScale", "SetScale", "Update"},
    "NiMesh": {"AsAVObject", "AsObject", "GetName", "SetName"},
    "NiPSParticleSystem": {"AsMesh", "AsAVObject", "AsObject"},
    "NiPortal": {"AsAVObject"},
    "NiRoom": {"AsNode"},
    "NiOldWall": {"AsNode"},
    "NiRoomGroup": {"AsNode"},
    "NiDataStream": {"AsObject"},
    "NiControllerSequence": {"AsObject", "GetName"},
    "NiSequenceData": {"AsObject", "GetName"},
    "NiTextKeyExtraData": {"AsObject"},
    "NiPSEmitter": {"AsObject", "GetName", "SetName"},
    "NiCollisionData": {"AsObject"},
    "NiRenderer": {"AsObject"},
    "NiMeshCullingProcess": {"AsCullingProcess"},
    "NiAlphaSortProcessor": {"AsRenderListProcessor"},
    "NiRenderView3D": {"AsRenderView"},
    "NiViewRenderClick": {"AsRenderClick"},
    "NiDefaultClickRenderStep": {"AsRenderStep"},
}

RAW_POINTER_RETURNS = {
    "NIF_DataStream_Lock",
    "NIF_DataStream_LockRegion",
    "NIF_Floodgate_Stream_GetData",
}

OPTIONAL_HANDLE_PARAMETERS = {
    ("NIF_Portal_Create", "adjoiner"),
    ("NIF_Portal_SetAdjoiner", "adjoiner"),
    ("NIF_Animation_SequenceData_SetTextKeys", "textKeys"),
    ("NIF_RoomGroup_WhichRoomFrom", "lastRoom"),
    ("NIF_RoomGroup_SetLastRoom", "room"),
    ("NIF_RenderView3D_Create", "camera"),
    ("NIF_RenderView3D_Create", "cullingProcess"),
    ("NIF_RenderView3D_SetCamera", "camera"),
    ("NIF_RenderView3D_SetCullingProcess", "cullingProcess"),
    ("NIF_RenderClick_SetRenderTargetGroup", "renderTargetGroup"),
    ("NIF_ViewRenderClick_SetProcessor", "processor"),
    ("NIF_RenderStep_SetOutputRenderTargetGroup", "renderTargetGroup"),
    ("NIF_Renderer_SetSorter", "accumulator"),
}

METHOD_RE = re.compile(
    r"\s*(public|internal) static extern (?P<ret>[^\s]+) "
    r"(?P<name>NIF_[A-Za-z0-9_]+)\((?P<params>[^;]*)\);"
)

@dataclass
class Parameter:
    type: str
    name: str
    modifier: str = ""
    attribute: str = ""

@dataclass
class Method:
    access: str
    ret: str
    name: str
    params: list[Parameter]


def split_params(value: str) -> list[str]:
    if not value.strip():
        return []
    result: list[str] = []
    current: list[str] = []
    depth = 0
    for char in value:
        if char in "[(<":
            depth += 1
        elif char in "])>":
            depth -= 1
        if char == "," and depth == 0:
            result.append("".join(current).strip())
            current = []
        else:
            current.append(char)
    if current:
        result.append("".join(current).strip())
    return result


def parse_parameter(value: str) -> Parameter:
    attribute = ""
    match = re.match(r"(\[[^\]]+\])\s*(.*)", value)
    if match:
        attribute, value = match.groups()
    modifier = ""
    match = re.match(r"(out|ref|in)\s+(.*)", value)
    if match:
        modifier, value = match.groups()
    type_name, name = value.rsplit(" ", 1)
    return Parameter(type_name.strip(), name.strip(), modifier, attribute)


def parse_methods() -> list[Method]:
    text = NATIVE_METHODS.read_text(encoding="utf-8")
    methods: list[Method] = []
    for match in METHOD_RE.finditer(text):
        methods.append(Method(
            match.group(1),
            match.group("ret"),
            match.group("name"),
            [parse_parameter(param) for param in split_params(match.group("params"))],
        ))
    return methods


def matching_prefix(config: Config, name: str) -> str | None:
    candidates = [prefix for prefix in config.prefixes if name.startswith(prefix)]
    return max(candidates, key=len) if candidates else None


def action_name(config: Config, method: Method) -> str | None:
    prefix = matching_prefix(config, method.name)
    if prefix is None:
        return None
    action = method.name[len(prefix):]
    if prefix == "NIF_BgfxRenderer_":
        action = "Bgfx" + action
    return action


def is_factory(config: Config, method: Method, action: str | None) -> bool:
    if action is None or method.ret != config.handle:
        return False
    return action.startswith(("Create", "Load"))


def wrapper_type(native_type: str, nullable: bool = False) -> str:
    config = BY_HANDLE.get(native_type)
    if config is None:
        return native_type
    suffix = "?" if nullable else ""
    return config.cls + suffix


def is_optional_handle(method: Method, parameter: Parameter) -> bool:
    return (method.name, clean_name(parameter.name)) in OPTIONAL_HANDLE_PARAMETERS


def public_parameter(method: Method, parameter: Parameter) -> str:
    if parameter.type in BY_HANDLE:
        nullable = parameter.modifier == "out" or is_optional_handle(method, parameter)
        type_name = wrapper_type(parameter.type, nullable)
        if parameter.modifier == "out":
            return f"out {type_name} {parameter.name}"
        return f"{type_name} {parameter.name}"
    modifier = f"{parameter.modifier} " if parameter.modifier else ""
    return f"{modifier}{parameter.type} {parameter.name}"


def native_argument(method: Method, parameter: Parameter) -> str:
    if parameter.type in BY_HANDLE:
        target = BY_HANDLE[parameter.type]
        if parameter.modifier == "out":
            return f"out {parameter.type} native{clean_name(parameter.name)}"
        guard = "GetOptional" if is_optional_handle(method, parameter) else "Get"
        return (
            f"NiHandleGuard.{guard}({parameter.name}, value => value.Native{target.field}, "
            f"nameof({parameter.name}))"
        )
    modifier = f"{parameter.modifier} " if parameter.modifier else ""
    return modifier + parameter.name


def clean_name(name: str) -> str:
    return name[1:] if name.startswith("@") else name


def handle_result_nullable(action: str) -> bool:
    return action.startswith(("Get", "Find", "Which", "Lookup"))


def return_signature(method: Method, action: str, static_factory: bool) -> str:
    if method.ret in BY_HANDLE:
        if static_factory:
            return BY_HANDLE[method.ret].cls
        nullable = handle_result_nullable(action)
        return wrapper_type(method.ret, nullable)
    if method.ret == "nint" and method.name not in RAW_POINTER_RETURNS:
        return "string"
    if method.ret == "int" and action.startswith(("Is", "Has")):
        return "bool"
    return method.ret


def wrap_handle_expression(native_type: str, expression: str, nullable: bool, operation: str) -> str:
    config = BY_HANDLE[native_type]
    if native_type == "SafeObjectHandle":
        if nullable:
            return (
                f"NiHandleGuard.WrapNullable({expression}, NiObjectFactory.WrapOwned)"
            )
        return f"NiObjectFactory.WrapOwned(NiHandleGuard.Require({expression}, nameof({operation})))"
    if native_type == "SafeAVObjectHandle":
        if nullable:
            return (
                f"NiHandleGuard.WrapNullable({expression}, NiObjectFactory.WrapAVOwned)"
            )
        return f"NiObjectFactory.WrapAVOwned(NiHandleGuard.Require({expression}, nameof({operation})))"
    if nullable:
        return (
            f"NiHandleGuard.WrapNullable({expression}, {config.cls}.FromOwnedHandle)"
        )
    return (
        f"{config.cls}.FromOwnedHandle(NiHandleGuard.Require({expression}, nameof({operation})))"
    )


def emit_method(config: Config, method: Method, action: str, static_factory: bool) -> list[str]:
    owner_removed = False
    parameters = list(method.params)
    if not static_factory and parameters and parameters[0].type == config.handle and not parameters[0].modifier:
        parameters = parameters[1:]
        owner_removed = True
    if not static_factory and not owner_removed:
        return []

    out_handles = [p for p in parameters if p.modifier == "out" and p.type in BY_HANDLE]
    public_action = ("Try" + action) if out_handles and not action.startswith("Try") else action
    signature_params = ", ".join(public_parameter(method, param) for param in parameters)
    ret_type = "bool" if out_handles else return_signature(method, action, static_factory)
    static_token = "static " if static_factory else ""
    lines = [f"    public {static_token}{ret_type} {public_action}({signature_params})", "    {"]

    native_args: list[str] = []
    if not static_factory:
        native_args.append(f"Native{config.field}")
    native_args.extend(native_argument(method, param) for param in parameters)
    invocation = f"NativeMethods.{method.name}({', '.join(native_args)})"

    if out_handles:
        lines.append(f"        int result = {invocation};")
        for param in out_handles:
            target = BY_HANDLE[param.type]
            clean = clean_name(param.name)
            native_name = f"native{clean}"
            if param.type == "SafeObjectHandle":
                factory = "NiObjectFactory.WrapOwned"
            elif param.type == "SafeAVObjectHandle":
                factory = "NiObjectFactory.WrapAVOwned"
            else:
                factory = f"{target.cls}.FromOwnedHandle"
            lines.append(
                f"        {param.name} = result != 0 ? NiHandleGuard.WrapNullable({native_name}, {factory}) : null;"
            )
            lines.append(f"        if (result == 0) {native_name}?.Dispose();")
        lines.append("        return result != 0;")
    elif method.ret == "void":
        lines.append(f"        {invocation};")
    elif method.ret in BY_HANDLE:
        nullable = False if static_factory else handle_result_nullable(action)
        lines.append(
            f"        return {wrap_handle_expression(method.ret, invocation, nullable, public_action)};"
        )
    elif method.ret == "nint" and method.name not in RAW_POINTER_RETURNS:
        lines.append(f"        return NifNative.Utf8String({invocation});")
    elif method.ret == "int" and action.startswith(("Is", "Has")):
        lines.append(f"        return {invocation} != 0;")
    else:
        lines.append(f"        return {invocation};")

    lines += ["    }", ""]
    return lines


def emit_class(config: Config, methods: list[Method]) -> list[str]:
    base = config.parent or "NiNativeObject"
    lines = [f"public unsafe partial class {config.cls} : {base}", "{"]
    lines.append(f"    private readonly {config.handle} _{lower_first(config.field)};")
    lines.append("")
    lines.append(f"    internal {config.handle} Native{config.field}")
    lines.append("    {")
    lines.append("        get")
    lines.append("        {")
    lines.append("            ThrowIfDisposed();")
    lines.append(f"            return _{lower_first(config.field)};")
    lines.append("        }")
    lines.append("    }")
    lines.append("")

    if config.parent is None:
        lines.append(f"    internal {config.cls}({config.handle} handle)")
        lines.append("    {")
        lines.append(f"        _{lower_first(config.field)} = Own(handle, nameof({config.cls}));")
        lines.append("    }")
    else:
        assert config.conversion_function and config.conversion_kind
        lines.append(f"    internal {config.cls}({config.handle} handle)")
        lines.append(f"        : base(GetBaseHandle(handle))")
        lines.append("    {")
        lines.append(f"        _{lower_first(config.field)} = Own(handle, nameof({config.cls}));")
        lines.append("    }")
        lines.append("")
        parent_handle = BY_CLASS[config.parent].handle
        lines.append(f"    private static {parent_handle} GetBaseHandle({config.handle} handle)")
        lines.append("    {")
        if config.conversion_kind == "direct":
            lines.append(
                f"        return NiHandleGuard.Require(NativeMethods.{config.conversion_function}(handle), nameof(NativeMethods.{config.conversion_function}));"
            )
        else:
            lines.append(
                f"        int converted = NativeMethods.{config.conversion_function}(handle, out {parent_handle} baseHandle);"
            )
            lines.append(
                f"        return NiHandleGuard.RequireConverted(converted, baseHandle, nameof(NativeMethods.{config.conversion_function}));"
            )
        lines.append("    }")

    lines.append("")
    lines.append(f"    internal static {config.cls} FromOwnedHandle({config.handle} handle)")
    lines.append("    {")
    lines.append(f"        NiHandleGuard.Require(handle, nameof({config.cls}));")
    lines.append("        try")
    lines.append("        {")
    lines.append(f"            return new {config.cls}(handle);")
    lines.append("        }")
    lines.append("        catch")
    lines.append("        {")
    lines.append("            handle.Dispose();")
    lines.append("            throw;")
    lines.append("        }")
    lines.append("    }")
    lines.append("")

    selected: list[tuple[Method, str, bool]] = []
    for method in methods:
        if method.access != "public":
            continue
        action = action_name(config, method)
        if action is None:
            continue
        static_factory = is_factory(config, method, action)
        instance = bool(method.params and method.params[0].type == config.handle and not method.params[0].modifier)
        if not static_factory and not instance:
            continue
        if action in SKIP_ACTIONS.get(config.cls, set()):
            continue
        selected.append((method, action, static_factory))

    emitted_signatures: set[tuple[str, tuple[str, ...], bool]] = set()
    for method, action, static_factory in selected:
        params = list(method.params)
        if not static_factory and params and params[0].type == config.handle:
            params = params[1:]
        out_handles = any(p.modifier == "out" and p.type in BY_HANDLE for p in params)
        public_action = "Try" + action if out_handles and not action.startswith("Try") else action
        sig = (public_action, tuple(public_parameter(method, p) for p in params), static_factory)
        if sig in emitted_signatures:
            raise RuntimeError(f"Duplicate wrapper signature in {config.cls}: {sig}")
        emitted_signatures.add(sig)
        lines.extend(emit_method(config, method, action, static_factory))

    lines.append("}")
    lines.append("")
    return lines


def lower_first(value: str) -> str:
    return value[0].lower() + value[1:]


methods = parse_methods()
lines = [
    "// <auto-generated>",
    "// Object-oriented wrappers generated from NativeMethods.cs.",
    "// Run generate_object_model.py after regenerating NativeMethods.cs.",
    "// </auto-generated>",
    "",
    "namespace NIFToolset.Managed;",
    "",
]
for config in CONFIGS:
    lines.extend(emit_class(config, methods))

OUTPUT.write_text("\n".join(lines), encoding="utf-8")
print(f"Wrote {OUTPUT} with {len(CONFIGS)} wrapper classes")
