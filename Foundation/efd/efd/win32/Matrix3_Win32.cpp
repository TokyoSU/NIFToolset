// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#include "efdPCH.h"

#include <efd/RTLib.h>
#include <efd/Matrix3.h>
#include <efd/EEMath.h>
#include <efd/SerializeRoutines.h>


using namespace efd;

const Matrix3 Matrix3::ZERO(
    Point3(0.0f, 0.0f, 0.0f),
    Point3(0.0f, 0.0f, 0.0f),
    Point3(0.0f, 0.0f, 0.0f));

const Matrix3 Matrix3::IDENTITY(
    Point3(1.0f, 0.0f, 0.0f),
    Point3(0.0f, 1.0f, 0.0f),
    Point3(0.0f, 0.0f, 1.0f));

const Float32 Matrix3::EE_RIGHT_ANGLE_EPSILON = 1e-3f;

//------------------------------------------------------------------------------------------------
Matrix3::Matrix3(
    const Point3& col0,
    const Point3& col1,
    const Point3& col2)
{
    SetCol(0, col0);
    SetCol(1, col1);
    SetCol(2, col2);
}

//------------------------------------------------------------------------------------------------
void Matrix3::MakeIdentity()
{
    m_pEntry[0][0] = 1.0f;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = 1.0f;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = 1.0f;
}

//------------------------------------------------------------------------------------------------
void Matrix3::MakeDiagonal(Float32 diag0, Float32 diag1, Float32 diag2)
{
    m_pEntry[0][0] = diag0;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = diag1;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = diag2;
}

//------------------------------------------------------------------------------------------------
void Matrix3::MakeXRotation(Float32 angle)
{
    Float32 sn, cs;
    efd::SinCos(angle, sn, cs);

    m_pEntry[0][0] = 1.0f;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = cs;
    m_pEntry[1][2] = sn;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = -sn;
    m_pEntry[2][2] = cs;
}

//------------------------------------------------------------------------------------------------
void Matrix3::MakeYRotation(Float32 angle)
{
    Float32 sn, cs;
    efd::SinCos(angle, sn, cs);

    m_pEntry[0][0] = cs;
    m_pEntry[0][1] = 0.0f;
    m_pEntry[0][2] = -sn;
    m_pEntry[1][0] = 0.0f;
    m_pEntry[1][1] = 1.0f;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = sn;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = cs;
}

//------------------------------------------------------------------------------------------------
void Matrix3::MakeZRotation (Float32 angle)
{
    Float32 sn, cs;
    efd::SinCos(angle, sn, cs);

    m_pEntry[0][0] = cs;
    m_pEntry[0][1] = sn;
    m_pEntry[0][2] = 0.0f;
    m_pEntry[1][0] = -sn;
    m_pEntry[1][1] = cs;
    m_pEntry[1][2] = 0.0f;
    m_pEntry[2][0] = 0.0f;
    m_pEntry[2][1] = 0.0f;
    m_pEntry[2][2] = 1.0f;
}

//------------------------------------------------------------------------------------------------
void Matrix3::MakeRotation (Float32 angle, Float32 x, Float32 y, Float32 z)
{
    Float32 sn, cs;
    efd::SinCos(angle, sn, cs);

    Float32 omcs = 1.0f-cs;
    Float32 x2 = x*x;
    Float32 y2 = y*y;
    Float32 z2 = z*z;
    Float32 xym = x*y*omcs;
    Float32 xzm = x*z*omcs;
    Float32 yzm = y*z*omcs;
    Float32 xsin = x*sn;
    Float32 ysin = y*sn;
    Float32 zsin = z*sn;

    m_pEntry[0][0] = x2*omcs+cs;
    m_pEntry[0][1] = xym+zsin;
    m_pEntry[0][2] = xzm-ysin;
    m_pEntry[1][0] = xym-zsin;
    m_pEntry[1][1] = y2*omcs+cs;
    m_pEntry[1][2] = yzm+xsin;
    m_pEntry[2][0] = xzm+ysin;
    m_pEntry[2][1] = yzm-xsin;
    m_pEntry[2][2] = z2*omcs+cs;
}

//------------------------------------------------------------------------------------------------
bool Matrix3::operator==(const Matrix3& mat) const
{
    return
        (m_pEntry[0][0] == mat.m_pEntry[0][0]) &&
        (m_pEntry[0][1] == mat.m_pEntry[0][1]) &&
        (m_pEntry[0][2] == mat.m_pEntry[0][2]) &&
        (m_pEntry[1][0] == mat.m_pEntry[1][0]) &&
        (m_pEntry[1][1] == mat.m_pEntry[1][1]) &&
        (m_pEntry[1][2] == mat.m_pEntry[1][2]) &&
        (m_pEntry[2][0] == mat.m_pEntry[2][0]) &&
        (m_pEntry[2][1] == mat.m_pEntry[2][1]) &&
        (m_pEntry[2][2] == mat.m_pEntry[2][2]);
}

//------------------------------------------------------------------------------------------------
Matrix3 Matrix3::operator+(const Matrix3& mat) const
{
    Matrix3 result = *this;
    result.m_pEntry[0][0] += mat.m_pEntry[0][0];
    result.m_pEntry[0][1] += mat.m_pEntry[0][1];
    result.m_pEntry[0][2] += mat.m_pEntry[0][2];
    result.m_pEntry[1][0] += mat.m_pEntry[1][0];
    result.m_pEntry[1][1] += mat.m_pEntry[1][1];
    result.m_pEntry[1][2] += mat.m_pEntry[1][2];
    result.m_pEntry[2][0] += mat.m_pEntry[2][0];
    result.m_pEntry[2][1] += mat.m_pEntry[2][1];
    result.m_pEntry[2][2] += mat.m_pEntry[2][2];
    return result;
}

