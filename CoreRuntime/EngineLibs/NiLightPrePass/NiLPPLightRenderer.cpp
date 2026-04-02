// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2011 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

#include "NiLightPrePassPCH.h"

#include "NiLPPLightRenderer.h"
#include "NiLPPLightMaterial.h"

#include <NiShaderFactory.h>
#include <NiSingleShaderMaterial.h>
#include <NiDirectionalLight.h>
#include <NiPointLight.h>
#include <NiSpotLight.h>
#include <NiAlphaProperty.h>
#include <NiWireframeProperty.h>
#include <NiNode.h>

#include <efd/Profiler.h>
#include <efd/Foundation.h>

//-------------------------------------------------------------------------------------------------
const efd::Float32 NiLPPLightRenderer::FDEG_TO_RAD = efd::Float32(NI_PI / 180.0);
// number of slices
const efd::UInt32 NiLPPLightRenderer::SHAPE_SLICES = 8;
// margin to extend circular geometry bounding a light (relative to its radius)
const efd::Float32 NiLPPLightRenderer::SHAPE_MARGIN = efd::Float32(1.0 / ::cos(NI_PI / double(SHAPE_SLICES)));
//-------------------------------------------------------------------------------------------------
EE_PROFILER_CONTEXT_EXTERN(LightPrePass);
//-------------------------------------------------------------------------------------------------
static inline bool IntersectSphereSphere(
    efd::Point3 kCenterA, efd::Float32 fRadiusA,
    efd::Point3 kCenterB, efd::Float32 fRadiusB)
{
    efd::Float32 fRange = fRadiusA + fRadiusB;
    efd::Float32 fRange2 = fRange * fRange;
    efd::Point3 kDist = kCenterA - kCenterB;
    efd::Float32 fDist2 = kDist.Dot(kDist);
    return fDist2 <= fRange2;
}

//-------------------------------------------------------------------------------------------------
static inline bool IntersectSphereCone(
    efd::Point3 kCenterA, efd::Float32 fRadiusA,
    efd::Point3 kStartB, efd::Point3 kDirB, efd::Float32 fLengthB, efd::Float32 fSinB, efd::Float32 fCosB)
{
    kCenterA -= kStartB; // translate to cone segment start

    // project sphere center onto cone axis
    const efd::Float32 fP = kDirB.Dot(kCenterA);
    if (fP > (fLengthB + fRadiusA))
    {
        // sphere is past the capping plane at end of cone
        return false;
    }
    const efd::Point3 kP = kDirB * fP;


    // minimum distance for sphere to intersect cone
    const efd::Float32 fTangent = (fRadiusA + (fP * fSinB)) / fCosB;
    if (fTangent < 0.0f)
    {
        // sphere is too far past cone point
        return false;
    }
    const efd::Float32 fTangent2 = fTangent * fTangent;

    // check sphere's distance to projected point
    const efd::Point3 kDiff = kCenterA - kP;
    const efd::Float32 fDiff2 = kDiff.Dot(kDiff);
    if (fDiff2 > fTangent2)
    {
        // sphere is too far away to touch cone
        return false;
    }

    // NOTE: not rejecting case where fP > fLengthB but only intersects past the cap plane;
    //       but for this application false positives are okay, and the difference is minor;
    //       there is no need to do more calculation here.

    return true;
}

