// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

//--------------------------------------------------------------------------------------------------
inline NiTerrainFileInterface::FileVersion NiTerrainFileInterface::GetFileVersion() const
{
    return m_kFileVersion;
}

//--------------------------------------------------------------------------------------------------
inline bool NiTerrainFileInterface::IsReady() const
{
    return m_bOpen;
}

//--------------------------------------------------------------------------------------------------
inline bool NiTerrainFileInterface::IsWritable() const
{
    return m_eAccessMode == efd::File::WRITE_ONLY;
}

//--------------------------------------------------------------------------------------------------