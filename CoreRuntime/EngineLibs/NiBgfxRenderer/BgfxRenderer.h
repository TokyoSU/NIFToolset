#pragma once
#ifndef BGFXRENDERER_H
#define BGFXRENDERER_H

#include "NiBgfxRendererLibType.h"
#include "NiBgfxContext.h"
#include "NiBgfxMath.h"

#include <NiRenderer.h>
#include <NiDepthStencilBuffer.h>
#include <NiRenderTargetGroup.h>
#include <bgfx/bgfx.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

class BgfxDataStreamFactory;

class NIBGFXRENDERER_ENTRY BgfxRenderer final : public NiRenderer
{
public:
    static BgfxRenderer* Create(void* nativeWindowHandle,
        unsigned int width, unsigned int height, bool vsync,
        const char* shaderRoot = nullptr);

    ~BgfxRenderer() override;

    bool Resize(unsigned int width, unsigned int height, bool vsync);
    bool IsInitialized() const;
    bool GetVSync() const { return m_vsync; }

    const char* GetDriverInfo() const override;
    unsigned int GetFlags() const override;
    NiSystemDesc::RendererID GetRendererID() const override;

    void SetDepthClear(float depthClear) override;
    float GetDepthClear() const override;
    void SetBackgroundColor(const NiColor& color) override;
    void SetBackgroundColor(const NiColorA& color) override;
    void GetBackgroundColor(NiColorA& color) const override;
    void SetStencilClear(unsigned int stencilClear) override;
    unsigned int GetStencilClear() const override;

    bool ValidateRenderTargetGroup(NiRenderTargetGroup* target) override;
    bool IsDepthBufferCompatible(Ni2DBuffer* buffer,
        NiDepthStencilBuffer* depthBuffer) override;
    NiRenderTargetGroup* GetDefaultRenderTargetGroup() const override;
    const NiRenderTargetGroup* GetCurrentRenderTargetGroup() const override;
    NiDepthStencilBuffer* GetDefaultDepthStencilBuffer() const override;
    Ni2DBuffer* GetDefaultBackBuffer() const override;

    bool CreateWindowRenderTargetGroup(NiWindowRef window) override;
    bool RecreateWindowRenderTargetGroup(NiWindowRef window) override;
    void ReleaseWindowRenderTargetGroup(NiWindowRef window) override;
    NiRenderTargetGroup* GetWindowRenderTargetGroup(NiWindowRef window) const override;

    const NiPixelFormat* FindClosestPixelFormat(
        NiTexture::FormatPrefs& prefs) const override;
    const NiPixelFormat* FindClosestDepthStencilFormat(
        const NiPixelFormat* frontBufferFormat, unsigned int depthBPP,
        unsigned int stencilBPP) const override;
    unsigned int GetMaxBuffersPerRenderTargetGroup() const override;
    bool GetIndependentBufferBitDepths() const override;

    void UseLegacyPipelineAsDefaultMaterial() override;
    bool PrecacheShader(NiRenderObject* renderObject) override;
    bool PrecacheTexture(NiTexture* texture) override;
    bool SetMipmapSkipLevel(unsigned int skip) override;
    unsigned int GetMipmapSkipLevel() const override;
    void PurgeMaterial(NiMaterialProperty* material) override;
    void PurgeEffect(NiDynamicEffect* effect) override;
    bool PurgeTexture(NiTexture* texture) override;
    bool PurgeAllTextures(bool purgeLocked) override;

    NiPixelData* TakeScreenShot(const NiRect<unsigned int>* screenRect,
        const NiRenderTargetGroup* target = nullptr) override;
    bool FastCopy(const Ni2DBuffer* src, Ni2DBuffer* dst,
        const NiRect<unsigned int>* srcRect = nullptr,
        unsigned int destX = 0, unsigned int destY = 0) override;
    bool Copy(const Ni2DBuffer* src, Ni2DBuffer* dst,
        const NiRect<unsigned int>* srcRect,
        const NiRect<unsigned int>* destRect,
        Ni2DBuffer::CopyFilterPreference pref) override;

