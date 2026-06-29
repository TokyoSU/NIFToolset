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

#ifndef NIRTTI_H
#define NIRTTI_H

#include "NiMainLibType.h"
#include <NiMemObject.h>

// run-time type information

enum class NiTypeMask : unsigned short
{
    NiMeshUpdateProcess,
    NiDataStream,
    NiDebugVisualizationClick,
    Ni2DBuffer,
    Ni3DRenderView,
    Ni2DRenderView,
    NiInstancingMeshModifier,
    NiMesh,
    NiMeshCullingProcess,
    NiMesh2DRenderView,
    NiMeshHWInstance,
    NiMeshModifier,
    NiAVObject,
    NiAVObjectPalette,
    NiAccumulator,
    NiCursor,
    ComputeRenderClick,
    NiAccumulatorProcessor,
    NiAlphaAccumulator,
    NiAlphaProperty,
    NiAdditionalGeometryData,
    NiMeshScreenElements,
    NiAmbientLight,
    NiAlphaSortProcessor,
    NiBSPNode,
    NiBackToFrontAccumulator,
    NiBackToFrontSortProcessor,
    NiBinaryExtraData,
    NiBillboardNode,
    NiBooleanExtraData,
    NiCamera,
    NiClickRenderStep,
    NiCollisionObject,
    NiColorExtraData,
    NiCulledObjectValidator,
    NiDefaultClickRenderStep,
    NiCullingProcess,
    NiDefaultShadowVisitor,
    NiDefaultAVObjectPalette,
    NiDefaultShadowClickGenerator,
    NiCompositeValidator,
    NiDepthStencilBuffer,
    NiDevImageConverter,
    NiDirectionalLight,
    NiDirectionalShadowWriteMaterial,
    NiDitherProperty,
    NiDynamicEffect,
    NiDynamicTexture,
    NiFloatExtraData,
    NiFogProperty,
    NiFragment,
    NiExtraData,
    NiFloatsExtraData,
    NiFragmentMaterial,
    NiGPUProgram,
    NiFragmentShaderInstanceDescriptor,
    NiGPUProgramCache,
    NiGeometry,
    NiGeometryData,
    NiIntegerExtraData,
    NiLODData,
    NiImageConverter,
    NiLODNode,
    NiIntegersExtraData,
    NiLines,
    NiLightManager,
    NiLight,
    NiLogicalANDCompositeValidator,
    NiLinesData,
    NiMaterial,
    NiMaterialFragmentNode,
    NiMaterialNode,
    NiMaterialProperty,
    NiMaterialResourceConsumerNode,
    NiMaterialResourceProducerNode,
    NiMaterialSwapProcessor,
    NiObject,
    NiNode,
    NiNoiseTexture,
    NiObjectNET,
    NiPalette,
    NiScreenGeometryData,
    NiScreenGeometry,
    NiScreenPolygon,
    NiScreenSpaceCamera,
    NiPSSMShadowClickGenerator,
    NiParallelUpdateTaskManager,
    NiParallelUpdateTaskManager_SignalTask,
    NiParticles,
    NiParticlesData,
    NiPersistentSrcTextureRendererData,
    NiPixelData,
    NiPointLight,
    NiPointShadowWriteMaterial,
    NiProperty,
    NiRangeLODData,
    NiRenderClick,
    NiRenderClickValidator,
    NiRenderStep,
    NiRenderListProcessor,
    NiRenderObject,
    NiRenderedCubeMap,
    NiRenderView,
    NiRenderTargetGroup,
    NiRenderedTexture,
    NiRenderer,
    NiSCMExtraData,
    NiRendererSpecificProperty,
    NiScreenElements,
    NiScreenElementsData,
    NiScreenFillingRenderView,
    NiScreenTexture,
    NiScreenLODData,
    NiShader,
    NiShaderInstanceDescriptor,
    NiShaderDeclaration,
    NiShadeProperty,
    NiShadowCubeMap,
    NiShaderTimeController,
    NiShadowClickGenerator,
    NiShadowClickValidator,
    NiShadowGenerator,
    NiShadowMap,
    NiShaderSortProcessor,
    NiShadowMaterialSwapProcessor,
    NiShadowRenderClick,
    NiShadowTechnique,
    NiShadowSortProcessor,
    NiSingleShaderMaterial,
    NiShadowVisitor,
    NiSkinPartition,
    NiSkinInstance,
    NiSkinData,
    NiSortAdjustNode,
    NiSpecularProperty,
    NiSourceCubeMap,
    NiSpotLight,
    NiSourceTexture,
    NiSpotShadowWriteMaterial,
    NiStandardMaterial,
    NiStencilProperty,
    NiStringExtraData,
    NiSwitchNode,
    NiStringsExtraData,
    NiSwitchStringExtraData,
    NiTextureEffect,
    NiTaskManager,
    NiTask,
    NiTexture,
    NiTimeSyncController,
    NiTriBasedGeom,
    NiTimeController,
    NiTriBasedGeomData,
    NiTextureValidator,
    NiTexturingProperty,
    NiTriShape,
    NiTriStripsData,
    NiTriStrips,
    NiTriShapeData,
    NiTriShapeDynamicData,
    NiUpdateProcess,
    NiVSMBlurMaterial,
    NiVectorExtraData,
    NiVertexColorProperty,
    NiVertWeightsExtraData,
    NiVSMShadowTechnique,
    NiWireframeProperty,
    NiViewRenderClick,
    NiZBufferProperty,
    NiMorphMeshModifier,
    NiScreenFillingRenderViewImpl,
    NiSkinningMeshModifier,
    NiToolDataStream,
    NiAlphaController,
    NiBSplineBasisData,
    NiBSplineColorEvaluator,
    NiBSplineColorInterpolator,
    NiBSplineCompColorInterpolator,
    NiBSplineCompColorEvaluator,
    NiBSplineCompFloatEvaluator,
    NiBSplineCompFloatInterpolator,
    NiBSplineCompTransformEvaluator,
    NiBSplineData,
    NiBSplineCompPoint3Evaluator,
    NiBSplineEvaluator,
    NiBSplineCompTransformInterpolator,
    NiBSplineFloatEvaluator,
    NiBSplineCompPoint3Interpolator,
    NiBSplineFloatInterpolator,
    NiBSplineInterpolator,
    NiBSplinePoint3Evaluator,
    NiBSplinePoint3Interpolator,
    NiBSplineTransformEvaluator,
    NiBSplineTransformInterpolator,
    NiBlendBoolInterpolator,
    NiBlendColorInterpolator,
    NiBlendFloatInterpolator,
    NiBlendInterpolator,
    NiBlendPoint3Interpolator,
    NiBlendQuaternionInterpolator,
    NiBlendTransformInterpolator,
    NiBoneLODController,
    NiBoolEvaluator,
    NiBoolData,
    NiBoolInterpController,
    NiBoolInterpolator,
    NiBoolTimelineEvaluator,
    NiBoolTimelineInterpolator,
    NiColorData,
    NiColorEvaluator,
    NiColorExtraDataController,
    NiColorInterpController,
    NiColorInterpolator,
    NiConstPoint3Evaluator,
    NiConstColorEvaluator,
    NiConstFloatEvaluator,
    NiConstBoolEvaluator,
    NiConstQuaternionEvaluator,
    NiConstTransformEvaluator,
    NiControllerSequence,
    NiControllerManager,
    NiEvaluator,
    NiExtraDataController,
    NiFloatData,
    NiFloatEvaluator,
    NiFlipController,
    NiFloatInterpolator,
    NiFloatExtraDataController,
    NiFloatInterpController,
    NiFloatsExtraDataController,
    NiFloatsExtraDataPoint3Controller,
    NiGeomMorpherController,
    NiInterpController,
    NiInterpolator,
    NiKeyBasedEvaluator,
    NiKeyBasedInterpolator,
    NiLightColorController,
    NiLightDimmerController,
    NiLookAtEvaluator,
    NiLookAtInterpolator,
    NiMaterialColorController,
    NiMorphData,
    NiMorphWeightsController,
    NiMultiTargetPoseHandler,
    NiMultiTargetTransformController,
    NiPathInterpolator,
    NiPathEvaluator,
    NiPoint3Evaluator,
    NiPoint3InterpController,
    NiPoint3Interpolator,
    NiPosData,
    NiPoseBinding,
    NiPoseBuffer,
    NiPoseBlender,
    NiQuaternionEvaluator,
    NiQuaternionInterpController,
    NiQuaternionInterpolator,
    NiRotData,
    NiScratchPad,
    NiSingleInterpController,
    NiSkinningLODController,
    NiSequenceData,
    NiStringPalette,
    NiTextKeyExtraData,
    NiTextKeyMatch,
    NiTransformData,
    NiTransformController,
    NiTextureTransformController,
    NiTransformInterpolator,
    NiTransformEvaluator,
    NiVisController,
    NiD3DDefaultShader,
    NiD3DHLSLPixelShader,
    NiD3DHLSLVertexShader,
    NiD3DPixelShader,
    NiD3DShader,
    NiD3DVertexShader,
    NiD3DShaderProgram,
    NiD3DShaderInterface,
    NiDX9DataStream,
    NiDX92DBufferData,
    NiDX9TextureBufferData,
    NiDX9Direct3DBufferData,
    NiDX9DepthStencilBufferData,
    NiDX9ImplicitDepthStencilBufferData,
    NiDX9Direct3DDepthStencilBufferData,
    NiDX9AdditionalDepthStencilBufferData,
    NiDX9OnscreenBufferData,
    NiDX9ImplicitBufferData,
    NiDX9SwapChainBufferData,
    NiDX9SwapChainDepthStencilBufferData,
    NiDX9Direct3DTexture,
    NiDX9ErrorShader,
    NiDX9FragmentShader,
    NiDX9PersistentSrcTextureRendererData,
    NiDX9Renderer,
    NiDX9ShadowWriteShader,
    NiCubeMapDepthStencilBuffer,
    NiD3D102DBufferData,
    NiD3D10RenderTargetBufferData,
    NiD3D10SwapChainBufferData,
    NiD3D10DepthStencilBufferData,
    NiD3D10Direct3DTexture,
    NiD3D10ErrorShader,
    NiD3D10FragmentShader,
    NiD3D10GeometryShader,
    NiD3D10PersistentSrcTextureRendererData,
    NiD3D10PixelShader,
    NiD3D10Renderer,
    NiD3D10Shader,
    NiD3D10ShaderInterface,
    NiD3D10ShaderProgram,
    NiD3D10ShadowWriteShader,
    NiD3D10VertexShader,
    D3D11RenderTargetBufferData,
    D3D112DBufferData,
    D3D11SwapChainBufferData,
    D3D11DepthStencilBufferData,
    D3D11ComputeShader,
    D3D11DataStream,
    D3D11DomainShader,
    D3D11Direct3DResource,
    D3D11ErrorShader,
    D3D11GeometryShader,
    D3D11HullShader,
    D3D11PersistentSrcTextureRendererData,
    D3D11PixelShader,
    D3D11FragmentShader,
    D3D11ShaderInterface,
    D3D11Renderer,
    D3D11ShaderCore,
    D3D11ShaderProgram,
    D3D11VertexShader,
    D3D11ShadowWriteShader,
    NiCollisionData,
    NiExternalAssetParams,
    NiParamsKFM,
    NiParamsKF,
    NiParamsNIF,
    NiSceneRenderView,
    NiCurve3,
    NiMeshPSysData,
    NiMeshParticleSystem,
    NiPSAirFieldSpreadCtlr,
    NiPSAlignedQuadGenerator,
    NiPSAirFieldAirFrictionCtlr,
    NiPSAirFieldInheritedVelocityCtlr,
    NiPSAirFieldForce,
    NiPSBoundUpdater,
    NiPSBoxEmitter,
    NiPSBombForce,
    NiPSCollider,
    NiPSCurveEmitter,
    NiPSCylinderEmitter,
    NiPSDragFieldForce,
    NiPSDragForce,
    NiPSEmitParticlesCtlr,
    NiPSEmitterDeclinationVarCtlr,
    NiPSEmitterDeclinationCtlr,
    NiPSEmitterCtlr,
    NiPSEmitterFloatCtlr,
    NiPSEmitterLifeSpanCtlr,
    NiPSEmitterLifeSpanVarCtlr,
    NiPSEmitterPlanarAngleVarCtlr,
    NiPSEmitterRadiusCtlr,
    NiPSEmitterPlanarAngleCtlr,
    NiPSEmitter,
    NiPSEmitterRadiusVarCtlr,
    NiPSEmitterRotAngleVarCtlr,
    NiPSEmitterRotAngleCtlr,
    NiPSEmitterRotSpeedVarCtlr,
    NiPSEmitterRotSpeedCtlr,
    NiPSEmitterSpeedCtlr,
    NiPSEmitterSpeedFlipRatioCtlr,
    NiPSEmitterSpeedVarCtlr,
    NiPSFacingQuadGenerator,
    NiPSFieldMagnitudeCtlr,
    NiPSFieldMaxDistanceCtlr,
    NiPSFieldAttenuationCtlr,
    NiPSForce,
    NiPSFieldForce,
    NiPSForceFloatCtlr,
    NiPSForceBoolCtlr,
    NiPSForceActiveCtlr,
    NiPSGravityFieldForce,
    NiPSGravityForce,
    NiPSGravityStrengthCtlr,
    NiPSForceCtlr,
    NiPSMeshEmitter,
    NiPSMeshParticleSystem,
    NiPSResetOnLoopCtlr,
    NiPSPlanarCollider,
    NiPSParticleSystem,
    NiPSSimulator,
    NiPSSimulatorCollidersStep,
    NiPSSimulatorFinalStep,
    NiPSRadialFieldForce,
    NiPSSimulatorForcesStep,
    NiPSSimulatorGeneralStep,
    NiPSSimulatorMeshAlignStep,
    NiPSSimulatorStep,
    NiPSSphereEmitter,
    NiPSSpawner,
    NiPSSphericalCollider,
    NiPSVolumeEmitter,
    NiPSTurbulenceFieldForce,
    NiPSTorusEmitter,
    NiPSVortexFieldForce,
    NiPSysAgeDeathModifier,
    NiPSysAirFieldInheritVelocityCtlr,
    NiPSysBombModifier,
    NiPSysAirFieldAirFrictionCtlr,
    NiPSysAirFieldModifier,
    NiPSysBoxEmitter,
    NiPSysAirFieldSpreadCtlr,
    NiPSysBoundUpdateModifier,
    NiPSysCollider,
    NiPSysColliderManager,
    NiPSysColorModifier,
    NiPSysCylinderEmitter,
    NiPSysData,
    NiPSysDragFieldModifier,
    NiPSysDragModifier,
    NiPSysEmitterCtlrData,
    NiPSysEmitterDeclinationCtlr,
    NiPSysEmitterLifeSpanCtlr,
    NiPSysEmitterInitialRadiusCtlr,
    NiPSysEmitterDeclinationVarCtlr,
    NiPSysEmitter,
    NiPSysEmitterPlanarAngleCtlr,
    NiPSysEmitterCtlr,
    NiPSysEmitterPlanarAngleVarCtlr,
    NiPSysEmitterSpeedCtlr,
    NiPSysFieldAttenuationCtlr,
    NiPSysFieldMagnitudeCtlr,
    NiPSysFieldMaxDistanceCtlr,
    NiPSysGravityStrengthCtlr,
    NiPSysGrowFadeModifier,
    NiPSysInitialRotAngleVarCtlr,
    NiPSysGravityModifier,
    NiPSysFieldModifier,
    NiPSysInitialRotSpeedCtlr,
    NiPSysInitialRotSpeedVarCtlr,
    NiPSysGravityFieldModifier,
    NiPSysInitialRotAngleCtlr,
    NiPSysMeshEmitter,
    NiPSysModifierBoolCtlr,
    NiPSysModifierCtlr,
    NiPSysModifierFloatCtlr,
    NiPSysModifierActiveCtlr,
    NiPSysModifier,
    NiPSysRadialFieldModifier,
    NiPSysPlanarCollider,
    NiPSysRotationModifier,
    NiPSysResetOnLoopCtlr,
    NiPSysMeshUpdateModifier,
    NiPSysPositionModifier,
    NiPSysSpawnModifier,
    NiPSysSphereEmitter,
    NiPSysSphericalCollider,
    NiPSysTurbulenceFieldModifier,
    NiPortal,
    NiPSysUpdateCtlr,
    NiPSysVortexFieldModifier,
    NiPSysVolumeEmitter,
    NiParticleSystem,
    NiRoom,
    NiOldWall,
    NiRoomGroup,
    NiFragmentOperations,
    NiFragmentLighting,
    NiFileInterface,
    NiTerrain,
    NiTerrainStoragePolicy,
    NiTerrainCellNode,
    NiTerrainCell,
    NiTerrainCellLeaf,
    NiTerrainCullingProcess,
    NiTerrainFileInterface,
    NiTerrainFileVersion0,
    NiTerrainFileVersion1,
    NiITerrainFileVersion4,
    NiTerrainFileVersion4,
    NiTerrainFileVersion2,
    NiTerrainMaterial,
    NiITerrainFileVersion3,
    NiTerrainFileVersion3,
    NiTerrainResourceManager,
    NiTerrainStandardResourceManager,
    NiTerrainSector,
    NiTerrainSectorFileVersion2,
    NiTerrainSectorFileVersion1,
    NiTerrainSectorFileVersion3,
    NiTerrainSectorFileVersion4,
    NiITerrainSectorFileVersion5,
    NiITerrainSectorFileVersion7,
    NiITerrainSectorFileVersion6,
    NiTerrainSectorSelector,
    NiTerrainSectorSelectorDefault,
    NiTerrainShadowClickValidator,
    NiTerrainShadowVisitor,
    NiTerrainSectorSelectorPager,
    NiTerrainStreamingTask,
    NiITerrainSurfacePackageFileVersion0,
    NiTerrainSurfacePackageFileVersion0,
    NiITerrainSurfacePackageFileVersion1,
    NiSkyFogBlendStage,
    NiSkyGradientBlendStage,
    NiSkySkyboxBlendStage,
    NiAtmosphere,
    NiCubeMapRenderStep,
    NiEnvironment,
    NiSky,
    NiSkyDome,
    NiSkyBlendStage,
    NiSkyMaterial,
    NiSkyRenderView,
    NiDecorationBillBoardGenerator,
    NiDecorationField,
    NiDecorationCrossBillBoardGenerator,
    NiDecorationGenerator,
    NiDecorationLayer,
    NiDecorationMaterial,
    NiDecorationMeshGenerator,
    NiDecorationPlane,
    NiDecorationSimpleMeshGenerator,
    NiLPPDecorationDepthNormalMaterial,
    NiLPPFinalMaterial,
    NiFragmentLightPrePass,
    NiLPPDecorationFinalMaterial,
    NiLPPDepthNormalMaterial,
    NiLPPLightMaterial,
    NiLPPMaterialSwapProcessorG,
    NiLPPMaterialSwapProcessorF,
    NiAudioListener,
    NiLPPTerrainFinalMaterial,
    NiLPPTerrainDepthNormalMaterial,
    NiLPPViewRenderClick,
    NiAudioSystem,
    NiAudioSource,
    NiBASSAudioListener,
    NiBASSAudioSource,
    NiBASSAudioSystem,
    NiCursorRenderClick,
    NiSystemCursor,
    Ni2DStringRenderClick,
    NiFont,
    NiFontString,
    NiDI8InputSystem,
    NiDI8InputSystem_DI8CreateParams,
    NiInputDI8GamePad,
    NiInputDI8Keyboard,
    NiInputDI8Mouse,
    NiInputDevice,
    NiInputGamePad,
    NiInputKeyboard,
    NiInputMouse,
    NiInputSystem,
    NiInputSystemCreateParams,
    NiInputXInputGamePad,
    RenderSurfaceStep,
    NiExtendedMaterial
};