//------------------------------------------------------------------------------------------------
Matrix3 Matrix3::operator-(const Matrix3& mat) const
{
    Matrix3 result = *this;
    result.m_pEntry[0][0] -= mat.m_pEntry[0][0];
    result.m_pEntry[0][1] -= mat.m_pEntry[0][1];
    result.m_pEntry[0][2] -= mat.m_pEntry[0][2];
    result.m_pEntry[1][0] -= mat.m_pEntry[1][0];
    result.m_pEntry[1][1] -= mat.m_pEntry[1][1];
    result.m_pEntry[1][2] -= mat.m_pEntry[1][2];
    result.m_pEntry[2][0] -= mat.m_pEntry[2][0];
    result.m_pEntry[2][1] -= mat.m_pEntry[2][1];
    result.m_pEntry[2][2] -= mat.m_pEntry[2][2];
    return result;
}

//------------------------------------------------------------------------------------------------
Point3 efd::operator*(const Point3& pt, const Matrix3& mat)
{
    return Point3
    (
        pt.x*mat.m_pEntry[0][0]+pt.y*mat.m_pEntry[1][0]+
            pt.z*mat.m_pEntry[2][0],
        pt.x*mat.m_pEntry[0][1]+pt.y*mat.m_pEntry[1][1]+
            pt.z*mat.m_pEntry[2][1],
        pt.x*mat.m_pEntry[0][2]+pt.y*mat.m_pEntry[1][2]+
            pt.z*mat.m_pEntry[2][2]
 );
}

//------------------------------------------------------------------------------------------------
bool Matrix3::Inverse(Matrix3& inv) const
{
    // Invert a 3x3 using cofactors.  This is about 8 times faster than
    // the Numerical Recipes code which uses Gaussian elimination.

    inv.m_pEntry[0][0] = m_pEntry[1][1]*m_pEntry[2][2]-
        m_pEntry[1][2]*m_pEntry[2][1];
    inv.m_pEntry[0][1] = m_pEntry[0][2]*m_pEntry[2][1]-
        m_pEntry[0][1]*m_pEntry[2][2];
    inv.m_pEntry[0][2] = m_pEntry[0][1]*m_pEntry[1][2]-
        m_pEntry[0][2]*m_pEntry[1][1];
    inv.m_pEntry[1][0] = m_pEntry[1][2]*m_pEntry[2][0]-
        m_pEntry[1][0]*m_pEntry[2][2];
    inv.m_pEntry[1][1] = m_pEntry[0][0]*m_pEntry[2][2]-
        m_pEntry[0][2]*m_pEntry[2][0];
    inv.m_pEntry[1][2] = m_pEntry[0][2]*m_pEntry[1][0]-
        m_pEntry[0][0]*m_pEntry[1][2];
    inv.m_pEntry[2][0] = m_pEntry[1][0]*m_pEntry[2][1]-
        m_pEntry[1][1]*m_pEntry[2][0];
    inv.m_pEntry[2][1] = m_pEntry[0][1]*m_pEntry[2][0]-
        m_pEntry[0][0]*m_pEntry[2][1];
    inv.m_pEntry[2][2] = m_pEntry[0][0]*m_pEntry[1][1]-
        m_pEntry[0][1]*m_pEntry[1][0];

    Float32 det = m_pEntry[0][0]*inv.m_pEntry[0][0]+
        m_pEntry[0][1]*inv.m_pEntry[1][0]+
        m_pEntry[0][2]*inv.m_pEntry[2][0];
    if (efd::Abs(det) <= 1e-06f)
        return false;

    Float32 invDet = 1.0f/det;
    for (SInt32 row = 0; row < 3; row++)
    {
        for (SInt32 col = 0; col < 3; col++)
        {
            inv.m_pEntry[row][col] *= invDet;
        }
    }

    return true;
}

//------------------------------------------------------------------------------------------------
Matrix3 Matrix3::Inverse() const
{
    Matrix3 inv;

    if (Inverse(inv) == false)
        inv.MakeZero();

    return inv;
}

//------------------------------------------------------------------------------------------------
Matrix3 Matrix3::Transpose() const
{
    Point3 row[3];

    GetRow(0, row[0]);
    GetRow(1, row[1]);
    GetRow(2, row[2]);

    return Matrix3(row[0], row[1], row[2]);
}