//-------------------------------------------------------------------------------------------------
static NiMesh* BuildSphere(
    efd::Point3 kCenter,
    efd::Float32 fRadius,
    efd::UInt32 uiNumSlices,
    efd::UInt32 uiNumStacks)
{
    efd::UInt32 uiNumVerts = 2 + (uiNumSlices) * (uiNumStacks-1);
    // Top and bottom stacks are triangles, intermediates are quads.
    efd::UInt32 uiNumIndices = (uiNumStacks-1) * uiNumSlices * 6;

    efd::Point3* akVerts = EE_STACK_ALLOC(efd::Point3, uiNumVerts);
    unsigned short int* auiIndices = EE_STACK_ALLOC(unsigned short int, uiNumIndices);

    // build sphere vertices
    {
        efd::Point3* pkPoints = akVerts;

        // Stacks along Z-axis.
        // Slices around Z-axis.

        *pkPoints++ = kCenter + efd::Point3(0, 0, fRadius);

        for (efd::UInt32 uiSt = 0; uiSt < uiNumStacks - 1; uiSt++)
        {
            efd::Float32 fAngle = (efd::Float32)(uiSt + 1) / (efd::Float32)(uiNumStacks) * NI_PI;

            efd::Float32 fZ;
            efd::Float32 fStackRadius;
            NiSinCos(fAngle, fStackRadius, fZ);
            fZ *= fRadius;
            fStackRadius *= fRadius;

            for (efd::UInt32 uiSl = 0; uiSl < uiNumSlices; uiSl++)
            {
                efd::Float32 fAng = NI_TWO_PI * (efd::Float32)uiSl / (efd::Float32)uiNumSlices;
                efd::Float32 fX;
                efd::Float32 fY;
                NiSinCos(fAng, fX, fY);

                pkPoints[uiSl] = efd::Point3(fX * fStackRadius, fY * fStackRadius,
                    fZ) + kCenter;
            }

            pkPoints += uiNumSlices;
        }

        *pkPoints++ = kCenter + efd::Point3(0, 0, -fRadius);
    }

    // build indices
    {
        unsigned short int* puiIndices = auiIndices;

        for (efd::UInt32 i = 0; i < uiNumSlices; i++)
        {
            efd::UInt32 uiOffset1 = i % uiNumSlices;
            efd::UInt32 uiOffset2 = (i+1) % uiNumSlices;

            const efd::UInt32 uiBottomOffset = 1;
            efd::UInt32 uiTopOffset = uiNumVerts - 1 - uiNumSlices;

            // Bottom
            puiIndices[0] = (unsigned short int)(0);
            puiIndices[1] = (unsigned short int)(uiBottomOffset + uiOffset2);
            puiIndices[2] = (unsigned short int)(uiBottomOffset + uiOffset1);

            // Top
            puiIndices[3] = (unsigned short int)(uiNumVerts - 1);
            puiIndices[4] = (unsigned short int)(uiTopOffset + uiOffset1);
            puiIndices[5] = (unsigned short int)(uiTopOffset + uiOffset2);

            puiIndices += 6;
        }

        // Intermediate stacks
        for (efd::UInt32 uiSl = 0; uiSl < uiNumSlices; uiSl++)
        {
            efd::UInt32 uiSlice1 = uiSl;
            efd::UInt32 uiSlice2 = (uiSl + 1) % uiNumSlices;

            for (efd::UInt32 uiSt = 0; uiSt < uiNumStacks - 2; uiSt++)
            {
                efd::UInt32 uiLowerStackOffset = 1 + uiSt * uiNumSlices;
                efd::UInt32 uiUpperStackOffset = uiLowerStackOffset + uiNumSlices;

                puiIndices[0] = (unsigned short int)(uiLowerStackOffset + uiSlice1);
                puiIndices[1] = (unsigned short int)(uiUpperStackOffset + uiSlice2);
                puiIndices[2] = (unsigned short int)(uiUpperStackOffset + uiSlice1);

                puiIndices[3] = (unsigned short int)(uiLowerStackOffset + uiSlice1);
                puiIndices[4] = (unsigned short int)(uiLowerStackOffset + uiSlice2);
                puiIndices[5] = (unsigned short int)(uiUpperStackOffset + uiSlice2);

                puiIndices += 6;
            }
        }
        EE_ASSERT(&auiIndices[uiNumIndices] == puiIndices);
    }

    // build mesh object
    NiDataStream* pkStreamIdx = NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_UINT16_1,
        uiNumIndices,
        NiDataStream::ACCESS_CPU_WRITE_STATIC | NiDataStream::ACCESS_GPU_READ,
        NiDataStream::USAGE_VERTEX_INDEX,
        auiIndices);
    EE_ASSERT(pkStreamIdx);

    NiDataStream* pkStreamPos = NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_FLOAT32_3,
        uiNumVerts,
        NiDataStream::ACCESS_CPU_WRITE_STATIC | NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_READ,
        NiDataStream::USAGE_VERTEX,
        akVerts);
    EE_ASSERT(pkStreamPos);

    NiMesh* pkMesh = EE_NEW NiMesh();
    pkMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);
    pkMesh->AddStreamRef(pkStreamIdx, NiCommonSemantics::INDEX(), 0);
    pkMesh->AddStreamRef(pkStreamPos, NiCommonSemantics::POSITION(), 0);
    pkMesh->SetSubmeshCount(1);

    EE_STACK_FREE(akVerts);
    EE_STACK_FREE(auiIndices);

    return pkMesh;
}

