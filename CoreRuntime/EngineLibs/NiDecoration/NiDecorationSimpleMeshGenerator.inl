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
inline void NiDecorationSimpleMeshGenerator::SetMeshPath(const efd::utf8string& kMeshPath)
{
    m_kMeshPath = kMeshPath;
}

//------------------------------------------------------------------------------------------------
inline const efd::utf8string& NiDecorationSimpleMeshGenerator::GetMeshPath() const
{
    return m_kMeshPath;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationSimpleMeshGenerator::SetObjectName(const efd::utf8string& kObjectName)
{
    m_kObjectName = kObjectName;
}

//------------------------------------------------------------------------------------------------
inline const efd::utf8string& NiDecorationSimpleMeshGenerator::GetObjectName() const
{
    return m_kObjectName;
}

//------------------------------------------------------------------------------------------------