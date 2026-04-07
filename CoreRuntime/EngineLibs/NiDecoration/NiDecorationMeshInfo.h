// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2010 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#ifndef NIDECORATIONMESHINFO_H
#define NIDECORATIONMESHINFO_H

#include <NiMesh.h>

/**
    Interface for mesh info
 */
class NiDecorationMeshInfo : public NiRefObject
{
public:
    NiDecorationMeshInfo(NiMesh* pkMesh);
    virtual ~NiDecorationMeshInfo();

    virtual NiMesh* GetMesh() const;

protected:
    NiMeshPtr m_spTargetMesh;
};

#endif // NIDECORATIONMESHINFO_H