//-------------------------------------------------------------------------------------------------
static NiMesh* BuildFullscreen()
{
    const efd::Point3 kScreenQuad[4] = {
        efd::Point3(0,0.5f,-0.5f),
        efd::Point3(0,0.5f,0.5f),
        efd::Point3(0,-0.5f,0.5f),
        efd::Point3(0,-0.5f,-0.5f)
    };
    const unsigned short int kScreenIndex[6] = {
        0, 3, 1, 2, 1, 3
    };

    efd::UInt32 uiNumVerts = 4;
    efd::UInt32 uiNumIndices = 6;

    NiDataStream* pkStreamIdx = NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_UINT16_1,
        uiNumIndices,
        NiDataStream::ACCESS_CPU_WRITE_STATIC | NiDataStream::ACCESS_GPU_READ,
        NiDataStream::USAGE_VERTEX_INDEX,
        kScreenIndex);
    EE_ASSERT(pkStreamIdx);

    NiDataStream* pkStreamPos = NiDataStream::CreateSingleElementDataStream(
        NiDataStreamElement::F_FLOAT32_3,
        uiNumVerts,
        NiDataStream::ACCESS_CPU_WRITE_STATIC | NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_READ,
        NiDataStream::USAGE_VERTEX,
        kScreenQuad);
    EE_ASSERT(pkStreamPos);

    NiMesh* pkMesh = EE_NEW NiMesh();
    pkMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);
    pkMesh->AddStreamRef(pkStreamIdx, NiCommonSemantics::INDEX(), 0);
    pkMesh->AddStreamRef(pkStreamPos, NiCommonSemantics::POSITION(), 0);
    pkMesh->SetSubmeshCount(1);

    return pkMesh;
}

//-------------------------------------------------------------------------------------------------
NiLPPLightRenderer::NiLPPLightRenderer()
{
    // TODO move this into an initialization step
    //      so we can do something besides assert on failure during construction.

    m_spLightMaterial = NiLPPLightMaterial::Create();
    EE_ASSERT(m_spLightMaterial);

    m_spScreenMesh = BuildFullscreen();
	m_spSphereMesh = BuildSphere(efd::Point3(0.0f,0.0f,0.0f), 1.0f, SHAPE_SLICES, SHAPE_SLICES);
    m_spConeMesh = m_spSphereMesh; // TODO create a cone mesh

    m_spParentNode = NiNew NiNode();

    // create properties
    const efd::UInt32 MESH_COUNT = 2;
    NiMesh* pkMeshes[MESH_COUNT] =
    {
        m_spScreenMesh,
        m_spSphereMesh
        // TODO m_spConeMesh
    };

    for (efd::UInt32 ui = 0; ui < MESH_COUNT; ++ui)
    {
        NiMesh* pkMesh = pkMeshes[ui];
        EE_ASSERT(pkMesh);

        // create base map texture property
        NiTexturingProperty* pkNiTexturingProperty = EE_NEW NiTexturingProperty();
        EE_ASSERT(pkNiTexturingProperty);

        NiTexturingProperty::Map* pkMap = EE_NEW NiTexturingProperty::Map();
        EE_ASSERT(pkMap);
        pkMap->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
        pkMap->SetFilterMode(NiTexturingProperty::FILTER_NEAREST);
        pkNiTexturingProperty->SetBaseMap(pkMap);
        pkNiTexturingProperty->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);

        // create NiMaterial

        pkMesh->ApplyAndSetActiveMaterial(m_spLightMaterial);

        // create properties

        NiAlphaProperty* pkAlphaProperty = EE_NEW NiAlphaProperty();
        EE_ASSERT(pkAlphaProperty);
        pkAlphaProperty->SetAlphaBlending(true);
        pkAlphaProperty->SetAlphaTesting(false);
        pkAlphaProperty->SetSrcBlendMode(NiAlphaProperty::ALPHA_ONE);
        pkAlphaProperty->SetDestBlendMode(NiAlphaProperty::ALPHA_ONE);

        NiWireframeProperty* pkWireframeProperty = EE_NEW NiWireframeProperty();
        EE_ASSERT(pkWireframeProperty);
        pkWireframeProperty->SetWireframe(false);

        NiZBufferProperty* pkNiZBufferProperty = EE_NEW NiZBufferProperty();
        EE_ASSERT(pkNiZBufferProperty);
        pkNiZBufferProperty->SetTestFunction(NiZBufferProperty::TEST_GREATEREQUAL);
        pkNiZBufferProperty->SetZBufferTest(true);
        pkNiZBufferProperty->SetZBufferWrite(false);

        NiStencilProperty* pkNiStencilProperty = EE_NEW NiStencilProperty();
        EE_ASSERT(pkNiStencilProperty);
        pkNiStencilProperty->SetStencilOn(true);
        pkNiStencilProperty->SetStencilFunction(NiStencilProperty::TEST_NOTEQUAL);
        pkNiStencilProperty->SetStencilReference(0);
        pkNiStencilProperty->SetStencilMask(0xFF);
        pkNiStencilProperty->SetStencilPassAction(NiStencilProperty::ACTION_KEEP);
        pkNiStencilProperty->SetStencilPassZFailAction(NiStencilProperty::ACTION_KEEP);
        pkNiStencilProperty->SetStencilFailAction(NiStencilProperty::ACTION_KEEP);
        pkNiStencilProperty->SetDrawMode(NiStencilProperty::DRAW_CCW);

        pkMesh->RemoveProperty(pkAlphaProperty->GetType());
        pkMesh->AttachProperty(pkAlphaProperty);

        pkMesh->RemoveProperty(pkWireframeProperty->GetType());
        pkMesh->AttachProperty(pkWireframeProperty);

        pkMesh->RemoveProperty(pkNiZBufferProperty->GetType());
        pkMesh->AttachProperty(pkNiZBufferProperty);

        pkMesh->RemoveProperty(pkNiStencilProperty->GetType());
        pkMesh->AttachProperty(pkNiStencilProperty);

        pkMesh->AttachProperty(pkNiTexturingProperty);

        pkMesh->UpdateEffects();
        pkMesh->UpdateProperties();

        m_spParentNode->AttachChild(pkMesh);
    }

    InitializeShaderConstants();
}

