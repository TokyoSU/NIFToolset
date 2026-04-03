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

//------------------------------------------------------------------------------------------------

inline void ecr::CameraData::SetFOV( const efd::Float32& FOV )
{
    m_Fov = FOV;
}

//------------------------------------------------------------------------------------------------

inline efd::Float32  ecr::CameraData::GetFOV()const
{
    return m_Fov;
}

//------------------------------------------------------------------------------------------------

inline void  ecr::CameraData::SetNearPlane(const efd::Float32& NearPlane)
{
    m_NearPlane = NearPlane;
}

//------------------------------------------------------------------------------------------------

inline efd::Float32  ecr::CameraData::GetNearPlane()const
{
    return m_NearPlane;
}

//------------------------------------------------------------------------------------------------

inline void  ecr::CameraData::SetFarPlane(const efd::Float32& FarPlane)
{
    m_FarPlane = FarPlane;
}

//------------------------------------------------------------------------------------------------

inline efd::Float32  ecr::CameraData::GetFarPlane()const
{
    return m_FarPlane;
}

//------------------------------------------------------------------------------------------------

inline void  ecr::CameraData::SetMinimumNearPlane(const efd::Float32& MinNearPlane)
{
    m_MinNearPlane = MinNearPlane;
}

//------------------------------------------------------------------------------------------------

inline efd::Float32  ecr::CameraData::GetMinimumNearPlane()const
{
    return m_MinNearPlane;
}

//------------------------------------------------------------------------------------------------

inline void  ecr::CameraData::SetMaximumFarToNearRatio(const efd::Float32& MaxFtoN)
{
    m_MaxFtoN = MaxFtoN;
}

//------------------------------------------------------------------------------------------------

inline efd::Float32  ecr::CameraData::GetMaximumFarToNearRatio()const
{
    return m_MaxFtoN;
}

//------------------------------------------------------------------------------------------------

inline void  ecr::CameraData::SetIsOrthographic(const efd::Bool& IsOrthographic)
{
    m_IsOrthographic = IsOrthographic;
}

//------------------------------------------------------------------------------------------------

inline efd::Bool  ecr::CameraData::GetIsOrthographic()const
{
    return m_IsOrthographic;
}

//------------------------------------------------------------------------------------------------

inline void  ecr::CameraData::SetLODAdjust(const efd::SInt32& LODAdjust)
{
    m_LODAdjust = LODAdjust;
}

//------------------------------------------------------------------------------------------------

inline efd::SInt32 ecr::CameraData::GetLODAdjust()const
{
    return m_LODAdjust;
}

//------------------------------------------------------------------------------------------------


inline void ecr::CameraData::SetCamera( NiCamera* camera )
{
    m_spCamera = camera;
}

//--------------------------------------------------------------------------------------------------

inline NiCamera* ecr::CameraData::GetCamera()const
{
    return m_spCamera;
}

//--------------------------------------------------------------------------------------------------