class NIMAIN_ENTRY NiRTTI : public NiMemObject
{
public:
    NiRTTI(const char* pcName, const NiRTTI* pkBaseRTTI, NiTypeMask uiTypeMask);

    inline const char* GetName() const { return m_pcName; }
    inline const NiRTTI* GetBaseRTTI() const { return m_pkBaseRTTI; }
    inline NiTypeMask GetTypeMask() const { return m_uiTypeMask; }

    bool CopyName(char* acNameBuffer, unsigned int uiMaxSize) const;

    bool operator==(const NiRTTI& kOther) const {
		return this->m_uiTypeMask == kOther.m_uiTypeMask;
    }

protected:
    const char* m_pcName;
    const NiRTTI* m_pkBaseRTTI;
	const NiTypeMask m_uiTypeMask;
};

// insert in root class declaration
#define NiDeclareRootRTTI(classname) \
    public: \
        static const NiRTTI ms_RTTI; \
        virtual const NiRTTI* GetRTTI() const {return &ms_RTTI;} \
        static bool IsExactKindOf(const NiRTTI* pkRTTI, \
            const classname* pkObject) \
        { \
            if (!pkObject) \
            { \
                return false; \
            } \
            return pkObject->IsExactKindOf(pkRTTI); \
        } \
        bool IsExactKindOf(const NiRTTI* pkRTTI) const \
        { \
            return (GetRTTI() == pkRTTI); \
        } \
        static bool IsKindOf(const NiRTTI* pkRTTI, \
            const classname* pkObject) \
        { \
            if (!pkObject) \
                return false; \
            return pkObject->IsKindOf(pkRTTI); \
        } \
        bool IsKindOf(const NiRTTI* pkRTTI) const \
        { \
            const NiRTTI* pkTmp = GetRTTI(); \
            do\
            { \
                if (pkTmp == pkRTTI) \
                    return true; \
                pkTmp = pkTmp->GetBaseRTTI(); \
            } while (pkTmp); \
            return false; \
        } \
        bool IsKindOfFast(NiTypeMask mask) const \
        { \
            return (GetRTTI()->GetTypeMask() == mask); \
        } \
        static classname* VerifyStaticCastDebug(const NiRTTI* pkRTTI, \
            const classname* pkObject) \
        { \
            if (!pkObject) \
            { \
                return NULL; \
            } \
            classname* pkDynamicCast = DynamicCast(pkRTTI, pkObject); \
            EE_ASSERT("NiVerifyStaticCast() caught an invalid type cast." \
                "Check callstack for invalid object typecast assumption." \
                && pkDynamicCast); \
            return pkDynamicCast; \
        } \
        static classname* DynamicCast(const NiRTTI* pkRTTI, \
            const classname* pkObject) \
        { \
            if (!pkObject) \
            { \
                return NULL; \
            } \
            return pkObject->DynamicCast(pkRTTI); \
        } \
        classname* DynamicCast(const NiRTTI* pkRTTI) const \
        { \
            return (IsKindOf(pkRTTI) ? (classname*) this : 0); \
        }