//-------------------------------------------------------------------------------------------------
NiLPPLightRenderer::~NiLPPLightRenderer()
{
    ShutdownShaderConstants();
}

//-------------------------------------------------------------------------------------------------
void NiLPPLightRenderer::InitializeShaderConstants()
{
    // Store the fixed strings
    m_kSCPosScale = "g_fPosScale";
    m_kSCProjSwitch = "g_fProjectionPersOrtho";
    m_kSCCamR = "g_fCamR";
    m_kSCCamU = "g_fCamU";
    m_kSCCamD = "g_fCamD";
    m_kSCCamPos = "g_fCamPos";
    m_kSCLightPos = "g_fLightPos";
    m_kSCLightRangeQuad = "g_fLightRangeQuad";

    float tempArray[16] = {0};

    // Initialize the shader constants
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCPosScale, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2, 2 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCProjSwitch, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT2, 2 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCCamR, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3, 3 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCCamU, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3, 3 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCCamD, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3, 3 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCCamPos, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT3, 3 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCLightPos, 
        NiShaderAttributeDesc::ATTRIB_TYPE_POINT4, 4 * sizeof(float), &tempArray[0]);
    NiShaderFactory::RegisterGlobalShaderConstant(m_kSCLightRangeQuad, 
        NiShaderAttributeDesc::ATTRIB_TYPE_FLOAT, 1 * sizeof(float), &tempArray[0]);
}

//-------------------------------------------------------------------------------------------------
void NiLPPLightRenderer::ShutdownShaderConstants()
{
    // Release the shader constants
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCPosScale);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCProjSwitch);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCCamR);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCCamU);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCCamD);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCCamPos);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCLightPos);
    NiShaderFactory::ReleaseGlobalShaderConstant(m_kSCLightRangeQuad);

    // Store the fixed strings
    m_kSCPosScale = 0;
    m_kSCProjSwitch = 0;
    m_kSCCamR = 0;
    m_kSCCamU = 0;
    m_kSCCamD = 0;
    m_kSCCamPos = 0;
    m_kSCLightPos = 0;
    m_kSCLightRangeQuad = 0;
}

