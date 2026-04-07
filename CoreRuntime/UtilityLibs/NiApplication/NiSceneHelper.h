#pragma once
#ifndef NISCENEHELPER_H
#define NISCENEHELPER_H

#include "NiLightHelper.h"
#include "NiMeshHelper.h"

#include <NiEnvironmentLibType.h>
#include <NiAtmosphere.h>
#include <NiCamera.h>
#include <NiDirectionalLight.h>
#include <NiEnvironment.h>
#include <NiFogProperty.h>
#include <NiImageConverter.h>
#include <NiMath.h>
#include <NiNode.h>
#include <NiRenderClick.h>
#include <NiRenderer.h>
#include <NiSkyBlendStage.h>
#include <NiSky.h>
#include <NiSourceCubeMap.h>
#include <NiStencilProperty.h>
#include <NiSystem.h>
#include <NiZBufferProperty.h>

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// NiSceneHelper
//
// Header-only helpers for quickly building a base scene graph.
//
// NiEnvironment setup distilled from the engine implementation:
//   1. Create the environment node.
//   2. Create atmosphere / sky / sun / fog from that environment.
//   3. Attach the environment node to the scene graph.
//   4. Apply the sun to the scene as an affected node.
//   5. Optionally attach the fog property to the scene root as well.
//
// Notes:
//   - The sky is not rendered automatically by the main scene pass.
//     Use `NiEnvironment::CreateSkyRenderClick(pkCamera)` and render that
//     click before the normal scene render if you want the sky/background.
//   - When an environment is enabled, the returned directional light is the
//     environment sun.
// ---------------------------------------------------------------------------
namespace NiSceneHelper
{
    struct DirectionalLightDesc
    {
        NiColor m_kAmbient  = NiColor(0.20f, 0.20f, 0.20f);
        NiColor m_kDiffuse  = NiColor(1.00f, 0.98f, 0.92f);
        NiColor m_kSpecular = NiColor(1.00f, 0.98f, 0.92f);
        float   m_fDimmer   = 1.0f;
        bool    m_bEnabled  = true;
        float   m_fPitchDeg = -45.0f;
        float   m_fYawDeg   = 45.0f;
    };

    struct EnvironmentDesc
    {
        bool    m_bCreateEnvironment        = true;
        bool    m_bAttachEnvironmentToScene = true;
        bool    m_bCreateAtmosphere         = true;
        bool    m_bCreateSkyDome            = true;
        bool    m_bCreateSkySphere          = false;
        bool    m_bCreateGradientBlendStage = false;
        bool    m_bCreateSkyboxBlendStage   = false;
        bool    m_bCreateSkyRenderClick     = true;
        bool    m_bCreateFogProperty        = true;
        bool    m_bEnableFog                = false;
        float   m_fFogDepth                 = 1.0f;
        NiColor m_kFogColor                 = NiColor(0.65f, 0.72f, 0.82f);
        bool    m_bAutoCalcFogColor         = true;
        bool    m_bAutoSetBackgroundColor   = true;
        NiUInt32 m_uiGradientStage          = 0;
        NiColorA m_kGradientSkyColor        = NiColorA(0.45f, 0.62f, 0.90f, 1.0f);
        NiColorA m_kGradientZenithColor     = NiColorA(0.18f, 0.28f, 0.52f, 1.0f);
        NiColorA m_kGradientHorizonColor    = NiColorA(0.95f, 0.80f, 0.25f, 1.0f);
        float    m_fGradientSkyBlend        = 0.35f;
        float    m_fGradientHorizonBias     = 0.10f;
        float    m_fGradientBiasExponent    = 3.0f;

        NiUInt32    m_uiSkyboxStage         = 0;
        std::string m_strSkyboxCubeMapPath;
        std::string m_strSkyboxPosXPath;
        std::string m_strSkyboxNegXPath;
        std::string m_strSkyboxPosYPath;
        std::string m_strSkyboxNegYPath;
        std::string m_strSkyboxPosZPath;
        std::string m_strSkyboxNegZPath;

        NiTexture*  m_pkSkyboxTexture       = nullptr;
        bool        m_bOrientSkybox         = false;
        NiMatrix3   m_kSkyboxOrientation    = NiMatrix3::IDENTITY;
        float       m_fSkyboxBlendConstant  = 1.0f;
    };

    struct SceneDesc
    {
        std::string         m_strSceneName = "Scene";
        NiCamera*           m_pkCamera = nullptr;
        DirectionalLightDesc m_kDirectionalLight;
        EnvironmentDesc      m_kEnvironment;
    };

