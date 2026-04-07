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
#include <efd/Quaternion.h>
#include <efd/Point3.h>

// Some system headers define a Quaternion, so we must explicitly scope all references here
using namespace efd;

const efd::Quaternion efd::Quaternion::IDENTITY(1.0f,0.0f,0.0f,0.0f);
const Float32 efd::Quaternion::ms_epsilon = 1e-03f;

//--------------------------------------------------------------------------------------------------
efd::Quaternion::Quaternion(Float32 angle, const Point3 &axis)
{
    FromAngleAxis(angle, axis);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::operator+(const efd::Quaternion& q) const
{
    return efd::Quaternion(m_fW + q.m_fW, m_fX + q.m_fX, m_fY + q.m_fY, m_fZ + q.m_fZ);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::operator-(const efd::Quaternion& q) const
{
    return efd::Quaternion(m_fW - q.m_fW, m_fX - q.m_fX, m_fY - q.m_fY, m_fZ - q.m_fZ);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::operator-() const
{
    return efd::Quaternion(-m_fW, -m_fX, -m_fY, -m_fZ);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::operator*(const efd::Quaternion& q) const
{
    // NOTE:  Multiplication is not generally commutative, so in most
    // cases p*q != q*p.

    /*
    // This code is left here for anyone who gets the urge to switch to
    // a "faster" quaternion multiplication routine.  While on some
    // architectures this might be, on a Pentium it is NOT.
    //
    Float32 A = (m_fW + m_fX) * (q.m_fW + q.m_fX);
    Float32 B = (m_fZ - m_fY) * (q.m_fY - q.m_fZ);
    Float32 C = (m_fX - m_fW) * (q.m_fY + q.m_fZ);
    Float32 D = (m_fY + m_fZ) * (q.m_fX - q.m_fW);
    Float32 E = (m_fX + m_fZ) * (q.m_fX + q.m_fY);
    Float32 F = (m_fX - m_fZ) * (q.m_fX - q.m_fY);
    Float32 G = (m_fW + m_fY) * (q.m_fW - q.m_fZ);
    Float32 H = (m_fW - m_fY) * (q.m_fW + q.m_fZ);

    return efd::Quaternion
    (
        B + (-E - F + G + H) / 2,
        A -  (E + F + G + H) / 2,
       -C +  (E - F + G - H) / 2,
       -D +  (E - F - G + H) / 2
   );
    */

    return efd::Quaternion(
        m_fW * q.m_fW - m_fX * q.m_fX - m_fY * q.m_fY - m_fZ * q.m_fZ,
        m_fW * q.m_fX + m_fX * q.m_fW + m_fY * q.m_fZ - m_fZ * q.m_fY,
        m_fW * q.m_fY + m_fY * q.m_fW + m_fZ * q.m_fX - m_fX * q.m_fZ,
        m_fW * q.m_fZ + m_fZ * q.m_fW + m_fX * q.m_fY - m_fY * q.m_fX);
}

//--------------------------------------------------------------------------------------------------
efd::Point3 efd::Quaternion::operator*(const efd::Point3& p) const
{
    efd::Quaternion v(0.0f, p.x, p.y, p.z);
    efd::Quaternion qStar(m_fW, -m_fX, -m_fY, -m_fZ);
    efd::Quaternion vQStar = v * qStar;
    v = (*this) * vQStar;
    return efd::Point3(v.m_fX, v.m_fY, v.m_fZ);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::UnitInverse(const efd::Quaternion& p)
{
    return efd::Quaternion(p.m_fW, -p.m_fX, -p.m_fY, -p.m_fZ);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::Log(const efd::Quaternion& q)
{
    // q = cos(A)+sin(A)*(x*i+y*j+z*k) where (x,y,z) is unit length
    // log(q) = A*(x*i+y*j+z*k)

    Float32 angle = efd::ACos(q.m_fW);
    Float32 sn = efd::Sin(angle);

    // When A is near zero, A/sin(A) is approximately 1.  Use
    // log(q) = sin(A)*(x*i+y*j+z*k)
    Float32 coeff = (efd::Abs(sn) < ms_epsilon ? 1.0f : angle/sn);

    return efd::Quaternion(0.0f, coeff * q.m_fX, coeff * q.m_fY, coeff * q.m_fZ);
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::Intermediate(
    const efd::Quaternion& q0,
    const efd::Quaternion& q1,
    const efd::Quaternion& q2)
{
    efd::Quaternion inv = UnitInverse(q1);
    efd::Quaternion exp = Exp(-0.25f * (Log(inv * q0) + Log(inv * q2)));

    return q1 * exp;
}

//--------------------------------------------------------------------------------------------------
efd::Quaternion efd::Quaternion::Squad(
    Float32 t,
    const efd::Quaternion& p,
    const efd::Quaternion& a,
    const efd::Quaternion& b,
    const efd::Quaternion& q)
{
    return Slerp(2.0f*t*(1.0f-t),Slerp(t,p,q),Slerp(t,a,b));
}

//--------------------------------------------------------------------------------------------------
void efd::Quaternion::FromAngleAxis(Float32 angle, const Point3& axis)
{
    Float32 halfAngle = angle * 0.5f;
    Float32 sn;

    efd::SinCos(halfAngle, sn, m_fW);
    m_fX = axis.x * sn;
    m_fY = axis.y * sn;
    m_fZ = axis.z * sn;
}

//--------------------------------------------------------------------------------------------------
void efd::Quaternion::ToAngleAxis(Float32& angle, Point3& axis) const
{
    Float32 length = efd::Sqrt(m_fX*m_fX + m_fY*m_fY + m_fZ*m_fZ);

    if (length < ms_epsilon)
    {
        angle = 0;
        axis.x = 0;
        axis.y = 0;
        axis.z = 0;
    }
    else
    {
        angle = 2.0f * efd::ACos(m_fW);
        Float32 invLength = 1.0f/length;

        axis.x = m_fX * invLength;
        axis.y = m_fY * invLength;
        axis.z = m_fZ * invLength;
    }
}

//--------------------------------------------------------------------------------------------------
void efd::Quaternion::FromRotation(const Matrix3& rot)
{
    // Algorithm in Ken Shoemake's article in 1987 SIGGraPH course notes
    // article "Quaternion Calculus and Fast Animation".

    Float32 trace = rot.GetEntry(0,0) + rot.GetEntry(1,1) + rot.GetEntry(2,2);
    Float32 root;

    if (trace > 0.0f)
    {
        // |w| > 1/2, may as well choose w > 1/2
        root = efd::Sqrt(trace+1.0f);  // 2w
        m_fW = 0.5f*root;
        root = 0.5f/root;  // 1/(4w)

        m_fX = (rot.GetEntry(2,1) - rot.GetEntry(1,2)) * root;
        m_fY = (rot.GetEntry(0,2) - rot.GetEntry(2,0)) * root;
        m_fZ = (rot.GetEntry(1,0) - rot.GetEntry(0,1)) * root;
    }
    else
    {
        // |w| <= 1/2
        const static SInt32 next[3] = { 1, 2, 0 };
        SInt32 i = 0;
        if (rot.GetEntry(1,1) > rot.GetEntry(0,0))
            i = 1;
        if (rot.GetEntry(2,2) > rot.GetEntry(i,i))
            i = 2;
        int j = next[i];
        int k = next[j];

        root = efd::Sqrt(rot.GetEntry(i,i) -
            rot.GetEntry(j,j) - rot.GetEntry(k,k) + 1.0f);

        Float32* quat[3] = { &m_fX, &m_fY, &m_fZ };
        *quat[i] = 0.5f*root;
        root = 0.5f/root;
        m_fW = (rot.GetEntry(k,j) - rot.GetEntry(j,k)) * root;
        *quat[j] = (rot.GetEntry(j,i) + rot.GetEntry(i,j)) * root;
        *quat[k] = (rot.GetEntry(k,i) + rot.GetEntry(i,k)) * root;
    }
}

//--------------------------------------------------------------------------------------------------
void efd::Quaternion::Snap()
{
    const Float32 epsilon = 1e-08f;
    if (efd::Abs(m_fX) <= epsilon && m_fX != 0.0f)
    {
        m_fX = 0.0f;
    }

    if (efd::Abs(m_fY) <= epsilon && m_fY != 0.0f)
    {
        m_fY = 0.0f;
    }

    if (efd::Abs(m_fZ) <= epsilon && m_fZ != 0.0f)
    {
        m_fZ = 0.0f;
    }

    if (efd::Abs(m_fW) <= epsilon && m_fW != 0.0f)
    {
        m_fW = 0.0f;
    }
}

//--------------------------------------------------------------------------------------------------
void efd::Quaternion::Normalize()
{
     Float32 length = m_fW * m_fW + m_fX * m_fX + m_fY * m_fY + m_fZ * m_fZ;
     Float32 invLength = 1.0f / efd::Sqrt(length);
     *this = *this * invLength;
}

//-------------------------------------------------------------------------------------------------
void efd::Quaternion::Serialize(efd::Archive& io_archive)
{
    efd::Serializer::SerializePrimitive<efd::Float32>(m_fW, io_archive);
    efd::Serializer::SerializePrimitive<efd::Float32>(m_fX, io_archive);
    efd::Serializer::SerializePrimitive<efd::Float32>(m_fY, io_archive);
    efd::Serializer::SerializePrimitive<efd::Float32>(m_fZ, io_archive);
}

//--------------------------------------------------------------------------------------------------