//-------------------------------------------------------------------------------------------------
void NiLPPLightRenderer::RenderLight(
    NiLight* pkLight,
    NiTexture* pkGBufferTexture,
    NiRenderer* pkRenderer)
{
    EE_UNUSED_ARG(pkRenderer);

    LightData kL(pkLight, GetMaxRange());

    // test the light for culling or fullscreen if camera is within the light
    TestResult eResult = TestLight(kL);
    if (eResult == TEST_CULL)
    {
        return;
    }
    bool bFullscreen = (eResult == TEST_FULLSCREEN);

    // TODO convert kL positions to camera relative? (need to move camera to origin as well)
    // this will avoid the need for doing the camera to world transformation in the shader
    // and maybe make culling easier?

    NiMesh* pkMesh = m_spScreenMesh;
    if (!bFullscreen)
    {
        if (kL.m_bSpot)
        {
            pkMesh = m_spConeMesh;
        }
        else
        {
            pkMesh = m_spSphereMesh;
        }
    }
    EE_ASSERT(pkMesh);

    {
        NiTexturingProperty* pkNiTexturingProperty = (NiTexturingProperty*)(pkMesh->GetProperty(NiProperty::TEXTURING));
        NiStencilProperty* pkNiStencilProperty = (NiStencilProperty*)(pkMesh->GetProperty(NiProperty::STENCIL));
        NiZBufferProperty* pkNiZBufferProperty = (NiZBufferProperty*)(pkMesh->GetProperty(NiProperty::ZBUFFER));
        EE_ASSERT(pkNiTexturingProperty);
        EE_ASSERT(pkNiStencilProperty);
        EE_ASSERT(pkNiZBufferProperty);

        // load the GBuffer as the base texture
        pkNiTexturingProperty->SetBaseTexture(pkGBufferTexture);

        if (eResult == TEST_BACK)
        {
            pkNiZBufferProperty->SetTestFunction(NiZBufferProperty::TEST_GREATEREQUAL);
            pkNiStencilProperty->SetDrawMode(NiStencilProperty::DRAW_CW);
        }
        else if (eResult == TEST_INTERSECT)
        {
            pkNiZBufferProperty->SetTestFunction(NiZBufferProperty::TEST_ALWAYS);
            pkNiStencilProperty->SetDrawMode(NiStencilProperty::DRAW_BOTH);
        }
        else
        {
            pkNiZBufferProperty->SetTestFunction(NiZBufferProperty::TEST_LESSEQUAL);
            pkNiStencilProperty->SetDrawMode(NiStencilProperty::DRAW_CCW);
        }

        // We're rendering a fullscreen quad. So turn off Z testing
        pkNiZBufferProperty->SetZBufferTest(!bFullscreen);

        // apply light group via stencil mask
        pkNiStencilProperty->SetStencilMask(pkLight->GetGroupMask());
    }

    {
        // add the light
        m_spParentNode->AttachEffect(pkLight);
        m_spParentNode->UpdateEffects();
        pkMesh->SetMaterialNeedsUpdate(true);
    }
    
    {
        // Pass in some extra light specific information
        NiPoint4 kLightPos(kL.m_position.x, kL.m_position.y, kL.m_position.z, 1.0f);
        efd::Float32 fLightRangeQuad = kL.m_range;
        NiShaderFactory::UpdateGlobalShaderConstant(m_kSCLightPos, sizeof(kLightPos), &kLightPos); 
        NiShaderFactory::UpdateGlobalShaderConstant(m_kSCLightRangeQuad, sizeof(fLightRangeQuad), &fLightRangeQuad); 
        // TODO eliminate use of g_fLightPos by getting that information from
        //      existing light constants.
    }

    {
        NiTransform kTransform;
        kTransform.MakeIdentity();

        if (!bFullscreen)
        {
            kTransform = kL.m_kTransform;
        }
        else
        {
            kTransform = m_kUnitQuadWSTransform;
        }

        pkMesh->SetLocalTransform(kTransform);
        pkMesh->UpdateWorldData();
    }
    
    {
        pkMesh->RenderImmediate(pkRenderer);
    }
    

    {
        m_spParentNode->DetachEffect(pkLight);
    }
    
}