    struct CloudLayerDesc
    {
        std::string m_strName = "CloudLayer";
        NiNode*     m_pkAttachParent = nullptr;
        NiPoint3    m_kCenter = NiPoint3::ZERO;
        float       m_fWidth = 1.0f;
        float       m_fHeight = 1.0f;
        std::string m_strTexturePath;
        NiSourceTexture* m_pkTexture = nullptr;
        NiPoint2    m_kUVScale = NiPoint2(1.0f, 1.0f);
        float       m_fScrollU = 0.0f;
        float       m_fScrollV = 0.0f;
        bool        m_bAlphaBlend = true;
        bool        m_bZBufferTest = true;
        bool        m_bZBufferWrite = false;
        bool        m_bBillboard = false;
        NiBillboardNode::FaceMode m_eBillboardMode =
            NiBillboardNode::ALWAYS_FACE_CAMERA;
    };

    struct BaseScene
    {
        NiPointer<NiNode>             m_spScene;
        NiPointer<NiCamera>           m_spCamera;
        NiPointer<NiEnvironment>      m_spEnvironment;
        NiPointer<NiAtmosphere>       m_spAtmosphere;
        NiPointer<NiSky>              m_spSky;
        NiPointer<NiSkyGradientBlendStage> m_spGradientBlendStage;
        NiPointer<NiSkySkyboxBlendStage> m_spSkyboxBlendStage;
        NiPointer<NiRenderClick>      m_spSkyRenderClick;
        NiPointer<NiDirectionalLight> m_spDirectionalLight;
        NiPointer<NiFogProperty>      m_spFogProperty;
    };

    struct CloudLayer
    {
        NiPointer<NiAVObject>      m_spRoot;
        NiPointer<NiMesh>          m_spMesh;
        NiPointer<NiSourceTexture> m_spTexture;
        NiScrollMaterial           m_kScrollMaterial;
    };

    namespace Detail
    {
        class FullSkySphere : public NiSky
        {
        public:
            FullSkySphere()
            {
                LoadDefaultConfiguration();
            }

        protected:
            virtual void LoadDefaultConfiguration()
            {
                NiSky::LoadDefaultConfiguration();
                SetAtmosphericCalcMode(NiSkyMaterial::AtmosphericCalcMode::NONE);
                m_bGeometrySettingsChanged = true;
                m_fRadius = 50000.0f;
                m_uiLongitudeSegments = 32;
                m_uiLatitudeSegments = 16;
            }

            virtual void UpdateGeometry()
            {
                if (m_spGeometry)
                    DetachChild(m_spGeometry);

                m_spGeometry = GenerateGeometry();
                EE_ASSERT(m_spGeometry);

                AttachExtraData(m_spGeometry);
                m_spGeometry->ApplyAndSetActiveMaterial(m_spSkyMaterial);
                AttachChild(m_spGeometry);
                m_spGeometry->UpdateProperties();
                m_spGeometry->UpdateEffects();
                m_spGeometry->Update(0.0f);
            }