//------------------------------------------------------------------------------------------------
void Matrix3::ExtractAngleAndAxis(
    Float32& angle,
    Float32& x,
    Float32& y,
    Float32& z) const
{
    // Let (x,y,z) be the unit-length axis and let A be an angle of rotation.
    // The rotation matrix is R = I + sin(A)*P + (1-cos(A))*P^2 where
    // I is the identity and
    //
    //       +-        -+
    //   P = |  0 +z -y |
    //       | -z  0 +x |
    //       | +y -x  0 |
    //       +-        -+
    //
    // Some algebra will show that
    //
    //   cos(A) = (trace(R)-1)/2  and  R - R^t = 2*sin(A)*P

    Float32 trace = m_pEntry[0][0]+m_pEntry[1][1]+m_pEntry[2][2];
    angle = efd::ACos(0.5f*(trace-1.0f));

    x = m_pEntry[1][2]-m_pEntry[2][1];
    y = m_pEntry[2][0]-m_pEntry[0][2];
    z = m_pEntry[0][1]-m_pEntry[1][0];
    Float32 length = efd::Sqrt(x*x + y*y + z*z);
    const Float32 EPSILON = 1e-06f;
    if (length > EPSILON)
    {
        Float32 invLength = 1.0f/length;
        x *= invLength;
        y *= invLength;
        z *= invLength;
    }
    else  // angle is 0 or pi
    {
        if (angle > 1.0f)  // any number strictly between 0 and pi works
        {
            // angle must be pi
            x = efd::Sqrt(0.5f * (1.0f+m_pEntry[0][0]));
            y = efd::Sqrt(0.5f * (1.0f+m_pEntry[1][1]));
            z = efd::Sqrt(0.5f * (1.0f+m_pEntry[2][2]));

            // determine signs of axis components
            Float32 tx, ty, tz;
            tx = m_pEntry[0][0]*x + m_pEntry[0][1]*y + m_pEntry[0][2]*z - x;
            ty = m_pEntry[1][0]*x + m_pEntry[1][1]*y + m_pEntry[1][2]*z - y;
            tz = m_pEntry[2][0]*x + m_pEntry[2][1]*y + m_pEntry[2][2]*z - z;
            length = tx*tx + ty*ty + tz*tz;
            if (length < EPSILON)
                return;

            z = -z;
            tx = m_pEntry[0][0]*x + m_pEntry[0][1]*y + m_pEntry[0][2]*z - x;
            ty = m_pEntry[1][0]*x + m_pEntry[1][1]*y + m_pEntry[1][2]*z - y;
            tz = m_pEntry[2][0]*x + m_pEntry[2][1]*y + m_pEntry[2][2]*z - z;
            length = tx*tx + ty*ty + tz*tz;
            if (length < EPSILON)
                return;

            y = -y;
            tx = m_pEntry[0][0]*x + m_pEntry[0][1]*y + m_pEntry[0][2]*z - x;
            ty = m_pEntry[1][0]*x + m_pEntry[1][1]*y + m_pEntry[1][2]*z - y;
            tz = m_pEntry[2][0]*x + m_pEntry[2][1]*y + m_pEntry[2][2]*z - z;
            length = tx*tx + ty*ty + tz*tz;
            if (length < EPSILON)
                return;
        }
        else
        {
            // Angle is zero, matrix is the identity, no unique axis, so
            // return (1,0,0) for as good a guess as any.
            x = 1.0f;
            y = 0.0f;
            z = 0.0f;
        }
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::Reorthogonalize ()
{
    // Factor M = QR where Q is orthogonal and R is upper triangular.
    // Algorithm uses Gram-Schmidt orthogonalization (the QR algorithm).
    //
    // If M = [ m0 | m1 | m2 ] and Q = [ q0 | q1 | q2 ], then
    //
    //   q0 = m0/|m0|
    //   q1 = (m1-(q0*m1)q0)/|m1-(q0*m1)q0|
    //   q2 = (m2-(q0*m2)q0-(q1*m2)q1)/|m2-(q0*m2)q0-(q1*m2)q1|
    //
    // where |V| indicates length of vector V and A*B indicates dot
    // product of vectors A and B.  The matrix R has entries
    //
    //   r00 = q0*m0  r01 = q0*m1  r02 = q0*m2
    //   r10 = 0      r11 = q1*m1  r12 = q1*m2
    //   r20 = 0      r21 = 0      r22 = q2*m2
    //
    // The reorthogonalization replaces current matrix by computed Q.

    const Float32 EPSILON = 1e-05f;

    // unitize column 0
    Float32 length = efd::Sqrt(m_pEntry[0][0]*m_pEntry[0][0] +
        m_pEntry[1][0]*m_pEntry[1][0] + m_pEntry[2][0]*m_pEntry[2][0]);

    if (length < EPSILON)
        return false;

    Float32 invLength = 1.0f/length;
    m_pEntry[0][0] *= invLength;
    m_pEntry[1][0] *= invLength;
    m_pEntry[2][0] *= invLength;

    // project out column 0 from column 1
    Float32 dot = m_pEntry[0][0]*m_pEntry[0][1] + m_pEntry[1][0]*m_pEntry[1][1]
        + m_pEntry[2][0]*m_pEntry[2][1];
    m_pEntry[0][1] -= dot*m_pEntry[0][0];
    m_pEntry[1][1] -= dot*m_pEntry[1][0];
    m_pEntry[2][1] -= dot*m_pEntry[2][0];

    // unitize column 1
    length = efd::Sqrt(m_pEntry[0][1]*m_pEntry[0][1] +
        m_pEntry[1][1]*m_pEntry[1][1] + m_pEntry[2][1]*m_pEntry[2][1]);
    if (length < EPSILON)
        return false;

    invLength = 1.0f/length;
    m_pEntry[0][1] *= invLength;
    m_pEntry[1][1] *= invLength;
    m_pEntry[2][1] *= invLength;

    // project out column 0 from column 2
    dot = m_pEntry[0][0]*m_pEntry[0][2] + m_pEntry[1][0]*m_pEntry[1][2] +
        m_pEntry[2][0]*m_pEntry[2][2];
    m_pEntry[0][2] -= dot*m_pEntry[0][0];
    m_pEntry[1][2] -= dot*m_pEntry[1][0];
    m_pEntry[2][2] -= dot*m_pEntry[2][0];

    // project out column 1 from column 2
    dot = m_pEntry[0][1]*m_pEntry[0][2] + m_pEntry[1][1]*m_pEntry[1][2] +
        m_pEntry[2][1]*m_pEntry[2][2];
    m_pEntry[0][2] -= dot*m_pEntry[0][1];
    m_pEntry[1][2] -= dot*m_pEntry[1][1];
    m_pEntry[2][2] -= dot*m_pEntry[2][1];

    // unitize column 2
    length = efd::Sqrt(m_pEntry[0][2]*m_pEntry[0][2] +
        m_pEntry[1][2]*m_pEntry[1][2] + m_pEntry[2][2]*m_pEntry[2][2]);
    if (length < EPSILON)
        return false;

    invLength = 1.0f/length;
    m_pEntry[0][2] *= invLength;
    m_pEntry[1][2] *= invLength;
    m_pEntry[2][2] *= invLength;

    return true;
}

//------------------------------------------------------------------------------------------------
Matrix3 Matrix3::TransposeTimes(const Matrix3& mat) const
{
    Matrix3 prd;

    prd.m_pEntry[0][0] =
        m_pEntry[0][0]*mat.m_pEntry[0][0]+
        m_pEntry[1][0]*mat.m_pEntry[1][0]+
        m_pEntry[2][0]*mat.m_pEntry[2][0];
    prd.m_pEntry[1][0] =
        m_pEntry[0][1]*mat.m_pEntry[0][0]+
        m_pEntry[1][1]*mat.m_pEntry[1][0]+
        m_pEntry[2][1]*mat.m_pEntry[2][0];
    prd.m_pEntry[2][0] =
        m_pEntry[0][2]*mat.m_pEntry[0][0]+
        m_pEntry[1][2]*mat.m_pEntry[1][0]+
        m_pEntry[2][2]*mat.m_pEntry[2][0];
    prd.m_pEntry[0][1] =
        m_pEntry[0][0]*mat.m_pEntry[0][1]+
        m_pEntry[1][0]*mat.m_pEntry[1][1]+
        m_pEntry[2][0]*mat.m_pEntry[2][1];
    prd.m_pEntry[1][1] =
        m_pEntry[0][1]*mat.m_pEntry[0][1]+
        m_pEntry[1][1]*mat.m_pEntry[1][1]+
        m_pEntry[2][1]*mat.m_pEntry[2][1];
    prd.m_pEntry[2][1] =
        m_pEntry[0][2]*mat.m_pEntry[0][1]+
        m_pEntry[1][2]*mat.m_pEntry[1][1]+
        m_pEntry[2][2]*mat.m_pEntry[2][1];
    prd.m_pEntry[0][2] =
        m_pEntry[0][0]*mat.m_pEntry[0][2]+
        m_pEntry[1][0]*mat.m_pEntry[1][2]+
        m_pEntry[2][0]*mat.m_pEntry[2][2];
    prd.m_pEntry[1][2] =
        m_pEntry[0][1]*mat.m_pEntry[0][2]+
        m_pEntry[1][1]*mat.m_pEntry[1][2]+
        m_pEntry[2][1]*mat.m_pEntry[2][2];
    prd.m_pEntry[2][2] =
        m_pEntry[0][2]*mat.m_pEntry[0][2]+
        m_pEntry[1][2]*mat.m_pEntry[1][2]+
        m_pEntry[2][2]*mat.m_pEntry[2][2];

    return prd;
}

//------------------------------------------------------------------------------------------------
Matrix3 Matrix3::Congruence(const Matrix3& rot) const
{
    Matrix3 prod;

    SInt32 row, col, mid;
    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 3; col++)
        {
            prod.m_pEntry[row][col] = 0.0f;
            for (mid = 0; mid < 3; mid++)
            {
                prod.m_pEntry[row][col] +=
                    rot.m_pEntry[row][mid]*m_pEntry[mid][col];
            }
        }
    }

    Matrix3 cong;
    for (row = 0; row < 3; row++)
    {
        for (col = 0; col < 3; col++)
        {
            cong.m_pEntry[row][col] = 0.0f;
            for (mid = 0; mid < 3; mid++)
            {
                cong.m_pEntry[row][col] +=
                    prod.m_pEntry[row][mid]*rot.m_pEntry[col][mid];
            }
        }
    }

    return cong;
}