// insert in class declaration
#define NiDeclareRTTI \
    public: \
        static const NiRTTI ms_RTTI; \
        virtual const NiRTTI* GetRTTI() const {return &ms_RTTI;}

// insert in root class source file
#define NiImplementRootRTTI(rootclassname, typemask) \
    const NiRTTI rootclassname::ms_RTTI(#rootclassname, 0, typemask)

// insert in class source file
#define NiImplementRTTI(classname, baseclassname, typemask) \
    const NiRTTI classname::ms_RTTI(#classname, &baseclassname::ms_RTTI, typemask)

// macros for run-time type testing
#define NiIsExactKindOf(classname, pkObject) \
    classname::IsExactKindOf(&classname::ms_RTTI, pkObject)

#define NiIsKindOf(classname, pkObject) \
    classname::IsKindOf(&classname::ms_RTTI, pkObject)

// macro for compile time type casting
#define NiStaticCast(classname, pkObject) \
    ((classname*) pkObject)

// macro for compile time type casting, with debug run-time assert
#ifdef NIDEBUG
    #define NiVerifyStaticCast(classname, pkObject) \
        ((classname*) classname::VerifyStaticCastDebug(\
            &classname::ms_RTTI, pkObject))
#else
    #define NiVerifyStaticCast(classname, pkObject) ((classname*) (pkObject))
#endif

// macro for run-time type casting, returns NULL if invalid cast
#define NiDynamicCast(classname, pkObject) \
    ((classname*) classname::DynamicCast(&classname::ms_RTTI, pkObject))

#endif