        private:
            NiPointer<NiMesh> GenerateGeometry() const
            {
                const NiUInt32 uiLongitudeSegments =
                    efd::Max(m_uiLongitudeSegments, 3u);
                const NiUInt32 uiLatitudeSegments =
                    efd::Max(m_uiLatitudeSegments, 2u);
                const NiUInt32 uiVertexCount =
                    (uiLatitudeSegments + 1) * (uiLongitudeSegments + 1);

                std::vector<NiPoint3> kPositions;
                std::vector<NiPoint3> kNormals;
                std::vector<NiUInt32> kIndices;
                kPositions.reserve(uiVertexCount);
                kNormals.reserve(uiVertexCount);
                kIndices.reserve(uiLatitudeSegments * uiLongitudeSegments * 6);

                for (NiUInt32 uiLat = 0; uiLat <= uiLatitudeSegments; ++uiLat)
                {
                    const float fPhi =
                        NiLerp(float(uiLat) / float(uiLatitudeSegments),
                            0.0f,
                            NI_PI);
                    float fSinPhi;
                    float fCosPhi;
                    NiSinCos(fPhi, fSinPhi, fCosPhi);

                    for (NiUInt32 uiLon = 0; uiLon <= uiLongitudeSegments; ++uiLon)
                    {
                        const float fTheta =
                            NiLerp(float(uiLon) / float(uiLongitudeSegments),
                                0.0f,
                                NI_PI * 2.0f);
                        float fSinTheta;
                        float fCosTheta;
                        NiSinCos(fTheta, fSinTheta, fCosTheta);

                        const NiPoint3 kDirection(
                            fSinPhi * fCosTheta,
                            fSinPhi * fSinTheta,
                            fCosPhi);
                        kPositions.push_back(kDirection * m_fRadius);
                        kNormals.push_back(-kDirection);
                    }
                }

                for (NiUInt32 uiLat = 0; uiLat < uiLatitudeSegments; ++uiLat)
                {
                    const NiUInt32 uiRowStart = uiLat * (uiLongitudeSegments + 1);
                    const NiUInt32 uiNextRowStart =
                        (uiLat + 1) * (uiLongitudeSegments + 1);

                    for (NiUInt32 uiLon = 0; uiLon < uiLongitudeSegments; ++uiLon)
                    {
                        const NiUInt32 ui0 = uiRowStart + uiLon;
                        const NiUInt32 ui1 = ui0 + 1;
                        const NiUInt32 ui2 = uiNextRowStart + uiLon;
                        const NiUInt32 ui3 = ui2 + 1;

                        kIndices.push_back(ui0);
                        kIndices.push_back(ui2);
                        kIndices.push_back(ui1);

                        kIndices.push_back(ui1);
                        kIndices.push_back(ui2);
                        kIndices.push_back(ui3);
                    }
                }

                NiMeshDesc kMeshDesc;
                kMeshDesc.pkPositions = kPositions.data();
                kMeshDesc.uiVertexCount = static_cast<NiUInt32>(kPositions.size());
                kMeshDesc.pkIndices = kIndices.data();
                kMeshDesc.uiIndexCount = static_cast<NiUInt32>(kIndices.size());
                kMeshDesc.pkNormals = kNormals.data();
                kMeshDesc.bZBufferTest = false;

                NiPointer<NiMesh> spMesh = NiMeshHelper::Create(kMeshDesc);
                if (!spMesh)
                    return nullptr;

                spMesh->SetName("SkySphere");
                spMesh->RecomputeBounds();

                NiStencilProperty* pkStencilProp = NiNew NiStencilProperty();
                pkStencilProp->SetDrawMode(NiStencilProperty::DRAW_BOTH);
                spMesh->AttachProperty(pkStencilProp);

                NiZBufferProperty* pkZProp = NiDynamicCast(
                    NiZBufferProperty,
                    spMesh->GetProperty(NiZBufferProperty::GetType()));
                if (!pkZProp)
                {
                    pkZProp = NiNew NiZBufferProperty();
                    spMesh->AttachProperty(pkZProp);
                }

                pkZProp->SetZBufferWrite(false);
                pkZProp->SetZBufferTest(false);
                pkZProp->SetTestFunction(NiZBufferProperty::TEST_LESSEQUAL);
                spMesh->UpdateProperties();

                return spMesh;
            }

            float m_fRadius;
            NiUInt32 m_uiLongitudeSegments;
            NiUInt32 m_uiLatitudeSegments;
        };

        inline bool HasSkyboxFacePaths(const EnvironmentDesc& kDesc)
        {
            return !kDesc.m_strSkyboxPosXPath.empty() &&
                !kDesc.m_strSkyboxNegXPath.empty() &&
                !kDesc.m_strSkyboxPosYPath.empty() &&
                !kDesc.m_strSkyboxNegYPath.empty() &&
                !kDesc.m_strSkyboxPosZPath.empty() &&
                !kDesc.m_strSkyboxNegZPath.empty();
        }

        inline NiPixelData* LoadSkyboxFacePixelData(const std::string& strPath)
        {
            if (strPath.empty())
                return nullptr;

            NiImageConverter* pkConverter = NiImageConverter::GetImageConverter();
            if (!pkConverter)
                return nullptr;

            NiFixedString kPath = strPath.c_str();
            NiStandardizeFilePath(kPath);

            NiFixedString kPlatformPath;
            NiImageConverter::ConvertFilenameToPlatformSpecific(
                kPath,
                kPlatformPath);

            NiPixelData* pkPixelData =
                pkConverter->ReadImageFile(kPlatformPath, nullptr);
            if (!pkPixelData)
            {
                NiOutputDebugString("NiSceneHelper skybox face load failed: ");
                NiOutputDebugString(kPlatformPath);
                NiOutputDebugString("\n");
            }

            return pkPixelData;
        }