//------------------------------------------------------------------------------------------------
void Matrix3::TransformVertices(
    const Matrix3& rot,
    const Point3& trn,
    UInt32 vertexCount,
    const Point3* pInVertex,
    Point3* pOutVertex)
{
    // out = rot*in + trn
    for (UInt32 i = 0; i < vertexCount; i++)
    {
        pOutVertex[i].x = trn.x +
            rot.m_pEntry[0][0]*pInVertex[i].x +
            rot.m_pEntry[0][1]*pInVertex[i].y +
            rot.m_pEntry[0][2]*pInVertex[i].z;
        pOutVertex[i].y = trn.y +
            rot.m_pEntry[1][0]*pInVertex[i].x +
            rot.m_pEntry[1][1]*pInVertex[i].y +
            rot.m_pEntry[1][2]*pInVertex[i].z;
        pOutVertex[i].z = trn.z +
            rot.m_pEntry[2][0]*pInVertex[i].x +
            rot.m_pEntry[2][1]*pInVertex[i].y +
            rot.m_pEntry[2][2]*pInVertex[i].z;
    }
}

//------------------------------------------------------------------------------------------------
void Matrix3::TransformNormals(
    const Matrix3& rot,
    UInt32 normalCount,
    const Point3* pInNormal,
    Point3* pOutNormal)
{
    // out = transpose(rot)*in
    for (UInt32 i = 0; i < normalCount; i++)
    {
        pOutNormal[i].x =
            rot.m_pEntry[0][0]*pInNormal[i].x +
            rot.m_pEntry[1][0]*pInNormal[i].y +
            rot.m_pEntry[2][0]*pInNormal[i].z;
        pOutNormal[i].y =
            rot.m_pEntry[0][1]*pInNormal[i].x +
            rot.m_pEntry[1][1]*pInNormal[i].y +
            rot.m_pEntry[2][1]*pInNormal[i].z;
        pOutNormal[i].z =
            rot.m_pEntry[0][2]*pInNormal[i].x +
            rot.m_pEntry[1][2]*pInNormal[i].y +
            rot.m_pEntry[2][2]*pInNormal[i].z;
    }
}

//------------------------------------------------------------------------------------------------
void Matrix3::TransformVerticesAndNormals (
    const Matrix3& rot,
    const Point3& trn,
    UInt32 count,
    const Point3* pInVertex,
    Point3* pOutVertex,
    const Point3* pInNormal,
    Point3* pOutNormal)
{
    // vOut = rot*vIn + trn
    // nOut = transpose(rot)*nIn
    for (UInt32 i = 0; i < count; i++)
    {
        pOutVertex[i].x = trn.x +
            rot.m_pEntry[0][0]*pInVertex[i].x +
            rot.m_pEntry[0][1]*pInVertex[i].y +
            rot.m_pEntry[0][2]*pInVertex[i].z;
        pOutVertex[i].y = trn.y +
            rot.m_pEntry[1][0]*pInVertex[i].x +
            rot.m_pEntry[1][1]*pInVertex[i].y +
            rot.m_pEntry[1][2]*pInVertex[i].z;
        pOutVertex[i].z = trn.z +
            rot.m_pEntry[2][0]*pInVertex[i].x +
            rot.m_pEntry[2][1]*pInVertex[i].y +
            rot.m_pEntry[2][2]*pInVertex[i].z;

        pOutNormal[i].x =
            rot.m_pEntry[0][0]*pInNormal[i].x +
            rot.m_pEntry[1][0]*pInNormal[i].y +
            rot.m_pEntry[2][0]*pInNormal[i].z;
        pOutNormal[i].y =
            rot.m_pEntry[0][1]*pInNormal[i].x +
            rot.m_pEntry[1][1]*pInNormal[i].y +
            rot.m_pEntry[2][1]*pInNormal[i].z;
        pOutNormal[i].z =
            rot.m_pEntry[0][2]*pInNormal[i].x +
            rot.m_pEntry[1][2]*pInNormal[i].y +
            rot.m_pEntry[2][2]*pInNormal[i].z;
    }
}