//-------------------------------------------------------------------------------------------------
void NiLPPLightRenderer::GenerateFullscreenUnitQuadTransform(NiTransform& transform)
{
    const NiFrustum& frustum = m_pkCamera->GetViewFrustum();
    float quadDepth = frustum.m_fNear + 1.0f;
    transform.m_Translate = 
        m_pkCamera->GetWorldTranslate() + m_pkCamera->GetWorldDirection() * (quadDepth);
    transform.m_Rotate = m_pkCamera->GetWorldRotate();

    // Calculate the size of the quad to make sure it covers the entire screen
    efd::Float32 size = efd::Max(frustum.m_fRight - frustum.m_fLeft,
        frustum.m_fTop - frustum.m_fBottom);
    transform.m_fScale = size * quadDepth;
}

//-------------------------------------------------------------------------------------------------
void NiLPPLightRenderer::DebugLight(
    const NiLight* pkLight,
    NiRenderer* pkRenderer)
{
    EE_UNUSED_ARG(pkRenderer);

    LightData kL(pkLight, GetMaxRange());

    TestResult eResult = TestLight(kL);
    if (eResult != TEST_FRONT && eResult != TEST_BACK)
    {
        return;
    }

    NiMesh* pkMesh = m_spScreenMesh;
    if (kL.m_bSpot)
    {
        pkMesh = m_spConeMesh;
    }
    else
    {
        pkMesh = m_spSphereMesh;
    }
    EE_ASSERT(pkMesh);

    NiMaterial* pkNiMaterial = NiMaterial::GetMaterial("NiStandardMaterial");
    pkMesh->SwapMaterial(pkNiMaterial);

    NiTexturingProperty* pkNiTexturingProperty = (NiTexturingProperty*)(pkMesh->GetProperty(NiProperty::TEXTURING));
    NiStencilProperty* pkNiStencilProperty = (NiStencilProperty*)(pkMesh->GetProperty(NiProperty::STENCIL));
    NiZBufferProperty* pkNiZBufferProperty = (NiZBufferProperty*)(pkMesh->GetProperty(NiProperty::ZBUFFER));
    NiAlphaProperty* pkAlphaProperty = (NiAlphaProperty*)(pkMesh->GetProperty(NiProperty::ALPHA));
    NiWireframeProperty* pkWireframeProperty = (NiWireframeProperty*)(pkMesh->GetProperty(NiProperty::WIREFRAME));
    EE_ASSERT(pkNiTexturingProperty);
    EE_ASSERT(pkNiStencilProperty);
    EE_ASSERT(pkNiZBufferProperty);
    EE_ASSERT(pkAlphaProperty);
    EE_ASSERT(pkWireframeProperty);

    pkNiTexturingProperty->SetBaseMap(NULL);
    pkNiZBufferProperty->SetTestFunction(NiZBufferProperty::TEST_LESSEQUAL);
    pkNiStencilProperty->SetDrawMode(NiStencilProperty::DRAW_CCW);
    bool bAlphaBlending = pkAlphaProperty->GetAlphaBlending();
    pkAlphaProperty->SetAlphaBlending(false);
    pkWireframeProperty->SetWireframe(true);

    pkRenderer->SetCameraData(m_pkCamera);

    pkMesh->SetLocalTransform(kL.m_kTransform);
    pkMesh->UpdateWorldData();

    pkMesh->RenderImmediate(pkRenderer);

    pkWireframeProperty->SetWireframe(false);
    pkAlphaProperty->SetAlphaBlending(bAlphaBlending);
    pkMesh->RestoreSwapMaterial();
}