        inline NiMatrix3 MakePitchYaw(float fPitchDeg, float fYawDeg)
        {
            NiMatrix3 kPitch;
            NiMatrix3 kYaw;
            kPitch.MakeXRotation(NiDegToRad(fPitchDeg));
            kYaw.MakeZRotation(NiDegToRad(fYawDeg));
            return kYaw * kPitch;
        }

        inline void ApplyDirectionalLight(
            NiDirectionalLight* pkLight,
            const DirectionalLightDesc& kDesc)
        {
            if (!pkLight)
                return;

            pkLight->SetAmbientColor(kDesc.m_kAmbient);
            pkLight->SetDiffuseColor(kDesc.m_kDiffuse);
            pkLight->SetSpecularColor(kDesc.m_kSpecular);
            pkLight->SetDimmer(kDesc.m_fDimmer);
            pkLight->SetSwitch(kDesc.m_bEnabled);
            pkLight->SetRotate(MakePitchYaw(kDesc.m_fPitchDeg, kDesc.m_fYawDeg));
        }

        inline void ApplyFogProperty(
            NiFogProperty* pkFogProperty,
            const EnvironmentDesc& kDesc)
        {
            if (!pkFogProperty)
                return;

            pkFogProperty->SetFog(kDesc.m_bEnableFog);
            pkFogProperty->SetFogFunction(NiFogProperty::FOG_Z_LINEAR);
            pkFogProperty->SetFogColor(kDesc.m_kFogColor);
            pkFogProperty->SetDepth(kDesc.m_fFogDepth);
        }

        inline void FinalizeScene(NiNode* pkScene)
        {
            if (!pkScene)
                return;

            pkScene->UpdateProperties();
            pkScene->UpdateEffects();
            pkScene->Update(0.0f);
        }

        inline NiColorA BlendColor(
            const NiColorA& kBaseColor,
            const NiColorA& kSkyColor,
            float fBlend)
        {
            const float fBaseWeight = 1.0f - fBlend;
            return NiColorA(
                kBaseColor.r * fBaseWeight + kSkyColor.r * fBlend,
                kBaseColor.g * fBaseWeight + kSkyColor.g * fBlend,
                kBaseColor.b * fBaseWeight + kSkyColor.b * fBlend,
                kBaseColor.a * fBaseWeight + kSkyColor.a * fBlend);
        }

        inline NiPointer<NiSkyGradientBlendStage> CreateGradientBlendStage(
            const EnvironmentDesc& kDesc)
        {
            NiPointer<NiSkyGradientBlendStage> spStage =
                NiNew NiSkyGradientBlendStage();

            spStage->SetZenithColor(BlendColor(
                kDesc.m_kGradientZenithColor,
                kDesc.m_kGradientSkyColor,
                kDesc.m_fGradientSkyBlend));
            spStage->SetHorizonColor(BlendColor(
                kDesc.m_kGradientHorizonColor,
                kDesc.m_kGradientSkyColor,
                kDesc.m_fGradientSkyBlend));
            spStage->SetGradientHorizonBias(kDesc.m_fGradientHorizonBias);
            spStage->SetGradientBiasExponent(kDesc.m_fGradientBiasExponent);
            return spStage;
        }

        inline NiPointer<NiSkyGradientBlendStage> AttachGradientBlendStage(
            NiSky* pkSky,
            const EnvironmentDesc& kDesc)
        {
            if (!pkSky || !kDesc.m_bCreateGradientBlendStage)
                return nullptr;

            NiPointer<NiSkyGradientBlendStage> spStage =
                CreateGradientBlendStage(kDesc);
            if (!spStage)
                return nullptr;

            pkSky->SetBlendStage(kDesc.m_uiGradientStage, spStage);
            return spStage;
        }

