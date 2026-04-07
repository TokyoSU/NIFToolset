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

#ifndef EE_REMOVE_BACK_COMPAT_STREAMING

//--------------------------------------------------------------------------------------------------
inline NiGeometry* NiPSysMeshEmitter::GetGeometryEmitter(
    unsigned int uiWhichEmitter)
{
    if (uiWhichEmitter < GetGeometryEmitterCount())
        return m_kGeomEmitterArray.GetAt(uiWhichEmitter);
    else
        return NULL;
}

//--------------------------------------------------------------------------------------------------
inline unsigned int NiPSysMeshEmitter::GetGeometryEmitterCount()
{
    return m_kGeomEmitterArray.GetEffectiveSize();
}

//--------------------------------------------------------------------------------------------------
inline unsigned int NiPSysMeshEmitter::GetGeometryEmitterArrayCount()
{
    return m_kGeomEmitterArray.GetSize();
}

//--------------------------------------------------------------------------------------------------
inline NiPSysMeshEmitter::MeshEmissionType
    NiPSysMeshEmitter::GetMeshEmissionType()
{
    return m_eEmissionType;
}

//--------------------------------------------------------------------------------------------------
inline NiPSysMeshEmitter::InitialVelocityType
    NiPSysMeshEmitter::GetInitialVelocityType()
{
    return m_eInitVelocityType;
}

//--------------------------------------------------------------------------------------------------
inline const NiPoint3& NiPSysMeshEmitter::GetEmitAxis()
{
    return m_kEmitAxis;
}

//--------------------------------------------------------------------------------------------------
#endif // #ifndef EE_REMOVE_BACK_COMPAT_STREAMING
