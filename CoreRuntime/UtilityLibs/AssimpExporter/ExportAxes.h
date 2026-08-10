#pragma once

#include <cstdint>

// Axis presets are proper right-handed basis rotations. Right-handed FBX is
// exported by default. The optional left-handed mode reflects the target
// basis after all meshes, nodes, bones, and animations have been assembled.
//
// Right-handed target conventions:
// Native: +Y forward, +Z up
// Unreal: +X forward, +Z up (-Y is right)
// Unity:  -Z forward, +Y up
//
// Left-handed target conventions preserve the engine-facing forward/up axes:
// Native: +Y forward, +Z up
// Unreal: +X forward, +Z up (+Y is right)
// Unity:  +Z forward, +Y up
//
// Keeping the axis preset and handedness as enums prevents ambiguous state.
enum class ExportAxisPreset : std::uint8_t
{
    Native,
    Unreal,
    Unity
};

enum class ExportHandedness : std::uint8_t
{
    Right,
    Left
};

inline const char* GetExportHandednessDescription(ExportHandedness eHandedness)
{
    return eHandedness == ExportHandedness::Left
        ? "left-handed"
        : "right-handed";
}

inline const char* GetExportAxisDescription(ExportAxisPreset ePreset,
    ExportHandedness eHandedness = ExportHandedness::Right)
{
    switch (ePreset)
    {
    case ExportAxisPreset::Unreal:
        return eHandedness == ExportHandedness::Left
            ? "Unreal (+X forward, +Z up, +Y right)"
            : "Unreal-style RH (+X forward, +Z up, -Y right)";
    case ExportAxisPreset::Unity:
        return eHandedness == ExportHandedness::Left
            ? "Unity (+Z forward, +Y up)"
            : "Unity-style RH (-Z forward, +Y up)";
    case ExportAxisPreset::Native:
    default:
        return eHandedness == ExportHandedness::Left
            ? "native axes LH (+Y forward, +Z up)"
            : "native NIF RH (+Y forward, +Z up)";
    }
}
