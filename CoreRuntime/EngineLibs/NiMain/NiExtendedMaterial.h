// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//      TokyoSU Copyright 2026
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#pragma once
#ifndef NIMAIN_NIEXTENDEDMATERIAL_H
#define NIMAIN_NIEXTENDEDMATERIAL_H

#include <NiStandardMaterial.h>
#include <NiSourceTexture.h>

NiSmartPointer(NiExtendedMaterial);

class NIMAIN_ENTRY NiExtendedMaterial : public NiStandardMaterial
{
    NiDeclareRTTI;

public:
    enum
    {
        MAX_TERRAIN_LAYERS = 32,
        EXTENDED_VERTEX_VERSION = NiStandardMaterial::VERTEX_VERSION,
        EXTENDED_GEOMETRY_VERSION = NiStandardMaterial::GEOMETRY_VERSION,
        EXTENDED_PIXEL_VERSION = NiStandardMaterial::PIXEL_VERSION + 10
    };

    static NiExtendedMaterial* Create();

    void SetTerrainEnabled(bool bEnabled);
    bool GetTerrainEnabled() const;

    void SetTerrainTextureArray(NiSourceTexture* pkTexture);
    NiSourceTexture* GetTerrainTextureArray() const;

    void SetTerrainAlphaArray(NiSourceTexture* pkTexture);
    NiSourceTexture* GetTerrainAlphaArray() const;

    void SetTerrainLayerCount(NiUInt32 uiLayerCount);
    NiUInt32 GetTerrainLayerCount() const;

    void SetTerrainLayer(
        NiUInt32 uiLayer,
        float fScaleU,
        float fScaleV,
        float fWeight,
        bool bInvertAlpha);

protected:
    NiExtendedMaterial(bool bAutoCreateCaches = true);
    virtual ~NiExtendedMaterial();

    virtual bool GenerateDescriptor(
        const NiRenderObject* pkGeometry,
        const NiPropertyState* pkState,
        const NiDynamicEffectState* pkEffects,
        NiMaterialDescriptor& kMaterialDesc) override;

    virtual bool GeneratePixelShadeTree(
        Context& kContext,
        NiGPUProgramDescriptor* pkDesc) override;

    virtual bool HandlePreLightTextureApplication(
        Context& kContext,
        NiStandardPixelProgramDescriptor* pkPixelDesc,
        NiMaterialResource*& pkWorldPos,
        NiMaterialResource*& pkWorldNormal,
        NiMaterialResource*& pkWorldBinormal,
        NiMaterialResource*& pkWorldTangent,
        NiMaterialResource*& pkWorldViewVector,
        NiMaterialResource*& pkTangentViewVector,
        NiMaterialResource*& pkMatDiffuseColor,
        NiMaterialResource*& pkMatSpecularColor,
        NiMaterialResource*& pkMatSpecularPower,
        NiMaterialResource*& pkMatGlossiness,
        NiMaterialResource*& pkMatAmbientColor,
        NiMaterialResource*& pkMatEmissiveColor,
        NiMaterialResource*& pkOpacityAccum,
        NiMaterialResource*& pkAmbientLightAccum,
        NiMaterialResource*& pkDiffuseLightAccum,
        NiMaterialResource*& pkSpecularLightAccum,
        NiMaterialResource*& pkTexDiffuseAccum,
        NiMaterialResource*& pkTexSpecularAccum) override;

private:
    bool m_bTerrainEnabled;

    NiSourceTexturePtr m_spTerrainTextureArray;
    NiSourceTexturePtr m_spTerrainAlphaArray;

    NiUInt32 m_uiTerrainLayerCount;
    NiPoint4 m_akTerrainLayerData[MAX_TERRAIN_LAYERS];
};

#endif  // #ifndef NIMAIN_NIEXTENDEDMATERIAL_H