        inline NiPointer<NiTexture> LoadSkyboxTexture(
            const EnvironmentDesc& kDesc)
        {
            if (kDesc.m_pkSkyboxTexture)
                return kDesc.m_pkSkyboxTexture;

            NiRenderer* pkRenderer = NiRenderer::GetRenderer();
            if (!pkRenderer)
                return nullptr;

            if (HasSkyboxFacePaths(kDesc))
            {
                NiPixelDataPtr spPosX = LoadSkyboxFacePixelData(kDesc.m_strSkyboxPosXPath);
                NiPixelDataPtr spNegX = LoadSkyboxFacePixelData(kDesc.m_strSkyboxNegXPath);
                NiPixelDataPtr spPosY = LoadSkyboxFacePixelData(kDesc.m_strSkyboxPosYPath);
                NiPixelDataPtr spNegY = LoadSkyboxFacePixelData(kDesc.m_strSkyboxNegYPath);
                NiPixelDataPtr spPosZ = LoadSkyboxFacePixelData(kDesc.m_strSkyboxPosZPath);
                NiPixelDataPtr spNegZ = LoadSkyboxFacePixelData(kDesc.m_strSkyboxNegZPath);

                if (!spPosX || !spNegX || !spPosY || !spNegY || !spPosZ || !spNegZ)
                    return nullptr;

                return NiSourceCubeMap::Create(
                    spPosX,
                    spNegX,
                    spPosY,
                    spNegY,
                    spPosZ,
                    spNegZ,
                    pkRenderer);
            }

            if (kDesc.m_strSkyboxCubeMapPath.empty())
                return nullptr;

            return NiSourceCubeMap::Create(
                kDesc.m_strSkyboxCubeMapPath.c_str(),
                pkRenderer);
        }

        inline NiPointer<NiSkySkyboxBlendStage> CreateSkyboxBlendStage(
            const EnvironmentDesc& kDesc)
        {
            NiPointer<NiTexture> spTexture = LoadSkyboxTexture(kDesc);
            if (!spTexture)
                return nullptr;

            NiPointer<NiSkySkyboxBlendStage> spStage = NiNew NiSkySkyboxBlendStage();
            spStage->SetTexture(spTexture);
            spStage->SetBlendConstant(kDesc.m_fSkyboxBlendConstant);
            spStage->SetOriented(kDesc.m_bOrientSkybox);
            if (kDesc.m_bOrientSkybox)
                spStage->SetOrientation(kDesc.m_kSkyboxOrientation);
            return spStage;
        }

        inline NiPointer<NiSkySkyboxBlendStage> AttachSkyboxBlendStage(
            NiSky* pkSky,
            const EnvironmentDesc& kDesc)
        {
            if (!pkSky || !kDesc.m_bCreateSkyboxBlendStage)
                return nullptr;

            NiPointer<NiSkySkyboxBlendStage> spStage =
                CreateSkyboxBlendStage(kDesc);
            if (!spStage)
                return nullptr;

            pkSky->SetBlendStage(kDesc.m_uiSkyboxStage, spStage);
            return spStage;
        }

        inline NiPointer<NiRenderClick> CreateSkyRenderClick(
            NiEnvironment* pkEnvironment,
            NiCamera* pkCamera)
        {
            if (!pkEnvironment || !pkCamera)
                return nullptr;

            NiPointer<NiRenderClick> spClick =
                pkEnvironment->CreateSkyRenderClick(pkCamera);
            if (spClick)
                spClick->SetName("Sky Render Click");
            return spClick;
        }

        inline NiPointer<NiSky> CreateSky(
            NiEnvironment* pkEnvironment,
            const EnvironmentDesc& kDesc)
        {
            if (!pkEnvironment)
                return nullptr;

            if (kDesc.m_bCreateSkySphere)
            {
                FullSkySphere* pkSky = NiNew FullSkySphere();
                pkSky->SetAtmosphere(pkEnvironment->GetAtmosphere());
                if (pkEnvironment->GetName())
                    pkSky->SetName(pkEnvironment->GetName());
                pkEnvironment->SetSky(pkSky);
                return pkSky;
            }

            return pkEnvironment->CreateSkyDome();
        }

        inline NiPointer<NiSourceTexture> LoadCloudTexture(
            const CloudLayerDesc& kDesc)
        {
            if (kDesc.m_pkTexture)
                return kDesc.m_pkTexture;

            if (kDesc.m_strTexturePath.empty())
                return nullptr;

            return NiSourceTexture::Create(kDesc.m_strTexturePath.c_str());
        }

        inline void ConfigureCloudMaterial(
            NiMesh* pkMesh,
            const CloudLayerDesc& kDesc,
            NiScrollMaterial& kScrollMaterial)
        {
            if (!pkMesh)
                return;

            NiTexturingProperty* pkTP = NiDynamicCast(NiTexturingProperty,
                pkMesh->GetProperty(NiProperty::TEXTURING));
            if (pkTP)
            {
                NiTexturingProperty::Map* pkBaseMap = pkTP->GetBaseMap();
                if (pkBaseMap)
                {
                    pkBaseMap->SetClampMode(NiTexturingProperty::WRAP_S_WRAP_T);
                    pkBaseMap->SetFilterMode(NiTexturingProperty::FILTER_TRILERP);
                }
            }

            if (NiZBufferProperty* pkZP = NiDynamicCast(NiZBufferProperty,
                pkMesh->GetProperty(NiProperty::ZBUFFER)))
            {
                pkZP->SetZBufferTest(kDesc.m_bZBufferTest);
                pkZP->SetZBufferWrite(kDesc.m_bZBufferWrite);
            }

            if (NiMaterialAnimHelper::Enable(
                kScrollMaterial,
                pkMesh,
                kDesc.m_fScrollU,
                kDesc.m_fScrollV))
            {
                kScrollMaterial.pkTransform->SetScale(kDesc.m_kUVScale);
            }
        }

