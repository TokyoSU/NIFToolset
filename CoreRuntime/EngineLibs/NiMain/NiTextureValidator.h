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

#pragma once
#ifndef NiTextureValidator_H
#define NiTextureValidator_H

class NiSourceTexture;
class NiPixelData;

class NIMAIN_ENTRY NiTextureValidator : public NiRefObject
{
    NiDeclareRootRTTI(NiTextureValidator);

public:

    NiTextureValidator();
    virtual ~NiTextureValidator();
    
    virtual void ValidateTexture(NiSourceTexture* pTexture, efd::SmartPointer<NiPixelData>& spPixelData);
};

typedef efd::SmartPointer<NiTextureValidator> NiTextureValidatorPtr;
//------------------------------------------------------------------------------------------------

#endif