//-------------------------------------------------------------------------------------------------
void NiLPPLightRenderer::SetCamera(NiCamera* pkCamera)
{
    m_pkCamera = pkCamera;

    // Cache a unit quad transform
    GenerateFullscreenUnitQuadTransform(m_kUnitQuadWSTransform);

    // Generate camera shader constants
    const NiFrustum& kFrustum = m_pkCamera->GetViewFrustum();
    const efd::Float32 fScaleX = kFrustum.m_fRight - kFrustum.m_fLeft;
    const efd::Float32 fScaleY = kFrustum.m_fTop - kFrustum.m_fBottom;
    // NOTE the use of fScaleX/Y assumes the frustum is symmetrical
    // about the origin; a skewed frustum would require additional
    // information passed to the shader.

    efd::Point2 kPosScale(fScaleX, fScaleY);
    efd::Point2 kProjSwitchPersOrtho(
        kFrustum.m_bOrtho ? 0.0f : 1.0f,
        kFrustum.m_bOrtho ? 1.0f : 0.0f);

    const efd::Point3 kCamR = m_pkCamera->GetWorldRightVector();
    const efd::Point3 kCamU = m_pkCamera->GetWorldUpVector();
    const efd::Point3 kCamD = m_pkCamera->GetWorldDirection();
    const efd::Point3 kCamPos = m_pkCamera->GetWorldLocation();

    // set additional shader constants
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCPosScale,   sizeof(kPosScale),   &kPosScale); 
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCProjSwitch, sizeof(kProjSwitchPersOrtho),
        &kProjSwitchPersOrtho); 
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCCamR,       sizeof(kCamR),       &kCamR);
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCCamU,       sizeof(kCamU),       &kCamU);
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCCamD,       sizeof(kCamD),       &kCamD);
    NiShaderFactory::UpdateGlobalShaderConstant(m_kSCCamPos,     sizeof(kCamPos),     &kCamPos);

    // Generate frustum planes
    m_kFrustumPlanes.Set(kFrustum, m_pkCamera->GetWorldTransform());
}

//-------------------------------------------------------------------------------------------------
efd::Float32 NiLPPLightRenderer::GetMaxRange() const
{
    if (!m_pkCamera)
    {
        return 1.0f;
    }

    const NiFrustum& kFrustum = m_pkCamera->GetViewFrustum();
    return 0.9f * (kFrustum.m_fFar - kFrustum.m_fNear);
}

//-------------------------------------------------------------------------------------------------
NiLPPLightRenderer::LightData::LightData(const NiLight *pkLight, efd::Float32 fMaxRange) :
    // defaults for values that are not guaranteed to be written
    m_position(0.0f, 0.0f, 0.0f),
    m_direction(0.0f, 0.0f, 1.0f),
    m_range(0.0f),
    m_falloff(1.0f),
    m_cone(1.0f, 1.0f, 1.0f),
    m_bDirectional(false),
    m_bSpot(false),
    m_bShadow(false),
    m_shapeRadius(1.0f),
    m_shapeSinAngle(0.0f),
    m_shapeCosAngle(1.0f),
    m_bValid(true)
{
    efd::Float32 fRange = pkLight->GetRange() <= 0.0f ? fMaxRange : pkLight->GetRange();
    if (fRange > fMaxRange)
    {
        fRange = fMaxRange;
    }

    // properties generic to lights
    m_shapeLength = fRange;
    m_range = pkLight->GetRange();
    m_falloff = pkLight->GetFalloff();
    m_kTransform.MakeIdentity();
    
	// light specific information

    NiDynamicEffect::EffectType eType = pkLight->GetEffectType();

    // convert shadow light types to their parent types
    switch(eType)
    {
        case NiDynamicEffect::SHADOWPOINT_LIGHT:
            eType = NiDynamicEffect::POINT_LIGHT;
            m_bShadow = true;
            break;
        case NiDynamicEffect::SHADOWDIR_LIGHT:
            eType = NiDynamicEffect::DIR_LIGHT;
            m_bShadow = true;
            break;
        case NiDynamicEffect::SHADOWSPOT_LIGHT:
            eType = NiDynamicEffect::SPOT_LIGHT;
            m_bShadow = true;
            break;
        default:
            break;
    }

    // fill in light-specific fields
    switch(eType)
    {
        case NiDynamicEffect::DIR_LIGHT:
        {
            NiDirectionalLight* pkDirLight = NiVerifyStaticCast(NiDirectionalLight, pkLight);

            m_direction = -(pkDirLight->GetWorldDirection());
            m_bDirectional = true;
        } break;
        case NiDynamicEffect::SPOT_LIGHT:
        {
            NiSpotLight* pkNiSpotLight = NiVerifyStaticCast(NiSpotLight, pkLight);

            m_direction = pkNiSpotLight->GetWorldDirection();

            m_cone = efd::Point3(
                pkNiSpotLight->GetInnerSpotAngleCos(),
                pkNiSpotLight->GetSpotAngleCos(),
                pkNiSpotLight->GetSpotExponent() );

            m_shapeRadius = ::tanf(pkNiSpotLight->GetSpotAngle() * FDEG_TO_RAD);
            m_shapeCosAngle = pkNiSpotLight->GetSpotAngleCos();
            m_shapeSinAngle = ::sinf(pkNiSpotLight->GetSpotAngle() * FDEG_TO_RAD);

            // setup rotation to aim cone
            efd::Point3 DefaultDir(1.0f, 0.0f, 0.0f);
            efd::Float32 fDot = DefaultDir.Dot(m_direction);
            if (fDot != 0.0f)
            {
                efd::Point3 kCross = DefaultDir.Cross(m_direction);
                if (kCross.Unitize() > 0.0f)
                {
                    efd::Float32 fAngle = -NiACos(fDot);
                    m_kTransform.m_Rotate.MakeRotation(fAngle, kCross);
                }
            }

            m_bSpot = true;
        }
        // fallthrough
        case NiDynamicEffect::POINT_LIGHT:
        {
            NiPointLight* pkNiPointLight = NiVerifyStaticCast(NiPointLight, pkLight);

            m_position = pkNiPointLight->GetWorldLocation();

            // NOTE m_shapeRadius = 1.0f before this line if POINT_LIGHT and not SPOT_LIGHT
			// Cone shape radius is ignored while sphere is being used to represent the spot light
            m_shapeRadius = m_shapeLength * SHAPE_MARGIN;
        } break;
        default:
            m_bValid = false; // unknown light type
            break;
    }

	m_kTransform.m_fScale = fRange > 0.0f ? m_shapeRadius : 1.0f;
	m_kTransform.m_Translate = m_position;
}

