#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = ROOT.parent.parent
NATIVE = ROOT / "NativeMethods.cs"

METHOD_RE = re.compile(r"(?:public|internal) static extern\s+\S+\s+(NIF_\w+)\(([^;]*)\);")
CLASS_RE = re.compile(r"public unsafe partial class\s+(Ni\w+)\s*:")


def split_top_level(value: str) -> list[str]:
    if not value.strip():
        return []
    result = []
    start = 0
    depth = 0
    for index, char in enumerate(value):
        if char in "([{":
            depth += 1
        elif char in ")]}" and depth:
            depth -= 1
        elif char == "," and depth == 0:
            result.append(value[start:index].strip())
            start = index + 1
    result.append(value[start:].strip())
    return result


native_text = NATIVE.read_text(encoding="utf-8")
native_counts = {name: len(split_top_level(parameters)) for name, parameters in METHOD_RE.findall(native_text)}

errors: list[str] = []
wrapper_classes: set[str] = set()
for path in ROOT.glob("ObjectModel*.cs"):
    text = path.read_text(encoding="utf-8")
    wrapper_classes.update(CLASS_RE.findall(text))

    for match in re.finditer(r"NativeMethods\.(NIF_\w+)\(", text):
        name = match.group(1)
        if name not in native_counts:
            errors.append(f"{path.name}: unknown native call {name}")
            continue
        cursor = match.end()
        depth = 1
        while cursor < len(text) and depth:
            if text[cursor] == "(":
                depth += 1
            elif text[cursor] == ")":
                depth -= 1
            cursor += 1
        if depth:
            errors.append(f"{path.name}: unterminated call to {name}")
            continue
        arguments = text[match.end():cursor - 1]
        actual = len(split_top_level(arguments))
        expected = native_counts[name]
        if actual != expected:
            line = text.count("\n", 0, match.start()) + 1
            errors.append(
                f"{path.name}:{line}: {name} expects {expected} arguments, wrapper passes {actual}")

expected_classes = {
    "NiStream", "NiObject", "NiAVObject", "NiNode", "NiBSPNode", "NiBillboardNode", "NiSwitchNode", "NiLODNode", "NiSortAdjustNode", "NiTerrain", "NiTerrainCell", "NiTerrainCellNode", "NiTerrainCellLeaf", "NiTerrainSector", "NiAtmosphere", "NiEnvironment", "NiSky", "NiSkyDome", "NiDecorationField", "NiDecorationLayer", "NiDecorationPlane", "NiCamera", "NiMesh",
    "NiPSParticleSystem", "NiPortal", "NiRoom", "NiOldWall", "NiRoomGroup", "NiDataStream",
    "NiDataStreamRef", "NiControllerSequence", "NiSequenceData", "NiTextKeyExtraData",
    "NiKFMTool", "NiActorManager", "NiPSEmitter", "NiCollisionData", "NiCollisionGroup",
    "NiRenderer", "NiRenderTargetGroup", "NiRenderBuffer", "NiDepthStencilBuffer",
    "NiCullingProcess", "NiMeshCullingProcess", "NiAlphaAccumulator",
    "NiRenderListProcessor", "NiAlphaSortProcessor", "NiRenderView", "NiRenderView3D",
    "NiRenderClick", "NiViewRenderClick", "NiRenderStep", "NiDefaultClickRenderStep",
    "NiSPTask", "NiSPWorkflow", "NiSPStream",
}
missing = expected_classes - wrapper_classes
if missing:
    errors.append("Missing wrapper classes: " + ", ".join(sorted(missing)))

# Discover every NiTypeMask entry whose actual C++ class derives from NiNode.
# This intentionally excludes material graph classes such as NiMaterialNode,
# because they derive from NiRefObject rather than the scene-graph NiNode.
parents: dict[str, str] = {}
class_pattern = re.compile(
    r"class\s+(?:[A-Za-z0-9_]+_ENTRY\s+)?([A-Za-z0-9_]+)\s*:\s*public\s+([A-Za-z0-9_]+)\s*\{"
)
for header in [*PROJECT_ROOT.rglob("*.h"), *PROJECT_ROOT.rglob("*.cpp")]:
    text = header.read_text(encoding="utf-8", errors="replace")
    for child, parent in class_pattern.findall(text):
        parents[child] = parent

rtti_path = PROJECT_ROOT / "CoreRuntime/EngineLibs/NiMain/NiRTTI.h"
rtti_text = rtti_path.read_text(encoding="utf-8", errors="replace")
rtti_match = re.search(r"enum class NiTypeMask[^\{]*\{(.*?)\};", rtti_text, re.DOTALL)
if not rtti_match:
    errors.append("Could not parse NiTypeMask from NiRTTI.h")
else:
    rtti_types = {
        line.strip().rstrip(",")
        for line in rtti_match.group(1).splitlines()
        if line.strip() and not line.strip().startswith("//")
    }

    def derives_from_node(type_name: str) -> bool:
        if type_name == "NiNode":
            return True
        visited: set[str] = set()
        current = type_name
        while current in parents and current not in visited:
            visited.add(current)
            current = parents[current]
            if current == "NiNode":
                return True
        return False

    rtti_node_types = {name for name in rtti_types if derives_from_node(name)}
    managed_node_types = {
        "NiNode", "NiBSPNode", "NiBillboardNode", "NiSwitchNode", "NiLODNode",
        "NiSortAdjustNode", "NiRoom", "NiOldWall", "NiRoomGroup", "NiTerrain", "NiTerrainCell",
        "NiTerrainCellNode", "NiTerrainCellLeaf", "NiTerrainSector", "NiAtmosphere",
        "NiEnvironment", "NiSky", "NiSkyDome", "NiDecorationField",
        "NiDecorationLayer", "NiDecorationPlane",
    }
    missing_node_wrappers = rtti_node_types - managed_node_types
    stale_node_wrappers = managed_node_types - rtti_node_types
    if missing_node_wrappers:
        errors.append(
            "NiRTTI node types missing managed wrappers: "
            + ", ".join(sorted(missing_node_wrappers)))
    if stale_node_wrappers:
        errors.append(
            "Managed node wrappers are not NiNode RTTI types: "
            + ", ".join(sorted(stale_node_wrappers)))

if errors:
    print("Object model verification failed:")
    for error in errors:
        print(" -", error)
    raise SystemExit(1)

print(f"Object model verification passed: {len(wrapper_classes)} classes, {len(native_counts)} native exports checked")