    bool GetLeftRightSwap() const override;
    bool SetLeftRightSwap(bool swap) override;
    float GetMaxFogValue() const override;
    void SetMaxFogValue(float fogValue) override;
    void SetMaxAnisotropy(unsigned short maxAnisotropy) override;

    bool CreateSourceTextureRendererData(NiSourceTexture* texture) override;
    bool CreateRenderedTextureRendererData(NiRenderedTexture* texture,
        Ni2DBuffer::MultiSamplePreference msaaPref =
            Ni2DBuffer::MULTISAMPLE_NONE) override;
    bool CreateSourceCubeMapRendererData(NiSourceCubeMap* cubeMap) override;
    bool CreateRenderedCubeMapRendererData(NiRenderedCubeMap* cubeMap) override;
    bool CreateDynamicTextureRendererData(NiDynamicTexture* texture) override;
    void CreatePaletteRendererData(NiPalette* palette) override;
    bool CreateDepthStencilRendererData(NiDepthStencilBuffer* buffer,
        const NiPixelFormat* format,
        Ni2DBuffer::MultiSamplePreference msaaPref =
            Ni2DBuffer::MULTISAMPLE_NONE) override;

    void* LockDynamicTexture(const NiTexture::RendererData* rendererData,
        int& pitch) override;
    bool UnLockDynamicTexture(const NiTexture::RendererData* rendererData)
        override;

    NiShader* GetFragmentShader(
        NiMaterialDescriptor* materialDescriptor) override;
    void SetDefaultProgramCache(NiFragmentMaterial* material,
        bool autoWriteToDisk, bool writeDebugFile, bool load,
        bool noNewProgramCreation, const char* workingDir) override;
    NiShader* GetShadowWriteShader(
        NiMaterialDescriptor* materialDescriptor) override;
    void SetRenderShadowCasterBackfaces(bool renderBackfaces) override;
    void SetRenderShadowTechnique(NiShadowTechnique* technique) override;

#if defined(EE_ASSERTS_ARE_ENABLED)
    void EnforceModifierPoliciy(NiVisibleArray* array) override;
#endif

protected:
    BgfxRenderer();

    bool Do_BeginFrame() override;
    bool Do_EndFrame() override;
    bool Do_DisplayFrame() override;
    void Do_ClearBuffer(const NiRect<float>* rect,
        unsigned int clearMode) override;
    void Do_SetCameraData(const NiPoint3& worldLoc,
        const NiPoint3& worldDir, const NiPoint3& worldUp,
        const NiPoint3& worldRight, const NiFrustum& frustum,
        const NiRect<float>& port) override;
    void Do_SetScreenSpaceCameraData(
        const NiRect<float>* port = nullptr) override;
    void Do_GetCameraData(NiPoint3& worldLoc, NiPoint3& worldDir,
        NiPoint3& worldUp, NiPoint3& worldRight, NiFrustum& frustum,
        NiRect<float>& port) override;
    bool Do_BeginUsingRenderTargetGroup(NiRenderTargetGroup* target,
        unsigned int clearMode) override;
    bool Do_EndUsingRenderTargetGroup() override;
    void Do_RenderMesh(NiMesh* mesh) override;

private:
    class TextureData;
    class BufferData;
    class TargetGroupData;
    class MeshCache;

    bool Initialize(void* nativeWindowHandle, unsigned int width,
        unsigned int height, bool vsync, const char* shaderRoot);
    void ShutdownBgfxResources();
    bool CreateDefaultTargets(unsigned int width, unsigned int height);
    void ResetDefaultTargetDimensions(unsigned int width, unsigned int height);

    bool LoadBasicProgram();
    bool LoadInstancedProgram();
    bool LoadSkinnedPrograms();
    bool LoadTerrainProgram();
    bool LoadExtendedPrograms();
    bool LoadDecorationPrograms();
    bool LoadSkyProgram();
    bool LoadLppPrograms();
    bool LoadShadowPrograms();
    bool LoadVsmBlurProgram();
    bool LoadCopyProgram();
    bgfx::ShaderHandle LoadShader(const char* name) const;
    std::string GetBackendShaderDirectory() const;

