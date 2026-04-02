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
inline NiUInt32 NiDecorationMeshGenerator::GetIndicesPerMesh() const
{
    return m_uiIndicesPerMesh;
}

//------------------------------------------------------------------------------------------------
inline NiUInt32 NiDecorationMeshGenerator::GetVerticesPerMesh() const
{
    return m_uiVerticesPerMesh;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationMeshGenerator::SetIndicesPerMesh(NiUInt32 uiNumIndices)
{
    m_uiIndicesPerMesh = uiNumIndices;
}

//------------------------------------------------------------------------------------------------
inline void NiDecorationMeshGenerator::SetVerticesPerMesh(NiUInt32 uiNumVertices)
{
    m_uiVerticesPerMesh = uiNumVertices;
}

//------------------------------------------------------------------------------------------------
template <class TFromType, class TToType> inline void NiDecorationMeshGenerator::CastStream(
    const TFromType* pCastFrom, void* pTo, NiUInt32 uiCount)
{
    TToType* pCastTo = (TToType*)pTo;
    for (NiUInt32 ui = 0; ui < uiCount; ++ui)
    {
        pCastTo[ui] = (TToType)pCastFrom[ui];
    }
}

//------------------------------------------------------------------------------------------------