        inline NiPointer<NiMesh> CreateCloudQuadMesh(
            const CloudLayerDesc& kDesc,
            NiTexture* pkTexture)
        {
            const float fHalfWidth = kDesc.m_fWidth * 0.5f;
            const float fHalfHeight = kDesc.m_fHeight * 0.5f;

            NiPoint3 akPositions[4];
            if (kDesc.m_bBillboard)
            {
                akPositions[0] = NiPoint3(0.0f, -fHalfWidth, -fHalfHeight);
                akPositions[1] = NiPoint3(0.0f,  fHalfWidth, -fHalfHeight);
                akPositions[2] = NiPoint3(0.0f,  fHalfWidth,  fHalfHeight);
                akPositions[3] = NiPoint3(0.0f, -fHalfWidth,  fHalfHeight);
            }
            else
            {
                akPositions[0] = NiPoint3(-fHalfWidth, -fHalfHeight, 0.0f);
                akPositions[1] = NiPoint3( fHalfWidth, -fHalfHeight, 0.0f);
                akPositions[2] = NiPoint3( fHalfWidth,  fHalfHeight, 0.0f);
                akPositions[3] = NiPoint3(-fHalfWidth,  fHalfHeight, 0.0f);
            }

            const NiPoint3 akNormals[4] = {
                NiPoint3::UNIT_Z,
                NiPoint3::UNIT_Z,
                NiPoint3::UNIT_Z,
                NiPoint3::UNIT_Z,
            };
            const NiPoint2 akUVs[4] = {
                NiPoint2(0.0f, 0.0f),
                NiPoint2(1.0f, 0.0f),
                NiPoint2(1.0f, 1.0f),
                NiPoint2(0.0f, 1.0f),
            };
            const NiUInt32 auiIndices[6] = { 0, 1, 2, 0, 2, 3 };

            NiMeshDesc kMeshDesc;
            kMeshDesc.pkPositions = akPositions;
            kMeshDesc.uiVertexCount = 4;
            kMeshDesc.pkIndices = auiIndices;
            kMeshDesc.uiIndexCount = 6;
            kMeshDesc.pkNormals = akNormals;
            kMeshDesc.pkUVs = akUVs;
            kMeshDesc.pkBaseTexture = pkTexture;
            kMeshDesc.bAlphaBlend = kDesc.m_bAlphaBlend;
            kMeshDesc.bZBufferTest = kDesc.m_bZBufferTest;

            return NiMeshHelper::Create(kMeshDesc);
        }
    }

