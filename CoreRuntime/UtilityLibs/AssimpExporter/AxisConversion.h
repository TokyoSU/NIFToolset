#pragma once

#include "ExportAxes.h"

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>

// Coordinate conversion used by the exporter.
//
// Grand Fantasia/Gamebryo assets are Z-up with +Y as the character forward
// direction. Each preset below is a proper right-handed basis rotation
// (determinant +1). FbxWriter optionally reflects the completed Assimp scene
// for left-handed output and reverses triangle winding separately.
//
// A basis conversion must be applied consistently to geometry, local node
// transforms, bind matrices, and animation keys. Applying it only to vertices
// would leave the skeleton and animations in a different coordinate system.
namespace AxisConversion
{
    inline aiVector3D ToTargetVector(const aiVector3D& kValue,
        ExportAxisPreset ePreset)
    {
        switch (ePreset)
        {
        case ExportAxisPreset::Unreal:
            // Rotate -90 degrees around +Z:
            // X_pre =  Y_nif, Y_pre = -X_nif, Z_pre = Z_nif.
            return aiVector3D(kValue.y, -kValue.x, kValue.z);

        case ExportAxisPreset::Unity:
            // Rotate -90 degrees around +X:
            // X_target = X_nif, Y_target = Z_nif, Z_target = -Y_nif.
            // This is the right-handed Y-up/-Z-forward FBX convention.
            // Left-handed export reflects target Z, producing Unity's
            // X-right, Y-up, +Z-forward convention.
            return aiVector3D(kValue.x, kValue.z, -kValue.y);

        case ExportAxisPreset::Native:
        default:
            return kValue;
        }
    }

    inline aiQuaternion ToTargetQuaternion(const aiQuaternion& kValue,
        ExportAxisPreset ePreset)
    {
        // q' = C * q * inverse(C). For a pure basis rotation, the scalar
        // component is unchanged and the quaternion vector part is rotated by C.
        switch (ePreset)
        {
        case ExportAxisPreset::Unreal:
            return aiQuaternion(kValue.w, kValue.y, -kValue.x, kValue.z);

        case ExportAxisPreset::Unity:
            return aiQuaternion(kValue.w, kValue.x, kValue.z, -kValue.y);

        case ExportAxisPreset::Native:
        default:
            return kValue;
        }
    }

    inline aiMatrix4x4 ToTargetMatrix(const aiMatrix4x4& kValue,
        ExportAxisPreset ePreset)
    {
        switch (ePreset)
        {
        case ExportAxisPreset::Unreal:
            // C * M * C^-1, where C maps (x,y,z) to (y,-x,z).
            return aiMatrix4x4(
                 kValue.b2, -kValue.b1,  kValue.b3,  kValue.b4,
                -kValue.a2,  kValue.a1, -kValue.a3, -kValue.a4,
                 kValue.c2, -kValue.c1,  kValue.c3,  kValue.c4,
                 kValue.d2, -kValue.d1,  kValue.d3,  kValue.d4);

        case ExportAxisPreset::Unity:
            // C * M * C^-1, where C maps (x,y,z) to (x,z,-y).
            return aiMatrix4x4(
                 kValue.a1,  kValue.a3, -kValue.a2,  kValue.a4,
                 kValue.c1,  kValue.c3, -kValue.c2,  kValue.c4,
                -kValue.b1, -kValue.b3,  kValue.b2, -kValue.b4,
                 kValue.d1,  kValue.d3, -kValue.d2,  kValue.d4);

        case ExportAxisPreset::Native:
        default:
            return kValue;
        }
    }
}