//-------------------------------------------------------------------------------------------------
NiLPPLightRenderer::TestResult NiLPPLightRenderer::TestLight(const LightData& L)
{
    if (!L.m_bValid)
    {
        // cull out unknown light types
        return TEST_CULL;
    }

    if (L.m_bDirectional || L.m_range <= 0.0f)
    {
        return TEST_FULLSCREEN;
    }

    // Remaining light uses geometry
    // Check that that geometry overlaps with the frustum.
    for (efd::UInt32 planeIndex = 0; planeIndex < NiFrustumPlanes::MAX_PLANES; ++planeIndex)
    {
        // Treat all planes as active.
        const NiPlane& plane = m_kFrustumPlanes.GetPlane(planeIndex);

        efd::Float32 result = plane.Distance(L.m_position);

        if (efd::Abs(result) > L.m_shapeRadius && 
            result < 0)
            return TEST_CULL;
    }
    
    // If the light shape intersects the area between the near-plane
    // and the camera, use backface rendering with a flipped depth test.

    // create near-plane-frustum bounding sphere
    const NiFrustum& kFrustum = m_pkCamera->GetViewFrustum();
    // NOTE: assumes symmetrical frustum (i.e. Right = -Left)
    const efd::Float32 fSpread = NiMax(
        NiAbs(kFrustum.m_fRight - kFrustum.m_fLeft),
        NiAbs(kFrustum.m_fTop - kFrustum.m_fBottom) );
    // TODO this sphere is much wider than it needs to be;
    // can move the centre forward and pull in the radius.
    const efd::Float32 fFrustumRadius = kFrustum.m_fNear * fSpread;
	
    const efd::Point3 kFrustumCenter = m_pkCamera->GetWorldLocation() + 
		(m_pkCamera->GetWorldDirection() * kFrustum.m_fNear);

    // TODO when the cone mesh is no longer a sphere, use this
#if 0
    if (L.m_bSpot) // cone
    {
        return IntersectSphereCone(
            kFrustumCenter,
            fFrustumRadius,
            L.m_position,
            L.m_direction,
            L.m_shapeLength,
            L.m_shapeSinAngle,
            L.m_shapeCosAngle) ?
                TEST_BACK :
                TEST_FRONT;
    }
    // else point light sphere
#endif

	// TODO Need to check if the camera intersects to return the INTERSECT result

	return IntersectSphereSphere(
		kFrustumCenter,
		fFrustumRadius,
		L.m_position,
		L.m_shapeRadius) ? TEST_BACK : TEST_FRONT;

    // TODO return TEST_CULL if the light does not intersect
}

//-------------------------------------------------------------------------------------------------