    inline BaseScene SetupBaseScene(
        NiNode* pkScene,
        const SceneDesc& kDesc = SceneDesc{})
    {
        BaseScene kResult;
        kResult.m_spScene = pkScene;
        kResult.m_spCamera = kDesc.m_pkCamera;

        if (!pkScene)
            return kResult;

        if (!kDesc.m_strSceneName.empty() && !pkScene->GetName())
            pkScene->SetName(kDesc.m_strSceneName.c_str());

        if (!kDesc.m_kEnvironment.m_bCreateEnvironment)
        {
            NiLightHelper::DirectionalDesc kLightDesc;
            kLightDesc.m_kAmbient  = kDesc.m_kDirectionalLight.m_kAmbient;
            kLightDesc.m_kDiffuse  = kDesc.m_kDirectionalLight.m_kDiffuse;
            kLightDesc.m_kSpecular = kDesc.m_kDirectionalLight.m_kSpecular;
            kLightDesc.m_fDimmer   = kDesc.m_kDirectionalLight.m_fDimmer;
            kLightDesc.m_bEnabled  = kDesc.m_kDirectionalLight.m_bEnabled;
            kLightDesc.m_fPitchDeg = kDesc.m_kDirectionalLight.m_fPitchDeg;
            kLightDesc.m_fYawDeg   = kDesc.m_kDirectionalLight.m_fYawDeg;

            kResult.m_spDirectionalLight =
                NiLightHelper::CreateDirectional(kLightDesc, pkScene);
            if (kResult.m_spDirectionalLight)
                kResult.m_spDirectionalLight->SetName("DirectionalLight");

            Detail::FinalizeScene(pkScene);
            return kResult;
        }

        kResult.m_spEnvironment = NiNew NiEnvironment();
        kResult.m_spEnvironment->SetName("Environment");
        kResult.m_spEnvironment->SetAutoCalcFogColor(
            kDesc.m_kEnvironment.m_bAutoCalcFogColor);
        kResult.m_spEnvironment->SetAutoSetBackgroundColor(
            kDesc.m_kEnvironment.m_bAutoSetBackgroundColor);

        if (kDesc.m_kEnvironment.m_bAttachEnvironmentToScene)
            pkScene->AttachChild(kResult.m_spEnvironment);

        if (kDesc.m_kEnvironment.m_bCreateAtmosphere ||
            kDesc.m_kEnvironment.m_bCreateSkyDome ||
            kDesc.m_kEnvironment.m_bCreateSkySphere)
        {
            kResult.m_spAtmosphere = kResult.m_spEnvironment->CreateAtmosphere();
        }

        if (kDesc.m_kEnvironment.m_bCreateSkyDome ||
            kDesc.m_kEnvironment.m_bCreateSkySphere ||
            kDesc.m_kEnvironment.m_bCreateGradientBlendStage ||
            kDesc.m_kEnvironment.m_bCreateSkyboxBlendStage)
        {
            kResult.m_spSky = Detail::CreateSky(
                kResult.m_spEnvironment,
                kDesc.m_kEnvironment);
        }

        if (kResult.m_spSky)
        {
            kResult.m_spGradientBlendStage = Detail::AttachGradientBlendStage(
                kResult.m_spSky,
                kDesc.m_kEnvironment);
            kResult.m_spSkyboxBlendStage = Detail::AttachSkyboxBlendStage(
                kResult.m_spSky,
                kDesc.m_kEnvironment);
        }

        if (kDesc.m_kEnvironment.m_bCreateSkyRenderClick)
        {
            kResult.m_spSkyRenderClick = Detail::CreateSkyRenderClick(
                kResult.m_spEnvironment,
                kResult.m_spCamera);
        }

        kResult.m_spDirectionalLight = kResult.m_spEnvironment->CreateSun();
        if (kResult.m_spDirectionalLight)
        {
            kResult.m_spDirectionalLight->SetName("Sun");
            kResult.m_spEnvironment->SetUseSunAngles(false);
            Detail::ApplyDirectionalLight(
                kResult.m_spDirectionalLight,
                kDesc.m_kDirectionalLight);
            kResult.m_spDirectionalLight->AttachAffectedNode(pkScene);
        }

        if (kDesc.m_kEnvironment.m_bCreateFogProperty)
        {
            kResult.m_spFogProperty = kResult.m_spEnvironment->CreateFogProperty();
            Detail::ApplyFogProperty(
                kResult.m_spFogProperty,
                kDesc.m_kEnvironment);
            pkScene->AttachProperty(kResult.m_spFogProperty);
        }

        Detail::FinalizeScene(pkScene);
        return kResult;
    }

    [[nodiscard]] inline BaseScene CreateBaseScene(
        const SceneDesc& kDesc = SceneDesc{})
    {
        BaseScene kResult;
        kResult.m_spScene = NiNew NiNode();

        if (!kDesc.m_strSceneName.empty())
            kResult.m_spScene->SetName(kDesc.m_strSceneName.c_str());

        return SetupBaseScene(kResult.m_spScene, kDesc);
    }

    inline NiRenderClick* CreateSkyRenderClick(
        BaseScene& kScene,
        NiCamera* pkCamera = nullptr)
    {
        if (pkCamera)
            kScene.m_spCamera = pkCamera;

        kScene.m_spSkyRenderClick = Detail::CreateSkyRenderClick(
            kScene.m_spEnvironment,
            kScene.m_spCamera);
        return kScene.m_spSkyRenderClick;
    }