//------------------------------------------------------------------------------------------------
void Matrix3::EigenSolveSymmetric(
    Float32 eigenValue[3],
    Point3 eigenVector[3])
{
    Matrix3 mat = *this;
    Float32 subd[2];
    bool reflection = mat.Tridiagonal(eigenValue, subd);
    bool converged = mat.QLAlgorithm(eigenValue, subd);
    EE_UNUSED_ARG(converged);
    EE_ASSERT(converged);

    // The columns of the matrix are the eigenvectors.  The tridiagonal
    // algorithm produces a reflection matrix, and the QL algorithm
    // postmultiplies by rotations.  The end result is a reflection matrix.
    // The last column has its signs changed to produce a rotation matrix,
    // therefore allowing an application to use the columns for a right-hand
    // coordinate system.
    eigenVector[0].x = mat.m_pEntry[0][0];
    eigenVector[0].y = mat.m_pEntry[1][0];
    eigenVector[0].z = mat.m_pEntry[2][0];
    eigenVector[1].x = mat.m_pEntry[0][1];
    eigenVector[1].y = mat.m_pEntry[1][1];
    eigenVector[1].z = mat.m_pEntry[2][1];
    if (reflection)
    {
        eigenVector[2].x = -mat.m_pEntry[0][2];
        eigenVector[2].y = -mat.m_pEntry[1][2];
        eigenVector[2].z = -mat.m_pEntry[2][2];
    }
    else
    {
        eigenVector[2].x = mat.m_pEntry[0][2];
        eigenVector[2].y = mat.m_pEntry[1][2];
        eigenVector[2].z = mat.m_pEntry[2][2];
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::Tridiagonal (Float32 diag[3], Float32 subd[2])
{
    // Householder reduction T = Q^t M Q
    //   Input:
    //     mat, symmetric 3x3 matrix M
    //   Output:
    //     mat, orthogonal matrix Q
    //     diag, diagonal entries of T
    //     subd, subdiagonal entries of T (T is symmetric)

    Float32 m00 = m_pEntry[0][0];
    Float32 m01 = m_pEntry[0][1];
    Float32 m02 = m_pEntry[0][2];
    Float32 m11 = m_pEntry[1][1];
    Float32 m12 = m_pEntry[1][2];
    Float32 m22 = m_pEntry[2][2];

    diag[0] = m00;

    const Float32 EPSILON = 1e-08f;
    if (efd::Abs(m02) >= EPSILON)
    {
        subd[0] = efd::Sqrt(m01 * m01 + m02 * m02);
        Float32 invLength = 1.0f / subd[0];
        m01 *= invLength;
        m02 *= invLength;
        Float32 tmp = 2.0f * m01 * m12 + m02 * (m22 - m11);
        diag[1] = m11 + m02 * tmp;
        diag[2] = m22 - m02 * tmp;
        subd[1] = m12 - m01 * tmp;
        m_pEntry[0][0] = 1.0f;
        m_pEntry[0][1] = 0.0f;
        m_pEntry[0][2] = 0.0f;
        m_pEntry[1][0] = 0.0f;
        m_pEntry[1][1] = m01;
        m_pEntry[1][2] = m02;
        m_pEntry[2][0] = 0.0f;
        m_pEntry[2][1] = m02;
        m_pEntry[2][2] = -m01;
        return true;
    }
    else
    {
        diag[1] = m11;
        diag[2] = m22;
        subd[0] = m01;
        subd[1] = m12;
        m_pEntry[0][0] = 1.0f;
        m_pEntry[0][1] = 0.0f;
        m_pEntry[0][2] = 0.0f;
        m_pEntry[1][0] = 0.0f;
        m_pEntry[1][1] = 1.0f;
        m_pEntry[1][2] = 0.0f;
        m_pEntry[2][0] = 0.0f;
        m_pEntry[2][1] = 0.0f;
        m_pEntry[2][2] = 1.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::QLAlgorithm(Float32 diag[3], Float32 subd[2])
{
    const int MAX_ITERATIONS = 32;
    for (SInt32 i = 0; i < MAX_ITERATIONS; i++)
    {
        Float32 sum, diff, discr, eValue0, eValue1, cosine, sine, tmp;
        SInt32 row;

        sum = efd::Abs(diag[0]) + efd::Abs(diag[1]);
        if (efd::Abs(subd[0]) + sum == sum)
        {
            // The matrix is effectively
            //       +-        -+
            //   M = | d0  0  0 |
            //       | 0  d1 s1 |
            //       | 0  s1 d2 |
            //       +-        -+

            subd[0] = 0.0f;

            // Test if the matrix is already diagonal.
            sum = efd::Abs(diag[1]) + efd::Abs(diag[2]);
            if (efd::Abs(subd[1]) + sum == sum)
            {
                // The current orthogonal matrix and the diagonal array do
                // not need to be updated.
                subd[1] = 0.0f;
                return true;
            }

            // Compute the eigenvalues as roots of a quadratic equation.
            sum = diag[1] + diag[2];
            diff = diag[1] - diag[2];
            discr = efd::Sqrt(diff * diff + 4.0f * subd[1] * subd[1]);
            eValue0 = 0.5f * (sum - discr);
            eValue1 = 0.5f * (sum + discr);

            // Compute the Givens rotation.
            if (diff >= 0.0f)
            {
                cosine = subd[1];
                sine = diag[1] - eValue0;
            }
            else
            {
                cosine = diag[2] - eValue0;
                sine = subd[1];
            }
            tmp = 1.0f / efd::Sqrt(cosine * cosine + sine * sine);
            cosine *= tmp;
            sine *= tmp;

            // Postmultiply the current orthogonal matrix with the Givens
            // rotation.
            for (row = 0; row < 3; row++)
            {
                tmp = m_pEntry[row][2];
                m_pEntry[row][2] = sine * m_pEntry[row][1] + cosine * tmp;
                m_pEntry[row][1] = cosine * m_pEntry[row][1] - sine * tmp;
            }

            // Update the tridiagonal matrix.
            diag[1] = eValue0;
            diag[2] = eValue1;
            subd[1] = 0.0f;
            return true;
        }

        sum = efd::Abs(diag[1]) + efd::Abs(diag[2]);
        if (efd::Abs(subd[1]) + sum == sum)
        {
            // The matrix is effectively
            //       +-         -+
            //   M = | d0  s0  0 |
            //       | s0  d1  0 |
            //       | 0   0  d2 |
            //       +-         -+

            subd[1] = 0.0f;

            // Test if the matrix is already diagonal.
            sum = efd::Abs(diag[0]) + efd::Abs(diag[1]);
            if (efd::Abs(subd[0]) + sum == sum)
            {
                // The current orthogonal matrix and the diagonal array do
                // not need to be updated.
                subd[0] = 0.0f;
                return true;
            }

            // Compute the eigenvalues as roots of a quadratic equation.
            sum = diag[0] + diag[1];
            diff = diag[0] - diag[1];
            discr = efd::Sqrt(diff * diff + 4.0f * subd[0] * subd[0]);
            eValue0 = 0.5f * (sum - discr);
            eValue1 = 0.5f * (sum + discr);

            // Compute the Givens rotation.
            if (diff >= 0.0f)
            {
                cosine = subd[0];
                sine = diag[0] - eValue0;
            }
            else
            {
                cosine = diag[1] - eValue0;
                sine = subd[0];
            }
            tmp = 1.0f / efd::Sqrt(cosine * cosine + sine * sine);
            cosine *= tmp;
            sine *= tmp;

            // Postmultiply the current orthogonal matrix with the Givens
            // rotation.
            for (row = 0; row < 3; row++)
            {
                tmp = m_pEntry[row][1];
                m_pEntry[row][1] = sine * m_pEntry[row][0] + cosine * tmp;
                m_pEntry[row][0] = cosine * m_pEntry[row][0] - sine * tmp;
            }

            // Update the tridiagonal matrix.
            diag[0] = eValue0;
            diag[1] = eValue1;
            subd[0] = 0.0f;
            return true;
        }

        // The matrix is
        //       +-        -+
        //   M = | d0 s0  0 |
        //       | s0 d1 s1 |
        //       | 0  s1 d2 |
        //       +-        -+

        // Set up the parameters for the first pass of the QL step.  The
        // value for A is the difference between diagonal term D[2] and the
        // implicit shift suggested by Wilkinson.
        Float32 ratio = (diag[1] - diag[0]) / (2.0f * subd[0]);
        Float32 root = efd::Sqrt(1.0f + ratio * ratio);
        Float32 b = subd[1];
        Float32 a = diag[2] - diag[0];
        if (ratio >= 0.0f)
            a += subd[0] / (ratio + root);
        else
            a += subd[0] / (ratio - root);

        // Compute the Givens rotation for the first pass.
        if (efd::Abs(b) >= efd::Abs(a))
        {
            ratio = a / b;
            sine = 1.0f / efd::Sqrt(1.0f + ratio * ratio);
            cosine = ratio * sine;
        }
        else
        {
            ratio = b / a;
            cosine = 1.0f / efd::Sqrt(1.0f + ratio * ratio);
            sine = ratio * cosine;
        }

        // Postmultiply the current orthogonal matrix with the Givens
        // rotation.
        for (row = 0; row < 3; row++)
        {
            tmp = m_pEntry[row][2];
            m_pEntry[row][2] = sine * m_pEntry[row][1] + cosine * tmp;
            m_pEntry[row][1] = cosine * m_pEntry[row][1] - sine * tmp;
        }

        // Set up the parameters for the second pass of the QL step.  The
        // values tmp0 and tmp1 are required to fully update the tridiagonal
        // matrix at the end of the second pass.
        Float32 tmp0 = (diag[1] - diag[2]) * sine +
            2.0f * subd[1] * cosine;
        Float32 tmp1 = cosine * subd[0];
        b = sine * subd[0];
        a = cosine * tmp0 - subd[1];
        tmp0 *= sine;

        // Compute the Givens rotation for the second pass.  The subdiagonal
        // term S[1] in the tridiagonal matrix is updated at this time.
        if (efd::Abs(b) >= efd::Abs(a))
        {
            ratio = a / b;
            root = efd::Sqrt(1.0f + ratio * ratio);
            subd[1] = b * root;
            sine = 1.0f / root;
            cosine = ratio * sine;
        }
        else
        {
            ratio = b / a;
            root = efd::Sqrt(1.0f + ratio * ratio);
            subd[1] = a * root;
            cosine = 1.0f / root;
            sine = ratio * cosine;
        }

        // Postmultiply the current orthogonal matrix with the Givens
        // rotation.
        for (row = 0; row < 3; row++)
        {
            tmp = m_pEntry[row][1];
            m_pEntry[row][1] = sine * m_pEntry[row][0] + cosine * tmp;
            m_pEntry[row][0] = cosine * m_pEntry[row][0] - sine * tmp;
        }

        // Complete the update of the tridiagonal matrix.
        Float32 tmp2 = diag[1] - tmp0;
        diag[2] += tmp0;
        tmp0 = (diag[0] - tmp2) * sine + 2.0f * tmp1 * cosine;
        subd[0] = cosine * tmp0 - tmp1;
        tmp0 *= sine;
        diag[1] = tmp2 + tmp0;
        diag[0] -= tmp0;
    }
    return false;
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesXYZ(
    Float32& xAngle,
    Float32& yAngle,
    Float32& zAngle) const
{
    yAngle = -efd::ASin(m_pEntry[0][2]);
    if (yAngle < EE_HALF_PI - EE_RIGHT_ANGLE_EPSILON)
    {
        if (yAngle > -EE_HALF_PI + EE_RIGHT_ANGLE_EPSILON)
        {
            xAngle = efd::FastATan2(m_pEntry[1][2],m_pEntry[2][2]);
            zAngle = efd::FastATan2(m_pEntry[0][1],m_pEntry[0][0]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            // In this situation, our remaining axes map to the exact same type of rotation
            // so we can choose a value for one and compute the other
            xAngle = -efd::FastATan2(m_pEntry[1][0],m_pEntry[1][1]);
            zAngle = 0.0f;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        // In this situation, our remaining axes map to the exact same type of rotation
        // so we can choose a value for one and compute the other
        xAngle = efd::FastATan2(m_pEntry[1][0],m_pEntry[1][1]);
        zAngle = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesXZY(
    Float32& xAngle,
    Float32& zAngle,
    Float32& yAngle) const
{
    zAngle = efd::ASin(m_pEntry[0][1]);
    if (zAngle < EE_HALF_PI - EE_RIGHT_ANGLE_EPSILON)
    {
        if (zAngle > -EE_HALF_PI + EE_RIGHT_ANGLE_EPSILON)
        {
            xAngle = -efd::FastATan2(m_pEntry[2][1],m_pEntry[1][1]);
            yAngle = -efd::FastATan2(m_pEntry[0][2],m_pEntry[0][0]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            xAngle = efd::FastATan2(-m_pEntry[2][0],m_pEntry[2][2]);
            yAngle = 0.0f;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        xAngle = efd::FastATan2(m_pEntry[2][0],m_pEntry[2][2]);
        yAngle = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesYXZ(
    Float32& yAngle,
    Float32& xAngle,
    Float32& zAngle) const
{
    xAngle = efd::ASin(m_pEntry[1][2]);
    if (xAngle < EE_HALF_PI - EE_RIGHT_ANGLE_EPSILON)
    {
        if (xAngle > -EE_HALF_PI + EE_RIGHT_ANGLE_EPSILON)
        {
            yAngle = -efd::FastATan2(m_pEntry[0][2],m_pEntry[2][2]);
            zAngle = -efd::FastATan2(m_pEntry[1][0],m_pEntry[1][1]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            yAngle = efd::FastATan2(-m_pEntry[0][1],m_pEntry[0][0]);
            zAngle = 0.0f;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        yAngle = efd::FastATan2(m_pEntry[0][1],m_pEntry[0][0]);
        zAngle = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesYZX(
    Float32& yAngle,
    Float32& zAngle,
    Float32& xAngle) const
{
    zAngle = -efd::ASin(m_pEntry[1][0]);
    if (zAngle < EE_HALF_PI - EE_RIGHT_ANGLE_EPSILON)
    {
        if (zAngle > -EE_HALF_PI + EE_RIGHT_ANGLE_EPSILON)
        {
            yAngle = efd::FastATan2(m_pEntry[2][0],m_pEntry[0][0]);
            xAngle = efd::FastATan2(m_pEntry[1][2],m_pEntry[1][1]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            // In this situation, our remaining axes map to the exact same type of rotation
            // so we can choose a value for one and compute the other
            yAngle = -efd::FastATan2(m_pEntry[2][1],m_pEntry[2][2]);
            xAngle = 0.0f;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        // In this situation, our remaining axes map to the exact same type of rotation
        // so we can choose a value for one and compute the other
        yAngle = efd::FastATan2(m_pEntry[2][1],m_pEntry[2][2]);
        xAngle = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesZXY(
    Float32& zAngle,
    Float32& xAngle,
    Float32& yAngle) const
{
    xAngle = -efd::ASin(m_pEntry[2][1]);
    if (xAngle < EE_HALF_PI - EE_RIGHT_ANGLE_EPSILON)
    {
        if (xAngle > -EE_HALF_PI + EE_RIGHT_ANGLE_EPSILON)
        {
            zAngle = efd::FastATan2(m_pEntry[0][1],m_pEntry[1][1]);
            yAngle = efd::FastATan2(m_pEntry[2][0],m_pEntry[2][2]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            // In this situation, our remaining axes map to the exact same type of rotation
            // so we can choose a value for one and compute the other
            zAngle = -efd::FastATan2(m_pEntry[0][2],m_pEntry[0][0]);
            yAngle = 0.0f;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        // In this situation, our remaining axes map to the exact same type of rotation
        // so we can choose a value for one and compute the other
        zAngle = efd::FastATan2(m_pEntry[0][2],m_pEntry[0][0]);
        yAngle = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesZYX(
    Float32& zAngle,
    Float32& yAngle,
    Float32& xAngle) const
{
    yAngle = efd::ASin(m_pEntry[2][0]);
    if (yAngle < EE_HALF_PI - EE_RIGHT_ANGLE_EPSILON)
    {
        if (yAngle > -EE_HALF_PI + EE_RIGHT_ANGLE_EPSILON)
        {
            zAngle = -efd::FastATan2(m_pEntry[1][0],m_pEntry[0][0]);
            xAngle = -efd::FastATan2(m_pEntry[2][1],m_pEntry[2][2]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            // In this situation, our remaining axes map to the exact same type of rotation
            // so we can choose a value for one and compute the other
            zAngle = efd::FastATan2(m_pEntry[0][1],m_pEntry[1][1]);
            xAngle = 0.0f;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        // In this situation, our remaining axes map to the exact same type of rotation
        // so we can choose a value for one and compute the other
        zAngle = efd::FastATan2(m_pEntry[0][1],m_pEntry[1][1]);
        xAngle = 0.0f;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
void Matrix3::FromEulerAnglesXYZ(Float32 xAngle, Float32 yAngle,
    Float32 zAngle)
{
    Matrix3 xRot, yRot, zRot;
    xRot.MakeXRotation(xAngle);
    yRot.MakeYRotation(yAngle);
    zRot.MakeZRotation(zAngle);
    *this = xRot*(yRot*zRot);
}

//------------------------------------------------------------------------------------------------
void Matrix3::FromEulerAnglesXZY (Float32 xAngle, Float32 zAngle,
    Float32 yAngle)
{
    Matrix3 xRot, yRot, zRot;
    xRot.MakeXRotation(xAngle);
    yRot.MakeYRotation(yAngle);
    zRot.MakeZRotation(zAngle);
    *this = xRot*(zRot*yRot);
}

//------------------------------------------------------------------------------------------------
void Matrix3::FromEulerAnglesYXZ (Float32 yAngle, Float32 xAngle,
    Float32 zAngle)
{
    Matrix3 xRot, yRot, zRot;
    xRot.MakeXRotation(xAngle);
    yRot.MakeYRotation(yAngle);
    zRot.MakeZRotation(zAngle);
    *this = yRot*(xRot*zRot);
}

//------------------------------------------------------------------------------------------------
void Matrix3::FromEulerAnglesYZX (Float32 yAngle, Float32 zAngle,
    Float32 xAngle)
{
    Matrix3 xRot, yRot, zRot;
    xRot.MakeXRotation(xAngle);
    yRot.MakeYRotation(yAngle);
    zRot.MakeZRotation(zAngle);
    *this = yRot*(zRot*xRot);
}

//------------------------------------------------------------------------------------------------
void Matrix3::FromEulerAnglesZXY (Float32 zAngle, Float32 xAngle,
    Float32 yAngle)
{
    Matrix3 xRot, yRot, zRot;
    xRot.MakeXRotation(xAngle);
    yRot.MakeYRotation(yAngle);
    zRot.MakeZRotation(zAngle);
    *this = zRot*(xRot*yRot);
}

//------------------------------------------------------------------------------------------------
void Matrix3::FromEulerAnglesZYX (Float32 zAngle, Float32 yAngle,
    Float32 xAngle)
{
    Matrix3 xRot, yRot, zRot;
    xRot.MakeXRotation(xAngle);
    yRot.MakeYRotation(yAngle);
    zRot.MakeZRotation(zAngle);
    *this = zRot*(yRot*xRot);
}

//------------------------------------------------------------------------------------------------
void Matrix3::Snap()
{
    const Float32 EPSILON = 1e-08f;
    for (SInt32 row = 0; row < 3; row++)
    {
        for (SInt32 col = 0; col < 3; col++)
        {
            if (efd::Abs(m_pEntry[row][col]) <= EPSILON &&
                m_pEntry[row][col] != 0.0f)
            {
                m_pEntry[row][col] = 0.0f;
            }
        }
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesXYZ_Legacy(
    Float32& xAngle,
    Float32& yAngle,
    Float32& zAngle) const
{
    yAngle = -efd::ASin(m_pEntry[0][2]);
    if (yAngle < EE_HALF_PI)
    {
        if (yAngle > -EE_HALF_PI)
        {
            xAngle = -efd::FastATan2(-m_pEntry[1][2],m_pEntry[2][2]);
            zAngle = -efd::FastATan2(-m_pEntry[0][1],m_pEntry[0][0]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            Float32 mY = efd::FastATan2(m_pEntry[1][0],m_pEntry[1][1]);
            zAngle = 0.0f;  // any angle works
            xAngle = mY - zAngle;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        Float32 y2 = efd::FastATan2(m_pEntry[1][0],m_pEntry[1][1]);
        zAngle = 0.0f;  // any angle works
        xAngle = zAngle - y2;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesYZX_Legacy(
    Float32& yAngle,
    Float32& zAngle,
    Float32& xAngle) const
{
    zAngle = -efd::ASin(m_pEntry[1][0]);
    if (zAngle < EE_HALF_PI)
    {
        if (zAngle > -EE_HALF_PI)
        {
            yAngle = -efd::FastATan2(-m_pEntry[2][0],m_pEntry[0][0]);
            xAngle = -efd::FastATan2(-m_pEntry[1][2],m_pEntry[1][1]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            Float32 mY = efd::FastATan2(m_pEntry[2][1],m_pEntry[2][2]);
            xAngle = 0.0f;  // any angle works
            yAngle = mY - xAngle;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        Float32 y2 = efd::FastATan2(m_pEntry[2][1],m_pEntry[2][2]);
        xAngle = 0.0f;  // any angle works
        yAngle = xAngle - y2;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesZXY_Legacy(
    Float32& zAngle,
    Float32& xAngle,
    Float32& yAngle) const
{
    xAngle = -efd::ASin(m_pEntry[2][1]);
    if (xAngle < EE_HALF_PI)
    {
        if (xAngle > -EE_HALF_PI)
        {
            zAngle = -efd::FastATan2(-m_pEntry[0][1],m_pEntry[1][1]);
            yAngle = -efd::FastATan2(-m_pEntry[2][0],m_pEntry[2][2]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            Float32 mY = efd::FastATan2(m_pEntry[0][2],m_pEntry[0][0]);
            yAngle = 0.0f;  // any angle works
            zAngle = mY - yAngle;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        Float32 y2 = efd::FastATan2(m_pEntry[0][2],m_pEntry[0][0]);
        yAngle = 0.0f;  // any angle works
        zAngle = yAngle - y2;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
bool Matrix3::ToEulerAnglesZYX_Legacy(
    Float32& zAngle,
    Float32& yAngle,
    Float32& xAngle) const
{
    yAngle = efd::ASin(m_pEntry[2][0]);
    if (yAngle < EE_HALF_PI)
    {
        if (yAngle > -EE_HALF_PI)
        {
            zAngle = -efd::FastATan2(m_pEntry[1][0],m_pEntry[0][0]);
            xAngle = -efd::FastATan2(m_pEntry[2][1],m_pEntry[2][2]);
            return true;
        }
        else
        {
            // WARNING.  Not a unique solution.
            Float32 mY = efd::FastATan2(-m_pEntry[0][1],m_pEntry[0][2]);
            xAngle = 0.0f;  // any angle works
            zAngle = mY - xAngle;
            return false;
        }
    }
    else
    {
        // WARNING.  Not a unique solution.
        Float32 y2 = efd::FastATan2(-m_pEntry[0][1],m_pEntry[0][2]);
        xAngle = 0.0f;  // any angle works
        zAngle = xAngle - y2;
        return false;
    }
}

//------------------------------------------------------------------------------------------------
void Matrix3::Serialize(efd::Archive& ar)
{
    // To deal with endianness we need to serialize each float separately:
    efd::SR_FixedArray<3, efd::SR_FixedArray<3> >::Serialize(m_pEntry, ar);
}

//------------------------------------------------------------------------------------------------