    TextureData* GetTextureData(const NiTexture* texture) const;
    BufferData* GetBufferData(const Ni2DBuffer* buffer) const;
    bool CreateTextureFromPixelData(NiTexture* texture,
        const NiPixelData* pixels, bool cubeMap);
    bool CreateTextureFromContainerFile(NiSourceTexture* texture);
    bool EnsureTexture(NiTexture* texture);
    bool AllocateView(const char* name = nullptr);
    bool AllocateAuxiliaryView(bgfx::ViewId& viewId,
        const char* name = nullptr);
    bool IsMeshGpuCacheable(const NiMesh* mesh) const;
    std::uint64_t BuildMeshCacheSignature(const NiMesh* mesh,
        unsigned int submesh) const;
    std::uint64_t BuildMeshDataRevision(const NiMesh* mesh,
        unsigned int usage) const;
    MeshCache* GetOrCreateMeshCache(NiMesh* mesh);
    void PurgeGpuMeshCache(bool forceAll = false);

    bool DrawScaledCopy(const Ni2DBuffer* src, Ni2DBuffer* dst,
        const NiRect<unsigned int>& srcRect,
        const NiRect<unsigned int>& dstRect,
        Ni2DBuffer::CopyFilterPreference pref);
    void BindMaterialAndTexture(NiMesh* mesh);
    bool BindTerrainMaterial(NiMesh* mesh);
    bool BindExtendedMaterial(NiMesh* mesh, const NiMaterial* material);
    bool BindDecorationMaterial(NiMesh* mesh);
    bool BindSkyMaterial(NiMesh* mesh);
    uint64_t BuildRenderState(bool shadowWrite = false) const;
    uint32_t BuildStencilState() const;
    void SetModelTransform(const NiTransform& transform) const;

    NiBgfxContext m_context;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    bool m_vsync = true;

    Ni2DBufferPtr m_defaultBackBuffer;
    NiDepthStencilBufferPtr m_defaultDepthBuffer;
    NiRenderTargetGroupPtr m_defaultTargetGroup;
    std::unordered_map<NiWindowRef, NiRenderTargetGroupPtr> m_windowTargets;

    NiColorA m_backgroundColor = NiColorA(0.0f, 0.0f, 0.0f, 1.0f);
    float m_depthClear = 1.0f;
    unsigned int m_stencilClear = 0;
    unsigned int m_mipmapSkip = 0;
    bool m_leftRightSwap = false;
    float m_maxFogValue = 1.0f;
    bool m_renderShadowBackfaces = false;
    NiShadowTechnique* m_shadowTechnique = nullptr;

    NiPoint3 m_worldLoc = NiPoint3::ZERO;
    NiPoint3 m_worldDir = NiPoint3::UNIT_Z;
    NiPoint3 m_worldUp = NiPoint3::UNIT_Y;
    NiPoint3 m_worldRight = NiPoint3::UNIT_X;
    NiFrustum m_frustum;
    NiRect<float> m_viewport = NiRect<float>(0.0f, 1.0f, 1.0f, 0.0f);

    bgfx::ViewId m_viewId = 0;
    std::uint16_t m_nextViewId = 0;
    static constexpr unsigned int MAX_STANDARD_MAPS = 11;
    static constexpr unsigned int MAX_STANDARD_LIGHTS = 8;
    static constexpr unsigned int MAX_PROJECTED_EFFECTS = 3;
    static constexpr unsigned int MAX_SKIN_BONES = 30;
    static constexpr unsigned int MAX_TERRAIN_LAYERS = 4;
    static constexpr unsigned int MAX_TERRAIN_SAMPLERS = 15;
    static constexpr unsigned int MAX_SKY_STAGES = 5;
    static constexpr unsigned int MAX_PSSM_SLICES = 16;
    static constexpr unsigned int PSSM_DISTANCE_VECS = (MAX_PSSM_SLICES + 3) / 4;