    [[nodiscard]] inline CloudLayer CreateCloudLayer(
        const CloudLayerDesc& kDesc)
    {
        CloudLayer kLayer;

        NiPointer<NiSourceTexture> spTexture = Detail::LoadCloudTexture(kDesc);
        if (!spTexture)
            return kLayer;

        NiPointer<NiMesh> spMesh = Detail::CreateCloudQuadMesh(kDesc, spTexture);
        if (!spMesh)
            return kLayer;

        if (!kDesc.m_strName.empty())
            spMesh->SetName(kDesc.m_strName.c_str());

        Detail::ConfigureCloudMaterial(spMesh, kDesc, kLayer.m_kScrollMaterial);

        if (kDesc.m_bBillboard)
        {
            NiPointer<NiBillboardNode> spBillboard = NiNew NiBillboardNode();
            if (!kDesc.m_strName.empty())
                spBillboard->SetName(kDesc.m_strName.c_str());
            spBillboard->SetMode(kDesc.m_eBillboardMode);
            spBillboard->SetTranslate(kDesc.m_kCenter);
            spBillboard->AttachChild(spMesh);
            kLayer.m_spRoot = spBillboard;
        }
        else
        {
            spMesh->SetTranslate(kDesc.m_kCenter);
            kLayer.m_spRoot = spMesh;
        }

        kLayer.m_spMesh = spMesh;
        kLayer.m_spTexture = spTexture;

        if (kDesc.m_pkAttachParent && kLayer.m_spRoot)
        {
            kDesc.m_pkAttachParent->AttachChild(kLayer.m_spRoot);
            kDesc.m_pkAttachParent->Update(0.0f);
        }

        return kLayer;
    }

    [[nodiscard]] inline CloudLayer CreateCloudLayer(
        const NiPoint3& kCenter,
        float fWidth,
        float fHeight,
        const char* pcTexturePath,
        NiNode* pkAttachParent = nullptr)
    {
        CloudLayerDesc kDesc;
        kDesc.m_pkAttachParent = pkAttachParent;
        kDesc.m_kCenter = kCenter;
        kDesc.m_fWidth = fWidth;
        kDesc.m_fHeight = fHeight;
        if (pcTexturePath)
            kDesc.m_strTexturePath = pcTexturePath;
        return CreateCloudLayer(kDesc);
    }

    inline bool UpdateCloudLayer(CloudLayer& kLayer, float fDeltaTime)
    {
        if (!kLayer.m_spMesh)
            return false;

        NiMaterialAnimHelper::Tick(kLayer.m_kScrollMaterial, fDeltaTime);
        return true;
    }

    inline void ReleaseCloudLayer(CloudLayer& kLayer)
    {
        NiNode* pkParent = kLayer.m_spRoot ? kLayer.m_spRoot->GetParent() : nullptr;
        if (pkParent)
            pkParent->DetachChild(kLayer.m_spRoot);

        NiMaterialAnimHelper::Disable(kLayer.m_kScrollMaterial);
        kLayer.m_spTexture = nullptr;
        kLayer.m_spMesh = nullptr;
        kLayer.m_spRoot = nullptr;
    }

	// Note: The sky is not rendered automatically by the main scene pass.
	// Use this helper to render the sky/background before the normal scene render if you want it.
    inline bool DrawSky(BaseScene& kScene)
    {
        if (!kScene.m_spSkyRenderClick)
            return false;
        NiRenderer* pkRenderer = NiRenderer::GetRenderer();
        if (!pkRenderer)
            return false;
        kScene.m_spSkyRenderClick->Render(pkRenderer->GetFrameID());
        return true;
	}
    
	// Note: The sky is not rendered automatically by the main scene pass.
	// Use this helper to update the sky before the normal scene update if you want it.
    inline bool UpdateSky(
        BaseScene& kScene,
        float fUpdateTime = 0.0f)
    {
        if (kScene.m_spEnvironment)
            kScene.m_spEnvironment->Update(fUpdateTime);
        return kScene.m_spSkyRenderClick != nullptr;
	}

    inline void ReleaseBaseScene(BaseScene& kScene)
    {
        if (kScene.m_spSkyRenderClick)
            kScene.m_spSkyRenderClick->ReleaseCaches();

        kScene.m_spSkyRenderClick = nullptr;
        kScene.m_spSkyboxBlendStage = nullptr;
        kScene.m_spGradientBlendStage = nullptr;
        kScene.m_spFogProperty = nullptr;
        kScene.m_spDirectionalLight = nullptr;
        kScene.m_spSky = nullptr;
        kScene.m_spAtmosphere = nullptr;
        kScene.m_spEnvironment = nullptr;
        kScene.m_spCamera = nullptr;
        kScene.m_spScene = nullptr;
    }
}

#endif // NISCENEHELPER_H
