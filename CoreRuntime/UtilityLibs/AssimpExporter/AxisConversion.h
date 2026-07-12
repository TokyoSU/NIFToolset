#pragma once

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>

// Coordinate conversion used by the exporter.
//
// Grand Fantasia/Gamebryo assets are Z-up with +Y as the character forward
// direction. Unreal assets are expected to be Z-up with +X forward. Keep Z
// unchanged and rotate the source basis -90 degrees around Z:
//
//     X_ue =  Y_nif
//     Y_ue = -X_nif
//     Z_ue =  Z_nif
//
// A basis conversion must be applied consistently to geometry, local node
// transforms, bind matrices and animation keys. Applying it only to vertices
// would leave the skeleton and animations in a different coordinate system.
namespace AxisConversion
{
    inline aiVector3D ToUnrealVector(const aiVector3D& kValue,
        bool bConvertToUnrealAxes)
    {
        if (!bConvertToUnrealAxes)
            return kValue;

        return aiVector3D(kValue.y, -kValue.x, kValue.z);
    }

    inline aiQuaternion ToUnrealQuaternion(const aiQuaternion& kValue,
        bool bConvertToUnrealAxes)
    {
        if (!bConvertToUnrealAxes)
            return kValue;

        // q' = C * q * inverse(C). For a pure basis rotation, the scalar
        // component is unchanged and the quaternion vector part is rotated by C.
        return aiQuaternion(kValue.w, kValue.y, -kValue.x, kValue.z);
    }

    inline aiMatrix4x4 ToUnrealMatrix(const aiMatrix4x4& kValue,
        bool bConvertToUnrealAxes)
    {
        if (!bConvertToUnrealAxes)
            return kValue;

        // C * M * C^-1, where C maps (x,y,z) to (y,-x,z).
        return aiMatrix4x4(
             kValue.b2, -kValue.b1,  kValue.b3,  kValue.b4,
            -kValue.a2,  kValue.a1, -kValue.a3, -kValue.a4,
             kValue.c2, -kValue.c1,  kValue.c3,  kValue.c4,
             kValue.d2, -kValue.d1,  kValue.d3,  kValue.d4);
    }
}