    bgfx::ProgramHandle m_basicProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_instancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_skinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_skinnedShadowProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_terrainProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_terrainCubeShadowProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_extendedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_extendedInstancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_extendedSkinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_decorationProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_decorationInstancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_decorationSkinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_skyProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppBasicGProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppBasicGInstancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppBasicGSkinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppBasicFProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppBasicFInstancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppBasicFSkinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppTerrainGProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppTerrainFProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppDecorationGProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppDecorationGInstancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppDecorationGSkinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppDecorationFProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppDecorationFInstancedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppDecorationFSkinnedProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_lppLightProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_shadowProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_instancedShadowProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_copyProgram = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_vsmBlurProgram = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_copyTextureUniform = BGFX_INVALID_HANDLE;
    std::array<bgfx::UniformHandle, MAX_STANDARD_MAPS> m_textureUniforms{};
    bgfx::UniformHandle m_materialAmbientUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_materialDiffuseUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_materialSpecularUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_materialEmissiveUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_alphaParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_textureParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_mapParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_mapTransform0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_mapTransform1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_bumpParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_cameraPositionUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_cameraDirectionUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sceneAmbientUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightCountUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightPositionTypeUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightDirectionRangeUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightDiffuseDimmerUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightAmbientFalloffUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightSpecularSpotUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightSpotParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightShadowParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightShadowExtraUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightShadowMatrix0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightShadowMatrix1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightShadowMatrix2Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lightShadowMatrix3Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_pssmParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_pssmSplitDistancesUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_pssmSplitRowsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_pssmViewportsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_pssmTransitionRowsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_pssmTransitionParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_fogColorUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_fogParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_envTexture2DUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_envTextureCubeUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_envParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_envTransform0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_envTransform1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_envTransform2Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_shadowWriteParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skinBonesUniform = BGFX_INVALID_HANDLE;
    std::array<bgfx::UniformHandle, MAX_TERRAIN_SAMPLERS> m_terrainTextureUniforms{};
    bgfx::UniformHandle m_terrainShadowTextureUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainShadowParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainLayerFeatures0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainLayerFeatures1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainLayerScaleUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainDistRampUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainParallaxStrengthUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainSpecPowerUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainSpecIntensityUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainDetailScaleUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainTexCoord0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainTexCoord1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainLowDetailUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainMorphUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainStitchingUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainEyeUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainDebugUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_terrainRenderParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_extendedTerrainInfoUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_extendedLayerDataUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_extendedAlphaInfoUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_decorationFadeTextureUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_decorationFadeUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_decorationParamsUniform = BGFX_INVALID_HANDLE;
    std::array<bgfx::UniformHandle, MAX_SKY_STAGES> m_skyTextureUniforms{};
    bgfx::UniformHandle m_skyStageConfigUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyStageModifierUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyGradientParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyGradientHorizonUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyGradientZenithUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyOrientation0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyOrientation1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyOrientation2Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyAtmosScatteringUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyRgbInvWavelengthUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyScaleDepthUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyPlanetDimensionsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyFrameDataUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skyUpModeUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_skySunSamplesUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_vsmBlurParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lppParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lppDepthScaleUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lppPosScaleUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lppProjectionSwitchUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lppCameraRightUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_lppCameraUpUniform = BGFX_INVALID_HANDLE;
    std::array<bgfx::UniformHandle, MAX_PROJECTED_EFFECTS> m_projectedTextureUniforms{};
    bgfx::UniformHandle m_projectedParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_projectedTransform0Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_projectedTransform1Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_projectedTransform2Uniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_projectedClipPlaneUniform = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_whiteTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_whiteCubeTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_blackTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_flatNormalTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_currentPssmTexture = BGFX_INVALID_HANDLE;
    std::uint32_t m_currentPssmSamplerFlags = BGFX_SAMPLER_NONE;
    bool m_currentPssmActive = false;
    bgfx::TextureHandle m_currentTerrainShadowTexture = BGFX_INVALID_HANDLE;
    std::uint32_t m_currentTerrainShadowSamplerFlags = BGFX_SAMPLER_NONE;
    int m_currentTerrainShadowLightIndex = -1;
    bool m_currentTerrainShadowCube = false;
    BgfxDataStreamFactory* m_dataStreamFactory = nullptr;
    std::unordered_map<const NiMesh*, MeshCache*> m_meshCache;
    std::uint64_t m_frameSerial = 0;
    std::string m_shaderRoot;
};

#endif
