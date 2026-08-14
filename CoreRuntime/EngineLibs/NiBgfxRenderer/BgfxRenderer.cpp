#include "BgfxRenderer.h"
#include "BgfxDataStreamFactory.h"

#include <NiAlphaProperty.h>
#include <NiDepthStencilBuffer.h>
#include <NiDynamicTexture.h>
#include <NiImageConverter.h>
#include <NiMaterialProperty.h>
#include <NiPalette.h>
#include <NiMesh.h>
#include <NiPixelData.h>
#include <NiRenderedCubeMap.h>
#include <NiRenderedTexture.h>
#include <NiRenderTargetGroup.h>
#include <NiSourceCubeMap.h>
#include <NiSourceTexture.h>
#include <NiStencilProperty.h>
#include <NiTexturingProperty.h>
#include <NiTextureEffect.h>
#include <NiVertexColorProperty.h>
#include <NiWireframeProperty.h>
#include <NiZBufferProperty.h>
#include <NiDataStreamElementLock.h>
#include <NiCommonSemantics.h>
#include <NiFloat16.h>
#include <NiFloatExtraData.h>
#include <NiFloatsExtraData.h>
#include <NiColorExtraData.h>
#include <NiIntegerExtraData.h>
#include <NiIntegersExtraData.h>
#include <NiFragmentShaderInstanceDescriptor.h>
#include <NiMaterialDescriptor.h>
#include <NiAmbientLight.h>
#include <NiDirectionalLight.h>
#include <NiPointLight.h>
#include <NiSpotLight.h>
#include <NiFogProperty.h>
#include <NiSpecularProperty.h>
#include <NiShadeProperty.h>
#include <NiMaterial.h>
#include <NiMaterialInstance.h>
#include <NiShadowTechnique.h>
#include <NiVSMShadowTechnique.h>
#include <NiShadowGenerator.h>
#include <NiShadowMap.h>
#include <NiShadowCubeMap.h>
#include <NiShadowManager.h>
#include <NiPSSMShadowClickGenerator.h>
#include <NiPSSMConfiguration.h>
#include <NiSkinningMeshModifier.h>
#if defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
#include <NiPSParticleSystem.h>
#include <NiPSFacingQuadGenerator.h>
#endif
#include <NiExtendedMaterial.h>
#include <NiLog.h>

#include <bx/math.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <vector>

#if defined(EE_PLATFORM_WIN32)
#include <Windows.h>
#endif

#ifndef NIBGFX_DEFAULT_SHADER_ROOT
#define NIBGFX_DEFAULT_SHADER_ROOT "Shaders/bgfx"
#endif

namespace
{
    const char* GetTextureDebugName(const NiTexture* texture)
    {
        if (!texture)
            return "<null>";

        if (NiIsKindOf(NiSourceTexture, texture))
        {
            const NiSourceTexture* source = static_cast<const NiSourceTexture*>(texture);
            const char* filename = static_cast<const char*>(source->GetFilename());
            if (filename && *filename)
                return filename;
        }

        const char* name = static_cast<const char*>(texture->GetName());
        return name && *name ? name : "<unnamed>";
    }

    const char* GetPixelFormatName(const NiPixelFormat& format)
    {
        switch (format.GetFormat())
        {
        case NiPixelFormat::FORMAT_RGB:           return "RGB";
        case NiPixelFormat::FORMAT_RGBA:          return "RGBA";
        case NiPixelFormat::FORMAT_PAL:           return "PAL";
        case NiPixelFormat::FORMAT_PALALPHA:      return "PALALPHA";
        case NiPixelFormat::FORMAT_DXT1:          return "DXT1";
        case NiPixelFormat::FORMAT_DXT3:          return "DXT3";
        case NiPixelFormat::FORMAT_DXT5:          return "DXT5";
        case NiPixelFormat::FORMAT_BUMP:          return "BUMP";
        case NiPixelFormat::FORMAT_BUMPLUMA:      return "BUMPLUMA";
        case NiPixelFormat::FORMAT_ONE_CHANNEL:   return "ONE_CHANNEL";
        case NiPixelFormat::FORMAT_TWO_CHANNEL:   return "TWO_CHANNEL";
        case NiPixelFormat::FORMAT_THREE_CHANNEL: return "THREE_CHANNEL";
        case NiPixelFormat::FORMAT_FOUR_CHANNEL:  return "FOUR_CHANNEL";
        case NiPixelFormat::FORMAT_DEPTH_STENCIL: return "DEPTH_STENCIL";
        default:                                  return "UNKNOWN";
        }
    }

    struct StandardVertex
    {
        float x, y, z;
        float nx, ny, nz;
        float tx, ty, tz;
        float bx, by, bz;
        float uv[8][2];
        float skinIndices[4];
        float skinWeights[4];
        std::uint32_t color;
    };

    struct ParticleVertex
    {
        float x, y, z;
        float u, v;
    };

    const bgfx::VertexLayout& GetParticleVertexLayout()
    {
        static const bgfx::VertexLayout layout = []
        {
            bgfx::VertexLayout value;
            value.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .end();
            return value;
        }();
        return layout;
    }

    const bgfx::VertexLayout& GetParticleInstanceLayout()
    {
        // bgfx maps i_data0..i_data4 onto TEXCOORD7 downwards when a
        // vertex/dynamic vertex buffer is used as the instance source.
        static const bgfx::VertexLayout layout = []
        {
            bgfx::VertexLayout value;
            value.begin()
                .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
                .end();
            return value;
        }();
        return layout;
    }

    const bgfx::VertexLayout& GetStandardVertexLayout()
    {
        // The layout is identical for every Gamebryo draw. Building all 15
        // attributes again in Do_RenderMesh is measurable in Debug builds.
        static const bgfx::VertexLayout layout = []
        {
            bgfx::VertexLayout value;
            value.begin()
                .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Bitangent, 3, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord2, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord3, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord4, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord5, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord6, 2, bgfx::AttribType::Float)
                .add(bgfx::Attrib::TexCoord7, 2, bgfx::AttribType::Float)
                // Store palette-local bone indices as floats. This is portable
                // across the D3D/OpenGL/Vulkan bgfx backends and avoids backend-
                // specific integer vertex attribute behavior.
                .add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float)
                .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
                .end();
            return value;
        }();
        return layout;
    }

    struct CopyVertex
    {
        float x, y, z;
        float u, v;
    };

    bgfx::VertexLayout GetCopyVertexLayout()
    {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
        return layout;
    }

    std::uint32_t PackColor(float r, float g, float b, float a)
    {
        // This runs per vertex when a mutable stream really changes. Avoid
        // std::clamp/std::less here: in MSVC Debug builds those tiny helpers
        // showed up as a surprisingly large renderer hot path.
        const auto toByte = [](float value) -> std::uint32_t
        {
            if (value <= 0.0f)
                return 0u;
            if (value >= 1.0f)
                return 255u;
            return static_cast<std::uint32_t>(value * 255.0f + 0.5f);
        };
        return toByte(r) | (toByte(g) << 8) | (toByte(b) << 16) | (toByte(a) << 24);
    }

    std::uint32_t ToBgfxClearColor(const NiColorA& color)
    {
        const auto toByte = [](float value) -> std::uint32_t
        {
            value = std::clamp(value, 0.0f, 1.0f);
            return static_cast<std::uint32_t>(value * 255.0f + 0.5f);
        };
        // bgfx clear color is RGBA packed as 0xRRGGBBAA.
        return (toByte(color.r) << 24) | (toByte(color.g) << 16) |
            (toByte(color.b) << 8) | toByte(color.a);
    }

    uint64_t AlphaBlendFactor(NiAlphaProperty::AlphaFunction function)
    {
        switch (function)
        {
        case NiAlphaProperty::ALPHA_ONE:             return BGFX_STATE_BLEND_ONE;
        case NiAlphaProperty::ALPHA_ZERO:            return BGFX_STATE_BLEND_ZERO;
        case NiAlphaProperty::ALPHA_SRCCOLOR:        return BGFX_STATE_BLEND_SRC_COLOR;
        case NiAlphaProperty::ALPHA_INVSRCCOLOR:     return BGFX_STATE_BLEND_INV_SRC_COLOR;
        case NiAlphaProperty::ALPHA_DESTCOLOR:       return BGFX_STATE_BLEND_DST_COLOR;
        case NiAlphaProperty::ALPHA_INVDESTCOLOR:    return BGFX_STATE_BLEND_INV_DST_COLOR;
        case NiAlphaProperty::ALPHA_SRCALPHA:        return BGFX_STATE_BLEND_SRC_ALPHA;
        case NiAlphaProperty::ALPHA_INVSRCALPHA:     return BGFX_STATE_BLEND_INV_SRC_ALPHA;
        case NiAlphaProperty::ALPHA_DESTALPHA:       return BGFX_STATE_BLEND_DST_ALPHA;
        case NiAlphaProperty::ALPHA_INVDESTALPHA:    return BGFX_STATE_BLEND_INV_DST_ALPHA;
        case NiAlphaProperty::ALPHA_SRCALPHASAT:     return BGFX_STATE_BLEND_SRC_ALPHA_SAT;
        default:                                      return BGFX_STATE_BLEND_ONE;
        }
    }

    uint64_t DepthTestState(NiZBufferProperty::TestFunction function)
    {
        switch (function)
        {
        case NiZBufferProperty::TEST_ALWAYS:       return BGFX_STATE_DEPTH_TEST_ALWAYS;
        case NiZBufferProperty::TEST_LESS:         return BGFX_STATE_DEPTH_TEST_LESS;
        case NiZBufferProperty::TEST_EQUAL:        return BGFX_STATE_DEPTH_TEST_EQUAL;
        case NiZBufferProperty::TEST_LESSEQUAL:    return BGFX_STATE_DEPTH_TEST_LEQUAL;
        case NiZBufferProperty::TEST_GREATER:      return BGFX_STATE_DEPTH_TEST_GREATER;
        case NiZBufferProperty::TEST_NOTEQUAL:     return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
        case NiZBufferProperty::TEST_GREATEREQUAL: return BGFX_STATE_DEPTH_TEST_GEQUAL;
        case NiZBufferProperty::TEST_NEVER:        return BGFX_STATE_DEPTH_TEST_NEVER;
        default:                                    return BGFX_STATE_DEPTH_TEST_LESS;
        }
    }

    uint32_t SamplerFlags(NiTexturingProperty::ClampMode clampMode,
        NiTexturingProperty::FilterMode filterMode)
    {
        uint32_t flags = BGFX_SAMPLER_NONE;
        switch (clampMode)
        {
        case NiTexturingProperty::CLAMP_S_CLAMP_T:
            flags |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP; break;
        case NiTexturingProperty::CLAMP_S_WRAP_T:
            flags |= BGFX_SAMPLER_U_CLAMP; break;
        case NiTexturingProperty::WRAP_S_CLAMP_T:
            flags |= BGFX_SAMPLER_V_CLAMP; break;
        default:
            break;
        }

        switch (filterMode)
        {
        case NiTexturingProperty::FILTER_NEAREST:
        case NiTexturingProperty::FILTER_NEAREST_MIPNEAREST:
            flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT; break;
        case NiTexturingProperty::FILTER_NEAREST_MIPLERP:
            flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT; break;
        case NiTexturingProperty::FILTER_BILERP_MIPNEAREST:
            flags |= BGFX_SAMPLER_MIP_POINT; break;
        case NiTexturingProperty::FILTER_ANISOTROPIC:
            flags |= BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC; break;
        default:
            break;
        }
        return flags;
    }

    uint32_t SamplerFlags(const NiTexturingProperty::Map* map)
    {
        return map ? SamplerFlags(map->GetClampMode(), map->GetFilterMode()) :
            BGFX_SAMPLER_NONE;
    }

    uint32_t SamplerFlags(const NiTextureEffect* effect)
    {
        return effect ? SamplerFlags(effect->GetTextureClamp(),
            effect->GetTextureFilter()) : BGFX_SAMPLER_NONE;
    }

    bgfx::TextureFormat::Enum GetBgfxTextureFormat(const NiPixelFormat& format)
    {
        if (format == NiPixelFormat::RGBA32)
            return bgfx::TextureFormat::RGBA8;
        if (format == NiPixelFormat::BGRA8888)
            return bgfx::TextureFormat::BGRA8;
        if (format == NiPixelFormat::DXT1)
            return bgfx::TextureFormat::BC1;
        if (format == NiPixelFormat::DXT3)
            return bgfx::TextureFormat::BC2;
        if (format == NiPixelFormat::DXT5)
            return bgfx::TextureFormat::BC3;
        if (format == NiPixelFormat::A8)
            return bgfx::TextureFormat::A8;
        if (format == NiPixelFormat::R16)
            return bgfx::TextureFormat::R16F;
        if (format == NiPixelFormat::R32)
            return bgfx::TextureFormat::R32F;
        if (format == NiPixelFormat::RG32)
            return bgfx::TextureFormat::RG16F;
        if (format == NiPixelFormat::RG64)
            return bgfx::TextureFormat::RG32F;
        if (format == NiPixelFormat::RGBA64)
            return bgfx::TextureFormat::RGBA16F;
        if (format == NiPixelFormat::RGBA128)
            return bgfx::TextureFormat::RGBA32F;
        // I8/L8 are converted to RGBA32 below so the shader receives legacy
        // grayscale RGB semantics instead of an R-only texture.
        return bgfx::TextureFormat::Unknown;
    }

    const NiPixelFormat& GetNiPixelFormatForBgfx(bgfx::TextureFormat::Enum format)
    {
        switch (format)
        {
        case bgfx::TextureFormat::RGBA8:   return NiPixelFormat::RGBA32;
        case bgfx::TextureFormat::BGRA8:   return NiPixelFormat::BGRA8888;
        case bgfx::TextureFormat::BC1:     return NiPixelFormat::DXT1;
        case bgfx::TextureFormat::BC2:     return NiPixelFormat::DXT3;
        case bgfx::TextureFormat::BC3:     return NiPixelFormat::DXT5;
        case bgfx::TextureFormat::A8:      return NiPixelFormat::A8;
        case bgfx::TextureFormat::R8:      return NiPixelFormat::L8;
        case bgfx::TextureFormat::R16F:    return NiPixelFormat::R16;
        case bgfx::TextureFormat::R32F:    return NiPixelFormat::R32;
        case bgfx::TextureFormat::RG16F:   return NiPixelFormat::RG32;
        case bgfx::TextureFormat::RG32F:   return NiPixelFormat::RG64;
        case bgfx::TextureFormat::RGBA16F: return NiPixelFormat::RGBA64;
        case bgfx::TextureFormat::RGBA32F: return NiPixelFormat::RGBA128;

        // Gamebryo has no native BC4/BC5/BC6/BC7 descriptors. Keep those
        // resources renderer-specific; bgfx still knows their real GPU format.
        case bgfx::TextureFormat::BC4:
        case bgfx::TextureFormat::BC5:
        case bgfx::TextureFormat::BC6H:
        case bgfx::TextureFormat::BC7:
            return NiPixelFormat::RENDERERSPECIFICCOMPRESSED;
        default:
            return NiPixelFormat::RENDERERSPECIFIC32;
        }
    }

    bool IsCompressedSupportedFormat(const NiPixelFormat& format)
    {
        return format == NiPixelFormat::DXT1 || format == NiPixelFormat::DXT3 ||
            format == NiPixelFormat::DXT5;
    }

    uint64_t RenderTargetFlags(Ni2DBuffer::MultiSamplePreference msaaPref)
    {
        unsigned int count = 1;
        unsigned int quality = 0;
        Ni2DBuffer::GetMSAACountAndQualityFromPref(msaaPref, count, quality);
        EE_UNUSED_ARG(quality);
        if (count >= 16) return BGFX_TEXTURE_RT_MSAA_X16;
        if (count >= 8)  return BGFX_TEXTURE_RT_MSAA_X8;
        if (count >= 4)  return BGFX_TEXTURE_RT_MSAA_X4;
        if (count >= 2)  return BGFX_TEXTURE_RT_MSAA_X2;
        return BGFX_TEXTURE_RT;
    }

    template <typename T>
    T ClampCast(unsigned int value)
    {
        return static_cast<T>(std::min<unsigned int>(value,
            static_cast<unsigned int>(std::numeric_limits<T>::max())));
    }

    // NiFragmentMaterial still uses NiShader objects as material-cache entries.
    // bgfx does not use Gamebryo's D3D-era NiGPUProgram objects, so this small
    // shader exists only to preserve that cache/descriptor contract. Actual
    // GPU program selection and resource binding lives in BgfxRenderer.
    class BgfxMaterialShader final : public NiShader
    {
    public:
        BgfxMaterialShader(NiMaterialDescriptor* descriptor, const char* name)
        {
            EE_ASSERT(descriptor);
            m_descriptor.m_spMatDesc = descriptor;
            SetName(name);
            SetImplementation(DEFAULT_IMPLEMENTATION);
        }

        bool SetupGeometry(NiRenderObject*, NiMaterialInstance*) override
        {
            return true;
        }

        void Do_RenderMeshes(NiVisibleArray* visibleArray) override
        {
            if (!visibleArray)
                return;

            NiRenderer* renderer = NiRenderer::GetRenderer();
            if (!renderer)
                return;

            // NiShaderSortProcessor and NiShadowSortProcessor render cached
            // material buckets through NiShader::RenderMeshes rather than
            // calling NiRenderObject::RenderImmediate directly. The bgfx
            // shader object is only a material-cache descriptor, so replay
            // the bucket through the renderer here. RenderImmediate also
            // completes SYNC_RENDER modifiers and installs property/effect
            // state exactly like the unsorted path.
            const unsigned int count = visibleArray->GetCount();
            for (unsigned int i = 0; i < count; ++i)
            {
                NiAVObject* object = &visibleArray->GetAt(i);
                NiRenderObject* renderObject = NiDynamicCast(NiRenderObject, object);
                if (renderObject)
                    renderObject->RenderImmediate(renderer);
            }
        }

        const NiShaderInstanceDescriptor* GetShaderInstanceDesc() const override
        {
            return &m_descriptor;
        }

    private:
        NiFragmentShaderInstanceDescriptor m_descriptor;
    };
}

class BgfxRenderer::TextureData final : public NiTexture::RendererData
{
public:
    TextureData(NiTexture* texture, bgfx::TextureHandle handle,
        bgfx::TextureFormat::Enum format, const NiPixelFormat& pixelFormat,
        bool owned = true, unsigned int width = 0, unsigned int height = 0)
        : NiTexture::RendererData(texture), m_handle(handle),
          m_format(format), m_owned(owned)
    {
        m_uiWidth = width ? width : (texture ? texture->GetWidth() : 0);
        m_uiHeight = height ? height : (texture ? texture->GetHeight() : 0);
        m_kPixelFormat = pixelFormat;
    }

    ~TextureData() override
    {
        if (m_owned && bgfx::isValid(m_handle))
            bgfx::destroy(m_handle);
    }

    bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
    bgfx::TextureFormat::Enum m_format = bgfx::TextureFormat::Unknown;
    bool m_owned = true;
    bool m_dynamic = false;
    unsigned int m_pitch = 0;
    unsigned int m_sourceRevision = 0;
    unsigned int m_paletteRevision = 0;
    unsigned int m_mipmapSkip = 0;
    unsigned int m_layers = 1;
    unsigned int m_mipCount = 1;
    std::vector<std::uint8_t> m_staging;
};

class BgfxRenderer::BufferData final : public Ni2DBuffer::RendererData
{
public:
    BufferData(Ni2DBuffer* buffer, const NiPixelFormat* format,
        bgfx::TextureHandle handle,
        Ni2DBuffer::MultiSamplePreference msaaPref = Ni2DBuffer::MULTISAMPLE_NONE,
        bool owned = false, std::uint16_t layer = 0)
        : Ni2DBuffer::RendererData(buffer), m_handle(handle), m_owned(owned),
          m_layer(layer)
    {
        m_pkPixelFormat = format;
        m_eMSAAPref = msaaPref;
    }

    ~BufferData() override
    {
        if (m_owned && bgfx::isValid(m_handle))
            bgfx::destroy(m_handle);
    }

    bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
    bool m_owned = false;
    std::uint16_t m_layer = 0;
};

class BgfxRenderer::TargetGroupData final : public NiRenderTargetGroup::RendererData
{
public:
    explicit TargetGroupData(bgfx::FrameBufferHandle handle) : m_handle(handle) {}
    ~TargetGroupData() override
    {
        if (bgfx::isValid(m_handle))
            bgfx::destroy(m_handle);
    }

    bgfx::FrameBufferHandle m_handle = BGFX_INVALID_HANDLE;
};

class BgfxRenderer::MeshCache final : public NiMemObject
{
public:
    struct Submesh
    {
        std::uint64_t m_signature = 0;
        bgfx::VertexBufferHandle m_vertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle m_indexBuffer = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle m_wireIndexBuffer = BGFX_INVALID_HANDLE;
        bgfx::DynamicVertexBufferHandle m_dynamicVertexBuffer = BGFX_INVALID_HANDLE;
        bgfx::DynamicIndexBufferHandle m_dynamicIndexBuffer = BGFX_INVALID_HANDLE;
        bgfx::DynamicIndexBufferHandle m_dynamicWireIndexBuffer = BGFX_INVALID_HANDLE;
        std::uint32_t m_vertexCount = 0;
        std::uint32_t m_indexCount = 0;
        std::uint32_t m_wireIndexCount = 0;
        std::uint64_t m_vertexRevision = 0;
        std::uint64_t m_indexRevision = 0;
        std::uint64_t m_wireIndexRevision = 0;
        bool m_index32 = false;
        bool m_wireIndex32 = false;

        void Reset()
        {
            if (bgfx::isValid(m_vertexBuffer))
                bgfx::destroy(m_vertexBuffer);
            if (bgfx::isValid(m_indexBuffer))
                bgfx::destroy(m_indexBuffer);
            if (bgfx::isValid(m_wireIndexBuffer))
                bgfx::destroy(m_wireIndexBuffer);
            if (bgfx::isValid(m_dynamicVertexBuffer))
                bgfx::destroy(m_dynamicVertexBuffer);
            if (bgfx::isValid(m_dynamicIndexBuffer))
                bgfx::destroy(m_dynamicIndexBuffer);
            if (bgfx::isValid(m_dynamicWireIndexBuffer))
                bgfx::destroy(m_dynamicWireIndexBuffer);
            m_vertexBuffer = BGFX_INVALID_HANDLE;
            m_indexBuffer = BGFX_INVALID_HANDLE;
            m_wireIndexBuffer = BGFX_INVALID_HANDLE;
            m_dynamicVertexBuffer = BGFX_INVALID_HANDLE;
            m_dynamicIndexBuffer = BGFX_INVALID_HANDLE;
            m_dynamicWireIndexBuffer = BGFX_INVALID_HANDLE;
            m_signature = 0;
            m_vertexCount = 0;
            m_indexCount = 0;
            m_wireIndexCount = 0;
            m_vertexRevision = 0;
            m_indexRevision = 0;
            m_wireIndexRevision = 0;
            m_index32 = false;
            m_wireIndex32 = false;
        }
    };

    ~MeshCache()
    {
        for (Submesh& submesh : m_submeshes)
            submesh.Reset();
    }

    bool HasDynamicBuffers() const
    {
        for (const Submesh& submesh : m_submeshes)
        {
            if (bgfx::isValid(submesh.m_dynamicVertexBuffer) ||
                bgfx::isValid(submesh.m_dynamicIndexBuffer) ||
                bgfx::isValid(submesh.m_dynamicWireIndexBuffer))
            {
                return true;
            }
        }
        return false;
    }

    std::vector<Submesh> m_submeshes;
    std::uint64_t m_lastUsedFrame = 0;
    bool m_shortLived = false;
};

BgfxRenderer::BgfxRenderer()
{
    for (auto& handle : m_textureUniforms)
        handle = BGFX_INVALID_HANDLE;
    for (auto& handle : m_projectedTextureUniforms)
        handle = BGFX_INVALID_HANDLE;
    for (auto& handle : m_terrainTextureUniforms)
        handle = BGFX_INVALID_HANDLE;
}


BgfxRenderer::~BgfxRenderer()
{
    ShutdownBgfxResources();
    m_context.Shutdown();
    NiDelete m_dataStreamFactory;
    m_dataStreamFactory = nullptr;
}

BgfxRenderer* BgfxRenderer::Create(void* nativeWindowHandle,
    unsigned int width, unsigned int height, bool vsync,
    const char* shaderRoot)
{
    BgfxRenderer* renderer = NiNew BgfxRenderer();
    if (!renderer->Initialize(nativeWindowHandle, width, height, vsync, shaderRoot))
    {
        NiDelete renderer;
        return nullptr;
    }
    return renderer;
}

bool BgfxRenderer::Initialize(void* nativeWindowHandle, unsigned int width,
    unsigned int height, bool vsync, const char* shaderRoot)
{
    if (!nativeWindowHandle || width == 0 || height == 0)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Invalid renderer parameters: window=%p size=%ux%u.",
            nativeWindowHandle, width, height);
        return false;
    }

    NiLogWriteFormat(NI_LOG_INFO, "NiBgfxRenderer", __FILE__, __LINE__,
        "Creating renderer: size=%ux%u vsync=%s shaderRoot='%s'.",
        width, height, vsync ? "true" : "false",
        shaderRoot && *shaderRoot ? shaderRoot : "<auto>");

    m_width = width;
    m_height = height;
    m_vsync = vsync;
    m_shaderRoot = shaderRoot ? shaderRoot : "";

    if (!m_context.Initialize(nativeWindowHandle, width, height, vsync))
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "NiBgfxContext initialization failed; renderer creation aborted.",
            __FILE__, __LINE__);
        return false;
    }

    if (!m_dataStreamFactory)
        m_dataStreamFactory = NiNew BgfxDataStreamFactory();

    // Match the legacy renderers: NiRenderer constructs the initial standard
    // material, but each concrete renderer must make it the active default.
    // NiGeometryConverter relies on this for meshes that do not carry an
    // explicit material.
    m_spCurrentDefaultMaterial = m_spInitialDefaultMaterial;

    if (!CreateDefaultTargets(width, height))
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "Failed to create the default Gamebryo back/depth render targets.",
            __FILE__, __LINE__);
        return false;
    }

    const std::uint32_t white = 0xffffffffu;
    const std::uint32_t whiteCube[6] = {
        white, white, white, white, white, white
    };
    const std::uint32_t black = 0xff000000u;
    const std::uint32_t flatNormal = 0xffff8080u;
    m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE,
        bgfx::copy(&white, sizeof(white)));
    m_whiteCubeTexture = bgfx::createTextureCube(1, false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE,
        bgfx::copy(whiteCube, sizeof(whiteCube)));
    m_blackTexture = bgfx::createTexture2D(1, 1, false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE,
        bgfx::copy(&black, sizeof(black)));
    m_flatNormalTexture = bgfx::createTexture2D(1, 1, false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE,
        bgfx::copy(&flatNormal, sizeof(flatNormal)));

    if (!bgfx::isValid(m_whiteTexture) || !bgfx::isValid(m_whiteCubeTexture) ||
        !bgfx::isValid(m_blackTexture) ||
        !bgfx::isValid(m_flatNormalTexture))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to create fallback textures (white2D=%s whiteCube=%s black=%s flatNormal=%s).",
            bgfx::isValid(m_whiteTexture) ? "ok" : "invalid",
            bgfx::isValid(m_whiteCubeTexture) ? "ok" : "invalid",
            bgfx::isValid(m_blackTexture) ? "ok" : "invalid",
            bgfx::isValid(m_flatNormalTexture) ? "ok" : "invalid");
        return false;
    }

    static const char* samplerNames[MAX_STANDARD_MAPS] =
    {
        "s_baseTexture", "s_darkTexture", "s_detailTexture",
        "s_glossTexture", "s_glowTexture", "s_bumpTexture",
        "s_normalTexture", "s_parallaxTexture", "s_decalTexture0",
        "s_decalTexture1", "s_decalTexture2"
    };
    for (unsigned int i = 0; i < MAX_STANDARD_MAPS; ++i)
        m_textureUniforms[i] = bgfx::createUniform(samplerNames[i], bgfx::UniformType::Sampler);

    m_materialAmbientUniform = bgfx::createUniform("u_materialAmbient", bgfx::UniformType::Vec4);
    m_materialDiffuseUniform = bgfx::createUniform("u_materialDiffuse", bgfx::UniformType::Vec4);
    m_materialSpecularUniform = bgfx::createUniform("u_materialSpecular", bgfx::UniformType::Vec4);
    m_materialEmissiveUniform = bgfx::createUniform("u_materialEmissive", bgfx::UniformType::Vec4);
    m_alphaParamsUniform = bgfx::createUniform("u_alphaParams", bgfx::UniformType::Vec4);
    m_textureParamsUniform = bgfx::createUniform("u_textureParams", bgfx::UniformType::Vec4);
    m_mapParamsUniform = bgfx::createUniform("u_mapParams", bgfx::UniformType::Vec4, MAX_STANDARD_MAPS);
    m_mapTransform0Uniform = bgfx::createUniform("u_mapTransform0", bgfx::UniformType::Vec4, MAX_STANDARD_MAPS);
    m_mapTransform1Uniform = bgfx::createUniform("u_mapTransform1", bgfx::UniformType::Vec4, MAX_STANDARD_MAPS);
    m_bumpParamsUniform = bgfx::createUniform("u_bumpParams", bgfx::UniformType::Vec4);
    m_cameraPositionUniform = bgfx::createUniform("u_cameraPosition", bgfx::UniformType::Vec4);
    m_cameraDirectionUniform = bgfx::createUniform("u_cameraDirection", bgfx::UniformType::Vec4);
    m_particleCameraRightUniform = bgfx::createUniform("u_particleCameraRight", bgfx::UniformType::Vec4);
    m_particleCameraUpUniform = bgfx::createUniform("u_particleCameraUp", bgfx::UniformType::Vec4);
    m_softParticleDepthUniform = bgfx::createUniform("s_softParticleDepth", bgfx::UniformType::Sampler);
    m_softParticleParamsUniform = bgfx::createUniform("u_softParticleParams", bgfx::UniformType::Vec4);
    m_sceneAmbientUniform = bgfx::createUniform("u_sceneAmbient", bgfx::UniformType::Vec4);
    m_lightCountUniform = bgfx::createUniform("u_lightCount", bgfx::UniformType::Vec4);
    m_lightPositionTypeUniform = bgfx::createUniform("u_lightPositionType", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightDirectionRangeUniform = bgfx::createUniform("u_lightDirectionRange", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightDiffuseDimmerUniform = bgfx::createUniform("u_lightDiffuseDimmer", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightAmbientFalloffUniform = bgfx::createUniform("u_lightAmbientFalloff", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightSpecularSpotUniform = bgfx::createUniform("u_lightSpecularSpot", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightSpotParamsUniform = bgfx::createUniform("u_lightSpotParams", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightShadowParamsUniform = bgfx::createUniform("u_lightShadowParams", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightShadowExtraUniform = bgfx::createUniform("u_lightShadowExtra", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightShadowMatrix0Uniform = bgfx::createUniform("u_lightShadowMatrix0", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightShadowMatrix1Uniform = bgfx::createUniform("u_lightShadowMatrix1", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightShadowMatrix2Uniform = bgfx::createUniform("u_lightShadowMatrix2", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_lightShadowMatrix3Uniform = bgfx::createUniform("u_lightShadowMatrix3", bgfx::UniformType::Vec4, MAX_STANDARD_LIGHTS);
    m_pssmParamsUniform = bgfx::createUniform("u_pssmParams", bgfx::UniformType::Vec4);
    m_pssmSplitDistancesUniform = bgfx::createUniform("u_pssmSplitDistances", bgfx::UniformType::Vec4, PSSM_DISTANCE_VECS);
    m_pssmSplitRowsUniform = bgfx::createUniform("u_pssmSplitRows", bgfx::UniformType::Vec4, MAX_PSSM_SLICES * 4);
    m_pssmViewportsUniform = bgfx::createUniform("u_pssmViewports", bgfx::UniformType::Vec4, MAX_PSSM_SLICES);
    m_pssmTransitionRowsUniform = bgfx::createUniform("u_pssmTransitionRows", bgfx::UniformType::Vec4, 4);
    m_pssmTransitionParamsUniform = bgfx::createUniform("u_pssmTransitionParams", bgfx::UniformType::Vec4);
    m_fogColorUniform = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
    m_fogParamsUniform = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);
    m_envTexture2DUniform = bgfx::createUniform("s_envTexture2D", bgfx::UniformType::Sampler);
    m_envTextureCubeUniform = bgfx::createUniform("s_envTextureCube", bgfx::UniformType::Sampler);
    m_envParamsUniform = bgfx::createUniform("u_envParams", bgfx::UniformType::Vec4);
    m_envTransform0Uniform = bgfx::createUniform("u_envTransform0", bgfx::UniformType::Vec4);
    m_envTransform1Uniform = bgfx::createUniform("u_envTransform1", bgfx::UniformType::Vec4);
    m_envTransform2Uniform = bgfx::createUniform("u_envTransform2", bgfx::UniformType::Vec4);
    m_shadowWriteParamsUniform = bgfx::createUniform("u_shadowWriteParams", bgfx::UniformType::Vec4);
    m_skinBonesUniform = bgfx::createUniform("u_skinBones", bgfx::UniformType::Vec4, MAX_SKIN_BONES * 3);
    static const char* terrainSamplerNames[MAX_TERRAIN_SAMPLERS] =
    {
        "s_terrainLowDiffuse", "s_terrainLowNormal", "s_terrainBlend",
        "s_terrainBase0", "s_terrainNormal0", "s_terrainSpec0",
        "s_terrainBase1", "s_terrainNormal1", "s_terrainSpec1",
        "s_terrainBase2", "s_terrainNormal2", "s_terrainSpec2",
        "s_terrainBase3", "s_terrainNormal3", "s_terrainSpec3"
    };
    for (unsigned int i = 0; i < MAX_TERRAIN_SAMPLERS; ++i)
        m_terrainTextureUniforms[i] = bgfx::createUniform(terrainSamplerNames[i], bgfx::UniformType::Sampler);
    m_terrainShadowTextureUniform = bgfx::createUniform("s_terrainShadow", bgfx::UniformType::Sampler);
    m_terrainShadowParamsUniform = bgfx::createUniform("u_terrainShadowParams", bgfx::UniformType::Vec4);
    m_terrainLayerFeatures0Uniform = bgfx::createUniform("u_terrainLayerFeatures0", bgfx::UniformType::Vec4, MAX_TERRAIN_LAYERS);
    m_terrainLayerFeatures1Uniform = bgfx::createUniform("u_terrainLayerFeatures1", bgfx::UniformType::Vec4, MAX_TERRAIN_LAYERS);
    m_terrainLayerScaleUniform = bgfx::createUniform("u_terrainLayerScale", bgfx::UniformType::Vec4);
    m_terrainDistRampUniform = bgfx::createUniform("u_terrainDistRamp", bgfx::UniformType::Vec4);
    m_terrainParallaxStrengthUniform = bgfx::createUniform("u_terrainParallaxStrength", bgfx::UniformType::Vec4);
    m_terrainSpecPowerUniform = bgfx::createUniform("u_terrainSpecPower", bgfx::UniformType::Vec4);
    m_terrainSpecIntensityUniform = bgfx::createUniform("u_terrainSpecIntensity", bgfx::UniformType::Vec4);
    m_terrainDetailScaleUniform = bgfx::createUniform("u_terrainDetailScale", bgfx::UniformType::Vec4);
    m_terrainTexCoord0Uniform = bgfx::createUniform("u_terrainTexCoord0", bgfx::UniformType::Vec4);
    m_terrainTexCoord1Uniform = bgfx::createUniform("u_terrainTexCoord1", bgfx::UniformType::Vec4);
    m_terrainLowDetailUniform = bgfx::createUniform("u_terrainLowDetail", bgfx::UniformType::Vec4);
    m_terrainMorphUniform = bgfx::createUniform("u_terrainMorph", bgfx::UniformType::Vec4);
    m_terrainStitchingUniform = bgfx::createUniform("u_terrainStitching", bgfx::UniformType::Vec4);
    m_terrainEyeUniform = bgfx::createUniform("u_terrainEye", bgfx::UniformType::Vec4);
    m_terrainDebugUniform = bgfx::createUniform("u_terrainDebug", bgfx::UniformType::Vec4);
    m_terrainRenderParamsUniform = bgfx::createUniform("u_terrainRenderParams", bgfx::UniformType::Vec4);
    m_extendedTerrainInfoUniform = bgfx::createUniform("u_extendedTerrainInfo", bgfx::UniformType::Vec4);
    m_extendedLayerDataUniform = bgfx::createUniform("u_extendedLayerData", bgfx::UniformType::Vec4, NiExtendedMaterial::MAX_TERRAIN_LAYERS);
    m_extendedAlphaInfoUniform = bgfx::createUniform("u_extendedAlphaInfo", bgfx::UniformType::Vec4);
    m_decorationFadeTextureUniform = bgfx::createUniform("s_decorationFadeMask", bgfx::UniformType::Sampler);
    m_decorationFadeUniform = bgfx::createUniform("u_decorationFade", bgfx::UniformType::Vec4);
    m_decorationParamsUniform = bgfx::createUniform("u_decorationParams", bgfx::UniformType::Vec4);
    static const char* skySamplerNames[MAX_SKY_STAGES] =
    {
        "s_skyStage0", "s_skyStage1", "s_skyStage2",
        "s_skyStage3", "s_skyStage4"
    };
    for (unsigned int i = 0; i < MAX_SKY_STAGES; ++i)
        m_skyTextureUniforms[i] = bgfx::createUniform(skySamplerNames[i], bgfx::UniformType::Sampler);
    m_skyStageConfigUniform = bgfx::createUniform("u_skyStageConfig", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyStageModifierUniform = bgfx::createUniform("u_skyStageModifier", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyGradientParamsUniform = bgfx::createUniform("u_skyGradientParams", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyGradientHorizonUniform = bgfx::createUniform("u_skyGradientHorizon", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyGradientZenithUniform = bgfx::createUniform("u_skyGradientZenith", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyOrientation0Uniform = bgfx::createUniform("u_skyOrientation0", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyOrientation1Uniform = bgfx::createUniform("u_skyOrientation1", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyOrientation2Uniform = bgfx::createUniform("u_skyOrientation2", bgfx::UniformType::Vec4, MAX_SKY_STAGES);
    m_skyAtmosScatteringUniform = bgfx::createUniform("u_skyAtmosScattering", bgfx::UniformType::Vec4);
    m_skyRgbInvWavelengthUniform = bgfx::createUniform("u_skyRgbInvWavelength", bgfx::UniformType::Vec4);
    m_skyScaleDepthUniform = bgfx::createUniform("u_skyScaleDepth", bgfx::UniformType::Vec4);
    m_skyPlanetDimensionsUniform = bgfx::createUniform("u_skyPlanetDimensions", bgfx::UniformType::Vec4);
    m_skyFrameDataUniform = bgfx::createUniform("u_skyFrameData", bgfx::UniformType::Vec4);
    m_skyUpModeUniform = bgfx::createUniform("u_skyUpMode", bgfx::UniformType::Vec4);
    m_skySunSamplesUniform = bgfx::createUniform("u_skySunSamples", bgfx::UniformType::Vec4);
    m_vsmBlurParamsUniform = bgfx::createUniform("u_vsmBlurParams", bgfx::UniformType::Vec4);
    static const char* projectedSamplerNames[MAX_PROJECTED_EFFECTS] =
    {
        "s_projectedTexture0", "s_projectedTexture1", "s_projectedTexture2"
    };
    for (unsigned int i = 0; i < MAX_PROJECTED_EFFECTS; ++i)
        m_projectedTextureUniforms[i] = bgfx::createUniform(projectedSamplerNames[i], bgfx::UniformType::Sampler);
    m_projectedParamsUniform = bgfx::createUniform("u_projectedParams", bgfx::UniformType::Vec4, MAX_PROJECTED_EFFECTS);
    m_projectedTransform0Uniform = bgfx::createUniform("u_projectedTransform0", bgfx::UniformType::Vec4, MAX_PROJECTED_EFFECTS);
    m_projectedTransform1Uniform = bgfx::createUniform("u_projectedTransform1", bgfx::UniformType::Vec4, MAX_PROJECTED_EFFECTS);
    m_projectedTransform2Uniform = bgfx::createUniform("u_projectedTransform2", bgfx::UniformType::Vec4, MAX_PROJECTED_EFFECTS);
    m_projectedClipPlaneUniform = bgfx::createUniform("u_projectedClipPlane", bgfx::UniformType::Vec4, MAX_PROJECTED_EFFECTS);
    m_copyTextureUniform = bgfx::createUniform("s_copyTexture", bgfx::UniformType::Sampler);

    bool uniformsValid = true;
    for (const auto handle : m_textureUniforms)
        uniformsValid = uniformsValid && bgfx::isValid(handle);
    for (const auto handle : m_projectedTextureUniforms)
        uniformsValid = uniformsValid && bgfx::isValid(handle);
    for (const auto handle : m_terrainTextureUniforms)
        uniformsValid = uniformsValid && bgfx::isValid(handle);
    for (const auto handle : m_skyTextureUniforms)
        uniformsValid = uniformsValid && bgfx::isValid(handle);
    uniformsValid = uniformsValid &&
        bgfx::isValid(m_materialAmbientUniform) &&
        bgfx::isValid(m_materialDiffuseUniform) &&
        bgfx::isValid(m_materialSpecularUniform) &&
        bgfx::isValid(m_materialEmissiveUniform) &&
        bgfx::isValid(m_alphaParamsUniform) &&
        bgfx::isValid(m_textureParamsUniform) &&
        bgfx::isValid(m_mapParamsUniform) &&
        bgfx::isValid(m_mapTransform0Uniform) &&
        bgfx::isValid(m_mapTransform1Uniform) &&
        bgfx::isValid(m_bumpParamsUniform) &&
        bgfx::isValid(m_cameraPositionUniform) &&
        bgfx::isValid(m_cameraDirectionUniform) &&
        bgfx::isValid(m_particleCameraRightUniform) &&
        bgfx::isValid(m_particleCameraUpUniform) &&
        bgfx::isValid(m_softParticleDepthUniform) &&
        bgfx::isValid(m_softParticleParamsUniform) &&
        bgfx::isValid(m_sceneAmbientUniform) &&
        bgfx::isValid(m_lightCountUniform) &&
        bgfx::isValid(m_lightPositionTypeUniform) &&
        bgfx::isValid(m_lightDirectionRangeUniform) &&
        bgfx::isValid(m_lightDiffuseDimmerUniform) &&
        bgfx::isValid(m_lightAmbientFalloffUniform) &&
        bgfx::isValid(m_lightSpecularSpotUniform) &&
        bgfx::isValid(m_lightSpotParamsUniform) &&
        bgfx::isValid(m_lightShadowParamsUniform) &&
        bgfx::isValid(m_lightShadowExtraUniform) &&
        bgfx::isValid(m_lightShadowMatrix0Uniform) &&
        bgfx::isValid(m_lightShadowMatrix1Uniform) &&
        bgfx::isValid(m_lightShadowMatrix2Uniform) &&
        bgfx::isValid(m_lightShadowMatrix3Uniform) &&
        bgfx::isValid(m_pssmParamsUniform) &&
        bgfx::isValid(m_pssmSplitDistancesUniform) &&
        bgfx::isValid(m_pssmSplitRowsUniform) &&
        bgfx::isValid(m_pssmViewportsUniform) &&
        bgfx::isValid(m_pssmTransitionRowsUniform) &&
        bgfx::isValid(m_pssmTransitionParamsUniform) &&
        bgfx::isValid(m_fogColorUniform) &&
        bgfx::isValid(m_fogParamsUniform) &&
        bgfx::isValid(m_envTexture2DUniform) &&
        bgfx::isValid(m_envTextureCubeUniform) &&
        bgfx::isValid(m_envParamsUniform) &&
        bgfx::isValid(m_envTransform0Uniform) &&
        bgfx::isValid(m_envTransform1Uniform) &&
        bgfx::isValid(m_envTransform2Uniform) &&
        bgfx::isValid(m_shadowWriteParamsUniform) &&
        bgfx::isValid(m_skinBonesUniform) &&
        bgfx::isValid(m_terrainShadowTextureUniform) &&
        bgfx::isValid(m_terrainShadowParamsUniform) &&
        bgfx::isValid(m_terrainLayerFeatures0Uniform) &&
        bgfx::isValid(m_terrainLayerFeatures1Uniform) &&
        bgfx::isValid(m_terrainLayerScaleUniform) &&
        bgfx::isValid(m_terrainDistRampUniform) &&
        bgfx::isValid(m_terrainParallaxStrengthUniform) &&
        bgfx::isValid(m_terrainSpecPowerUniform) &&
        bgfx::isValid(m_terrainSpecIntensityUniform) &&
        bgfx::isValid(m_terrainDetailScaleUniform) &&
        bgfx::isValid(m_terrainTexCoord0Uniform) &&
        bgfx::isValid(m_terrainTexCoord1Uniform) &&
        bgfx::isValid(m_terrainLowDetailUniform) &&
        bgfx::isValid(m_terrainMorphUniform) &&
        bgfx::isValid(m_terrainStitchingUniform) &&
        bgfx::isValid(m_terrainEyeUniform) &&
        bgfx::isValid(m_terrainDebugUniform) &&
        bgfx::isValid(m_terrainRenderParamsUniform) &&
        bgfx::isValid(m_extendedTerrainInfoUniform) &&
        bgfx::isValid(m_extendedLayerDataUniform) &&
        bgfx::isValid(m_extendedAlphaInfoUniform) &&
        bgfx::isValid(m_decorationFadeTextureUniform) &&
        bgfx::isValid(m_decorationFadeUniform) &&
        bgfx::isValid(m_decorationParamsUniform) &&
        bgfx::isValid(m_skyStageConfigUniform) &&
        bgfx::isValid(m_skyStageModifierUniform) &&
        bgfx::isValid(m_skyGradientParamsUniform) &&
        bgfx::isValid(m_skyGradientHorizonUniform) &&
        bgfx::isValid(m_skyGradientZenithUniform) &&
        bgfx::isValid(m_skyOrientation0Uniform) &&
        bgfx::isValid(m_skyOrientation1Uniform) &&
        bgfx::isValid(m_skyOrientation2Uniform) &&
        bgfx::isValid(m_skyAtmosScatteringUniform) &&
        bgfx::isValid(m_skyRgbInvWavelengthUniform) &&
        bgfx::isValid(m_skyScaleDepthUniform) &&
        bgfx::isValid(m_skyPlanetDimensionsUniform) &&
        bgfx::isValid(m_skyFrameDataUniform) &&
        bgfx::isValid(m_skyUpModeUniform) &&
        bgfx::isValid(m_skySunSamplesUniform) &&
        bgfx::isValid(m_vsmBlurParamsUniform) &&
        bgfx::isValid(m_projectedParamsUniform) &&
        bgfx::isValid(m_projectedTransform0Uniform) &&
        bgfx::isValid(m_projectedTransform1Uniform) &&
        bgfx::isValid(m_projectedTransform2Uniform) &&
        bgfx::isValid(m_projectedClipPlaneUniform) &&
        bgfx::isValid(m_copyTextureUniform);
    if (!uniformsValid)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "One or more required bgfx uniforms/samplers could not be created. Check bgfx trace output for duplicate/type/resource errors.",
            __FILE__, __LINE__);
        return false;
    }

    if (!LoadBasicProgram())
    {
        Error("BgfxRenderer: failed to load basic shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadInstancedProgram())
    {
        Error("BgfxRenderer: failed to load instanced shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
#if defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
    if (!LoadParticlePrograms())
    {
        Error("BgfxRenderer: failed to load particle billboard shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!CreateParticleResources())
    {
        Error("BgfxRenderer: failed to create shared particle billboard geometry.");
        return false;
    }
    if (bgfx::isValid(m_softParticleProgram) &&
        !CreateSoftParticleTargets(width, height))
    {
        NiLogWrite(NI_LOG_WARNING, "NiBgfxRenderer",
            "Soft-particle render targets are unavailable; particles will use the normal hard-intersection path.",
            __FILE__, __LINE__);
    }
    NiLogWriteFormat(NI_LOG_INFO, "NiBgfxRenderer", __FILE__, __LINE__,
        "[ParticleInstancing] Debug enabled. Phase-1 facing-quad path is ready; "
        "bgfx instancing capability=%s. The first successful batch will emit "
        "an ACTIVE line, followed by 120-frame TRACE summaries.",
        (bgfx::getCaps()->supported & BGFX_CAPS_INSTANCING) != 0 ?
            "yes" : "no");
#endif
    if (!LoadSkinnedPrograms())
    {
        Error("BgfxRenderer: failed to load skinned shader programs from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadTerrainProgram())
    {
        Error("BgfxRenderer: failed to load terrain shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadExtendedPrograms())
    {
        Error("BgfxRenderer: failed to load NiExtendedMaterial shader programs from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadDecorationPrograms())
    {
        Error("BgfxRenderer: failed to load NiDecorationMaterial shader programs from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadSkyProgram())
    {
        Error("BgfxRenderer: failed to load NiSkyMaterial shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadShadowPrograms())
    {
        Error("BgfxRenderer: failed to load shadow shader programs from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadVsmBlurProgram())
    {
        Error("BgfxRenderer: failed to load VSM blur shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }
    if (!LoadCopyProgram())
    {
        Error("BgfxRenderer: failed to load copy shader program from '%s'.",
            GetBackendShaderDirectory().c_str());
        return false;
    }

    NiLogWriteFormat(NI_LOG_INFO, "NiBgfxRenderer", __FILE__, __LINE__,
        "Renderer initialization complete. Backend shader directory: '%s'. Capabilities: instancing=%s hardwareSkinning=%s.",
        GetBackendShaderDirectory().c_str(),
        (GetFlags() & CAPS_HARDWAREINSTANCING) != 0 ? "enabled" : "disabled",
        (GetFlags() & CAPS_HARDWARESKINNING) != 0 ? "enabled" : "disabled");
    return true;
}

void BgfxRenderer::ShutdownBgfxResources()
{
    if (m_context.IsInitialized())
    {
        PurgeGpuMeshCache(true);
        PurgeAllTextures(true);
    }

    // Destroy swap-chain/offscreen framebuffer objects while bgfx is alive.
    for (auto& entry : m_windowTargets)
    {
        if (entry.second)
            entry.second->SetRendererData(nullptr);
    }
    m_windowTargets.clear();

    DestroySoftParticleTargets();

    if (m_defaultTargetGroup)
        m_defaultTargetGroup->SetRendererData(nullptr);

    m_defaultTargetGroup = nullptr;
    m_defaultDepthBuffer = nullptr;
    m_defaultBackBuffer = nullptr;

    for (ParticleInstancePage& page : m_particleInstancePages)
    {
        if (bgfx::isValid(page.m_handle))
            bgfx::destroy(page.m_handle);
        page.m_handle = BGFX_INVALID_HANDLE;
        page.m_capacity = 0;
        page.m_cursor = 0;
    }
    m_particleInstancePages.clear();
    m_particleInstanceScratch.clear();
    m_particleOrderScratch.clear();
    if (bgfx::isValid(m_particleQuadVertexBuffer))
        bgfx::destroy(m_particleQuadVertexBuffer);
    if (bgfx::isValid(m_particleQuadIndexBuffer))
        bgfx::destroy(m_particleQuadIndexBuffer);

    if (bgfx::isValid(m_basicProgram))
        bgfx::destroy(m_basicProgram);
    if (bgfx::isValid(m_instancedProgram))
        bgfx::destroy(m_instancedProgram);
    if (bgfx::isValid(m_particleProgram))
        bgfx::destroy(m_particleProgram);
    if (bgfx::isValid(m_softParticleProgram))
        bgfx::destroy(m_softParticleProgram);
    if (bgfx::isValid(m_softParticleFallbackProgram))
        bgfx::destroy(m_softParticleFallbackProgram);
    if (bgfx::isValid(m_softDepthProgram))
        bgfx::destroy(m_softDepthProgram);
    if (bgfx::isValid(m_softDepthInstancedProgram))
        bgfx::destroy(m_softDepthInstancedProgram);
    if (bgfx::isValid(m_softDepthSkinnedProgram))
        bgfx::destroy(m_softDepthSkinnedProgram);
    if (bgfx::isValid(m_softDepthTerrainProgram))
        bgfx::destroy(m_softDepthTerrainProgram);
    if (bgfx::isValid(m_skinnedProgram))
        bgfx::destroy(m_skinnedProgram);
    if (bgfx::isValid(m_skinnedShadowProgram))
        bgfx::destroy(m_skinnedShadowProgram);
    if (bgfx::isValid(m_terrainProgram))
        bgfx::destroy(m_terrainProgram);
    if (bgfx::isValid(m_terrainCubeShadowProgram))
        bgfx::destroy(m_terrainCubeShadowProgram);
    if (bgfx::isValid(m_extendedProgram))
        bgfx::destroy(m_extendedProgram);
    if (bgfx::isValid(m_extendedInstancedProgram))
        bgfx::destroy(m_extendedInstancedProgram);
    if (bgfx::isValid(m_extendedSkinnedProgram))
        bgfx::destroy(m_extendedSkinnedProgram);
    if (bgfx::isValid(m_decorationProgram))
        bgfx::destroy(m_decorationProgram);
    if (bgfx::isValid(m_decorationInstancedProgram))
        bgfx::destroy(m_decorationInstancedProgram);
    if (bgfx::isValid(m_decorationSkinnedProgram))
        bgfx::destroy(m_decorationSkinnedProgram);
    if (bgfx::isValid(m_skyProgram))
        bgfx::destroy(m_skyProgram);
    if (bgfx::isValid(m_shadowProgram))
        bgfx::destroy(m_shadowProgram);
    if (bgfx::isValid(m_instancedShadowProgram))
        bgfx::destroy(m_instancedShadowProgram);
    if (bgfx::isValid(m_copyProgram))
        bgfx::destroy(m_copyProgram);
    if (bgfx::isValid(m_vsmBlurProgram))
        bgfx::destroy(m_vsmBlurProgram);
    if (bgfx::isValid(m_copyTextureUniform))
        bgfx::destroy(m_copyTextureUniform);
    for (auto& handle : m_textureUniforms)
    {
        if (bgfx::isValid(handle))
            bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
    for (auto& handle : m_projectedTextureUniforms)
    {
        if (bgfx::isValid(handle))
            bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
    for (auto& handle : m_terrainTextureUniforms)
    {
        if (bgfx::isValid(handle))
            bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }
    for (auto& handle : m_skyTextureUniforms)
    {
        if (bgfx::isValid(handle))
            bgfx::destroy(handle);
        handle = BGFX_INVALID_HANDLE;
    }

    bgfx::UniformHandle* uniformHandles[] =
    {
        &m_materialAmbientUniform, &m_materialDiffuseUniform,
        &m_materialSpecularUniform, &m_materialEmissiveUniform,
        &m_alphaParamsUniform, &m_textureParamsUniform, &m_mapParamsUniform,
        &m_mapTransform0Uniform, &m_mapTransform1Uniform, &m_bumpParamsUniform,
        &m_cameraPositionUniform, &m_cameraDirectionUniform,
        &m_particleCameraRightUniform, &m_particleCameraUpUniform,
        &m_softParticleDepthUniform, &m_softParticleParamsUniform,
        &m_sceneAmbientUniform, &m_lightCountUniform,
        &m_lightPositionTypeUniform, &m_lightDirectionRangeUniform,
        &m_lightDiffuseDimmerUniform, &m_lightAmbientFalloffUniform,
        &m_lightSpecularSpotUniform, &m_lightSpotParamsUniform,
        &m_lightShadowParamsUniform, &m_lightShadowExtraUniform,
        &m_lightShadowMatrix0Uniform, &m_lightShadowMatrix1Uniform,
        &m_lightShadowMatrix2Uniform, &m_lightShadowMatrix3Uniform,
        &m_pssmParamsUniform, &m_pssmSplitDistancesUniform,
        &m_pssmSplitRowsUniform, &m_pssmViewportsUniform,
        &m_pssmTransitionRowsUniform, &m_pssmTransitionParamsUniform,
        &m_fogColorUniform, &m_fogParamsUniform,
        &m_envTexture2DUniform, &m_envTextureCubeUniform,
        &m_envParamsUniform, &m_envTransform0Uniform,
        &m_envTransform1Uniform, &m_envTransform2Uniform,
        &m_shadowWriteParamsUniform, &m_skinBonesUniform,
        &m_terrainShadowTextureUniform, &m_terrainShadowParamsUniform,
        &m_terrainLayerFeatures0Uniform, &m_terrainLayerFeatures1Uniform,
        &m_terrainLayerScaleUniform, &m_terrainDistRampUniform,
        &m_terrainParallaxStrengthUniform, &m_terrainSpecPowerUniform,
        &m_terrainSpecIntensityUniform, &m_terrainDetailScaleUniform,
        &m_terrainTexCoord0Uniform, &m_terrainTexCoord1Uniform,
        &m_terrainLowDetailUniform, &m_terrainMorphUniform,
        &m_terrainStitchingUniform, &m_terrainEyeUniform,
        &m_terrainDebugUniform, &m_terrainRenderParamsUniform,
        &m_extendedTerrainInfoUniform, &m_extendedLayerDataUniform,
        &m_extendedAlphaInfoUniform, &m_decorationFadeTextureUniform,
        &m_decorationFadeUniform, &m_decorationParamsUniform,
        &m_skyStageConfigUniform, &m_skyStageModifierUniform,
        &m_skyGradientParamsUniform, &m_skyGradientHorizonUniform,
        &m_skyGradientZenithUniform, &m_skyOrientation0Uniform,
        &m_skyOrientation1Uniform, &m_skyOrientation2Uniform,
        &m_skyAtmosScatteringUniform, &m_skyRgbInvWavelengthUniform,
        &m_skyScaleDepthUniform, &m_skyPlanetDimensionsUniform,
        &m_skyFrameDataUniform, &m_skyUpModeUniform,
        &m_skySunSamplesUniform, &m_vsmBlurParamsUniform,
        &m_projectedParamsUniform,
        &m_projectedTransform0Uniform, &m_projectedTransform1Uniform,
        &m_projectedTransform2Uniform, &m_projectedClipPlaneUniform
    };
    for (bgfx::UniformHandle* handle : uniformHandles)
    {
        if (bgfx::isValid(*handle))
            bgfx::destroy(*handle);
        *handle = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_whiteTexture)) bgfx::destroy(m_whiteTexture);
    if (bgfx::isValid(m_whiteCubeTexture)) bgfx::destroy(m_whiteCubeTexture);
    if (bgfx::isValid(m_blackTexture)) bgfx::destroy(m_blackTexture);
    if (bgfx::isValid(m_flatNormalTexture)) bgfx::destroy(m_flatNormalTexture);

    m_basicProgram = BGFX_INVALID_HANDLE;
    m_instancedProgram = BGFX_INVALID_HANDLE;
    m_particleProgram = BGFX_INVALID_HANDLE;
    m_softParticleProgram = BGFX_INVALID_HANDLE;
    m_softParticleFallbackProgram = BGFX_INVALID_HANDLE;
    m_softDepthProgram = BGFX_INVALID_HANDLE;
    m_softDepthInstancedProgram = BGFX_INVALID_HANDLE;
    m_softDepthSkinnedProgram = BGFX_INVALID_HANDLE;
    m_softDepthTerrainProgram = BGFX_INVALID_HANDLE;
    m_skinnedProgram = BGFX_INVALID_HANDLE;
    m_skinnedShadowProgram = BGFX_INVALID_HANDLE;
    m_terrainProgram = BGFX_INVALID_HANDLE;
    m_terrainCubeShadowProgram = BGFX_INVALID_HANDLE;
    m_extendedProgram = BGFX_INVALID_HANDLE;
    m_extendedInstancedProgram = BGFX_INVALID_HANDLE;
    m_extendedSkinnedProgram = BGFX_INVALID_HANDLE;
    m_decorationProgram = BGFX_INVALID_HANDLE;
    m_decorationInstancedProgram = BGFX_INVALID_HANDLE;
    m_decorationSkinnedProgram = BGFX_INVALID_HANDLE;
    m_skyProgram = BGFX_INVALID_HANDLE;
    m_shadowProgram = BGFX_INVALID_HANDLE;
    m_instancedShadowProgram = BGFX_INVALID_HANDLE;
    m_copyProgram = BGFX_INVALID_HANDLE;
    m_vsmBlurProgram = BGFX_INVALID_HANDLE;
    m_copyTextureUniform = BGFX_INVALID_HANDLE;
    m_whiteTexture = BGFX_INVALID_HANDLE;
    m_whiteCubeTexture = BGFX_INVALID_HANDLE;
    m_blackTexture = BGFX_INVALID_HANDLE;
    m_flatNormalTexture = BGFX_INVALID_HANDLE;
    m_particleQuadVertexBuffer = BGFX_INVALID_HANDLE;
    m_particleQuadIndexBuffer = BGFX_INVALID_HANDLE;
}

bool BgfxRenderer::CreateDefaultTargets(unsigned int width, unsigned int height)
{
    m_defaultBackBuffer = Ni2DBuffer::Create(width, height);
    if (!m_defaultBackBuffer)
        return false;

    m_defaultBackBuffer->SetRendererData(NiNew BufferData(
        m_defaultBackBuffer, &NiPixelFormat::RGBA32, BGFX_INVALID_HANDLE));

    m_defaultDepthBuffer = NiDepthStencilBuffer::Create(width, height,
        static_cast<Ni2DBuffer::RendererData*>(nullptr));
    if (!m_defaultDepthBuffer)
        return false;

    // The default depth buffer is supplied by the swap chain; don't allocate
    // a separate texture for it.
    m_defaultDepthBuffer->SetRendererData(NiNew BufferData(
        m_defaultDepthBuffer, &NiPixelFormat::STENCILDEPTH824,
        BGFX_INVALID_HANDLE));

    m_defaultTargetGroup = NiRenderTargetGroup::Create(
        m_defaultBackBuffer, this, m_defaultDepthBuffer);
    return m_defaultTargetGroup != nullptr;
}

void BgfxRenderer::ResetDefaultTargetDimensions(unsigned int width,
    unsigned int height)
{
    if (m_defaultBackBuffer)
        m_defaultBackBuffer->ResetDimensions(width, height);
    if (m_defaultDepthBuffer)
        m_defaultDepthBuffer->ResetDimensions(width, height);
}

void BgfxRenderer::DestroySoftParticleTargets()
{
    if (bgfx::isValid(m_softParticleDepthFrameBuffer))
        bgfx::destroy(m_softParticleDepthFrameBuffer);
    if (bgfx::isValid(m_softParticleDepthColor))
        bgfx::destroy(m_softParticleDepthColor);
    if (bgfx::isValid(m_softParticleDepthZ))
        bgfx::destroy(m_softParticleDepthZ);

    m_softParticleDepthFrameBuffer = BGFX_INVALID_HANDLE;
    m_softParticleDepthColor = BGFX_INVALID_HANDLE;
    m_softParticleDepthZ = BGFX_INVALID_HANDLE;
    m_softParticleDepthViewActive = false;
    m_softParticleDepthClearedThisFrame = false;
}

bool BgfxRenderer::CreateSoftParticleTargets(unsigned int width,
    unsigned int height)
{
    DestroySoftParticleTargets();
    if (!m_context.IsInitialized() || width == 0 || height == 0)
        return false;

    const std::uint64_t colorFlags = BGFX_TEXTURE_RT |
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP |
        BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
        BGFX_SAMPLER_MIP_POINT;

    m_softParticleDepthColor = bgfx::createTexture2D(
        ClampCast<std::uint16_t>(width), ClampCast<std::uint16_t>(height),
        false, 1, bgfx::TextureFormat::R32F, colorFlags);
    if (!bgfx::isValid(m_softParticleDepthColor))
    {
        // R16F is sufficient because depth is normalized to [0,1]. Keep this
        // fallback for older GL/D3D feature levels.
        m_softParticleDepthColor = bgfx::createTexture2D(
            ClampCast<std::uint16_t>(width), ClampCast<std::uint16_t>(height),
            false, 1, bgfx::TextureFormat::R16F, colorFlags);
    }

    const std::uint64_t depthFlags = BGFX_TEXTURE_RT;
    m_softParticleDepthZ = bgfx::createTexture2D(
        ClampCast<std::uint16_t>(width), ClampCast<std::uint16_t>(height),
        false, 1, bgfx::TextureFormat::D32F, depthFlags);
    if (!bgfx::isValid(m_softParticleDepthZ))
    {
        m_softParticleDepthZ = bgfx::createTexture2D(
            ClampCast<std::uint16_t>(width), ClampCast<std::uint16_t>(height),
            false, 1, bgfx::TextureFormat::D24S8, depthFlags);
    }

    if (!bgfx::isValid(m_softParticleDepthColor) ||
        !bgfx::isValid(m_softParticleDepthZ))
    {
        DestroySoftParticleTargets();
        return false;
    }

    const bgfx::TextureHandle attachments[2] =
    {
        m_softParticleDepthColor, m_softParticleDepthZ
    };
    m_softParticleDepthFrameBuffer = bgfx::createFrameBuffer(
        2, attachments, false);
    if (!bgfx::isValid(m_softParticleDepthFrameBuffer))
    {
        DestroySoftParticleTargets();
        return false;
    }

    bgfx::setName(m_softParticleDepthColor, "NiBgfx soft-particle linear depth");
    bgfx::setName(m_softParticleDepthZ, "NiBgfx soft-particle depth test");
    return true;
}

bool BgfxRenderer::GetSoftParticlesSupported() const
{
#if defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
    return bgfx::isValid(m_softParticleProgram) &&
        bgfx::isValid(m_softParticleFallbackProgram) &&
        bgfx::isValid(m_softDepthProgram) &&
        bgfx::isValid(m_softDepthInstancedProgram) &&
        bgfx::isValid(m_softDepthSkinnedProgram) &&
        bgfx::isValid(m_softDepthTerrainProgram) &&
        bgfx::isValid(m_softParticleDepthUniform) &&
        bgfx::isValid(m_softParticleParamsUniform) &&
        bgfx::isValid(m_softParticleDepthColor) &&
        bgfx::isValid(m_softParticleDepthFrameBuffer);
#else
    return false;
#endif
}

bool BgfxRenderer::CanUseSoftParticles() const
{
    return m_softParticlesEnabled && GetSoftParticlesSupported() &&
        m_softParticleDepthViewActive &&
        m_softParticleDepthClearedThisFrame;
}

void BgfxRenderer::SetSoftParticleParams() const
{
    const float farPlane = std::max(m_frustum.m_fFar, 0.001f);
    const float params[4] =
    {
        std::max(m_softParticleFadeDistance, 0.001f),
        1.0f / static_cast<float>(std::max(1u, m_width)),
        1.0f / static_cast<float>(std::max(1u, m_height)),
        farPlane
    };
    bgfx::setUniform(m_softParticleParamsUniform, params);
}

void BgfxRenderer::BindSoftParticleDepth()
{
    if (!CanUseSoftParticles())
        return;

    SetSoftParticleParams();
    bgfx::setTexture(15, m_softParticleDepthUniform,
        m_softParticleDepthColor,
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP |
        BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
        BGFX_SAMPLER_MIP_POINT);
}

bool BgfxRenderer::Resize(unsigned int width, unsigned int height, bool vsync)
{
    if (!m_context.IsInitialized())
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "Resize requested before the bgfx context was initialized.",
            __FILE__, __LINE__);
        return false;
    }
    if (width == 0 || height == 0)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Resize rejected invalid dimensions %ux%u.", width, height);
        return false;
    }

    m_width = width;
    m_height = height;
    m_vsync = vsync;
    NiLogWriteFormat(NI_LOG_INFO, "NiBgfxRenderer", __FILE__, __LINE__,
        "Resizing renderer to %ux%u (vsync=%s).", width, height,
        vsync ? "true" : "false");
    m_context.Reset(width, height, vsync);
    ResetDefaultTargetDimensions(width, height);
#if defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
    if (bgfx::isValid(m_softParticleProgram) &&
        !CreateSoftParticleTargets(width, height))
    {
        NiLogWrite(NI_LOG_WARNING, "NiBgfxRenderer",
            "Soft-particle depth targets could not be recreated after resize; soft particles will fall back to hard intersections.",
            __FILE__, __LINE__);
    }
#endif
    return true;
}

bool BgfxRenderer::IsInitialized() const
{
    return m_context.IsInitialized();
}

void BgfxRenderer::SetSoftParticlesEnabled(bool enabled)
{
    m_softParticlesEnabled = enabled;
}

void BgfxRenderer::SetSoftParticleFadeDistance(float distance)
{
    m_softParticleFadeDistance = std::max(distance, 0.001f);
}

const char* BgfxRenderer::GetDriverInfo() const
{
    return bgfx::getRendererName(bgfx::getRendererType());
}

unsigned int BgfxRenderer::GetFlags() const
{
    unsigned int flags = CAPS_NONPOW2_TEXT | CAPS_ANISO_FILTERING |
        CAPS_AA_RENDERED_TEXTURES;
    if (m_context.IsInitialized() &&
        (bgfx::getCaps()->supported & BGFX_CAPS_INSTANCING) != 0 &&
        bgfx::isValid(m_instancedProgram))
    {
        flags |= CAPS_HARDWAREINSTANCING;
    }
    if (m_context.IsInitialized() && bgfx::isValid(m_skinnedProgram) &&
        bgfx::isValid(m_skinnedShadowProgram))
    {
        flags |= CAPS_HARDWARESKINNING;
    }
    return flags;
}

NiSystemDesc::RendererID BgfxRenderer::GetRendererID() const
{
    return NiSystemDesc::RENDERER_BGFX;
}

void BgfxRenderer::SetDepthClear(float depthClear) { m_depthClear = depthClear; }
float BgfxRenderer::GetDepthClear() const { return m_depthClear; }
void BgfxRenderer::SetBackgroundColor(const NiColor& color)
{
    m_backgroundColor = NiColorA(color.r, color.g, color.b, 1.0f);
}
void BgfxRenderer::SetBackgroundColor(const NiColorA& color) { m_backgroundColor = color; }
void BgfxRenderer::GetBackgroundColor(NiColorA& color) const { color = m_backgroundColor; }
void BgfxRenderer::SetStencilClear(unsigned int stencilClear) { m_stencilClear = stencilClear; }
unsigned int BgfxRenderer::GetStencilClear() const { return m_stencilClear; }

bool BgfxRenderer::ValidateRenderTargetGroup(NiRenderTargetGroup* target)
{
    // Do not call NiRenderTargetGroup::IsValid() here. IsValid() delegates
    // straight back to the active renderer's ValidateRenderTargetGroup(), so
    // doing so would recurse until the stack is exhausted. Renderer validation
    // is the implementation behind NiRenderTargetGroup::IsValid().
    if (!target)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "Render-target validation failed: target is null.", __FILE__, __LINE__);
        return false;
    }

    const unsigned int bufferCount = target->GetBufferCount();
    const unsigned int maxBuffers = GetMaxBuffersPerRenderTargetGroup();
    if (bufferCount > maxBuffers)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Render-target validation failed: %u color buffers requested, bgfx supports %u.",
            bufferCount, maxBuffers);
        return false;
    }

    // bgfx supports depth-only framebuffers, but an RTG with no attachments at
    // all cannot be used as a framebuffer.
    if (bufferCount == 0 && !target->HasDepthStencil())
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "Render-target validation failed: group contains no attachments.",
            __FILE__, __LINE__);
        return false;
    }

    unsigned int width = 0;
    unsigned int height = 0;
    Ni2DBuffer::MultiSamplePreference msaa = Ni2DBuffer::MULTISAMPLE_NONE;
    bool haveAttachment = false;

    for (unsigned int i = 0; i < bufferCount; ++i)
    {
        Ni2DBuffer* buffer = target->GetBuffer(i);
        if (!buffer)
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "Render-target validation failed: color buffer %u is null.", i);
            return false;
        }

        if (!buffer->GetRendererData())
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "Render-target validation failed: color buffer %u has no renderer data.", i);
            return false;
        }

        const unsigned int bufferWidth = buffer->GetWidth();
        const unsigned int bufferHeight = buffer->GetHeight();
        if (bufferWidth == 0 || bufferHeight == 0)
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "Render-target validation failed: color buffer %u has invalid dimensions %ux%u.",
                i, bufferWidth, bufferHeight);
            return false;
        }

        if (!haveAttachment)
        {
            width = bufferWidth;
            height = bufferHeight;
            msaa = buffer->GetMSAAPref();
            haveAttachment = true;
        }
        else
        {
            if (bufferWidth != width || bufferHeight != height)
            {
                NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                    "Render-target validation failed: color buffer %u is %ux%u but the first attachment is %ux%u.",
                    i, bufferWidth, bufferHeight, width, height);
                return false;
            }

            if (buffer->GetMSAAPref() != msaa)
            {
                NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                    "Render-target validation failed: color buffer %u has a different MSAA preference.", i);
                return false;
            }
        }
    }

    if (target->HasDepthStencil())
    {
        NiDepthStencilBuffer* depthBuffer = target->GetDepthStencilBuffer();
        if (!depthBuffer || !depthBuffer->GetRendererData())
        {
            NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
                "Render-target validation failed: depth/stencil buffer has no renderer data.",
                __FILE__, __LINE__);
            return false;
        }

        const unsigned int depthWidth = depthBuffer->GetWidth();
        const unsigned int depthHeight = depthBuffer->GetHeight();
        if (depthWidth == 0 || depthHeight == 0)
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "Render-target validation failed: depth/stencil buffer has invalid dimensions %ux%u.",
                depthWidth, depthHeight);
            return false;
        }

        if (!haveAttachment)
        {
            width = depthWidth;
            height = depthHeight;
            msaa = depthBuffer->GetMSAAPref();
            haveAttachment = true;
        }
        else
        {
            if (depthWidth != width || depthHeight != height)
            {
                NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                    "Render-target validation failed: depth buffer is %ux%u but color attachments are %ux%u.",
                    depthWidth, depthHeight, width, height);
                return false;
            }

            if (depthBuffer->GetMSAAPref() != msaa)
            {
                NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
                    "Render-target validation failed: depth/stencil MSAA preference does not match the color attachments.",
                    __FILE__, __LINE__);
                return false;
            }
        }
    }

    return haveAttachment;
}

bool BgfxRenderer::IsDepthBufferCompatible(Ni2DBuffer* buffer,
    NiDepthStencilBuffer* depthBuffer)
{
    if (!buffer)
        return false;

    // A missing depth buffer is valid; it simply means depth/stencil is not
    // requested for the target.
    if (!depthBuffer)
        return true;

    return buffer->GetWidth() == depthBuffer->GetWidth() &&
        buffer->GetHeight() == depthBuffer->GetHeight() &&
        buffer->GetMSAAPref() == depthBuffer->GetMSAAPref();
}

NiRenderTargetGroup* BgfxRenderer::GetDefaultRenderTargetGroup() const
{
    return m_defaultTargetGroup;
}

const NiRenderTargetGroup* BgfxRenderer::GetCurrentRenderTargetGroup() const
{
    return m_pkCurrentRenderTargetGroup;
}

NiDepthStencilBuffer* BgfxRenderer::GetDefaultDepthStencilBuffer() const
{
    return m_defaultDepthBuffer;
}

Ni2DBuffer* BgfxRenderer::GetDefaultBackBuffer() const
{
    return m_defaultBackBuffer;
}

bool BgfxRenderer::CreateWindowRenderTargetGroup(NiWindowRef window)
{
    if (!window)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "CreateWindowRenderTargetGroup rejected a null native window.",
            __FILE__, __LINE__);
        return false;
    }
    if (m_windowTargets.find(window) != m_windowTargets.end())
    {
        NiLogWriteFormat(NI_LOG_WARNING, "NiBgfxRenderer", __FILE__, __LINE__,
            "A render-target group already exists for native window %p.",
            static_cast<void*>(window));
        return false;
    }

#if defined(EE_PLATFORM_WIN32)
    RECT rect = {};
    if (!GetClientRect(window, &rect))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "GetClientRect failed while creating a render target for native window %p (Win32 error=%lu).",
            static_cast<void*>(window), static_cast<unsigned long>(GetLastError()));
        return false;
    }

    const unsigned int width = static_cast<unsigned int>(std::max<LONG>(1, rect.right - rect.left));
    const unsigned int height = static_cast<unsigned int>(std::max<LONG>(1, rect.bottom - rect.top));

    bgfx::FrameBufferHandle frameBuffer = bgfx::createFrameBuffer(
        static_cast<void*>(window), ClampCast<std::uint16_t>(width),
        ClampCast<std::uint16_t>(height), bgfx::TextureFormat::BGRA8,
        bgfx::TextureFormat::D24S8);
    if (!bgfx::isValid(frameBuffer))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "bgfx::createFrameBuffer failed for native window %p (%ux%u, color=BGRA8, depth=D24S8).",
            static_cast<void*>(window), width, height);
        return false;
    }

    Ni2DBufferPtr backBuffer = Ni2DBuffer::Create(width, height);
    NiDepthStencilBufferPtr depthBuffer = NiDepthStencilBuffer::Create(
        width, height, static_cast<Ni2DBuffer::RendererData*>(nullptr));
    if (!backBuffer || !depthBuffer)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to create Gamebryo back/depth buffer wrappers for native window %p.",
            static_cast<void*>(window));
        bgfx::destroy(frameBuffer);
        return false;
    }

    backBuffer->SetRendererData(NiNew BufferData(
        backBuffer, &NiPixelFormat::RGBA32, BGFX_INVALID_HANDLE));
    depthBuffer->SetRendererData(NiNew BufferData(
        depthBuffer, &NiPixelFormat::STENCILDEPTH824, BGFX_INVALID_HANDLE));

    NiRenderTargetGroupPtr target = NiRenderTargetGroup::Create(
        backBuffer, this, depthBuffer);
    if (!target)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "NiRenderTargetGroup::Create failed for native window %p.",
            static_cast<void*>(window));
        bgfx::destroy(frameBuffer);
        return false;
    }

    target->SetRendererData(NiNew TargetGroupData(frameBuffer));
    m_windowTargets.emplace(window, target);
    NiLogWriteFormat(NI_LOG_INFO, "NiBgfxRenderer", __FILE__, __LINE__,
        "Created secondary-window framebuffer %p at %ux%u.",
        static_cast<void*>(window), width, height);
    return true;
#else
    EE_UNUSED_ARG(window);
    NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
        "CreateWindowRenderTargetGroup is only implemented for the Win32 platform in this port.",
        __FILE__, __LINE__);
    return false;
#endif
}

bool BgfxRenderer::RecreateWindowRenderTargetGroup(NiWindowRef window)
{
    auto found = m_windowTargets.find(window);
    if (found == m_windowTargets.end())
        return CreateWindowRenderTargetGroup(window);

#if defined(EE_PLATFORM_WIN32)
    RECT rect = {};
    if (!GetClientRect(window, &rect))
        return false;

    const unsigned int width = static_cast<unsigned int>(std::max<LONG>(1, rect.right - rect.left));
    const unsigned int height = static_cast<unsigned int>(std::max<LONG>(1, rect.bottom - rect.top));
    bgfx::FrameBufferHandle frameBuffer = bgfx::createFrameBuffer(
        static_cast<void*>(window), ClampCast<std::uint16_t>(width),
        ClampCast<std::uint16_t>(height), bgfx::TextureFormat::BGRA8,
        bgfx::TextureFormat::D24S8);
    if (!bgfx::isValid(frameBuffer))
        return false;

    NiRenderTargetGroup* target = found->second;
    if (!target || target->GetBufferCount() == 0)
    {
        bgfx::destroy(frameBuffer);
        return false;
    }

    target->SetRendererData(nullptr);
    target->GetBuffer(0)->ResetDimensions(width, height);
    if (target->HasDepthStencil())
        target->GetDepthStencilBuffer()->ResetDimensions(width, height);
    target->SetRendererData(NiNew TargetGroupData(frameBuffer));
    return true;
#else
    EE_UNUSED_ARG(window);
    return false;
#endif
}

void BgfxRenderer::ReleaseWindowRenderTargetGroup(NiWindowRef window)
{
    auto found = m_windowTargets.find(window);
    if (found == m_windowTargets.end())
        return;
    if (found->second)
        found->second->SetRendererData(nullptr);
    m_windowTargets.erase(found);
}

NiRenderTargetGroup* BgfxRenderer::GetWindowRenderTargetGroup(NiWindowRef window) const
{
    auto found = m_windowTargets.find(window);
    return found != m_windowTargets.end() ? found->second.data() : nullptr;
}

const NiPixelFormat* BgfxRenderer::FindClosestPixelFormat(
    NiTexture::FormatPrefs& prefs) const
{
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::COMPRESSED)
        return prefs.m_eAlphaFmt == NiTexture::FormatPrefs::SMOOTH ?
            &NiPixelFormat::DXT5 : &NiPixelFormat::DXT1;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::SINGLE_COLOR_8)
        return &NiPixelFormat::I8;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::SINGLE_COLOR_16)
        return &NiPixelFormat::R16;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::FLOAT_COLOR_32)
        return &NiPixelFormat::RGBA32;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::SINGLE_COLOR_32)
        return &NiPixelFormat::R32;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::DOUBLE_COLOR_32)
        return &NiPixelFormat::RG32;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::DOUBLE_COLOR_64)
        return &NiPixelFormat::RG64;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::FLOAT_COLOR_64)
        return &NiPixelFormat::RGBA64;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::FLOAT_COLOR_128)
        return &NiPixelFormat::RGBA128;
    if (prefs.m_ePixelLayout == NiTexture::FormatPrefs::DEPTH_24_X8)
        return &NiPixelFormat::STENCILDEPTH824;
    return &NiPixelFormat::RGBA32;
}

const NiPixelFormat* BgfxRenderer::FindClosestDepthStencilFormat(
    const NiPixelFormat*, unsigned int depthBPP, unsigned int stencilBPP) const
{
    if (stencilBPP != 0)
        return &NiPixelFormat::STENCILDEPTH824;
    if (depthBPP <= 16)
        return &NiPixelFormat::DEPTH16;
    return &NiPixelFormat::DEPTH32;
}

unsigned int BgfxRenderer::GetMaxBuffersPerRenderTargetGroup() const
{
    return std::min<unsigned int>(bgfx::getCaps()->limits.maxFBAttachments,
        NiRenderTargetGroup::MAX_RENDER_BUFFERS);
}

bool BgfxRenderer::GetIndependentBufferBitDepths() const { return true; }
void BgfxRenderer::UseLegacyPipelineAsDefaultMaterial() {}

bool BgfxRenderer::PrecacheShader(NiRenderObject* renderObject)
{
    if (!renderObject || !NiIsKindOf(NiMesh, renderObject))
        return false;

    NiMesh* mesh = static_cast<NiMesh*>(renderObject);

    // NiShaderSortProcessor relies on PrecacheShader() guaranteeing that the
    // active material instance owns a valid cached NiShader before the mesh is
    // inserted into a shader bucket. The legacy DX renderers did this through
    // GetShaderAndVertexDecl(); bgfx has no D3D vertex declaration object, but
    // it must still preserve the Gamebryo material-cache contract.
    if (!mesh->GetActiveMaterial())
    {
        NiMaterial* defaultMaterial = GetDefaultMaterial();
        if (!defaultMaterial)
            defaultMaterial = GetInitialDefaultMaterial();

        if (!defaultMaterial)
            return false;

        mesh->ApplyAndSetActiveMaterial(defaultMaterial);
    }

    const NiMaterialInstance* materialInstance =
        mesh->GetActiveMaterialInstance();
    if (!materialInstance || !materialInstance->GetMaterial())
        return false;

    // GetShader() is the fast path when the cached descriptor is still valid.
    // GetShaderFromMaterial() resolves/generates the BgfxMaterialShader cache
    // entry when the material is dirty or has never been resolved.
    NiShader* shader = mesh->GetShader();
    if (!shader)
        shader = mesh->GetShaderFromMaterial();

    if (!shader)
    {
        const char* meshName = static_cast<const char*>(mesh->GetName());
        const char* materialName = static_cast<const char*>(
            materialInstance->GetMaterial()->GetName());
        NiLogWriteFormat(NI_LOG_WARNING, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to precache shader for mesh '%s' material '%s'.",
            meshName && *meshName ? meshName : "<unnamed>",
            materialName && *materialName ? materialName : "<unnamed>");
        return false;
    }

    return true;
}

bool BgfxRenderer::PrecacheTexture(NiTexture* texture)
{
    return EnsureTexture(texture);
}

bool BgfxRenderer::SetMipmapSkipLevel(unsigned int skip)
{
    // Existing source textures are lazily recreated by EnsureTexture when it
    // notices that their upload used a different skip level. Render targets
    // and dynamic textures are intentionally unaffected.
    m_mipmapSkip = skip;
    return true;
}

unsigned int BgfxRenderer::GetMipmapSkipLevel() const { return m_mipmapSkip; }
void BgfxRenderer::PurgeMaterial(NiMaterialProperty*) {}
void BgfxRenderer::PurgeEffect(NiDynamicEffect*) {}

bool BgfxRenderer::PurgeTexture(NiTexture* texture)
{
    if (!texture)
        return false;

    // Rendered textures share one bgfx handle between the texture renderer data
    // (owner) and their Ni2DBuffer renderer data (non-owner). Drop buffer data
    // first so no stale handle remains after the texture releases it.
    if (NiIsKindOf(NiRenderedCubeMap, texture))
    {
        NiRenderedCubeMap* cubeMap = static_cast<NiRenderedCubeMap*>(texture);
        for (unsigned int face = 0; face < NiRenderedCubeMap::FACE_NUM; ++face)
        {
            Ni2DBuffer* buffer = cubeMap->GetFaceBuffer(
                static_cast<NiRenderedCubeMap::FaceID>(face));
            if (buffer)
                buffer->SetRendererData(nullptr);
        }
    }
    else if (NiIsKindOf(NiRenderedTexture, texture))
    {
        NiRenderedTexture* rendered = static_cast<NiRenderedTexture*>(texture);
        if (rendered->GetBuffer())
            rendered->GetBuffer()->SetRendererData(nullptr);
    }

    // NiTexture::SetRendererData is a raw-pointer assignment (unlike
    // Ni2DBuffer's smart renderer-data holder). Explicitly destroy the old
    // renderer data or every PurgeTexture call leaks its bgfx TextureHandle.
    NiTexture::RendererData* rendererData = texture->GetRendererData();
    texture->SetRendererData(nullptr);
    NiDelete rendererData;
    return true;
}

bool BgfxRenderer::PurgeAllTextures(bool)
{
    NiTexture::LockTextureList();
    for (NiTexture* texture = NiTexture::GetListHead(); texture;
        texture = texture->GetListNext())
    {
        PurgeTexture(texture);
    }
    NiTexture::UnlockTextureList();
    return true;
}

NiPixelData* BgfxRenderer::TakeScreenShot(const NiRect<unsigned int>* screenRect,
    const NiRenderTargetGroup* target)
{
    if (!m_context.IsInitialized())
        return nullptr;

    const NiRenderTargetGroup* sourceTarget = target ? target : m_defaultTargetGroup.data();
    if (!sourceTarget || sourceTarget->GetBufferCount() == 0)
        return nullptr;

    const Ni2DBuffer* sourceBuffer = sourceTarget->GetBuffer(0);
    const BufferData* sourceData = GetBufferData(sourceBuffer);

    std::vector<std::uint8_t> pixels;
    unsigned int sourceWidth = sourceBuffer ? sourceBuffer->GetWidth() : 0;
    unsigned int sourceHeight = sourceBuffer ? sourceBuffer->GetHeight() : 0;
    unsigned int sourcePitch = sourceWidth * 4u;
    bool sourceIsBGRA = false;
    bool sourceYFlip = false;

    if (!sourceData || !bgfx::isValid(sourceData->m_handle))
    {
        // Native-window back buffers have no TextureHandle. bgfx exposes
        // both the main swap chain (invalid handle) and secondary native
        // window framebuffers through CallbackI::screenShot.
        bgfx::FrameBufferHandle captureTarget = BGFX_INVALID_HANDLE;
        if (sourceTarget != m_defaultTargetGroup.data())
        {
            const TargetGroupData* targetData = static_cast<const TargetGroupData*>(
                sourceTarget->GetRendererData());
            if (!targetData || !bgfx::isValid(targetData->m_handle))
                return nullptr;
            captureTarget = targetData->m_handle;
        }

        if (!m_context.CaptureFrameBuffer(captureTarget, pixels, sourceWidth,
            sourceHeight, sourcePitch, sourceYFlip))
        {
            return nullptr;
        }
        sourceIsBGRA = true;
    }
    else
    {
        if (!(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_READ_BACK) ||
            !(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_BLIT) ||
            sourceWidth == 0 || sourceHeight == 0)
        {
            return nullptr;
        }

        // readTexture resources cannot simultaneously be render targets, so
        // copy the rendered texture into a dedicated read-back texture first.
        bgfx::TextureHandle readback = bgfx::createTexture2D(
            ClampCast<std::uint16_t>(sourceWidth),
            ClampCast<std::uint16_t>(sourceHeight), false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
        if (!bgfx::isValid(readback))
            return nullptr;

        bgfx::ViewId readbackView = 0;
        if (!AllocateAuxiliaryView(readbackView, "NiBgfx Screenshot Readback"))
        {
            bgfx::destroy(readback);
            return nullptr;
        }

        bgfx::blit(readbackView, readback, 0, 0, 0, 0, sourceData->m_handle,
            0, 0, 0, sourceData->m_layer,
            ClampCast<std::uint16_t>(sourceWidth),
            ClampCast<std::uint16_t>(sourceHeight), 1);

        pixels.resize(static_cast<size_t>(sourceWidth) * sourceHeight * 4u);
        const std::uint32_t readyFrame = bgfx::readTexture(readback, pixels.data());

        // readTexture completion is asynchronous. Do not spin forever if the
        // device/backend stops advancing (device loss, minimized surface,
        // driver failure, etc.). The native-window screenshot callback uses
        // the same bounded policy in NiBgfxContext.
        constexpr unsigned int MAX_READBACK_FRAMES = 16;
        std::uint32_t frame = 0;
        bool ready = readyFrame == 0;
        for (unsigned int i = 0; i < MAX_READBACK_FRAMES && !ready; ++i)
        {
            frame = m_context.Frame();
            ready = frame >= readyFrame;
        }
        bgfx::destroy(readback);
        if (!ready)
            return nullptr;
    }

    if (sourceWidth == 0 || sourceHeight == 0 || pixels.empty() ||
        sourcePitch < sourceWidth * 4u)
    {
        return nullptr;
    }

    unsigned int left = 0;
    unsigned int top = 0;
    unsigned int right = sourceWidth;
    unsigned int bottom = sourceHeight;
    if (screenRect)
    {
        left = std::min(screenRect->m_left, screenRect->m_right);
        right = std::max(screenRect->m_left, screenRect->m_right);
        top = std::min(screenRect->m_top, screenRect->m_bottom);
        bottom = std::max(screenRect->m_top, screenRect->m_bottom);
        left = std::min(left, sourceWidth);
        right = std::min(right, sourceWidth);
        top = std::min(top, sourceHeight);
        bottom = std::min(bottom, sourceHeight);
    }
    if (right <= left || bottom <= top)
        return nullptr;

    const unsigned int width = right - left;
    const unsigned int height = bottom - top;
    NiPixelData* result = NiNew NiPixelData(width, height, NiPixelFormat::RGBA32);
    if (!result)
        return nullptr;

    unsigned char* dst = result->GetPixels();
    for (unsigned int y = 0; y < height; ++y)
    {
        const unsigned int logicalY = top + y;
        const unsigned int sourceY = sourceYFlip ?
            (sourceHeight - 1u - logicalY) : logicalY;
        const std::uint8_t* src = pixels.data() +
            static_cast<size_t>(sourceY) * sourcePitch +
            static_cast<size_t>(left) * 4u;
        unsigned char* out = dst + static_cast<size_t>(y) * width * 4u;

        if (!sourceIsBGRA)
        {
            std::memcpy(out, src, static_cast<size_t>(width) * 4u);
            continue;
        }

        for (unsigned int x = 0; x < width; ++x)
        {
            out[x * 4u + 0u] = src[x * 4u + 2u];
            out[x * 4u + 1u] = src[x * 4u + 1u];
            out[x * 4u + 2u] = src[x * 4u + 0u];
            out[x * 4u + 3u] = src[x * 4u + 3u];
        }
    }
    result->MarkAsChanged();
    return result;
}

bool BgfxRenderer::FastCopy(const Ni2DBuffer* src, Ni2DBuffer* dst,
    const NiRect<unsigned int>* srcRect, unsigned int destX, unsigned int destY)
{
    if (!src || !dst || !(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_BLIT))
        return false;

    if (!src->GetPixelFormat() || !dst->GetPixelFormat() ||
        *src->GetPixelFormat() != *dst->GetPixelFormat())
    {
        return false;
    }

    const BufferData* srcData = GetBufferData(src);
    BufferData* dstData = GetBufferData(dst);
    if (!srcData || !dstData || !bgfx::isValid(srcData->m_handle) ||
        !bgfx::isValid(dstData->m_handle))
        return false;

    const unsigned int srcX = srcRect ? std::min(srcRect->m_left, srcRect->m_right) : 0;
    const unsigned int srcY = srcRect ? std::min(srcRect->m_top, srcRect->m_bottom) : 0;
    const unsigned int width = srcRect ? srcRect->GetWidth() : src->GetWidth();
    const unsigned int height = srcRect ? srcRect->GetHeight() : src->GetHeight();
    if (width == 0 || height == 0 || srcX + width > src->GetWidth() ||
        srcY + height > src->GetHeight() || destX + width > dst->GetWidth() ||
        destY + height > dst->GetHeight())
        return false;

    bgfx::ViewId blitView = 0;
    if (!AllocateAuxiliaryView(blitView, "NiBgfx Texture Blit"))
        return false;

    if (srcData->m_handle.idx == dstData->m_handle.idx &&
        srcData->m_layer == dstData->m_layer)
    {
        // bgfx does not define overlapping in-place blits. Stage through a
        // temporary texture so self copies match the old renderer semantics.
        const bgfx::TextureFormat::Enum tempFormat =
            GetBgfxTextureFormat(*src->GetPixelFormat());
        if (tempFormat == bgfx::TextureFormat::Unknown)
            return false;

        bgfx::TextureHandle temporary = bgfx::createTexture2D(
            ClampCast<std::uint16_t>(width), ClampCast<std::uint16_t>(height),
            false, 1, tempFormat, BGFX_TEXTURE_BLIT_DST |
            BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
        if (!bgfx::isValid(temporary))
            return false;

        bgfx::blit(blitView, temporary, 0, 0, 0, 0, srcData->m_handle, 0,
            ClampCast<std::uint16_t>(srcX), ClampCast<std::uint16_t>(srcY),
            srcData->m_layer, ClampCast<std::uint16_t>(width),
            ClampCast<std::uint16_t>(height), 1);
        bgfx::blit(blitView, dstData->m_handle, 0,
            ClampCast<std::uint16_t>(destX), ClampCast<std::uint16_t>(destY),
            dstData->m_layer, temporary, 0, 0, 0, 0,
            ClampCast<std::uint16_t>(width), ClampCast<std::uint16_t>(height), 1);
        bgfx::destroy(temporary);
        return true;
    }

    bgfx::blit(blitView, dstData->m_handle, 0,
        ClampCast<std::uint16_t>(destX), ClampCast<std::uint16_t>(destY),
        dstData->m_layer, srcData->m_handle, 0,
        ClampCast<std::uint16_t>(srcX), ClampCast<std::uint16_t>(srcY),
        srcData->m_layer, ClampCast<std::uint16_t>(width),
        ClampCast<std::uint16_t>(height), 1);
    return true;
}

bool BgfxRenderer::Copy(const Ni2DBuffer* src, Ni2DBuffer* dst,
    const NiRect<unsigned int>* srcRect, const NiRect<unsigned int>* destRect,
    Ni2DBuffer::CopyFilterPreference pref)
{
    if (!src || !dst)
        return false;

    // Match the legacy D3D renderers: the GPU copy path only supports equal
    // pixel layouts. Cross-format conversion belongs in NiImageConverter.
    if (!src->GetPixelFormat() || !dst->GetPixelFormat() ||
        *src->GetPixelFormat() != *dst->GetPixelFormat())
    {
        return false;
    }

    NiRect<unsigned int> actualSrc = srcRect ? *srcRect :
        NiRect<unsigned int>(0, src->GetWidth(), 0, src->GetHeight());
    NiRect<unsigned int> actualDst = destRect ? *destRect :
        NiRect<unsigned int>(0, dst->GetWidth(), 0, dst->GetHeight());

    if (actualSrc.m_left > actualSrc.m_right)
        std::swap(actualSrc.m_left, actualSrc.m_right);
    if (actualSrc.m_top > actualSrc.m_bottom)
        std::swap(actualSrc.m_top, actualSrc.m_bottom);
    if (actualDst.m_left > actualDst.m_right)
        std::swap(actualDst.m_left, actualDst.m_right);
    if (actualDst.m_top > actualDst.m_bottom)
        std::swap(actualDst.m_top, actualDst.m_bottom);

    if (actualSrc.GetWidth() == 0 || actualSrc.GetHeight() == 0 ||
        actualDst.GetWidth() == 0 || actualDst.GetHeight() == 0 ||
        actualSrc.m_right > src->GetWidth() || actualSrc.m_bottom > src->GetHeight() ||
        actualDst.m_right > dst->GetWidth() || actualDst.m_bottom > dst->GetHeight())
    {
        return false;
    }

    if (actualSrc.GetWidth() == actualDst.GetWidth() &&
        actualSrc.GetHeight() == actualDst.GetHeight())
    {
        return FastCopy(src, dst, &actualSrc, actualDst.m_left, actualDst.m_top);
    }

    const BufferData* srcData = GetBufferData(src);
    const BufferData* dstData = GetBufferData(dst);
    if (!srcData || !dstData)
        return false;

    if (srcData->m_handle.idx == dstData->m_handle.idx &&
        srcData->m_layer == dstData->m_layer)
    {
        // Render-to-self feedback is invalid. Stage just the source rectangle,
        // then use the normal filtered copy pass from the temporary texture.
        const unsigned int tempWidth = actualSrc.GetWidth();
        const unsigned int tempHeight = actualSrc.GetHeight();
        const bgfx::TextureFormat::Enum tempFormat =
            GetBgfxTextureFormat(*src->GetPixelFormat());
        if (tempFormat == bgfx::TextureFormat::Unknown ||
            tempFormat == bgfx::TextureFormat::BC1 ||
            tempFormat == bgfx::TextureFormat::BC2 ||
            tempFormat == bgfx::TextureFormat::BC3)
        {
            return false;
        }

        bgfx::TextureHandle temporary = bgfx::createTexture2D(
            ClampCast<std::uint16_t>(tempWidth), ClampCast<std::uint16_t>(tempHeight),
            false, 1, tempFormat, BGFX_TEXTURE_BLIT_DST |
            BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
        if (!bgfx::isValid(temporary))
            return false;

        bgfx::ViewId blitView = 0;
        if (!AllocateAuxiliaryView(blitView, "NiBgfx Self-Copy Stage"))
        {
            bgfx::destroy(temporary);
            return false;
        }
        bgfx::blit(blitView, temporary, 0, 0, 0, 0, srcData->m_handle, 0,
            ClampCast<std::uint16_t>(actualSrc.m_left),
            ClampCast<std::uint16_t>(actualSrc.m_top), srcData->m_layer,
            ClampCast<std::uint16_t>(tempWidth),
            ClampCast<std::uint16_t>(tempHeight), 1);

        Ni2DBufferPtr tempBuffer = Ni2DBuffer::Create(tempWidth, tempHeight);
        if (!tempBuffer)
        {
            bgfx::destroy(temporary);
            return false;
        }
        tempBuffer->SetRendererData(NiNew BufferData(tempBuffer,
            src->GetPixelFormat(), temporary, Ni2DBuffer::MULTISAMPLE_NONE, false));
        const NiRect<unsigned int> tempRect(0, tempWidth, 0, tempHeight);
        const bool result = DrawScaledCopy(tempBuffer, dst, tempRect, actualDst, pref);
        tempBuffer->SetRendererData(nullptr);
        bgfx::destroy(temporary);
        return result;
    }

    return DrawScaledCopy(src, dst, actualSrc, actualDst, pref);
}

bool BgfxRenderer::DrawScaledCopy(const Ni2DBuffer* src, Ni2DBuffer* dst,
    const NiRect<unsigned int>& srcRect, const NiRect<unsigned int>& dstRect,
    Ni2DBuffer::CopyFilterPreference pref)
{
    if (!bgfx::isValid(m_copyProgram) || !bgfx::isValid(m_copyTextureUniform))
        return false;

    const BufferData* srcData = GetBufferData(src);
    BufferData* dstData = GetBufferData(dst);
    if (!srcData || !dstData || !bgfx::isValid(srcData->m_handle) ||
        !bgfx::isValid(dstData->m_handle) || srcData->m_handle.idx == dstData->m_handle.idx)
    {
        return false;
    }

    bgfx::Attachment attachment;
    attachment.init(dstData->m_handle, bgfx::Access::Write, dstData->m_layer,
        1, 0, BGFX_RESOLVE_NONE);
    const bgfx::FrameBufferHandle frameBuffer =
        bgfx::createFrameBuffer(1, &attachment, false);
    if (!bgfx::isValid(frameBuffer))
        return false;

    bgfx::ViewId viewId = 0;
    if (!AllocateAuxiliaryView(viewId, "NiBgfx Scaled Texture Copy"))
    {
        bgfx::destroy(frameBuffer);
        return false;
    }

    bgfx::setViewFrameBuffer(viewId, frameBuffer);
    bgfx::setViewRect(viewId, 0, 0, ClampCast<std::uint16_t>(dst->GetWidth()),
        ClampCast<std::uint16_t>(dst->GetHeight()));
    bgfx::setViewTransform(viewId, nullptr, nullptr);

    const bgfx::VertexLayout layout = GetCopyVertexLayout();
    if (bgfx::getAvailTransientVertexBuffer(4, layout) != 4)
    {
        bgfx::destroy(frameBuffer);
        return false;
    }

    bgfx::TransientVertexBuffer vb;
    bgfx::allocTransientVertexBuffer(&vb, 4, layout);
    CopyVertex* vertices = reinterpret_cast<CopyVertex*>(vb.data);

    const float invDstW = 1.0f / static_cast<float>(dst->GetWidth());
    const float invDstH = 1.0f / static_cast<float>(dst->GetHeight());
    const float left = static_cast<float>(dstRect.m_left) * invDstW * 2.0f - 1.0f;
    const float right = static_cast<float>(dstRect.m_right) * invDstW * 2.0f - 1.0f;
    const float top = 1.0f - static_cast<float>(dstRect.m_top) * invDstH * 2.0f;
    const float bottom = 1.0f - static_cast<float>(dstRect.m_bottom) * invDstH * 2.0f;

    const float invSrcW = 1.0f / static_cast<float>(src->GetWidth());
    const float invSrcH = 1.0f / static_cast<float>(src->GetHeight());
    const float u0 = static_cast<float>(srcRect.m_left) * invSrcW;
    const float u1 = static_cast<float>(srcRect.m_right) * invSrcW;
    float v0 = static_cast<float>(srcRect.m_top) * invSrcH;
    float v1 = static_cast<float>(srcRect.m_bottom) * invSrcH;
    if (bgfx::getCaps()->originBottomLeft)
    {
        v0 = 1.0f - v0;
        v1 = 1.0f - v1;
    }

    vertices[0] = { left,  top,    0.0f, u0, v0 };
    vertices[1] = { right, top,    0.0f, u1, v0 };
    vertices[2] = { left,  bottom, 0.0f, u0, v1 };
    vertices[3] = { right, bottom, 0.0f, u1, v1 };

    uint32_t samplerFlags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    if (pref != Ni2DBuffer::COPY_FILTER_LINEAR)
    {
        samplerFlags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
            BGFX_SAMPLER_MIP_POINT;
    }

    bgfx::setVertexBuffer(0, &vb);
    bgfx::setTexture(0, m_copyTextureUniform, srcData->m_handle, samplerFlags);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_PT_TRISTRIP);
    bgfx::setStencil(BGFX_STENCIL_NONE);
    bgfx::submit(viewId, m_copyProgram);
    bgfx::destroy(frameBuffer);
    return true;
}

bool BgfxRenderer::GetLeftRightSwap() const { return m_leftRightSwap; }
bool BgfxRenderer::SetLeftRightSwap(bool swap) { m_leftRightSwap = swap; return true; }
float BgfxRenderer::GetMaxFogValue() const { return m_maxFogValue; }
void BgfxRenderer::SetMaxFogValue(float fogValue) { m_maxFogValue = fogValue; }
void BgfxRenderer::SetMaxAnisotropy(unsigned short value)
{
    m_usMaxAnisotropy = value;
}

BgfxRenderer::TextureData* BgfxRenderer::GetTextureData(const NiTexture* texture) const
{
    return texture ? static_cast<TextureData*>(texture->GetRendererData()) : nullptr;
}

BgfxRenderer::BufferData* BgfxRenderer::GetBufferData(const Ni2DBuffer* buffer) const
{
    return buffer ? static_cast<BufferData*>(buffer->GetRendererData()) : nullptr;
}

bool BgfxRenderer::CreateTextureFromPixelData(NiTexture* texture,
    const NiPixelData* sourcePixels, bool cubeMap)
{
    if (!texture || !sourcePixels)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "CreateTextureFromPixelData received invalid input (texture=%p pixels=%p).",
            static_cast<void*>(texture), static_cast<const void*>(sourcePixels));
        return false;
    }

    NiPixelDataPtr converted;
    const NiPixelData* pixels = sourcePixels;
    bgfx::TextureFormat::Enum format = GetBgfxTextureFormat(pixels->GetPixelFormat());

    // Keep BC1/2/3 compressed data as-is. Convert all other unsupported
    // formats through Gamebryo's existing image converter to RGBA32.
    if (format == bgfx::TextureFormat::Unknown && !IsCompressedSupportedFormat(pixels->GetPixelFormat()))
    {
        NiImageConverter* converter = NiImageConverter::GetImageConverter();
        if (!converter || !converter->CanConvertPixelData(
            pixels->GetPixelFormat(), NiPixelFormat::RGBA32))
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "Texture '%s' uses unsupported format %s and no RGBA32 converter is available.",
                GetTextureDebugName(texture), GetPixelFormatName(pixels->GetPixelFormat()));
            return false;
        }

        converted = converter->ConvertPixelData(*pixels, NiPixelFormat::RGBA32,
            nullptr, pixels->GetNumMipmapLevels() > 1);
        if (!converted)
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "RGBA32 conversion failed for texture '%s' (source format=%s).",
                GetTextureDebugName(texture), GetPixelFormatName(pixels->GetPixelFormat()));
            return false;
        }
        pixels = converted;
        format = bgfx::TextureFormat::RGBA8;
    }

    if (format == bgfx::TextureFormat::Unknown)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Texture '%s' still has no bgfx format mapping after conversion (format=%s).",
            GetTextureDebugName(texture), GetPixelFormatName(pixels->GetPixelFormat()));
        return false;
    }

    const unsigned int mipCount = pixels->GetNumMipmapLevels();
    if (mipCount == 0)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Texture '%s' contains zero mip levels.", GetTextureDebugName(texture));
        return false;
    }
    const unsigned int mipSkip = std::min(m_mipmapSkip, mipCount - 1);
    const unsigned int uploadMipCount = mipCount - mipSkip;
    const bool hasMips = uploadMipCount > 1;
    const unsigned int uploadWidth = pixels->GetWidth(mipSkip);
    const unsigned int uploadHeight = pixels->GetHeight(mipSkip);

    // NiPixelData stores each face as a chain of mip levels. When skipping
    // the largest mips, repack only the surviving chain(s) so bgfx sees the
    // requested mip as level zero.
    std::vector<std::uint8_t> skippedMipData;
    const void* uploadPixels = pixels->GetPixels();
    size_t uploadSize = pixels->GetTotalSizeInBytes();
    if (mipSkip != 0)
    {
        uploadSize = 0;
        for (unsigned int face = 0; face < pixels->GetNumFaces(); ++face)
            for (unsigned int mip = mipSkip; mip < mipCount; ++mip)
                uploadSize += pixels->GetSizeInBytes(mip, face);

        skippedMipData.reserve(uploadSize);
        for (unsigned int face = 0; face < pixels->GetNumFaces(); ++face)
        {
            for (unsigned int mip = mipSkip; mip < mipCount; ++mip)
            {
                const std::uint8_t* begin = pixels->GetPixels(mip, face);
                const size_t size = pixels->GetSizeInBytes(mip, face);
                skippedMipData.insert(skippedMipData.end(), begin, begin + size);
            }
        }
        uploadPixels = skippedMipData.data();
        uploadSize = skippedMipData.size();
    }

    if (uploadSize > std::numeric_limits<std::uint32_t>::max())
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Texture '%s' upload is too large for bgfx (%llu bytes).",
            GetTextureDebugName(texture), static_cast<unsigned long long>(uploadSize));
        return false;
    }
    const unsigned int faceCount = pixels->GetNumFaces();
    if (cubeMap && (faceCount != 6 || uploadWidth != uploadHeight))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Cube texture '%s' is invalid: faces=%u dimensions=%ux%u (expected 6 square faces).",
            GetTextureDebugName(texture), faceCount, uploadWidth, uploadHeight);
        return false;
    }
    if (faceCount == 0 || faceCount > std::numeric_limits<std::uint16_t>::max())
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Texture '%s' has invalid face/layer count %u.",
            GetTextureDebugName(texture), faceCount);
        return false;
    }

    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    if (cubeMap)
    {
        const bgfx::Memory* memory = bgfx::copy(uploadPixels,
            static_cast<std::uint32_t>(uploadSize));
        handle = bgfx::createTextureCube(ClampCast<std::uint16_t>(uploadWidth),
            hasMips, 1, format, BGFX_SAMPLER_NONE, memory);
    }
    else if (faceCount == 1)
    {
        const bgfx::Memory* memory = bgfx::copy(uploadPixels,
            static_cast<std::uint32_t>(uploadSize));
        handle = bgfx::createTexture2D(ClampCast<std::uint16_t>(uploadWidth),
            ClampCast<std::uint16_t>(uploadHeight), hasMips, 1, format,
            BGFX_SAMPLER_NONE, memory);
    }
    else
    {
        // D3D10/11 treated a non-cube NiPixelData with multiple faces as a
        // Texture2DArray. Create the same resource in bgfx. Upload each
        // layer/mip explicitly so we do not depend on backend-specific packed
        // initial-data ordering.
        handle = bgfx::createTexture2D(ClampCast<std::uint16_t>(uploadWidth),
            ClampCast<std::uint16_t>(uploadHeight), hasMips,
            ClampCast<std::uint16_t>(faceCount), format, BGFX_SAMPLER_NONE);
        if (bgfx::isValid(handle))
        {
            for (unsigned int face = 0; face < faceCount; ++face)
            {
                for (unsigned int mip = mipSkip; mip < mipCount; ++mip)
                {
                    const size_t size = pixels->GetSizeInBytes(mip, face);
                    if (size > std::numeric_limits<std::uint32_t>::max())
                    {
                        bgfx::destroy(handle);
                        handle = BGFX_INVALID_HANDLE;
                        break;
                    }
                    const bgfx::Memory* levelMemory = bgfx::copy(
                        pixels->GetPixels(mip, face),
                        static_cast<std::uint32_t>(size));
                    bgfx::updateTexture2D(handle,
                        ClampCast<std::uint16_t>(face),
                        static_cast<std::uint8_t>(mip - mipSkip),
                        0, 0,
                        ClampCast<std::uint16_t>(pixels->GetWidth(mip, face)),
                        ClampCast<std::uint16_t>(pixels->GetHeight(mip, face)),
                        levelMemory);
                }
                if (!bgfx::isValid(handle))
                    break;
            }
        }
    }

    if (!bgfx::isValid(handle))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "bgfx texture creation failed for '%s' (%ux%u, faces=%u, mips=%u, format=%s, cube=%s).",
            GetTextureDebugName(texture), uploadWidth, uploadHeight, faceCount,
            uploadMipCount, GetPixelFormatName(pixels->GetPixelFormat()),
            cubeMap ? "true" : "false");
        return false;
    }

    TextureData* rendererData = NiNew TextureData(texture, handle, format,
        pixels->GetPixelFormat());
    rendererData->m_sourceRevision = sourcePixels->GetRevisionID();
    rendererData->m_mipmapSkip = mipSkip;
    rendererData->m_layers = faceCount;
    rendererData->m_mipCount = uploadMipCount;
    const NiPalette* palette = sourcePixels->GetPalette();
    rendererData->m_paletteRevision = palette ? palette->GetRevisionID() : 0;
    texture->SetRendererData(rendererData);
    NiLogWriteFormat(NI_LOG_TRACE, "NiBgfxRenderer", __FILE__, __LINE__,
        "Uploaded texture '%s' (%ux%u, faces=%u, mips=%u, skipped=%u, format=%s).",
        GetTextureDebugName(texture), uploadWidth, uploadHeight, faceCount,
        uploadMipCount, mipSkip, GetPixelFormatName(pixels->GetPixelFormat()));
    return true;
}

bool BgfxRenderer::CreateTextureFromContainerFile(NiSourceTexture* texture)
{
    if (!texture)
        return false;

    const char* filename = texture->GetPlatformSpecificFilename();
    if (!filename || !*filename)
        return false;

    std::ifstream input(filename, std::ios::binary | std::ios::ate);
    if (!input)
        return false;

    const std::streamoff end = input.tellg();
    if (end <= 0 || static_cast<unsigned long long>(end) >
        std::numeric_limits<std::uint32_t>::max())
    {
        return false;
    }

    std::vector<std::uint8_t> fileData(static_cast<size_t>(end));
    input.seekg(0, std::ios::beg);
    if (!input.read(reinterpret_cast<char*>(fileData.data()),
        static_cast<std::streamsize>(end)))
        return false;

    // bgfx's container loader parses DDS/KTX/PVR itself. This is important
    // for Texture2DArray DDS files because NiImageConverter/NiPixelData only
    // covers the legacy Gamebryo image path and cannot represent every modern
    // container format (Grand Fantasia's alpha array is BC4, for example).
    bgfx::TextureInfo info{};
    const bgfx::Memory* memory = bgfx::copy(fileData.data(),
        static_cast<std::uint32_t>(fileData.size()));
    const bgfx::TextureHandle handle = bgfx::createTexture(memory,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
        static_cast<std::uint8_t>(std::min<unsigned int>(m_mipmapSkip, 255u)),
        &info);

    if (!bgfx::isValid(handle))
        return false;

    if (info.width == 0 || info.height == 0 || info.numLayers == 0)
    {
        bgfx::destroy(handle);
        return false;
    }

    const NiPixelFormat& pixelFormat = GetNiPixelFormatForBgfx(info.format);
    TextureData* rendererData = NiNew TextureData(texture, handle, info.format,
        pixelFormat, true, info.width, info.height);
    rendererData->m_mipmapSkip = m_mipmapSkip;
    rendererData->m_layers = info.numLayers;
    rendererData->m_mipCount = info.numMips;
    texture->SetRendererData(rendererData);

    NiLogWriteFormat(NI_LOG_TRACE, "NiBgfxRenderer", __FILE__, __LINE__,
        "Loaded texture container '%s' directly through bgfx "
        "(%ux%u, layers=%u, mips=%u, format=%u).",
        GetTextureDebugName(texture),
        static_cast<unsigned int>(info.width),
        static_cast<unsigned int>(info.height),
        static_cast<unsigned int>(info.numLayers),
        static_cast<unsigned int>(info.numMips),
        static_cast<unsigned int>(info.format));
    return true;
}

bool BgfxRenderer::CreateSourceTextureRendererData(NiSourceTexture* texture)
{
    if (!texture)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "CreateSourceTextureRendererData received a null texture.", __FILE__, __LINE__);
        return false;
    }
    if (texture->GetRendererData())
        return true;

    // Match the old D3D renderer contract: when the source texture requests
    // direct-to-renderer loading, give the backend the original file before
    // forcing it through NiImageConverter. bgfx can parse DDS texture arrays
    // directly, including BC4 alpha arrays used by Grand Fantasia terrain.
    if (!texture->GetSourcePixelData() && texture->GetLoadDirectToRendererHint() &&
        CreateTextureFromContainerFile(texture))
    {
        return true;
    }

    if (!texture->GetSourcePixelData())
        texture->LoadPixelDataFromFile();

    if (texture->GetSourcePixelData())
        return CreateTextureFromPixelData(texture, texture->GetSourcePixelData(), false);

    // Also try the container path as a fallback. This covers external DDS
    // arrays even if callers did not explicitly set the direct-load hint.
    if (CreateTextureFromContainerFile(texture))
        return true;

    NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
        "Failed to load texture '%s' through both NiPixelData and bgfx's container loader.",
        GetTextureDebugName(texture));
    return false;
}

bool BgfxRenderer::CreateSourceCubeMapRendererData(NiSourceCubeMap* cubeMap)
{
    if (!cubeMap)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "CreateSourceCubeMapRendererData received a null cube map.", __FILE__, __LINE__);
        return false;
    }
    if (cubeMap->GetRendererData())
        return true;

    // Single-file DDS/KTX/PVR cubemaps can use the same direct container path.
    // Multi-file legacy cubemaps simply fall through to NiSourceCubeMap's
    // existing application-pixel-data loader.
    if (!cubeMap->GetSourcePixelData() && cubeMap->GetLoadDirectToRendererHint() &&
        CreateTextureFromContainerFile(cubeMap))
    {
        return true;
    }

    if (!cubeMap->GetSourcePixelData())
        cubeMap->LoadPixelDataFromFile();
    if (!cubeMap->GetSourcePixelData())
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to load source pixel data for cube map '%s'.",
            GetTextureDebugName(cubeMap));
        return false;
    }
    return CreateTextureFromPixelData(cubeMap, cubeMap->GetSourcePixelData(), true);
}

bool BgfxRenderer::CreateRenderedTextureRendererData(NiRenderedTexture* texture,
    Ni2DBuffer::MultiSamplePreference msaaPref)
{
    if (!texture || !texture->GetBuffer())
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "CreateRenderedTextureRendererData received invalid input (texture=%p buffer=%p).",
            static_cast<void*>(texture), texture ? static_cast<void*>(texture->GetBuffer()) : nullptr);
        return false;
    }

    if (texture->GetRendererData())
        return true;

    Ni2DBuffer* buffer = texture->GetBuffer();
    const bool depthTexture = NiIsKindOf(NiDepthStencilBuffer, buffer);

    // Preserve Gamebryo render-target precision.  LPP and VSM explicitly ask
    // for FLOAT_COLOR_64, which maps to RGBA16F; HDR users may request
    // FLOAT_COLOR_128/RGBA32F.  The old bring-up path forced every color
    // render target to RGBA8, silently truncating these buffers.
    NiTexture::FormatPrefs prefs = texture->GetFormatPreferences();
    const NiPixelFormat* pixelFormat = depthTexture ?
        &NiPixelFormat::STENCILDEPTH824 : FindClosestPixelFormat(prefs);
    if (!pixelFormat)
        pixelFormat = &NiPixelFormat::RGBA32;

    bgfx::TextureFormat::Enum bgfxFormat = depthTexture ?
        bgfx::TextureFormat::D24S8 : GetBgfxTextureFormat(*pixelFormat);
    if (bgfxFormat == bgfx::TextureFormat::Unknown)
    {
        pixelFormat = &NiPixelFormat::RGBA32;
        bgfxFormat = bgfx::TextureFormat::RGBA8;
    }
    uint64_t flags = RenderTargetFlags(msaaPref) |
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    if (!depthTexture)
        flags |= BGFX_TEXTURE_BLIT_DST;

    bgfx::TextureHandle handle = bgfx::createTexture2D(
        ClampCast<std::uint16_t>(texture->GetWidth()),
        ClampCast<std::uint16_t>(texture->GetHeight()), false, 1,
        bgfxFormat, flags);
    if (!bgfx::isValid(handle))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to create rendered texture '%s' (%ux%u, format=%s, bgfxFormat=%u, msaa=%u, depth=%s).",
            GetTextureDebugName(texture), texture->GetWidth(), texture->GetHeight(),
            GetPixelFormatName(*pixelFormat), static_cast<unsigned int>(bgfxFormat),
            static_cast<unsigned int>(msaaPref), depthTexture ? "true" : "false");
        return false;
    }

    texture->SetRendererData(NiNew TextureData(texture, handle, bgfxFormat, *pixelFormat));
    buffer->SetRendererData(NiNew BufferData(buffer, pixelFormat, handle, msaaPref));
    return true;
}

bool BgfxRenderer::CreateRenderedCubeMapRendererData(NiRenderedCubeMap* cubeMap)
{
    if (!cubeMap)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "CreateRenderedCubeMapRendererData received a null cube map.", __FILE__, __LINE__);
        return false;
    }
    if (cubeMap->GetRendererData())
        return true;

    NiTexture::FormatPrefs prefs = cubeMap->GetFormatPreferences();
    const NiPixelFormat* pixelFormat = FindClosestPixelFormat(prefs);
    if (!pixelFormat)
        pixelFormat = &NiPixelFormat::RGBA32;
    bgfx::TextureFormat::Enum bgfxFormat = GetBgfxTextureFormat(*pixelFormat);
    if (bgfxFormat == bgfx::TextureFormat::Unknown)
    {
        pixelFormat = &NiPixelFormat::RGBA32;
        bgfxFormat = bgfx::TextureFormat::RGBA8;
    }

    bgfx::TextureHandle handle = bgfx::createTextureCube(
        ClampCast<std::uint16_t>(cubeMap->GetWidth()), false, 1,
        bgfxFormat, BGFX_TEXTURE_RT | BGFX_TEXTURE_BLIT_DST |
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
    if (!bgfx::isValid(handle))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to create rendered cube map '%s' (size=%u, format=%s, bgfxFormat=%u).",
            GetTextureDebugName(cubeMap), cubeMap->GetWidth(),
            GetPixelFormatName(*pixelFormat), static_cast<unsigned int>(bgfxFormat));
        return false;
    }

    cubeMap->SetRendererData(NiNew TextureData(cubeMap, handle,
        bgfxFormat, *pixelFormat));

    for (unsigned int face = 0; face < NiRenderedCubeMap::FACE_NUM; ++face)
    {
        Ni2DBuffer* buffer = cubeMap->GetFaceBuffer(
            static_cast<NiRenderedCubeMap::FaceID>(face));
        if (!buffer)
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "Rendered cube map '%s' is missing face buffer %u.",
                GetTextureDebugName(cubeMap), face);
            NiTexture::RendererData* rendererData = cubeMap->GetRendererData();
            cubeMap->SetRendererData(nullptr);
            NiDelete rendererData;
            return false;
        }
        buffer->SetRendererData(NiNew BufferData(buffer, pixelFormat,
            handle, Ni2DBuffer::MULTISAMPLE_NONE, false,
            static_cast<std::uint16_t>(face)));
    }
    return true;
}

bool BgfxRenderer::CreateDynamicTextureRendererData(NiDynamicTexture* texture)
{
    if (!texture)
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "CreateDynamicTextureRendererData received a null texture.", __FILE__, __LINE__);
        return false;
    }
    if (texture->GetRendererData())
        return true;

    bgfx::TextureHandle handle = bgfx::createTexture2D(
        ClampCast<std::uint16_t>(texture->GetWidth()),
        ClampCast<std::uint16_t>(texture->GetHeight()), false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE);
    if (!bgfx::isValid(handle))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to create dynamic texture '%s' (%ux%u RGBA8).",
            GetTextureDebugName(texture), texture->GetWidth(), texture->GetHeight());
        return false;
    }

    TextureData* data = NiNew TextureData(texture, handle, bgfx::TextureFormat::RGBA8,
        NiPixelFormat::RGBA32);
    data->m_dynamic = true;
    data->m_pitch = texture->GetWidth() * 4;
    data->m_staging.resize(static_cast<size_t>(data->m_pitch) * texture->GetHeight());
    texture->SetRendererData(data);
    return true;
}

void BgfxRenderer::CreatePaletteRendererData(NiPalette*) {}

bool BgfxRenderer::CreateDepthStencilRendererData(NiDepthStencilBuffer* buffer,
    const NiPixelFormat* format, Ni2DBuffer::MultiSamplePreference msaaPref)
{
    if (!buffer || !format)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "CreateDepthStencilRendererData received invalid input (buffer=%p format=%p).",
            static_cast<void*>(buffer), static_cast<const void*>(format));
        return false;
    }
    if (buffer->GetRendererData())
        return true;

    bgfx::TextureHandle handle = bgfx::createTexture2D(
        ClampCast<std::uint16_t>(buffer->GetWidth()),
        ClampCast<std::uint16_t>(buffer->GetHeight()), false, 1,
        bgfx::TextureFormat::D24S8, RenderTargetFlags(msaaPref) |
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    if (!bgfx::isValid(handle))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to create depth/stencil texture %ux%u D24S8 (requested format=%s, msaa=%u).",
            buffer->GetWidth(), buffer->GetHeight(), GetPixelFormatName(*format),
            static_cast<unsigned int>(msaaPref));
        return false;
    }

    buffer->SetRendererData(NiNew BufferData(buffer, format, handle, msaaPref, true));
    return true;
}

void* BgfxRenderer::LockDynamicTexture(const NiTexture::RendererData* rendererData,
    int& pitch)
{
    TextureData* data = const_cast<TextureData*>(
        static_cast<const TextureData*>(rendererData));
    if (!data || !data->m_dynamic || data->m_staging.empty())
    {
        pitch = 0;
        return nullptr;
    }
    pitch = static_cast<int>(data->m_pitch);
    return data->m_staging.data();
}

bool BgfxRenderer::UnLockDynamicTexture(const NiTexture::RendererData* rendererData)
{
    TextureData* data = const_cast<TextureData*>(
        static_cast<const TextureData*>(rendererData));
    if (!data || !data->m_dynamic || !bgfx::isValid(data->m_handle))
        return false;

    bgfx::updateTexture2D(data->m_handle, 0, 0, 0, 0,
        ClampCast<std::uint16_t>(data->GetWidth()),
        ClampCast<std::uint16_t>(data->GetHeight()),
        bgfx::copy(data->m_staging.data(),
            static_cast<std::uint32_t>(data->m_staging.size())),
        static_cast<std::uint16_t>(data->m_pitch));
    return true;
}

NiShader* BgfxRenderer::GetFragmentShader(NiMaterialDescriptor* descriptor)
{
    return descriptor ? NiNew BgfxMaterialShader(descriptor, "BgfxFragmentShader") : nullptr;
}
void BgfxRenderer::SetDefaultProgramCache(NiFragmentMaterial*, bool, bool, bool,
    bool, const char*) {}
NiShader* BgfxRenderer::GetShadowWriteShader(NiMaterialDescriptor* descriptor)
{
    return descriptor ? NiNew BgfxMaterialShader(descriptor, "BgfxShadowWriteShader") : nullptr;
}
void BgfxRenderer::SetRenderShadowCasterBackfaces(bool renderBackfaces)
{
    m_renderShadowBackfaces = renderBackfaces;
}

void BgfxRenderer::SetRenderShadowTechnique(NiShadowTechnique* technique)
{
    m_shadowTechnique = technique;
}

#if defined(EE_ASSERTS_ARE_ENABLED)
void BgfxRenderer::EnforceModifierPoliciy(NiVisibleArray* array)
{
    if (!array)
        return;

    // Match the D3D11 renderer's debug policy check. BgfxRenderer submits
    // meshes directly rather than through a NiShader::Do_RenderMeshes path,
    // but SYNC_RENDER modifiers must still have been completed before GPU
    // data is consumed.
    NiSyncArgs syncArgs;
    syncArgs.m_uiSubmitPoint = NiSyncArgs::SYNC_ANY;
    syncArgs.m_uiCompletePoint = NiSyncArgs::SYNC_RENDER;

    const unsigned int objectCount = array->GetCount();
    for (unsigned int i = 0; i < objectCount; ++i)
    {
        NiMesh* mesh = NiDynamicCast(NiMesh, &array->GetAt(i));
        if (!mesh)
            continue;

        const unsigned int modifierCount = mesh->GetModifierCount();
        for (unsigned int j = 0; j < modifierCount; ++j)
        {
            NiMeshModifier* modifier = mesh->GetModifierAt(j);
            EE_ASSERT(!modifier || modifier->IsComplete(mesh, &syncArgs));
        }
    }
}
#endif

bool BgfxRenderer::Do_BeginFrame()
{
    if (!m_context.IsInitialized())
        return false;

    ++m_frameSerial;
    for (ParticleInstancePage& page : m_particleInstancePages)
        page.m_cursor = 0;

    // Mutable meshes use bgfx dynamic-buffer handles, whose pool is much
    // smaller than the number of meshes a large streamed scene can visit.
    // Reclaim caches that have left the visible set regularly instead of
    // retaining every dynamic handle for hundreds of frames.
    if ((m_frameSerial % 8u) == 0u)
        PurgeGpuMeshCache(false);

    // A bgfx View is a render pass. Reuse IDs from zero each Gamebryo frame,
    // but allocate a different View for every render-target pass so framebuffer
    // and camera state from one click cannot overwrite another.
    m_nextViewId = 0;
    m_viewId = 0;
    m_softParticleDepthViewActive = false;
    m_softParticleDepthClearedThisFrame = false;
    return true;
}

bool BgfxRenderer::Do_EndFrame()
{
#if defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
    // Keep particle-instancing diagnostics useful without producing one log
    // line per system per frame.  The counters aggregate 120 frames and only
    // emit while at least one facing-quad particle system was actually seen.
    if ((m_frameSerial % 120u) == 0u &&
        m_particleInstancingDebugStats.m_candidateChecks != 0u)
    {
        const ParticleInstancingDebugStats& stats =
            m_particleInstancingDebugStats;
        NiLogWriteFormat(NI_LOG_TRACE, "NiBgfxRenderer", __FILE__, __LINE__,
            "[ParticleInstancing] 120-frame summary: candidateChecks=%llu "
            "instancedBatches=%llu instancedParticles=%llu pages=%u "
            "flags{animatedTexture=%llu} "
            "fallbacks{renderer=%llu shadow=%llu layout=%llu wireframe=%llu "
            "missingData=%llu upload=%llu}.",
            static_cast<unsigned long long>(stats.m_candidateChecks),
            static_cast<unsigned long long>(stats.m_instancedBatches),
            static_cast<unsigned long long>(stats.m_instancedParticles),
            static_cast<unsigned int>(m_particleInstancePages.size()),
            static_cast<unsigned long long>(stats.m_animatedTextureFlagsSeen),
            static_cast<unsigned long long>(stats.m_rendererUnavailable),
            static_cast<unsigned long long>(stats.m_shadowFallbacks),
            static_cast<unsigned long long>(stats.m_layoutFallbacks),
            static_cast<unsigned long long>(stats.m_wireframeFallbacks),
            static_cast<unsigned long long>(stats.m_missingDataFallbacks),
            static_cast<unsigned long long>(stats.m_uploadFallbacks));

        m_particleInstancingDebugStats = ParticleInstancingDebugStats{};
    }
#endif
    return m_context.IsInitialized();
}

bool BgfxRenderer::Do_DisplayFrame()
{
    if (!m_context.IsInitialized())
        return false;
    m_context.Frame();
    return true;
}

void BgfxRenderer::Do_ClearBuffer(const NiRect<float>*, unsigned int clearMode)
{
    std::uint16_t flags = BGFX_CLEAR_NONE;
    if (clearMode & CLEAR_BACKBUFFER) flags |= BGFX_CLEAR_COLOR;
    if (clearMode & CLEAR_ZBUFFER) flags |= BGFX_CLEAR_DEPTH;
    if (clearMode & CLEAR_STENCIL) flags |= BGFX_CLEAR_STENCIL;

    bgfx::setViewClear(m_viewId, flags, ToBgfxClearColor(m_backgroundColor),
        m_depthClear, static_cast<std::uint8_t>(m_stencilClear));
    bgfx::touch(m_viewId);
}

void BgfxRenderer::Do_SetCameraData(const NiPoint3& worldLoc,
    const NiPoint3& worldDir, const NiPoint3& worldUp,
    const NiPoint3& worldRight, const NiFrustum& frustum,
    const NiRect<float>& port)
{
    m_worldLoc = worldLoc;
    m_worldDir = worldDir;
    m_worldUp = worldUp;
    m_worldRight = worldRight;
    m_frustum = frustum;
    m_viewport = port;

    const bx::Vec3 eye = { worldLoc.x, worldLoc.y, worldLoc.z };
    const bx::Vec3 at = { worldLoc.x + worldDir.x,
        worldLoc.y + worldDir.y, worldLoc.z + worldDir.z };
    const bx::Vec3 up = { worldUp.x, worldUp.y, worldUp.z };

    float view[16];
    bx::mtxLookAt(view, eye, at, up, bx::Handedness::Left);

    const bool homogeneousDepth = bgfx::getCaps()->homogeneousDepth;
    float proj[16];
    if (frustum.m_bOrtho)
    {
        float left = frustum.m_fLeft;
        float right = frustum.m_fRight;
        if (m_leftRightSwap)
            std::swap(left, right);
        bx::mtxOrtho(proj, left, right, frustum.m_fBottom, frustum.m_fTop,
            frustum.m_fNear, frustum.m_fFar, 0.0f, homogeneousDepth,
            bx::Handedness::Left);
    }
    else
    {
        const float nearPlane = frustum.m_fNear;
        float left = frustum.m_fLeft * nearPlane;
        float right = frustum.m_fRight * nearPlane;
        if (m_leftRightSwap)
            std::swap(left, right);
        bx::mtxProj(proj, frustum.m_fTop * nearPlane,
            frustum.m_fBottom * nearPlane, left, right,
            nearPlane, frustum.m_fFar, homogeneousDepth,
            bx::Handedness::Left);
    }

    bgfx::setViewTransform(m_viewId, view, proj);
    if (m_softParticleDepthViewActive)
        bgfx::setViewTransform(m_softParticleDepthViewId, view, proj);

    const NiRenderTargetGroup* target = m_pkCurrentRenderTargetGroup ?
        m_pkCurrentRenderTargetGroup : m_defaultTargetGroup.data();
    const unsigned int width = target ? target->GetWidth(0) : m_width;
    const unsigned int height = target ? target->GetHeight(0) : m_height;

    const float x = port.m_left * width;
    const float y = (1.0f - port.m_top) * height;
    const float w = (port.m_right - port.m_left) * width;
    const float h = (port.m_top - port.m_bottom) * height;
    const std::uint16_t viewX = ClampCast<std::uint16_t>(
        static_cast<unsigned int>(std::max(0.0f, x)));
    const std::uint16_t viewY = ClampCast<std::uint16_t>(
        static_cast<unsigned int>(std::max(0.0f, y)));
    const std::uint16_t viewW = ClampCast<std::uint16_t>(
        static_cast<unsigned int>(std::max(1.0f, w)));
    const std::uint16_t viewH = ClampCast<std::uint16_t>(
        static_cast<unsigned int>(std::max(1.0f, h)));
    bgfx::setViewRect(m_viewId, viewX, viewY, viewW, viewH);
    if (m_softParticleDepthViewActive)
        bgfx::setViewRect(m_softParticleDepthViewId, viewX, viewY, viewW, viewH);
}

void BgfxRenderer::Do_SetScreenSpaceCameraData(const NiRect<float>* port)
{
    NiRect<float> viewport = port ? *port : NiRect<float>(0.0f, 1.0f, 1.0f, 0.0f);
    NiFrustum frustum;
    frustum.m_fLeft = -0.5f;
    frustum.m_fRight = 0.5f;
    frustum.m_fTop = 0.5f;
    frustum.m_fBottom = -0.5f;
    frustum.m_fNear = 1.0f;
    frustum.m_fFar = 10000.0f;
    frustum.m_bOrtho = true;
    Do_SetCameraData(NiPoint3(0.5f, 0.5f, -1.0f), NiPoint3::UNIT_Z,
        NiPoint3(0.0f, -1.0f, 0.0f), NiPoint3::UNIT_X, frustum, viewport);
}

void BgfxRenderer::Do_GetCameraData(NiPoint3& worldLoc, NiPoint3& worldDir,
    NiPoint3& worldUp, NiPoint3& worldRight, NiFrustum& frustum,
    NiRect<float>& port)
{
    worldLoc = m_worldLoc;
    worldDir = m_worldDir;
    worldUp = m_worldUp;
    worldRight = m_worldRight;
    frustum = m_frustum;
    port = m_viewport;
}

bool BgfxRenderer::Do_BeginUsingRenderTargetGroup(NiRenderTargetGroup* target,
    unsigned int clearMode)
{
    if (!ValidateRenderTargetGroup(target))
        return false;

    const bool isDefaultTarget = target == m_defaultTargetGroup;
    m_softParticleDepthViewActive = false;

    // Keep the default back-buffer clear in a dedicated bgfx view. Views are
    // executed in ascending ViewId order, so this guarantees that the clear
    // happens before every Gamebryo draw pass and before CEGUI's reserved
    // 240-255 views. It also prevents later camera/framebuffer setup from
    // accidentally changing the view that owns the clear operation.
    if (isDefaultTarget && clearMode != CLEAR_NONE)
    {
        bgfx::ViewId clearView = 0;
        if (!AllocateAuxiliaryView(clearView, "NiBgfx Default Backbuffer Clear"))
            return false;

        bgfx::setViewFrameBuffer(clearView, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(clearView, 0, 0,
            ClampCast<std::uint16_t>(target->GetWidth(0)),
            ClampCast<std::uint16_t>(target->GetHeight(0)));

        std::uint16_t clearFlags = BGFX_CLEAR_NONE;
        if (clearMode & CLEAR_BACKBUFFER) clearFlags |= BGFX_CLEAR_COLOR;
        if (clearMode & CLEAR_ZBUFFER) clearFlags |= BGFX_CLEAR_DEPTH;
        if (clearMode & CLEAR_STENCIL) clearFlags |= BGFX_CLEAR_STENCIL;

        const std::uint32_t clearColor = ToBgfxClearColor(m_backgroundColor);
        bgfx::setViewClear(clearView, clearFlags, clearColor, m_depthClear,
            static_cast<std::uint8_t>(m_stencilClear));
        bgfx::touch(clearView);
    }

    // The default swap-chain depth buffer cannot be sampled by bgfx. When
    // soft particles are enabled, reserve an earlier view that mirrors only
    // depth-writing scene geometry into a tiny-purpose R32F linear-depth
    // target. The regular scene still renders directly to the swap chain.
    if (isDefaultTarget && m_softParticlesEnabled && GetSoftParticlesSupported())
    {
        if (!AllocateAuxiliaryView(m_softParticleDepthViewId,
            "NiBgfx Soft Particle Depth"))
        {
            return false;
        }
        m_softParticleDepthViewActive = true;
        bgfx::setViewFrameBuffer(m_softParticleDepthViewId,
            m_softParticleDepthFrameBuffer);
        bgfx::setViewRect(m_softParticleDepthViewId, 0, 0,
            ClampCast<std::uint16_t>(target->GetWidth(0)),
            ClampCast<std::uint16_t>(target->GetHeight(0)));

        const bool clearSoftDepth = !m_softParticleDepthClearedThisFrame ||
            (clearMode & CLEAR_ZBUFFER) != 0;
        if (clearSoftDepth)
        {
            // White means normalized far depth (1.0). The real depth buffer is
            // cleared normally so the nearest mirrored surface wins.
            bgfx::setViewClear(m_softParticleDepthViewId,
                BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                0xffffffffu, 1.0f, 0);
            bgfx::touch(m_softParticleDepthViewId);
            m_softParticleDepthClearedThisFrame = true;
        }
    }

    if (!AllocateView(isDefaultTarget ?
        "NiBgfx Default Backbuffer" : "NiBgfx Render Target"))
    {
        return false;
    }

    if (isDefaultTarget)
    {
        bgfx::setViewFrameBuffer(m_viewId, BGFX_INVALID_HANDLE);
    }
    else
    {
        const TargetGroupData* targetData =
            static_cast<const TargetGroupData*>(target->GetRendererData());
        if (!targetData)
        {
            std::vector<bgfx::Attachment> attachments;
            attachments.resize(target->GetBufferCount() +
                (target->HasDepthStencil() ? 1u : 0u));
            unsigned int attachmentIndex = 0;
            for (unsigned int i = 0; i < target->GetBufferCount(); ++i)
            {
                BufferData* data = GetBufferData(target->GetBuffer(i));
                if (!data || !bgfx::isValid(data->m_handle))
                {
                    NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                        "Cannot bind render-target group: color attachment %u has no valid bgfx texture handle.", i);
                    return false;
                }
                attachments[attachmentIndex++].init(data->m_handle, bgfx::Access::Write,
                    data->m_layer, 1, 0, BGFX_RESOLVE_NONE);
            }
            if (target->HasDepthStencil())
            {
                BufferData* depthData = GetBufferData(target->GetDepthStencilBuffer());
                if (!depthData || !bgfx::isValid(depthData->m_handle))
                {
                    NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
                        "Cannot bind render-target group: depth/stencil attachment has no valid bgfx texture handle.",
                        __FILE__, __LINE__);
                    return false;
                }
                attachments[attachmentIndex++].init(depthData->m_handle, bgfx::Access::Write,
                    depthData->m_layer, 1, 0, BGFX_RESOLVE_NONE);
            }

            bgfx::FrameBufferHandle frameBuffer = bgfx::createFrameBuffer(
                static_cast<std::uint8_t>(attachments.size()), attachments.data(), false);
            if (!bgfx::isValid(frameBuffer))
            {
                NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                    "bgfx::createFrameBuffer failed for render-target group %ux%u with %u attachment(s).",
                    target->GetWidth(0), target->GetHeight(0),
                    static_cast<unsigned int>(attachments.size()));
                return false;
            }

            TargetGroupData* newTargetData = NiNew TargetGroupData(frameBuffer);
            target->SetRendererData(newTargetData);
            targetData = newTargetData;
        }
        bgfx::setViewFrameBuffer(m_viewId, targetData->m_handle);
    }

    bgfx::setViewRect(m_viewId, 0, 0,
        ClampCast<std::uint16_t>(target->GetWidth(0)),
        ClampCast<std::uint16_t>(target->GetHeight(0)));

    // The default target's requested clear was already submitted through its
    // dedicated lower-numbered clear view above. Offscreen targets still clear
    // their own draw view because their attachments are unique to that pass.
    if (!isDefaultTarget)
        Do_ClearBuffer(nullptr, clearMode);

    return true;
}

bool BgfxRenderer::Do_EndUsingRenderTargetGroup()
{
    return true;
}

uint64_t BgfxRenderer::BuildRenderState(bool shadowWrite) const
{
    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

    const NiZBufferProperty* zBuffer = m_pkCurrProp ? m_pkCurrProp->GetZBuffer() : nullptr;
    if (!zBuffer)
        zBuffer = NiZBufferProperty::GetDefault();
    if (zBuffer)
    {
        if (zBuffer->GetZBufferTest())
            state |= DepthTestState(zBuffer->GetTestFunction());
        if (zBuffer->GetZBufferWrite())
            state |= BGFX_STATE_WRITE_Z;
    }

    const NiAlphaProperty* alpha = m_pkCurrProp ? m_pkCurrProp->GetAlpha() : nullptr;
    if (!alpha)
        alpha = NiAlphaProperty::GetDefault();
    if (alpha && alpha->GetAlphaBlending())
        state |= BGFX_STATE_BLEND_FUNC(AlphaBlendFactor(alpha->GetSrcBlendMode()),
            AlphaBlendFactor(alpha->GetDestBlendMode()));

    const NiStencilProperty* stencil = m_pkCurrProp ? m_pkCurrProp->GetStencil() : nullptr;
    if (!stencil)
        stencil = NiStencilProperty::GetDefault();
    if (stencil && !(shadowWrite && m_renderShadowBackfaces))
    {
        switch (stencil->GetDrawMode())
        {
        // Match the legacy D3D11 renderer exactly. DRAW_CCW_OR_BOTH is the
        // default Gamebryo mode, but on D3D11 it behaves as DRAW_CCW (back-face
        // culling), not as a two-sided material. Falling through to no culling
        // here made nearly every ordinary mesh double-sided in the bgfx port.
        case NiStencilProperty::DRAW_CCW_OR_BOTH:
        case NiStencilProperty::DRAW_CCW:
            state |= BGFX_STATE_CULL_CCW;
            break;
        case NiStencilProperty::DRAW_CW:
            state |= BGFX_STATE_CULL_CW;
            break;
        case NiStencilProperty::DRAW_BOTH:
            break;
        default:
            state |= BGFX_STATE_CULL_CCW;
            break;
        }
    }

    // Wireframe is handled in Do_RenderMesh by generating a line-list index
    // stream. There is no portable polygon fill-mode flag in bgfx state.

    return state;
}

uint32_t BgfxRenderer::BuildStencilState() const
{
    const NiStencilProperty* stencil = m_pkCurrProp ? m_pkCurrProp->GetStencil() : nullptr;
    if (!stencil)
        stencil = NiStencilProperty::GetDefault();
    if (!stencil || !stencil->GetStencilOn())
        return BGFX_STENCIL_NONE;

    uint32_t state = BGFX_STENCIL_FUNC_REF(stencil->GetStencilReference() & 0xffu) |
        BGFX_STENCIL_FUNC_RMASK(stencil->GetStencilMask() & 0xffu);

    switch (stencil->GetStencilFunction())
    {
    case NiStencilProperty::TEST_NEVER:        state |= BGFX_STENCIL_TEST_NEVER; break;
    case NiStencilProperty::TEST_LESS:         state |= BGFX_STENCIL_TEST_LESS; break;
    case NiStencilProperty::TEST_EQUAL:        state |= BGFX_STENCIL_TEST_EQUAL; break;
    case NiStencilProperty::TEST_LESSEQUAL:    state |= BGFX_STENCIL_TEST_LEQUAL; break;
    case NiStencilProperty::TEST_GREATER:      state |= BGFX_STENCIL_TEST_GREATER; break;
    case NiStencilProperty::TEST_NOTEQUAL:     state |= BGFX_STENCIL_TEST_NOTEQUAL; break;
    case NiStencilProperty::TEST_GREATEREQUAL: state |= BGFX_STENCIL_TEST_GEQUAL; break;
    case NiStencilProperty::TEST_ALWAYS:       state |= BGFX_STENCIL_TEST_ALWAYS; break;
    default:                                   state |= BGFX_STENCIL_TEST_ALWAYS; break;
    }

    const auto failOp = [](NiStencilProperty::Action action) -> uint32_t
    {
        switch (action)
        {
        case NiStencilProperty::ACTION_ZERO:      return BGFX_STENCIL_OP_FAIL_S_ZERO;
        case NiStencilProperty::ACTION_REPLACE:   return BGFX_STENCIL_OP_FAIL_S_REPLACE;
        case NiStencilProperty::ACTION_INCREMENT: return BGFX_STENCIL_OP_FAIL_S_INCRSAT;
        case NiStencilProperty::ACTION_DECREMENT: return BGFX_STENCIL_OP_FAIL_S_DECRSAT;
        case NiStencilProperty::ACTION_INVERT:    return BGFX_STENCIL_OP_FAIL_S_INVERT;
        case NiStencilProperty::ACTION_KEEP:
        default:                                  return BGFX_STENCIL_OP_FAIL_S_KEEP;
        }
    };
    const auto zFailOp = [](NiStencilProperty::Action action) -> uint32_t
    {
        switch (action)
        {
        case NiStencilProperty::ACTION_ZERO:      return BGFX_STENCIL_OP_FAIL_Z_ZERO;
        case NiStencilProperty::ACTION_REPLACE:   return BGFX_STENCIL_OP_FAIL_Z_REPLACE;
        case NiStencilProperty::ACTION_INCREMENT: return BGFX_STENCIL_OP_FAIL_Z_INCRSAT;
        case NiStencilProperty::ACTION_DECREMENT: return BGFX_STENCIL_OP_FAIL_Z_DECRSAT;
        case NiStencilProperty::ACTION_INVERT:    return BGFX_STENCIL_OP_FAIL_Z_INVERT;
        case NiStencilProperty::ACTION_KEEP:
        default:                                  return BGFX_STENCIL_OP_FAIL_Z_KEEP;
        }
    };
    const auto passOp = [](NiStencilProperty::Action action) -> uint32_t
    {
        switch (action)
        {
        case NiStencilProperty::ACTION_ZERO:      return BGFX_STENCIL_OP_PASS_Z_ZERO;
        case NiStencilProperty::ACTION_REPLACE:   return BGFX_STENCIL_OP_PASS_Z_REPLACE;
        case NiStencilProperty::ACTION_INCREMENT: return BGFX_STENCIL_OP_PASS_Z_INCRSAT;
        case NiStencilProperty::ACTION_DECREMENT: return BGFX_STENCIL_OP_PASS_Z_DECRSAT;
        case NiStencilProperty::ACTION_INVERT:    return BGFX_STENCIL_OP_PASS_Z_INVERT;
        case NiStencilProperty::ACTION_KEEP:
        default:                                  return BGFX_STENCIL_OP_PASS_Z_KEEP;
        }
    };

    state |= failOp(stencil->GetStencilFailAction());
    state |= zFailOp(stencil->GetStencilPassZFailAction());
    state |= passOp(stencil->GetStencilPassAction());
    return state;
}

void BgfxRenderer::SetModelTransform(const NiTransform& transform) const
{
    const float s = transform.m_fScale;
    float matrix[16] = {
        transform.m_Rotate.GetEntry(0, 0) * s,
        transform.m_Rotate.GetEntry(1, 0) * s,
        transform.m_Rotate.GetEntry(2, 0) * s,
        0.0f,
        transform.m_Rotate.GetEntry(0, 1) * s,
        transform.m_Rotate.GetEntry(1, 1) * s,
        transform.m_Rotate.GetEntry(2, 1) * s,
        0.0f,
        transform.m_Rotate.GetEntry(0, 2) * s,
        transform.m_Rotate.GetEntry(1, 2) * s,
        transform.m_Rotate.GetEntry(2, 2) * s,
        0.0f,
        transform.m_Translate.x,
        transform.m_Translate.y,
        transform.m_Translate.z,
        1.0f
    };
    bgfx::setTransform(matrix);
}

bool BgfxRenderer::AllocateAuxiliaryView(bgfx::ViewId& viewId,
    const char* name)
{
    // Views 240-255 are intentionally reserved for external renderers such as
    // CEGUI. The CEGUI bgfx renderer uses view 240 as its default base and may
    // consume up to 16 views, so Gamebryo must never allocate into that range.
    // Auxiliary blit/readback passes deliberately do not replace m_viewId, so
    // a copy cannot corrupt a still-active render pass's framebuffer/camera
    // state.
    constexpr std::uint16_t kFirstExternalView = 240;
    if (m_nextViewId >= kFirstExternalView)
    {
        Warning("BgfxRenderer: exhausted reserved Gamebryo bgfx views (0-239) for the current frame; views 240-255 are reserved for external renderers.");
        return false;
    }

    viewId = static_cast<bgfx::ViewId>(m_nextViewId++);
    bgfx::resetView(viewId);
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
    if (name && *name)
        bgfx::setViewName(viewId, name);
    return true;
}

bool BgfxRenderer::AllocateView(const char* name)
{
    bgfx::ViewId viewId = 0;
    if (!AllocateAuxiliaryView(viewId, name))
        return false;
    m_viewId = viewId;
    return true;
}

bool BgfxRenderer::IsMeshGpuCacheable(const NiMesh* mesh) const
{
    if (!mesh || mesh->GetInputDataIsFromStreamOut())
        return false;

    const unsigned int streamCount = mesh->GetStreamRefCount();
    for (unsigned int i = 0; i < streamCount; ++i)
    {
        const NiDataStreamRef* ref = mesh->GetStreamRefAt(i);
        if (!ref || ref->IsPerInstance())
            continue;

        const NiDataStream* stream = ref->GetDataStream();
        if (!stream)
            return false;

        const NiDataStream::Usage usage = stream->GetUsage();
        if (usage != NiDataStream::USAGE_VERTEX &&
            usage != NiDataStream::USAGE_VERTEX_INDEX)
        {
            continue;
        }

        const NiUInt8 access = stream->GetAccessMask();
        if ((access & (NiDataStream::ACCESS_CPU_WRITE_MUTABLE |
            NiDataStream::ACCESS_CPU_WRITE_VOLATILE |
            NiDataStream::ACCESS_GPU_WRITE)) != 0)
        {
            return false;
        }

        // A static stream becomes immutable only after its one legal write
        // has completed. Caching it before that point would freeze
        // uninitialized/partially initialized contents on the GPU.
        if ((access & NiDataStream::ACCESS_CPU_WRITE_STATIC) != 0 &&
            (access & NiDataStream::ACCESS_CPU_WRITE_STATIC_INITIALIZED) == 0)
        {
            return false;
        }
    }

    return true;
}

std::uint64_t BgfxRenderer::BuildMeshCacheSignature(const NiMesh* mesh,
    unsigned int submesh) const
{
    if (!mesh)
        return 0;

    // FNV-1a over the stream topology. Static data streams are immutable once
    // initialized, so pointer/shape/semantic identity is sufficient to detect
    // rebinding, region changes, and allocator-address reuse without hashing
    // megabytes of vertex data every draw.
    std::uint64_t hash = 1469598103934665603ull;
    const auto mixBytes = [&hash](const void* data, size_t size)
    {
        const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
        for (size_t i = 0; i < size; ++i)
        {
            hash ^= static_cast<std::uint64_t>(bytes[i]);
            hash *= 1099511628211ull;
        }
    };
    const auto mixValue = [&mixBytes](const auto& value)
    {
        mixBytes(&value, sizeof(value));
    };

    const auto primitive = mesh->GetPrimitiveType();
    const unsigned int submeshCount = mesh->GetSubmeshCount();
    const unsigned int streamCount = mesh->GetStreamRefCount();
    mixValue(primitive);
    mixValue(submesh);
    mixValue(submeshCount);
    mixValue(streamCount);

    for (unsigned int i = 0; i < streamCount; ++i)
    {
        const NiDataStreamRef* ref = mesh->GetStreamRefAt(i);
        const std::uintptr_t refAddress = reinterpret_cast<std::uintptr_t>(ref);
        mixValue(refAddress);
        if (!ref)
            continue;

        const bool perInstance = ref->IsPerInstance();
        mixValue(perInstance);
        if (perInstance)
            continue;

        const NiDataStream* stream = ref->GetDataStream();
        const std::uintptr_t streamAddress =
            reinterpret_cast<std::uintptr_t>(stream);
        mixValue(streamAddress);
        if (!stream)
            continue;

        const auto usage = stream->GetUsage();
        const NiUInt8 access = stream->GetAccessMask();
        const NiUInt32 stride = stream->GetStride();
        const NiUInt32 size = stream->GetSize();
        const NiUInt32 elementCount = ref->GetElementDescCount();
        mixValue(usage);
        mixValue(access);
        mixValue(stride);
        mixValue(size);
        mixValue(elementCount);

        if (submesh < ref->GetSubmeshRemapCount())
        {
            const NiDataStream::Region& region = ref->GetRegionForSubmesh(submesh);
            const NiUInt32 start = region.GetStartIndex();
            const NiUInt32 range = region.GetRange();
            mixValue(start);
            mixValue(range);
        }

        for (NiUInt32 element = 0; element < elementCount; ++element)
        {
            const auto format = ref->GetElementDescAt(element).GetFormat();
            const NiUInt32 semanticIndex = ref->GetSemanticIndexAt(element);
            const NiFixedString& semantic = ref->GetSemanticNameAt(element);
            mixValue(format);
            mixValue(semanticIndex);
            const char* text = semantic.c_str();
            if (text)
                mixBytes(text, semantic.GetLength());
            const std::uint8_t separator = 0xffu;
            mixValue(separator);
        }
    }

    return hash == 0 ? 1 : hash;
}

std::uint64_t BgfxRenderer::BuildMeshDataRevision(const NiMesh* mesh,
    unsigned int usage) const
{
    if (!mesh)
        return 0;

    // Hash only runtime content revisions. Legacy meshes are frequently
    // marked CPU_WRITE_MUTABLE even though their data stays unchanged for
    // long periods. This lets dynamic bgfx buffers remain cached until an
    // actual write lock is released.
    std::uint64_t hash = 1469598103934665603ull;
    bool found = false;
    const unsigned int streamCount = mesh->GetStreamRefCount();
    for (unsigned int i = 0; i < streamCount; ++i)
    {
        const NiDataStreamRef* ref = mesh->GetStreamRefAt(i);
        if (!ref || ref->IsPerInstance())
            continue;

        const NiDataStream* stream = ref->GetDataStream();
        if (!stream || static_cast<unsigned int>(stream->GetUsage()) != usage)
            continue;

        found = true;
        const std::uint64_t address = static_cast<std::uint64_t>(
            reinterpret_cast<std::uintptr_t>(stream));
        const std::uint64_t revision = stream->GetRevisionID();
        hash ^= address + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
        hash ^= revision + 0xc2b2ae3d27d4eb4full + (hash << 6) + (hash >> 2);
    }

    return found ? (hash == 0 ? 1 : hash) : 0;
}

BgfxRenderer::MeshCache* BgfxRenderer::GetOrCreateMeshCache(NiMesh* mesh)
{
    // Stream-out meshes are produced by the GPU and cannot be repacked from
    // their CPU streams here. Every other mesh gets a cache entry. Immutable
    // meshes use static bgfx buffers. Mutable/volatile meshes also start
    // static and are promoted to dynamic only after a stream revision really
    // changes. This avoids consuming bgfx's finite transient arenas and its
    // finite dynamic-handle pool for legacy meshes that never actually mutate.
    if (!mesh || mesh->GetInputDataIsFromStreamOut())
        return nullptr;

    auto found = m_meshCache.find(mesh);
    MeshCache* cache = found != m_meshCache.end() ? found->second : nullptr;
    if (!cache)
    {
        cache = NiNew MeshCache();
        m_meshCache[mesh] = cache;
    }

    const unsigned int submeshCount = mesh->GetSubmeshCount();
    if (cache->m_submeshes.size() < submeshCount)
        cache->m_submeshes.resize(submeshCount);
    cache->m_shortLived = !IsMeshGpuCacheable(mesh);
    cache->m_lastUsedFrame = m_frameSerial;
    return cache;
}

void BgfxRenderer::PurgeGpuMeshCache(bool forceAll)
{
    // Static buffers are cheap to retain because revisiting them avoids CPU
    // repacking. Dynamic-buffer handles are a fixed bgfx resource pool
    // (4096 by default), so keeping off-screen mutable meshes alive for the
    // same 600-frame lifetime eventually exhausts the pool while moving
    // through a large scene. Keep only a short grace period for dynamic
    // caches; currently visible caches have m_lastUsedFrame == m_frameSerial
    // and are never removed here.
    constexpr std::uint64_t kStaticUnusedFrameLifetime = 600;
    constexpr std::uint64_t kDynamicUnusedFrameLifetime = 16;

    for (auto it = m_meshCache.begin(); it != m_meshCache.end(); )
    {
        MeshCache* cache = it->second;
        const std::uint64_t lifetime = cache &&
            (cache->m_shortLived || cache->HasDynamicBuffers()) ?
            kDynamicUnusedFrameLifetime : kStaticUnusedFrameLifetime;
        const bool expired = !cache ||
            (m_frameSerial > cache->m_lastUsedFrame &&
             m_frameSerial - cache->m_lastUsedFrame > lifetime);
        if (forceAll || expired)
        {
            NiDelete cache;
            it = m_meshCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool BgfxRenderer::EnsureTexture(NiTexture* texture)
{
    if (!texture)
        return false;

    // Source textures can be edited in-place (terrain painting, tools, etc.).
    // Legacy D3D renderer data tracked NiPixelData/Palette revisions and
    // refreshed the GPU copy. Do the same here; recreating is intentionally
    // conservative and also handles format/mipmap changes safely.
    if (texture->GetRendererData() &&
        (NiIsKindOf(NiSourceTexture, texture) ||
         NiIsKindOf(NiSourceCubeMap, texture)))
    {
        NiPixelData* sourcePixels = nullptr;
        if (NiIsKindOf(NiSourceCubeMap, texture))
        {
            NiSourceCubeMap* sourceCube = static_cast<NiSourceCubeMap*>(texture);
            sourcePixels = sourceCube->GetSourcePixelData();
        }
        else
        {
            NiSourceTexture* sourceTexture = static_cast<NiSourceTexture*>(texture);
            sourcePixels = sourceTexture->GetSourcePixelData();
        }

        TextureData* data = GetTextureData(texture);
        if (!data)
            return false;

        // Static NiSourceTexture instances intentionally discard application
        // pixel data after their renderer copy is created. Reloading the file
        // here turns every material bind into disk/image-decoder work. If the
        // source pixels are not resident, the existing renderer data remains
        // authoritative until the texture is explicitly released/recreated.
        if (!sourcePixels)
            return true;

        const NiPalette* palette = sourcePixels->GetPalette();
        const unsigned int paletteRevision = palette ? palette->GetRevisionID() : 0;
        if (data->m_sourceRevision == sourcePixels->GetRevisionID() &&
            data->m_paletteRevision == paletteRevision &&
            data->m_mipmapSkip == std::min(m_mipmapSkip,
                sourcePixels->GetNumMipmapLevels() > 0 ?
                    sourcePixels->GetNumMipmapLevels() - 1 : 0))
        {
            return true;
        }

        NiTexture::RendererData* staleRendererData = texture->GetRendererData();
        texture->SetRendererData(nullptr);
        NiDelete staleRendererData;
    }
    else if (texture->GetRendererData())
    {
        return true;
    }

    if (NiIsKindOf(NiSourceCubeMap, texture))
        return CreateSourceCubeMapRendererData(static_cast<NiSourceCubeMap*>(texture));
    if (NiIsKindOf(NiSourceTexture, texture))
        return CreateSourceTextureRendererData(static_cast<NiSourceTexture*>(texture));
    if (NiIsKindOf(NiDynamicTexture, texture))
        return CreateDynamicTextureRendererData(static_cast<NiDynamicTexture*>(texture));
    if (NiIsKindOf(NiRenderedTexture, texture))
        return CreateRenderedTextureRendererData(static_cast<NiRenderedTexture*>(texture));
    return false;
}

void BgfxRenderer::BindMaterialAndTexture(NiMesh* mesh)
{
    m_currentPssmActive = false;
    m_currentPssmTexture = BGFX_INVALID_HANDLE;
    m_currentPssmSamplerFlags = BGFX_SAMPLER_NONE;
    m_currentTerrainShadowTexture = BGFX_INVALID_HANDLE;
    m_currentTerrainShadowSamplerFlags = BGFX_SAMPLER_NONE;
    m_currentTerrainShadowLightIndex = -1;
    m_currentTerrainShadowCube = false;

    enum StandardMapSlot : unsigned int
    {
        MAP_BASE = 0,
        MAP_DARK,
        MAP_DETAIL,
        MAP_GLOSS,
        MAP_GLOW,
        MAP_BUMP,
        MAP_NORMAL,
        MAP_PARALLAX,
        MAP_DECAL0,
        MAP_DECAL1,
        MAP_DECAL2
    };

    const NiMaterial* activeMaterial = mesh ? mesh->GetActiveMaterial() : nullptr;
    const NiFixedString activeMaterialName = activeMaterial ?
        activeMaterial->GetName() : NiFixedString();
    const bool extendedMaterial = activeMaterialName == "NiExtendedMaterial";
    const bool decorationMaterial =
        activeMaterialName == "NiDecorationMaterial" ||
        activeMaterialName == "NiLPPDecorationFinalMaterial" ||
        activeMaterialName == "NiLPPDecorationDepthNormalMaterial";

    const NiTexturingProperty* texturing =
        m_pkCurrProp ? m_pkCurrProp->GetTexturing() : nullptr;

    std::array<const NiTexturingProperty::Map*, MAX_STANDARD_MAPS> maps{};
    if (texturing)
    {
        maps[MAP_BASE] = texturing->GetBaseMap();
        maps[MAP_DARK] = texturing->GetDarkMap();
        maps[MAP_DETAIL] = texturing->GetDetailMap();
        maps[MAP_GLOSS] = texturing->GetGlossMap();
        maps[MAP_GLOW] = texturing->GetGlowMap();
        maps[MAP_BUMP] = texturing->GetBumpMap();
        maps[MAP_NORMAL] = texturing->GetNormalMap();
        maps[MAP_PARALLAX] = texturing->GetParallaxMap();
        for (unsigned int i = 0; i < 3 && i < texturing->GetDecalMapCount(); ++i)
            maps[MAP_DECAL0 + i] = texturing->GetDecalMap(i);
    }

    std::array<NiBgfxMath::Vec4, MAX_STANDARD_MAPS> mapParams{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_MAPS> mapTransform0{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_MAPS> mapTransform1{};

    for (unsigned int i = 0; i < MAX_STANDARD_MAPS; ++i)
    {
        mapTransform0[i] = { 1.0f, 0.0f, 0.0f, 0.0f };
        mapTransform1[i] = { 0.0f, 1.0f, 0.0f, 0.0f };

        const NiTexturingProperty::Map* map = maps[i];
        bgfx::TextureHandle handle = m_whiteTexture;
        if (i == MAP_GLOW || i == MAP_PARALLAX)
            handle = m_blackTexture;
        else if (i == MAP_BUMP || i == MAP_NORMAL)
            handle = m_flatNormalTexture;

        bool enabled = false;
        if (map && map->GetTexture() && EnsureTexture(map->GetTexture()))
        {
            TextureData* data = GetTextureData(map->GetTexture());
            if (data && bgfx::isValid(data->m_handle))
            {
                handle = data->m_handle;
                enabled = true;
            }
        }

        // bgfx exposes TEXCOORD0..7. Legacy assets using a higher set keep
        // rendering deterministically by falling back to TEXCOORD0.
        const unsigned int uvSet = map ? map->GetTextureIndex() : 0;
        mapParams[i] = {
            enabled ? 1.0f : 0.0f,
            static_cast<float>(uvSet < 8 ? uvSet : 0),
            0.0f,
            0.0f
        };

        if (map && map->GetTextureTransform())
        {
            const NiMatrix3* transform = map->GetTextureTransform()->GetMatrix();
            if (transform)
            {
                mapTransform0[i][0] = transform->GetEntry(0, 0);
                mapTransform0[i][1] = transform->GetEntry(0, 1);
                mapTransform0[i][2] = transform->GetEntry(0, 2);
                mapTransform1[i][0] = transform->GetEntry(1, 0);
                mapTransform1[i][1] = transform->GetEntry(1, 1);
                mapTransform1[i][2] = transform->GetEntry(1, 2);
            }
        }

        bgfx::setTexture(static_cast<std::uint8_t>(i),
            m_textureUniforms[i], handle, SamplerFlags(map));
    }

    bgfx::setUniform(m_mapParamsUniform, mapParams.data(), MAX_STANDARD_MAPS);
    bgfx::setUniform(m_mapTransform0Uniform, mapTransform0.data(), MAX_STANDARD_MAPS);
    bgfx::setUniform(m_mapTransform1Uniform, mapTransform1.data(), MAX_STANDARD_MAPS);

    float bumpParams[4] = { 1.0f, 0.0f, 0.05f, 0.0f };
    if (texturing && texturing->GetBumpMap())
    {
        const NiTexturingProperty::BumpMap* bump = texturing->GetBumpMap();
        bumpParams[0] = bump->GetLumaScale();
        bumpParams[1] = bump->GetLumaOffset();
    }
    if (texturing && texturing->GetParallaxMap())
        bumpParams[2] = texturing->GetParallaxMap()->GetOffset();
    bgfx::setUniform(m_bumpParamsUniform, bumpParams);

    const NiMaterialProperty* material =
        m_pkCurrProp ? m_pkCurrProp->GetMaterial() : nullptr;
    if (!material)
        material = NiMaterialProperty::GetDefault();

    float matAmbient[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float matDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float matSpecular[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float matEmissive[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (material)
    {
        const NiColor& ambient = material->GetAmbientColor();
        const NiColor& diffuse = material->GetDiffuseColor();
        const NiColor& specular = material->GetSpecularColor();
        const NiColor& emissive = material->GetEmittance();
        const float alpha = material->GetAlpha();
        matAmbient[0] = ambient.r; matAmbient[1] = ambient.g; matAmbient[2] = ambient.b; matAmbient[3] = alpha;
        matDiffuse[0] = diffuse.r; matDiffuse[1] = diffuse.g; matDiffuse[2] = diffuse.b; matDiffuse[3] = alpha;
        matSpecular[0] = specular.r; matSpecular[1] = specular.g; matSpecular[2] = specular.b;
        matSpecular[3] = std::max(material->GetShineness(), 1.0f);
        matEmissive[0] = emissive.r; matEmissive[1] = emissive.g; matEmissive[2] = emissive.b; matEmissive[3] = alpha;
    }
    bgfx::setUniform(m_materialAmbientUniform, matAmbient);
    bgfx::setUniform(m_materialDiffuseUniform, matDiffuse);
    bgfx::setUniform(m_materialSpecularUniform, matSpecular);
    bgfx::setUniform(m_materialEmissiveUniform, matEmissive);

    const NiVertexColorProperty* vertexColor =
        m_pkCurrProp ? m_pkCurrProp->GetVertexColor() : nullptr;
    if (!vertexColor)
        vertexColor = NiVertexColorProperty::GetDefault();

    const NiSpecularProperty* specular =
        m_pkCurrProp ? m_pkCurrProp->GetSpecular() : nullptr;
    if (!specular)
        specular = NiSpecularProperty::GetDefault();

    float textureParams[4] = {
        static_cast<float>(texturing ? texturing->GetApplyMode() :
            NiTexturingProperty::APPLY_MODULATE),
        static_cast<float>(vertexColor ? vertexColor->GetSourceMode() :
            NiVertexColorProperty::SOURCE_IGNORE),
        static_cast<float>(vertexColor ? vertexColor->GetLightingMode() :
            NiVertexColorProperty::LIGHTING_E_A_D),
        specular && specular->GetSpecular() ? 1.0f : 0.0f
    };
    bgfx::setUniform(m_textureParamsUniform, textureParams);

    float alphaParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const NiAlphaProperty* alpha = m_pkCurrProp ? m_pkCurrProp->GetAlpha() : nullptr;
    if (!alpha)
        alpha = NiAlphaProperty::GetDefault();
    if (alpha && alpha->GetAlphaTesting())
    {
        alphaParams[0] = static_cast<float>(alpha->GetTestRef()) / 255.0f;
        alphaParams[1] = 1.0f;
        alphaParams[2] = static_cast<float>(alpha->GetTestMode());
    }
    // Soft-particle shader variants use w to identify blend modes whose
    // source contribution does not depend on source alpha (notably ONE/ONE).
    // Those need RGB fading in addition to alpha fading.
    if (alpha && alpha->GetAlphaBlending() &&
        alpha->GetSrcBlendMode() == NiAlphaProperty::ALPHA_ONE)
    {
        alphaParams[3] = 1.0f;
    }
    bgfx::setUniform(m_alphaParamsUniform, alphaParams);

    const float cameraPosition[4] = { m_worldLoc.x, m_worldLoc.y, m_worldLoc.z, 1.0f };
    const float cameraDirection[4] = { m_worldDir.x, m_worldDir.y, m_worldDir.z, 0.0f };
    bgfx::setUniform(m_cameraPositionUniform, cameraPosition);
    bgfx::setUniform(m_cameraDirectionUniform, cameraDirection);

    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightPositionType{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightDirectionRange{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightDiffuseDimmer{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightAmbientFalloff{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightSpecularSpot{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightSpotParams{};
    std::array<NiLight*, MAX_STANDARD_LIGHTS> boundLights{};
    float sceneAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    unsigned int lightCount = 0;

    if (m_pkCurrEffects)
    {
        NiDynEffectStateIter iter = m_pkCurrEffects->GetLightHeadPos();
        while (NiLight* light = m_pkCurrEffects->GetNextLight(iter))
        {
            if (!light->GetSwitch())
                continue;

            const float dimmer = light->GetDimmer();
            const NiColor& ambient = light->GetAmbientColor();
            if (light->GetEffectType() == NiDynamicEffect::AMBIENT_LIGHT)
            {
                sceneAmbient[0] += ambient.r * dimmer;
                sceneAmbient[1] += ambient.g * dimmer;
                sceneAmbient[2] += ambient.b * dimmer;
                continue;
            }

            if (lightCount >= MAX_STANDARD_LIGHTS)
                continue;

            auto& posType = lightPositionType[lightCount];
            auto& dirRange = lightDirectionRange[lightCount];
            auto& diffDim = lightDiffuseDimmer[lightCount];
            auto& ambFalloff = lightAmbientFalloff[lightCount];
            auto& specSpot = lightSpecularSpot[lightCount];
            auto& spotParams = lightSpotParams[lightCount];

            const NiColor& diffuse = light->GetDiffuseColor();
            const NiColor& lightSpecular = light->GetSpecularColor();
            diffDim = { diffuse.r, diffuse.g, diffuse.b, dimmer };
            ambFalloff = { ambient.r, ambient.g, ambient.b,
                std::max(light->GetFalloff(), 0.0f) };
            specSpot = { lightSpecular.r, lightSpecular.g, lightSpecular.b, 0.0f };

            const NiDynamicEffect::EffectType effectType = light->GetEffectType();
            if (effectType == NiDynamicEffect::DIR_LIGHT ||
                effectType == NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                const NiDirectionalLight* directional =
                    static_cast<const NiDirectionalLight*>(light);
                const NiPoint3& direction = directional->GetWorldDirection();
                posType = { 0.0f, 0.0f, 0.0f, 0.0f };
                dirRange = { direction.x, direction.y, direction.z, 0.0f };
            }
            else
            {
                const NiPointLight* point = static_cast<const NiPointLight*>(light);
                const NiPoint3& position = point->GetWorldLocation();
                posType = { position.x, position.y, position.z,
                    (effectType == NiDynamicEffect::SPOT_LIGHT ||
                     effectType == NiDynamicEffect::SHADOWSPOT_LIGHT) ? 2.0f : 1.0f };
                dirRange[3] = std::max(light->GetRange(), 0.0f);

                if (posType[3] > 1.5f)
                {
                    const NiSpotLight* spot = static_cast<const NiSpotLight*>(light);
                    const NiPoint3& direction = spot->GetWorldDirection();
                    dirRange[0] = direction.x;
                    dirRange[1] = direction.y;
                    dirRange[2] = direction.z;
                    spotParams = { spot->GetInnerSpotAngleCos(),
                        spot->GetSpotAngleCos(),
                        std::max(spot->GetSpotExponent(), 0.0f), 1.0f };
                }
            }
            boundLights[lightCount] = light;
            ++lightCount;
        }
    }

    bgfx::setUniform(m_sceneAmbientUniform, sceneAmbient);
    const float lightCountParams[4] = {
        static_cast<float>(lightCount), 0.0f, 0.0f, 0.0f
    };
    bgfx::setUniform(m_lightCountUniform, lightCountParams);
    bgfx::setUniform(m_lightPositionTypeUniform, lightPositionType.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightDirectionRangeUniform, lightDirectionRange.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightDiffuseDimmerUniform, lightDiffuseDimmer.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightAmbientFalloffUniform, lightAmbientFalloff.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightSpecularSpotUniform, lightSpecularSpot.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightSpotParamsUniform, lightSpotParams.data(), MAX_STANDARD_LIGHTS);

    float fogColor[4] = { 0.0f, 0.0f, 0.0f, std::clamp(m_maxFogValue, 0.0f, 1.0f) };
    float fogParams[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const NiFogProperty* fog = m_pkCurrProp ? m_pkCurrProp->GetFog() : nullptr;
    if (!fog)
        fog = NiFogProperty::GetDefault();
    if (fog && fog->GetFog() && fog->GetFogFunction() != NiFogProperty::FOG_VERTEX_ALPHA)
    {
        const NiColor& color = fog->GetFogColor();
        fogColor[0] = color.r; fogColor[1] = color.g; fogColor[2] = color.b;
        const float depthRange = std::max(m_frustum.m_fFar - m_frustum.m_fNear, 1e-5f);
        const float worldDepth = std::max(depthRange * fog->GetDepth(), 1e-5f);
        const float maxFog = std::max(m_maxFogValue, 1e-5f);
        const float maxFogFactor = 1.0f / maxFog - 1.0f;
        fogParams[0] = 1.0f;
        fogParams[1] = static_cast<float>(fog->GetFogFunction());
        fogParams[2] = m_frustum.m_fFar - worldDepth;
        fogParams[3] = m_frustum.m_fFar + maxFogFactor * worldDepth;
    }
    bgfx::setUniform(m_fogColorUniform, fogColor);
    bgfx::setUniform(m_fogParamsUniform, fogParams);

    // Environment NiTextureEffect. The legacy standard material generates
    // projected coordinates from one of world position, world normal, or the
    // world reflection vector depending on CoordGenType. Preserve the
    // Gamebryo-computed world projection matrix/translation here so the same
    // data drives the backend-neutral shader.
    bgfx::TextureHandle env2D = m_whiteTexture;
    bgfx::TextureHandle envCube = m_whiteCubeTexture;
    bool envUses2D = false;
    bool envUsesCube = false;
    float envParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float envTransform0[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    float envTransform1[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
    float envTransform2[4] = { 0.0f, 0.0f, 1.0f, 0.0f };

    NiTextureEffect* envEffect = m_pkCurrEffects ?
        m_pkCurrEffects->GetEnvironmentMap() : nullptr;
    if (envEffect && envEffect->GetSwitch() && envEffect->GetEffectTexture())
    {
        NiTexture* effectTexture = envEffect->GetEffectTexture();
        const NiTextureEffect::CoordGenType coordGen = envEffect->GetTextureCoordGen();
        const bool wantsCube = coordGen == NiTextureEffect::SPECULAR_CUBE_MAP ||
            coordGen == NiTextureEffect::DIFFUSE_CUBE_MAP;
        const bool isCube = NiIsKindOf(NiSourceCubeMap, effectTexture) ||
            NiIsKindOf(NiRenderedCubeMap, effectTexture);

        if (wantsCube == isCube && EnsureTexture(effectTexture))
        {
            TextureData* textureData = GetTextureData(effectTexture);
            if (textureData && bgfx::isValid(textureData->m_handle))
            {
                if (wantsCube)
                {
                    envCube = textureData->m_handle;
                    envUsesCube = true;
                }
                else
                {
                    env2D = textureData->m_handle;
                    envUses2D = true;
                }

                envParams[0] = 1.0f;
                envParams[1] = static_cast<float>(coordGen);

                const NiMatrix3& projection = envEffect->GetWorldProjectionMatrix();
                const NiPoint3& translation = envEffect->GetWorldProjectionTranslation();
                envTransform0[0] = projection.GetEntry(0, 0);
                envTransform0[1] = projection.GetEntry(0, 1);
                envTransform0[2] = projection.GetEntry(0, 2);
                envTransform0[3] = translation.x;
                envTransform1[0] = projection.GetEntry(1, 0);
                envTransform1[1] = projection.GetEntry(1, 1);
                envTransform1[2] = projection.GetEntry(1, 2);
                envTransform1[3] = translation.y;
                envTransform2[0] = projection.GetEntry(2, 0);
                envTransform2[1] = projection.GetEntry(2, 1);
                envTransform2[2] = projection.GetEntry(2, 2);
                envTransform2[3] = translation.z;
            }
        }
    }

    const uint32_t envSamplerFlags = SamplerFlags(envEffect);
    bgfx::setTexture(11, m_envTexture2DUniform, env2D, envSamplerFlags);
    bgfx::setTexture(12, m_envTextureCubeUniform, envCube, envSamplerFlags);
    bgfx::setUniform(m_envParamsUniform, envParams);
    bgfx::setUniform(m_envTransform0Uniform, envTransform0);
    bgfx::setUniform(m_envTransform1Uniform, envTransform1);
    bgfx::setUniform(m_envTransform2Uniform, envTransform2);

    // Projected lights and projected shadows share the remaining three
    // portable sampler slots after the eleven standard maps plus the 2D/cube
    // environment samplers. This matches the standard material's per-kind
    // ordering and degrades deterministically when a material would exceed
    // the backend sampler budget.
    std::array<NiBgfxMath::Vec4, MAX_PROJECTED_EFFECTS> projectedParams{};
    std::array<NiBgfxMath::Vec4, MAX_PROJECTED_EFFECTS> projectedTransform0{};
    std::array<NiBgfxMath::Vec4, MAX_PROJECTED_EFFECTS> projectedTransform1{};
    std::array<NiBgfxMath::Vec4, MAX_PROJECTED_EFFECTS> projectedTransform2{};
    std::array<NiBgfxMath::Vec4, MAX_PROJECTED_EFFECTS> projectedClipPlane{};
    std::array<NiTextureEffect*, MAX_PROJECTED_EFFECTS> projectedEffects{};
    unsigned int projectedCount = 0;
    const unsigned int maxProjectedEffects = decorationMaterial ? 2u :
        MAX_PROJECTED_EFFECTS;

    const auto appendProjectedEffect = [&](NiTextureEffect* effect, float effectKind)
    {
        if (!effect || projectedCount >= maxProjectedEffects ||
            !effect->GetSwitch() || !effect->GetEffectTexture() ||
            !EnsureTexture(effect->GetEffectTexture()))
        {
            return;
        }

        TextureData* textureData = GetTextureData(effect->GetEffectTexture());
        if (!textureData || !bgfx::isValid(textureData->m_handle))
            return;

        const unsigned int slot = projectedCount++;
        projectedEffects[slot] = effect;
        projectedParams[slot] =
        {
            1.0f, effectKind,
            effect->GetTextureCoordGen() == NiTextureEffect::WORLD_PERSPECTIVE ? 1.0f : 0.0f,
            effect->GetClippingPlaneEnable() ? 1.0f : 0.0f
        };

        const NiMatrix3& projection = effect->GetWorldProjectionMatrix();
        const NiPoint3& translation = effect->GetWorldProjectionTranslation();
        projectedTransform0[slot] = { projection.GetEntry(0, 0), projection.GetEntry(0, 1), projection.GetEntry(0, 2), translation.x };
        projectedTransform1[slot] = { projection.GetEntry(1, 0), projection.GetEntry(1, 1), projection.GetEntry(1, 2), translation.y };
        projectedTransform2[slot] = { projection.GetEntry(2, 0), projection.GetEntry(2, 1), projection.GetEntry(2, 2), translation.z };
        const NiPlane& clip = effect->GetWorldClippingPlane();
        projectedClipPlane[slot] = { clip.GetNormal().x, clip.GetNormal().y, clip.GetNormal().z, clip.GetConstant() };
        bgfx::setTexture(static_cast<std::uint8_t>(13u + slot),
            m_projectedTextureUniforms[slot], textureData->m_handle,
            SamplerFlags(effect));
    };

    if (m_pkCurrEffects)
    {
        NiDynEffectStateIter iter = m_pkCurrEffects->GetProjLightHeadPos();
        while (iter && projectedCount < maxProjectedEffects)
        {
            NiTextureEffect* effect = m_pkCurrEffects->GetNextProjLight(iter);
            if (effect && effect->GetTextureType() == NiTextureEffect::PROJECTED_LIGHT)
                appendProjectedEffect(effect, 0.0f);
        }
        iter = m_pkCurrEffects->GetProjShadowHeadPos();
        while (iter && projectedCount < maxProjectedEffects)
        {
            NiTextureEffect* effect = m_pkCurrEffects->GetNextProjShadow(iter);
            if (effect && effect->GetTextureType() == NiTextureEffect::PROJECTED_SHADOW)
                appendProjectedEffect(effect, 1.0f);
        }

        if (projectedCount < maxProjectedEffects)
        {
            NiTextureEffect* fogEffect = m_pkCurrEffects->GetFogMap();
            if (fogEffect && fogEffect->GetTextureType() == NiTextureEffect::FOG_MAP)
                appendProjectedEffect(fogEffect, 2.0f);
        }
    }

    // Dynamic light shadow receivers. Gamebryo's standard material binds shadow
    // maps as object textures. bgfx has the same D3D11-era 16-stage practical
    // sampler budget, so reuse inactive 2D stages instead of declaring extra
    // samplers. If the draw is already saturated, shadowing takes priority over
    // the last projected effect, matching the legacy fallback intent.
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightShadowParams{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightShadowExtra{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightShadowMatrix0{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightShadowMatrix1{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightShadowMatrix2{};
    std::array<NiBgfxMath::Vec4, MAX_STANDARD_LIGHTS> lightShadowMatrix3{};

    std::array<bool, 16> samplerUsed{};
    for (unsigned int i = 0; i < MAX_STANDARD_MAPS; ++i)
        samplerUsed[i] = mapParams[i][0] > 0.5f;
    samplerUsed[11] = envUses2D;
    samplerUsed[12] = envUsesCube;
    for (unsigned int i = 0; i < projectedCount; ++i)
        samplerUsed[13u + i] = projectedParams[i][0] > 0.5f;

    // Material-specialized fragment shaders repurpose several standard
    // sampler stages. Never lend those stages to a dynamic shadow map because
    // BindExtendedMaterial/BindDecorationMaterial will overwrite them later.
    if (extendedMaterial)
    {
        samplerUsed[0] = true; // terrain diffuse Texture2DArray
        samplerUsed[1] = true; // terrain alpha Texture2DArray
    }
    if (decorationMaterial)
        samplerUsed[15] = true; // screen-door fade mask

    const auto samplerUniformFor2DSlot = [&](unsigned int slot) -> bgfx::UniformHandle
    {
        if (slot < MAX_STANDARD_MAPS)
            return m_textureUniforms[slot];
        if (slot == 11)
            return m_envTexture2DUniform;
        if (slot >= 13 && slot < 16)
            return m_projectedTextureUniforms[slot - 13];
        return BGFX_INVALID_HANDLE;
    };

    const auto reserve2DShadowSlot = [&]() -> int
    {
        // Prefer the high optional stages first, then inactive decal/detail maps.
        static const unsigned int preferred[] =
        {
            15, 14, 13, 11, 10, 9, 8, 4, 3, 2, 1, 7, 6, 5, 0
        };
        for (unsigned int slot : preferred)
        {
            if (!samplerUsed[slot] && bgfx::isValid(samplerUniformFor2DSlot(slot)))
            {
                samplerUsed[slot] = true;
                return static_cast<int>(slot);
            }
        }

        // No spare stage: remove the lowest-priority projected effect.
        for (int i = static_cast<int>(MAX_PROJECTED_EFFECTS) - 1; i >= 0; --i)
        {
            const unsigned int slot = 13u + static_cast<unsigned int>(i);
            if (projectedParams[i][0] > 0.5f)
            {
                projectedParams[i][0] = 0.0f;
                projectedEffects[i] = nullptr;
                bgfx::setTexture(static_cast<std::uint8_t>(slot),
                    m_projectedTextureUniforms[i], m_whiteTexture);
                samplerUsed[slot] = true;
                return static_cast<int>(slot);
            }
        }
        return -1;
    };

    std::array<NiBgfxMath::Vec4, PSSM_DISTANCE_VECS> pssmDistances{};
    std::array<NiBgfxMath::Vec4, MAX_PSSM_SLICES * 4> pssmRows{};
    std::array<NiBgfxMath::Vec4, MAX_PSSM_SLICES> pssmViewports{};
    std::array<NiBgfxMath::Vec4, 4> pssmTransitionRows{};
    float pssmParams[4] = { 0.0f, -1.0f, 0.0f, -1.0f };
    float pssmTransitionParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (unsigned int i = 0; i < PSSM_DISTANCE_VECS; ++i)
        pssmDistances[i] =
        {
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(), std::numeric_limits<float>::max()
        };

    NiPSSMShadowClickGenerator* pssmGenerator = nullptr;
    if (NiShadowManager::GetShadowManager())
    {
        pssmGenerator = NiDynamicCast(NiPSSMShadowClickGenerator,
            NiShadowManager::GetActiveShadowClickGenerator());
    }

    bool cubeShadowSlotClaimed = false;
    for (unsigned int lightIndex = 0; lightIndex < lightCount; ++lightIndex)
    {
        NiLight* light = boundLights[lightIndex];
        if (!light)
            continue;

        NiShadowGenerator* generator = light->GetShadowGenerator();
        if (!generator || !generator->GetActive() || !generator->GetShadowTechnique())
            continue;

        NiShadowMap* shadowMap = generator->RetrieveShadowMap(
            NiShadowGenerator::AUTO_DETERMINE_SM_INDEX, mesh);
        if (!shadowMap)
            continue;

        NiShadowTechnique* technique = generator->GetShadowTechnique();
        const bool pointCube = lightPositionType[lightIndex][3] > 0.5f &&
            lightPositionType[lightIndex][3] < 1.5f &&
            technique->GetUseCubeMapForPointLight() &&
            shadowMap->GetTextureType() == NiShadowMap::TT_CUBE;

        NiTexture* shadowTexture = nullptr;
        if (pointCube && NiIsKindOf(NiShadowCubeMap, shadowMap))
            shadowTexture = static_cast<NiShadowCubeMap*>(shadowMap)->GetCubeMapTexture();
        else
            shadowTexture = shadowMap->GetTexture();
        if (!shadowTexture || !EnsureTexture(shadowTexture))
            continue;

        TextureData* shadowData = GetTextureData(shadowTexture);
        if (!shadowData || !bgfx::isValid(shadowData->m_handle))
            continue;

        float biasData[4] = { generator->GetDepthBias(), 0.0f, 0.0f, 0.0f };
        float vsmData[4] = { 10.0f, 0.001f, 0.0f, 0.0f };
        if (mesh && m_pkCurrProp)
        {
            generator->GetShaderConstantData(biasData, sizeof(biasData), mesh,
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX,
                NiShaderConstantMap::SCM_OBJ_SHADOWBIAS, m_pkCurrProp,
                m_pkCurrEffects, mesh->GetWorldTransform(), mesh->GetWorldBound(), 0);
            generator->GetShaderConstantData(vsmData, sizeof(vsmData), mesh,
                NiShadowGenerator::AUTO_DETERMINE_SM_INDEX,
                NiShaderConstantMap::SCM_OBJ_SHADOW_VSM_POWER_EPSILON,
                m_pkCurrProp, m_pkCurrEffects, mesh->GetWorldTransform(),
                mesh->GetWorldBound(), 0);
        }

        const bool isVsm = NiIsKindOf(NiVSMShadowTechnique, technique);
        const bool isPcf = technique->GetName() == NiFixedString("NiPCFShadowTechnique");
        const float techniqueMode = isVsm ? 2.0f : (isPcf ? 1.0f : 0.0f);

        if (m_currentTerrainShadowLightIndex < 0)
        {
            m_currentTerrainShadowTexture = shadowData->m_handle;
            m_currentTerrainShadowSamplerFlags = SamplerFlags(
                shadowMap->GetClampMode(), shadowMap->GetFilterMode());
            m_currentTerrainShadowLightIndex = static_cast<int>(lightIndex);
            m_currentTerrainShadowCube = pointCube;
        }

        int samplerSlot = -1;
        if (pointCube)
        {
            // The monolithic standard shader has one cube sampler. Preserve
            // correctness by accepting only the first point-cube shadow in a
            // pass; additional cube-shadowed point lights remain lit but
            // unshadowed instead of accidentally sampling another light's map.
            if (cubeShadowSlotClaimed)
                continue;
            cubeShadowSlotClaimed = true;

            // Shadow correctness has priority over a simultaneous environment
            // cube on this draw.
            samplerSlot = 12;
            if (envUsesCube)
            {
                envUsesCube = false;
                envParams[0] = 0.0f;
            }
            bgfx::setTexture(12, m_envTextureCubeUniform, shadowData->m_handle,
                SamplerFlags(shadowMap->GetClampMode(), shadowMap->GetFilterMode()));
            samplerUsed[12] = true;
        }
        else
        {
            samplerSlot = reserve2DShadowSlot();
            if (samplerSlot < 0)
                continue;
            bgfx::UniformHandle sampler = samplerUniformFor2DSlot(
                static_cast<unsigned int>(samplerSlot));
            bgfx::setTexture(static_cast<std::uint8_t>(samplerSlot), sampler,
                shadowData->m_handle,
                SamplerFlags(shadowMap->GetClampMode(), shadowMap->GetFilterMode()));
        }

        lightShadowParams[lightIndex] =
        {
            1.0f, static_cast<float>(samplerSlot), techniqueMode,
            std::max(vsmData[0], 0.0f)
        };
        lightShadowExtra[lightIndex] =
        {
            biasData[0],
            shadowTexture->GetWidth() ? 1.0f / static_cast<float>(shadowTexture->GetWidth()) : 0.0f,
            shadowTexture->GetHeight() ? 1.0f / static_cast<float>(shadowTexture->GetHeight()) : 0.0f,
            std::max(vsmData[1], 0.0f)
        };

        if (!pointCube)
        {
            const float* matrix = shadowMap->GetWorldToShadowMap();
            if (matrix)
            {
                lightShadowMatrix0[lightIndex] = { matrix[0], matrix[1], matrix[2], matrix[3] };
                lightShadowMatrix1[lightIndex] = { matrix[4], matrix[5], matrix[6], matrix[7] };
                lightShadowMatrix2[lightIndex] = { matrix[8], matrix[9], matrix[10], matrix[11] };
                lightShadowMatrix3[lightIndex] = { matrix[12], matrix[13], matrix[14], matrix[15] };
            }
        }

        // GameFramework/ecr permits PSSM on one directional light. Use the
        // already-packed matrices/viewports generated by Gamebryo so culling,
        // atlas layout, and sub-texel stabilization stay authoritative.
        if (!m_currentPssmActive && pssmGenerator && !pointCube &&
            lightPositionType[lightIndex][3] < 0.5f &&
            pssmGenerator->LightSupportsPSSM(generator, light))
        {
            NiPSSMConfiguration* config = pssmGenerator->GetConfiguration(generator, false);
            if (config && config->GetNumSlices() > 1 &&
                config->GetNumSlices() <= MAX_PSSM_SLICES)
            {
                const unsigned int sliceCount = config->GetNumSlices();
                const unsigned int distanceVecs = (sliceCount + 3u) / 4u;
                std::memcpy(pssmDistances.data(), config->GetPackedSplitDistances(),
                    distanceVecs * sizeof(float) * 4u);
                std::memcpy(pssmRows.data(), config->GetPackedSplitMatrices(),
                    sliceCount * sizeof(float) * 16u);
                std::memcpy(pssmViewports.data(), config->GetPackedSplitViewports(),
                    sliceCount * sizeof(float) * 4u);

                if (config->GetSliceTransitionEnabled() &&
                    config->GetPackedTransitionMatrix())
                {
                    // The legacy D3D11 path binds the PSSM generator's real
                    // NiNoiseTexture (wrap + nearest) for screen-door cascade
                    // transitions. The old bgfx port used a procedural hash,
                    // which changes abruptly as the transition matrix moves and
                    // causes visible foliage shimmer/blinking. Reuse one of the
                    // dynamically available 2D sampler stages instead.
                    NiTexturingProperty::Map* noiseMap =
                        pssmGenerator->GetNoiseTextureMap();
                    NiTexture* noiseTexture = noiseMap ? noiseMap->GetTexture() : nullptr;
                    if (noiseTexture && EnsureTexture(noiseTexture))
                    {
                        TextureData* noiseData = GetTextureData(noiseTexture);
                        const int noiseSlot = noiseData && bgfx::isValid(noiseData->m_handle) ?
                            reserve2DShadowSlot() : -1;
                        if (noiseSlot >= 0)
                        {
                            bgfx::UniformHandle noiseSampler = samplerUniformFor2DSlot(
                                static_cast<unsigned int>(noiseSlot));
                            if (bgfx::isValid(noiseSampler))
                            {
                                bgfx::setTexture(static_cast<std::uint8_t>(noiseSlot),
                                    noiseSampler, noiseData->m_handle,
                                    SamplerFlags(noiseMap));
                                std::memcpy(pssmTransitionRows.data(),
                                    config->GetPackedTransitionMatrix(), sizeof(float) * 16u);
                                pssmTransitionParams[0] = 1.0f;
                                pssmTransitionParams[1] = config->GetSliceTransitionSize();
                                pssmTransitionParams[2] = static_cast<float>(noiseSlot);
                            }
                        }
                    }
                }

                pssmParams[0] = 1.0f;
                pssmParams[1] = static_cast<float>(lightIndex);
                pssmParams[2] = static_cast<float>(sliceCount);
                pssmParams[3] = static_cast<float>(samplerSlot);
                m_currentPssmTexture = shadowData->m_handle;
                m_currentPssmSamplerFlags = SamplerFlags(
                    shadowMap->GetClampMode(), shadowMap->GetFilterMode());
                m_currentPssmActive = true;
                m_currentTerrainShadowTexture = shadowData->m_handle;
                m_currentTerrainShadowSamplerFlags = m_currentPssmSamplerFlags;
                m_currentTerrainShadowLightIndex = static_cast<int>(lightIndex);
                m_currentTerrainShadowCube = false;
            }
        }
    }

    for (unsigned int i = 0; i < MAX_PROJECTED_EFFECTS; ++i)
    {
        if (projectedParams[i][0] <= 0.5f && !samplerUsed[13u + i])
            bgfx::setTexture(static_cast<std::uint8_t>(13u + i),
                m_projectedTextureUniforms[i], m_whiteTexture);
    }

    bgfx::setUniform(m_lightShadowParamsUniform, lightShadowParams.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightShadowExtraUniform, lightShadowExtra.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightShadowMatrix0Uniform, lightShadowMatrix0.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightShadowMatrix1Uniform, lightShadowMatrix1.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightShadowMatrix2Uniform, lightShadowMatrix2.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_lightShadowMatrix3Uniform, lightShadowMatrix3.data(), MAX_STANDARD_LIGHTS);
    bgfx::setUniform(m_pssmParamsUniform, pssmParams);
    bgfx::setUniform(m_pssmSplitDistancesUniform, pssmDistances.data(), PSSM_DISTANCE_VECS);
    bgfx::setUniform(m_pssmSplitRowsUniform, pssmRows.data(), MAX_PSSM_SLICES * 4);
    bgfx::setUniform(m_pssmViewportsUniform, pssmViewports.data(), MAX_PSSM_SLICES);
    bgfx::setUniform(m_pssmTransitionRowsUniform, pssmTransitionRows.data(), 4);
    bgfx::setUniform(m_pssmTransitionParamsUniform, pssmTransitionParams);

    // envParams may have been disabled to make room for a point-light cube
    // shadow after the environment state was initially submitted.
    bgfx::setUniform(m_envParamsUniform, envParams);

    bgfx::setUniform(m_projectedParamsUniform, projectedParams.data(), MAX_PROJECTED_EFFECTS);
    bgfx::setUniform(m_projectedTransform0Uniform, projectedTransform0.data(), MAX_PROJECTED_EFFECTS);
    bgfx::setUniform(m_projectedTransform1Uniform, projectedTransform1.data(), MAX_PROJECTED_EFFECTS);
    bgfx::setUniform(m_projectedTransform2Uniform, projectedTransform2.data(), MAX_PROJECTED_EFFECTS);
    bgfx::setUniform(m_projectedClipPlaneUniform, projectedClipPlane.data(), MAX_PROJECTED_EFFECTS);
}

bool BgfxRenderer::BindTerrainMaterial(NiMesh* mesh)
{
    if (!mesh || !bgfx::isValid(m_terrainProgram) || !m_pkCurrProp)
        return false;

    NiTexturingProperty* texProp = m_pkCurrProp->GetTexturing();
    if (!texProp)
        return false;

    constexpr unsigned int BLEND_MAP_ID = 1;
    constexpr unsigned int BASE_MAP_ID = 2;
    constexpr unsigned int NORMAL_MAP_ID = 3;
    constexpr unsigned int SPEC_MAP_ID = 4;

    constexpr int CAPS_DIFFUSE = 0x0001;
    constexpr int CAPS_DETAIL = 0x0002;
    constexpr int CAPS_NORMAL = 0x0004;
    constexpr int CAPS_PARALLAX = 0x0008;
    constexpr int CAPS_DISTRIBUTION = 0x0010;
    constexpr int CAPS_SPECULAR = 0x0020;

    constexpr int DEBUG_MODE_MASK = 0x000fffff;
    constexpr int DEBUG_DISABLE_NORMAL_MAPS = 0x01000000;
    constexpr int DEBUG_DISABLE_PARALLAX_MAPS = 0x02000000;
    constexpr int DEBUG_DISABLE_SPECULAR_MAPS = 0x04000000;
    constexpr int DEBUG_DISABLE_DETAIL_MAPS = 0x08000000;
    constexpr int DEBUG_DISABLE_DISTRIBUTION_MASKS = 0x10000000;
    constexpr int DEBUG_DISABLE_LIGHTING = 0x20000000;
    constexpr int DEBUG_DISABLE_HIGH_DETAIL = 0x40000000;
    constexpr unsigned int DEBUG_DISABLE_BASE_NORMAL_MAP = 0x80000000u;
    constexpr int LOD_MORPH_ENABLE = 0x08;

    const auto getFloats = [mesh](const char* name, float* values,
        unsigned int count)
    {
        NiFloatsExtraData* data = NiDynamicCast(NiFloatsExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        if (!data)
            return false;
        unsigned int size = 0;
        float* source = nullptr;
        data->GetArray(size, source);
        if (!source || size < count)
            return false;
        std::copy_n(source, count, values);
        return true;
    };
    const auto getFloat = [mesh](const char* name, float fallback)
    {
        NiFloatExtraData* data = NiDynamicCast(NiFloatExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        return data ? data->GetValue() : fallback;
    };
    const auto getInteger = [mesh](const char* name, int fallback)
    {
        NiIntegerExtraData* data = NiDynamicCast(NiIntegerExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        return data ? data->GetValue() : fallback;
    };

    unsigned int layerCount = 0;
    std::array<int, MAX_TERRAIN_LAYERS> layerCaps{};
    if (texProp->GetExtraDataSize() > 0)
    {
        NiIntegersExtraData* layerInfo = NiDynamicCast(NiIntegersExtraData,
            texProp->GetExtraDataAt(0));
        if (layerInfo)
        {
            unsigned int infoSize = 0;
            int* info = nullptr;
            layerInfo->GetArray(infoSize, info);
            if (info && infoSize > 0)
            {
                layerCount = std::min<unsigned int>(MAX_TERRAIN_LAYERS,
                    std::max(info[0], 0));
                for (unsigned int i = 0; i < layerCount && i + 1 < infoSize; ++i)
                    layerCaps[i] = info[i + 1];
            }
        }
    }

    // Older/hand-authored terrain can omit the layer-capability extra data.
    // The shader-map packing is fixed to blend + three maps per layer, so it
    // still gives us a deterministic fallback layer count.
    if (layerCount == 0 && texProp->GetShaderMapCount() > 1)
        layerCount = std::min<unsigned int>(MAX_TERRAIN_LAYERS,
            (texProp->GetShaderMapCount() - 1u + 2u) / 3u);

    int debugValue = getInteger("g_TerrainDebugMode", 0);
    const int renderMode = getInteger("g_TerrainRenderMode", 0);
    if ((debugValue & DEBUG_DISABLE_HIGH_DETAIL) != 0 && renderMode != 1)
        layerCount = 0;

    std::array<NiBgfxMath::Vec4, MAX_TERRAIN_LAYERS> features0{};
    std::array<NiBgfxMath::Vec4, MAX_TERRAIN_LAYERS> features1{};
    for (unsigned int i = 0; i < layerCount; ++i)
    {
        int caps = layerCaps[i];
        if (caps == 0)
            caps = CAPS_DIFFUSE | CAPS_DETAIL | CAPS_NORMAL | CAPS_PARALLAX |
                CAPS_DISTRIBUTION | CAPS_SPECULAR;
        features0[i] =
        {
            (caps & CAPS_DIFFUSE) ? 1.0f : 0.0f,
            (caps & CAPS_NORMAL) ? 1.0f : 0.0f,
            (caps & CAPS_PARALLAX) ? 1.0f : 0.0f,
            (caps & CAPS_DETAIL) ? 1.0f : 0.0f
        };
        features1[i] =
        {
            (caps & CAPS_DISTRIBUTION) ? 1.0f : 0.0f,
            (caps & CAPS_SPECULAR) ? 1.0f : 0.0f,
            1.0f, 0.0f
        };

        if ((debugValue & DEBUG_DISABLE_NORMAL_MAPS) != 0)
            features0[i][1] = 0.0f;
        if ((debugValue & DEBUG_DISABLE_PARALLAX_MAPS) != 0)
            features0[i][2] = 0.0f;
        if ((debugValue & DEBUG_DISABLE_DETAIL_MAPS) != 0)
            features0[i][3] = 0.0f;
        if ((debugValue & DEBUG_DISABLE_DISTRIBUTION_MASKS) != 0)
            features1[i][0] = 0.0f;
        if ((debugValue & DEBUG_DISABLE_SPECULAR_MAPS) != 0)
            features1[i][1] = 0.0f;

        // NiTerrainMaterial's BAKE_DIFFUSE descriptor disables normal and
        // parallax mapping before it builds the pixel graph.
        if (renderMode == 1)
        {
            features0[i][1] = 0.0f;
            features0[i][2] = 0.0f;
        }
    }

    // Terrain owns the complete portable 16-sampler budget: two baked maps,
    // one blend mask, and three packed maps for each of four surface layers.
    std::array<bgfx::TextureHandle, MAX_TERRAIN_SAMPLERS> handles{};
    std::array<uint32_t, MAX_TERRAIN_SAMPLERS> samplerFlags{};
    handles[0] = m_whiteTexture;
    handles[1] = m_flatNormalTexture;
    handles[2] = m_blackTexture;
    for (unsigned int i = 0; i < MAX_TERRAIN_LAYERS; ++i)
    {
        handles[3 + i * 3 + 0] = m_whiteTexture;
        handles[3 + i * 3 + 1] = m_flatNormalTexture;
        handles[3 + i * 3 + 2] = m_blackTexture;
    }

    const auto bindMapTexture = [&](unsigned int slot,
        const NiTexturingProperty::Map* map)
    {
        if (!map || !map->GetTexture() || !EnsureTexture(map->GetTexture()))
            return false;
        TextureData* data = GetTextureData(map->GetTexture());
        if (!data || !bgfx::isValid(data->m_handle))
            return false;
        handles[slot] = data->m_handle;
        samplerFlags[slot] = SamplerFlags(map);
        return true;
    };

    const bool lowDiffuseEnabled = bindMapTexture(0, texProp->GetBaseMap());
    const bool lowNormalEnabled =
        (static_cast<unsigned int>(debugValue) & DEBUG_DISABLE_BASE_NORMAL_MAP) == 0u &&
        bindMapTexture(1, texProp->GetNormalMap());

    const unsigned int shaderMapCount = texProp->GetShaderMapCount();
    unsigned int packedLayer = 0;
    for (unsigned int mapIndex = 0; mapIndex < shaderMapCount; ++mapIndex)
    {
        const NiTexturingProperty::ShaderMap* map = texProp->GetShaderMap(mapIndex);
        if (!map)
            continue;

        if (map->GetID() == BLEND_MAP_ID)
        {
            bindMapTexture(2, map);
            continue;
        }

        // NiTerrainMaterial emits base/normal/spec maps in groups of three.
        // Use the stream order to distinguish repeated map IDs for each layer.
        if (mapIndex == 0)
            continue;
        packedLayer = (mapIndex - 1u) / 3u;
        if (packedLayer >= MAX_TERRAIN_LAYERS)
            continue;
        const unsigned int baseSlot = 3u + packedLayer * 3u;
        if (map->GetID() == BASE_MAP_ID)
            bindMapTexture(baseSlot + 0u, map);
        else if (map->GetID() == NORMAL_MAP_ID)
            bindMapTexture(baseSlot + 1u, map);
        else if (map->GetID() == SPEC_MAP_ID)
            bindMapTexture(baseSlot + 2u, map);
    }

    for (unsigned int slot = 0; slot < MAX_TERRAIN_SAMPLERS; ++slot)
        bgfx::setTexture(static_cast<std::uint8_t>(slot),
            m_terrainTextureUniforms[slot], handles[slot], samplerFlags[slot]);

    float layerScale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float distRamp[4] = {};
    float parallax[4] = { 0.05f, 0.05f, 0.05f, 0.05f };
    float specPower[4] = {};
    float specIntensity[4] = {};
    float detailScale[4] = {};
    float blendScale[2] = { 1.0f, 1.0f };
    float blendOffset[2] = {};
    float lowScale[2] = { 1.0f, 1.0f };
    float lowOffset[2] = {};
    float lowSize[2] = { 1.0f, 1.0f };
    float lowSpecular[2] = { 1.0f, 1.0f };
    float stitching[4] = {};
    float eye[3] = {};

    getFloats("g_LayerScale", layerScale, 4);
    getFloats("g_DistRamp", distRamp, 4);
    getFloats("g_ParallaxStrength", parallax, 4);
    getFloats("g_SpecPower", specPower, 4);
    getFloats("g_SpecIntensity", specIntensity, 4);
    getFloats("g_DetailMapScale", detailScale, 4);
    getFloats("g_BlendMapScale", blendScale, 2);
    getFloats("g_BlendMapOffset", blendOffset, 2);
    getFloats("g_LowDetailTextureScale", lowScale, 2);
    getFloats("g_LowDetailTextureOffset", lowOffset, 2);
    getFloats("g_LowDetailTextureSizes", lowSize, 2);
    getFloats("g_LowDetailTextureSpecularConstants", lowSpecular, 2);
    getFloats("g_StitchingInfo", stitching, 4);
    getFloats("g_AdjustedEyePos", eye, 3);

    int rawMorphMode = getInteger("g_MorphMode", 0);
    int shaderMorphMode = 0;
    if ((rawMorphMode & LOD_MORPH_ENABLE) != 0)
        shaderMorphMode = (rawMorphMode & ~LOD_MORPH_ENABLE) + 1;

    const float morph[4] =
    {
        getFloat("g_LODThreshold", 0.0f),
        getFloat("g_MorphDistance", 0.0f),
        static_cast<float>(shaderMorphMode),
        static_cast<float>(layerCount)
    };
    const float texCoord0[4] =
        { blendScale[0], blendScale[1], blendOffset[0], blendOffset[1] };
    const float texCoord1[4] =
        { lowScale[0], lowScale[1], lowOffset[0], lowOffset[1] };
    const float lowDetail[4] =
        { lowSize[0], lowSize[1], lowSpecular[0], lowSpecular[1] };
    const float terrainEye[4] = { eye[0], eye[1], eye[2], 0.0f };
    const int debugMode = debugValue & DEBUG_MODE_MASK;
    // The legacy debug descriptor turns lighting off for diagnostic outputs
    // except normal and gloss views. Preserve the explicit flag too.
    const bool disableLighting =
        (debugValue & DEBUG_DISABLE_LIGHTING) != 0 ||
        (debugMode != 0 && debugMode != 1 && debugMode != 5);
    const float terrainDebug[4] =
    {
        static_cast<float>(debugMode),
        static_cast<float>(renderMode),
        lowDiffuseEnabled ? 1.0f : 0.0f,
        lowNormalEnabled ? 1.0f : 0.0f
    };
    const float terrainRenderParams[4] =
    {
        disableLighting ? 1.0f : 0.0f,
        getFloat("NDL_UpdateTime", 0.0f),
        0.0f, 0.0f
    };

    const bool terrainShadowActive = m_currentTerrainShadowLightIndex >= 0 &&
        bgfx::isValid(m_currentTerrainShadowTexture);
    bgfx::setTexture(15, m_terrainShadowTextureUniform,
        terrainShadowActive ? m_currentTerrainShadowTexture : m_whiteTexture,
        terrainShadowActive ? m_currentTerrainShadowSamplerFlags : BGFX_SAMPLER_NONE);
    const float terrainShadowParams[4] =
    {
        terrainShadowActive ? 1.0f : 0.0f,
        static_cast<float>(m_currentTerrainShadowLightIndex),
        (terrainShadowActive && !m_currentTerrainShadowCube &&
            m_currentPssmActive) ? 1.0f : 0.0f,
        (terrainShadowActive && m_currentTerrainShadowCube) ? 1.0f : 0.0f
    };
    bgfx::setUniform(m_terrainShadowParamsUniform, terrainShadowParams);

    bgfx::setUniform(m_terrainLayerFeatures0Uniform, features0.data(), MAX_TERRAIN_LAYERS);
    bgfx::setUniform(m_terrainLayerFeatures1Uniform, features1.data(), MAX_TERRAIN_LAYERS);
    bgfx::setUniform(m_terrainLayerScaleUniform, layerScale);
    bgfx::setUniform(m_terrainDistRampUniform, distRamp);
    bgfx::setUniform(m_terrainParallaxStrengthUniform, parallax);
    bgfx::setUniform(m_terrainSpecPowerUniform, specPower);
    bgfx::setUniform(m_terrainSpecIntensityUniform, specIntensity);
    bgfx::setUniform(m_terrainDetailScaleUniform, detailScale);
    bgfx::setUniform(m_terrainTexCoord0Uniform, texCoord0);
    bgfx::setUniform(m_terrainTexCoord1Uniform, texCoord1);
    bgfx::setUniform(m_terrainLowDetailUniform, lowDetail);
    bgfx::setUniform(m_terrainMorphUniform, morph);
    bgfx::setUniform(m_terrainStitchingUniform, stitching);
    bgfx::setUniform(m_terrainEyeUniform, terrainEye);
    bgfx::setUniform(m_terrainDebugUniform, terrainDebug);
    bgfx::setUniform(m_terrainRenderParamsUniform, terrainRenderParams);
    return true;
}

bool BgfxRenderer::BindExtendedMaterial(NiMesh* mesh, const NiMaterial* material)
{
    if (!mesh || !material || material->GetName() != "NiExtendedMaterial" ||
        !bgfx::isValid(m_extendedProgram))
    {
        return false;
    }

    const NiExtendedMaterial* extended =
        static_cast<const NiExtendedMaterial*>(material);
    if (!extended->GetTerrainEnabled())
        return false;

    // Grand Fantasia authors the terrain resources per mesh: shader map 0 is
    // the diffuse Texture2DArray, shader map 1 is the alpha Texture2DArray,
    // and TerrainInfo/TerrainLayerData live in NiFloatsExtraData.  Prefer that
    // per-mesh state over NiExtendedMaterial's optional shared fallback state;
    // NiExtendedMaterial::Create() returns a shared material instance, so
    // storing scene-specific arrays only on the material would be incorrect.
    const NiTexturingProperty* texturing =
        m_pkCurrProp ? m_pkCurrProp->GetTexturing() : nullptr;
    const NiTexturingProperty::ShaderMap* diffuseMap =
        texturing && texturing->GetShaderMapCount() > 0 ?
        texturing->GetShaderMap(0) : nullptr;
    const NiTexturingProperty::ShaderMap* alphaMap =
        texturing && texturing->GetShaderMapCount() > 1 ?
        texturing->GetShaderMap(1) : nullptr;

    NiTexture* diffuseArray = diffuseMap && diffuseMap->GetTexture() ?
        diffuseMap->GetTexture() : extended->GetTerrainTextureArray();
    NiTexture* alphaArray = alphaMap && alphaMap->GetTexture() ?
        alphaMap->GetTexture() : extended->GetTerrainAlphaArray();

    if (!diffuseArray || !alphaArray || !EnsureTexture(diffuseArray) ||
        !EnsureTexture(alphaArray))
    {
        return false;
    }

    TextureData* diffuseData = GetTextureData(diffuseArray);
    TextureData* alphaData = GetTextureData(alphaArray);
    if (!diffuseData || !alphaData ||
        !bgfx::isValid(diffuseData->m_handle) || !bgfx::isValid(alphaData->m_handle))
    {
        return false;
    }

    // Start from NiExtendedMaterial's fallback values.  The mesh extra data
    // below overrides these values when present, matching the Grand Fantasia
    // custom TerrainSplatTextureArray node exactly.
    float terrainInfo[4] =
    {
        static_cast<float>(extended->GetTerrainLayerCount()),
        0.0f,
        0.001f,
        0.0f
    };

    const auto copyExtraFloats = [mesh](const char* name, float* output,
        unsigned int capacity, unsigned int* copied = nullptr)
    {
        NiFloatsExtraData* data = NiDynamicCast(NiFloatsExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        if (!data)
            return false;

        unsigned int size = 0;
        float* values = nullptr;
        data->GetArray(size, values);
        if (!values || size == 0)
            return false;

        const unsigned int count = std::min(size, capacity);
        std::copy_n(values, count, output);
        if (copied)
            *copied = count;
        return true;
    };

    copyExtraFloats("TerrainInfo", terrainInfo, 4);

    std::array<NiBgfxMath::Vec4, NiExtendedMaterial::MAX_TERRAIN_LAYERS> layers{};
    const NiPoint4* materialLayers = extended->GetTerrainLayerData();
    for (unsigned int i = 0; i < NiExtendedMaterial::MAX_TERRAIN_LAYERS; ++i)
    {
        layers[i] =
        {
            materialLayers[i].X(), materialLayers[i].Y(),
            materialLayers[i].Z(), materialLayers[i].W()
        };
    }

    std::array<float, NiExtendedMaterial::MAX_TERRAIN_LAYERS * 4> extraLayerData{};
    unsigned int copiedLayerFloats = 0;
    if (copyExtraFloats("TerrainLayerData", extraLayerData.data(),
        static_cast<unsigned int>(extraLayerData.size()), &copiedLayerFloats))
    {
        const unsigned int extraLayerCount = copiedLayerFloats / 4;
        for (unsigned int i = 0; i < extraLayerCount; ++i)
        {
            layers[i] =
            {
                extraLayerData[i * 4 + 0],
                extraLayerData[i * 4 + 1],
                extraLayerData[i * 4 + 2],
                extraLayerData[i * 4 + 3]
            };
        }

        // Hand-authored users may provide TerrainLayerData without the
        // companion TerrainInfo. Infer a usable layer count in that case.
        if (terrainInfo[0] <= 0.0f)
            terrainInfo[0] = static_cast<float>(extraLayerCount);
    }

    unsigned int layerCount = terrainInfo[0] > 0.0f ?
        static_cast<unsigned int>(terrainInfo[0] + 0.5f) : 0u;
    layerCount = std::min<unsigned int>(layerCount,
        NiExtendedMaterial::MAX_TERRAIN_LAYERS);
    layerCount = std::min(layerCount, diffuseData->m_layers);
    layerCount = std::min(layerCount, alphaData->m_layers);
    if (layerCount == 0)
        return false;

    // Clamp the value sent to the shader to the actual available array slices.
    terrainInfo[0] = static_cast<float>(layerCount);
    terrainInfo[1] = std::max(terrainInfo[1], 0.0f);
    terrainInfo[2] = std::max(terrainInfo[2], 0.001f);

    // Preserve the sampler semantics authored by SceneTerrainBuilder:
    // diffuse = wrap + anisotropic, alpha = clamp + trilinear.
    bgfx::setTexture(0, m_textureUniforms[0], diffuseData->m_handle,
        SamplerFlags(diffuseMap));
    bgfx::setTexture(1, m_textureUniforms[1], alphaData->m_handle,
        SamplerFlags(alphaMap));

    const float alphaInfo[4] =
    {
        alphaData->GetWidth() ?
            1.0f / static_cast<float>(alphaData->GetWidth()) : 0.0f,
        alphaData->GetHeight() ?
            1.0f / static_cast<float>(alphaData->GetHeight()) : 0.0f,
        0.0f,
        0.0f
    };

    bgfx::setUniform(m_extendedTerrainInfoUniform, terrainInfo);
    bgfx::setUniform(m_extendedAlphaInfoUniform, alphaInfo);
    bgfx::setUniform(m_extendedLayerDataUniform, layers.data(),
        NiExtendedMaterial::MAX_TERRAIN_LAYERS);
    return true;
}

bool BgfxRenderer::BindDecorationMaterial(NiMesh* mesh)
{
    if (!mesh || !m_pkCurrProp || !bgfx::isValid(m_decorationProgram))
        return false;

    const auto getFloatData = [mesh](const char* name, float fallback,
        bool* found = nullptr)
    {
        NiFloatExtraData* data = NiDynamicCast(NiFloatExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        if (found)
            *found = data != nullptr;
        return data ? data->GetValue() : fallback;
    };

    bool haveOuterMin = false;
    bool haveOuterMax = false;
    bool haveInnerMin = false;
    bool haveInnerMax = false;
    const float fadeValues[4] =
    {
        getFloatData("g_FadeOuterMinDistSqr", 6.0f, &haveOuterMin),
        getFloatData("g_FadeOuterMaxDistSqr", 10.0f, &haveOuterMax),
        getFloatData("g_FadeInnerMinDistSqr", 0.01f, &haveInnerMin),
        getFloatData("g_FadeInnerMaxDistSqr", 0.0f, &haveInnerMax)
    };
    const float saturation =
        getFloatData("g_DiffuseSaturationMultiplier", 1.0f);

    NiTexturingProperty* texturing = m_pkCurrProp->GetTexturing();
    const NiTexturingProperty::ShaderMap* fadeMap = texturing ?
        texturing->GetShaderMap(0) : nullptr;
    bool fadeEnabled = haveOuterMin && haveOuterMax && haveInnerMin &&
        haveInnerMax && fadeMap && fadeMap->GetTexture();

    bgfx::TextureHandle fadeHandle = m_whiteTexture;
    std::uint32_t fadeSamplerFlags = BGFX_SAMPLER_NONE;
    if (fadeEnabled)
    {
        NiTexture* fadeTexture = fadeMap->GetTexture();
        fadeEnabled = EnsureTexture(fadeTexture);
        TextureData* data = fadeEnabled ? GetTextureData(fadeTexture) : nullptr;
        fadeEnabled = data && bgfx::isValid(data->m_handle);
        if (fadeEnabled)
        {
            fadeHandle = data->m_handle;
            fadeSamplerFlags = SamplerFlags(fadeMap);
        }
    }

    const float params[4] =
    {
        saturation, fadeEnabled ? 1.0f : 0.0f, 0.0f, 0.0f
    };
    bgfx::setUniform(m_decorationFadeUniform, fadeValues);
    bgfx::setUniform(m_decorationParamsUniform, params);

    // Decoration's screen-door mask consumes sampler 15. The dedicated
    // fragment shader therefore keeps two projected effects (slots 13/14)
    // and reserves the final portable D3D11 pixel-sampler slot for FadeMask.
    bgfx::setTexture(15, m_decorationFadeTextureUniform, fadeHandle,
        fadeSamplerFlags);
    return true;
}

bool BgfxRenderer::BindSkyMaterial(NiMesh* mesh)
{
    if (!mesh || !bgfx::isValid(m_skyProgram))
        return false;

    const NiMaterial* material = mesh->GetActiveMaterial();
    if (!material || material->GetName() != "NiSkyMaterial")
        return false;

    const auto getFloat = [mesh](const char* name, float fallback)
    {
        NiFloatExtraData* data = NiDynamicCast(NiFloatExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        return data ? data->GetValue() : fallback;
    };
    const auto getInteger = [mesh](const char* name, int fallback)
    {
        NiIntegerExtraData* data = NiDynamicCast(NiIntegerExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        return data ? data->GetValue() : fallback;
    };
    const auto copyFloats = [mesh](const char* name, float* output,
        unsigned int count)
    {
        NiFloatsExtraData* data = NiDynamicCast(NiFloatsExtraData,
            mesh->GetExtraData(NiFixedString(name)));
        if (!data)
            return false;
        unsigned int size = 0;
        float* values = nullptr;
        data->GetArray(size, values);
        if (!values || size < count)
            return false;
        std::copy(values, values + count, output);
        return true;
    };

    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> stageConfig{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> stageModifier{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> gradientParams{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> gradientHorizon{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> gradientZenith{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> orientation0{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> orientation1{};
    std::array<NiBgfxMath::Vec4, MAX_SKY_STAGES> orientation2{};

    NiTexturingProperty* texturing = m_pkCurrProp ?
        m_pkCurrProp->GetTexturing() : nullptr;

    for (unsigned int stage = 0; stage < MAX_SKY_STAGES; ++stage)
    {
        orientation0[stage] = { 1.0f, 0.0f, 0.0f, 0.0f };
        orientation1[stage] = { 0.0f, 1.0f, 0.0f, 0.0f };
        orientation2[stage] = { 0.0f, 0.0f, 1.0f, 0.0f };
        gradientHorizon[stage] = { 1.0f, 1.0f, 1.0f, 1.0f };
        gradientZenith[stage] = { 1.0f, 1.0f, 1.0f, 1.0f };

        char name[64];
        NiSprintf(name, sizeof(name), "g_Stage%dConfiguration",
            static_cast<int>(stage));
        NiIntegerExtraData* configuration = NiDynamicCast(NiIntegerExtraData,
            mesh->GetExtraData(NiFixedString(name)));

        // Missing stage configuration is exactly how NiSkyMaterial disables
        // a blend stage.
        unsigned int colorMap = 0;
        unsigned int modifier = 5;
        unsigned int blend = 0;
        bool enabled = false;
        if (configuration)
        {
            const unsigned int packed =
                static_cast<unsigned int>(configuration->GetValue());
            colorMap = (packed & 0x00ff0000u) >> 16;
            modifier = (packed & 0x0000ff00u) >> 8;
            blend = packed & 0x000000ffu;
            enabled = modifier != 5u && colorMap != 0u && blend != 0u;
        }
        stageConfig[stage] =
        {
            static_cast<float>(colorMap), static_cast<float>(modifier),
            static_cast<float>(blend), enabled ? 1.0f : 0.0f
        };

        NiTexturingProperty::ShaderMap* shaderMap = texturing ?
            texturing->GetShaderMap(stage) : nullptr;
        bgfx::TextureHandle texture = m_whiteCubeTexture;
        std::uint32_t samplerFlags = BGFX_SAMPLER_NONE;
        bool validCubeMap = false;
        if (shaderMap && shaderMap->GetTexture())
        {
            NiTexture* stageTexture = shaderMap->GetTexture();
            validCubeMap = NiIsKindOf(NiSourceCubeMap, stageTexture) ||
                NiIsKindOf(NiRenderedCubeMap, stageTexture);
            if (validCubeMap && EnsureTexture(stageTexture))
            {
                TextureData* data = GetTextureData(stageTexture);
                if (data && bgfx::isValid(data->m_handle))
                {
                    texture = data->m_handle;
                    samplerFlags = SamplerFlags(shaderMap);
                }
                else
                {
                    validCubeMap = false;
                }
            }
            else if (validCubeMap)
            {
                validCubeMap = false;
            }
        }

        // SKYBOX and ORIENTED_SKYBOX stages require an actual cube texture.
        // The legacy material fails descriptor generation for an invalid map;
        // disabling only that stage is the closest safe runtime equivalent.
        if ((colorMap == 1u || colorMap == 4u) && !validCubeMap)
        {
            stageConfig[stage][3] = 0.0f;
        }
        bgfx::setTexture(static_cast<std::uint8_t>(stage),
            m_skyTextureUniforms[stage], texture, samplerFlags);

        NiSprintf(name, sizeof(name), "g_Stage%dModifierConstant",
            static_cast<int>(stage));
        stageModifier[stage][0] = getFloat(name, 1.0f);
        NiSprintf(name, sizeof(name), "g_Stage%dModifierExponent",
            static_cast<int>(stage));
        stageModifier[stage][1] = getFloat(name, 1.0f);
        NiSprintf(name, sizeof(name), "g_Stage%dModifierHorizonBias",
            static_cast<int>(stage));
        stageModifier[stage][2] = getFloat(name, 0.0f);

        NiSprintf(name, sizeof(name), "g_Stage%dGradientExponent",
            static_cast<int>(stage));
        gradientParams[stage][0] = getFloat(name, 1.0f);
        NiSprintf(name, sizeof(name), "g_Stage%dGradientHorizonBias",
            static_cast<int>(stage));
        gradientParams[stage][1] = getFloat(name, 0.0f);

        NiSprintf(name, sizeof(name), "g_Stage%dGradientHorizonColor",
            static_cast<int>(stage));
        if (NiColorExtraData* color = NiDynamicCast(NiColorExtraData,
            mesh->GetExtraData(NiFixedString(name))))
        {
            const NiColorA value = color->GetValue();
            gradientHorizon[stage] = { value.r, value.g, value.b, value.a };
        }
        NiSprintf(name, sizeof(name), "g_Stage%dGradientZenithColor",
            static_cast<int>(stage));
        if (NiColorExtraData* color = NiDynamicCast(NiColorExtraData,
            mesh->GetExtraData(NiFixedString(name))))
        {
            const NiColorA value = color->GetValue();
            gradientZenith[stage] = { value.r, value.g, value.b, value.a };
        }

        NiSprintf(name, sizeof(name), "g_Stage%dSkyboxOrientation",
            static_cast<int>(stage));
        float orientation[16] =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        copyFloats(name, orientation, 16);
        orientation0[stage] =
            { orientation[0], orientation[1], orientation[2], orientation[3] };
        orientation1[stage] =
            { orientation[4], orientation[5], orientation[6], orientation[7] };
        orientation2[stage] =
            { orientation[8], orientation[9], orientation[10], orientation[11] };
    }

    bgfx::setUniform(m_skyStageConfigUniform, stageConfig.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyStageModifierUniform, stageModifier.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyGradientParamsUniform, gradientParams.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyGradientHorizonUniform, gradientHorizon.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyGradientZenithUniform, gradientZenith.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyOrientation0Uniform, orientation0.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyOrientation1Uniform, orientation1.data(), MAX_SKY_STAGES);
    bgfx::setUniform(m_skyOrientation2Uniform, orientation2.data(), MAX_SKY_STAGES);

    float scattering[4] = { 0.0375f, 0.0225f, 0.0314159f, 0.0188496f };
    float rgbInvWavelength[4] = { 5.60204f, 9.47328f, 19.6438f, 2.0f };
    float scaleDepth[4] = { 4.0f, 0.25f, 16.0f, 1.0f };
    float planetDimensions[4] = { 10.25f, 105.0625f, 10.0f, 100.0f };
    float frameData[4] = { -0.99f, 0.9801f, 0.0f, 0.0f };
    float upMode[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    float sunSamples[4] = { 0.0f, 0.0f, -1.0f, 5.0f };

    copyFloats("g_AtmosphericScatteringConsts", scattering, 4);
    copyFloats("g_RGBInvWavelength4", rgbInvWavelength, 3);
    rgbInvWavelength[3] = getFloat("g_HDRExposure", rgbInvWavelength[3]);
    copyFloats("g_AtmosphericScaleDepth", scaleDepth, 4);
    copyFloats("g_PlanetDimensions", planetDimensions, 4);
    copyFloats("g_FrameData", frameData, 4);
    copyFloats("g_UpVector", upMode, 3);
    int atmosphereMode = getInteger("g_AtmosphericCalculationMode", 0);
    // NiSkyMaterial exposes CPU mode in its enum but the legacy shader path
    // explicitly rejects it. bgfx supports the two GPU modes only.
    if (atmosphereMode < 0 || atmosphereMode > 2)
        atmosphereMode = 0;
    upMode[3] = static_cast<float>(atmosphereMode);
    sunSamples[3] = std::max(getFloat("g_fNumSamples",
        static_cast<float>(getInteger("g_iNumSamples", 5))), 1.0f);

    // NiSkyMaterial uses the first directional light in the effect state as
    // the atmospheric sun, matching GenerateDescriptor().
    bool foundAtmosphericSun = false;
    if (m_pkCurrEffects && upMode[3] > 0.5f)
    {
        NiDynEffectStateIter iter = m_pkCurrEffects->GetLightHeadPos();
        while (NiLight* light = m_pkCurrEffects->GetNextLight(iter))
        {
            if (!light->GetSwitch())
                continue;
            const NiDynamicEffect::EffectType type = light->GetEffectType();
            if (type == NiDynamicEffect::DIR_LIGHT ||
                type == NiDynamicEffect::SHADOWDIR_LIGHT)
            {
                const NiDirectionalLight* directional =
                    static_cast<const NiDirectionalLight*>(light);
                const NiPoint3& direction = directional->GetWorldDirection();
                sunSamples[0] = direction.x;
                sunSamples[1] = direction.y;
                sunSamples[2] = direction.z;
                foundAtmosphericSun = true;
                break;
            }
        }
    }
    if (upMode[3] > 0.5f && !foundAtmosphericSun)
    {
        // Descriptor generation rejects atmospheric sky without a directional
        // light. Keep non-atmospheric sky stages visible instead of feeding a
        // fabricated sun direction to the scattering equations.
        upMode[3] = 0.0f;
    }

    bgfx::setUniform(m_skyAtmosScatteringUniform, scattering);
    bgfx::setUniform(m_skyRgbInvWavelengthUniform, rgbInvWavelength);
    bgfx::setUniform(m_skyScaleDepthUniform, scaleDepth);
    bgfx::setUniform(m_skyPlanetDimensionsUniform, planetDimensions);
    bgfx::setUniform(m_skyFrameDataUniform, frameData);
    bgfx::setUniform(m_skyUpModeUniform, upMode);
    bgfx::setUniform(m_skySunSamplesUniform, sunSamples);
    return true;
}

void BgfxRenderer::Do_RenderMesh(NiMesh* mesh)
{
    if (!mesh || !bgfx::isValid(m_basicProgram))
        return;

    // Shadow write materials are regular NiStandardMaterial derivatives at
    // the Gamebryo layer, but their generated pixel graph writes encoded
    // depth rather than surface color. Select the bgfx-native equivalent
    // from the active material name; SetRenderShadowTechnique supplies the
    // selected PCF/VSM encoding.
    int shadowWriteMode = -1;
    const NiMaterial* activeMaterial = mesh->GetActiveMaterial();
    if (activeMaterial)
    {
        const NiFixedString& materialName = activeMaterial->GetName();
        if (materialName == "NiPointShadowWriteMat")
            shadowWriteMode = 2;
        else if (materialName == "NiDirShadowWriteMat" ||
            materialName == "NiSpotShadowWriteMat")
        {
            shadowWriteMode = (m_shadowTechnique &&
                m_shadowTechnique->GetName() == "NiVSMShadowTechnique") ? 1 : 0;
        }
    }
    const bool shadowWrite = shadowWriteMode >= 0 &&
        bgfx::isValid(m_shadowProgram);
    const bool vsmBlur = activeMaterial &&
        activeMaterial->GetName() == "NiVSMBlurMaterial" &&
        bgfx::isValid(m_vsmBlurProgram);
    const bool terrainMaterial = activeMaterial &&
        activeMaterial->GetName() == "NiTerrainMaterial" &&
        bgfx::isValid(m_terrainProgram);
    const bool extendedMaterial = activeMaterial &&
        activeMaterial->GetName() == "NiExtendedMaterial" &&
        bgfx::isValid(m_extendedProgram);
    const bool decorationMaterial = activeMaterial &&
        (activeMaterial->GetName() == "NiDecorationMaterial" ||
         activeMaterial->GetName() == "NiLPPDecorationFinalMaterial" ||
         activeMaterial->GetName() == "NiLPPDecorationDepthNormalMaterial") &&
        bgfx::isValid(m_decorationProgram);
    const bool skyMaterial = activeMaterial &&
        activeMaterial->GetName() == "NiSkyMaterial" &&
        bgfx::isValid(m_skyProgram);
    if (shadowWrite)
    {
        const float shadowWriteParams[4] =
        {
            static_cast<float>(shadowWriteMode), 0.0f, 0.0f, 0.0f
        };
        bgfx::setUniform(m_shadowWriteParamsUniform, shadowWriteParams);
    }

#if defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
    const bool isParticleSystem = NiDynamicCast(NiPSParticleSystem, mesh) != nullptr;
#else
    const bool isParticleSystem = false;
#endif

    // Phase 1 particle instancing: NiPSFacingQuadGenerator systems are
    // expanded from one shared quad in the vertex shader. The existing
    // Gamebryo simulation/generator remains attached as a correctness
    // fallback for unsupported particle variants while this path is proven.
    if (!vsmBlur && !terrainMaterial && !extendedMaterial &&
        !decorationMaterial && !skyMaterial &&
        TryRenderFacingQuadParticles(mesh, shadowWrite))
    {
        return;
    }

    NiSkinningMeshModifier* skinModifier =
        NiGetModifier(NiSkinningMeshModifier, mesh);
    const bool hardwareSkinned = !skyMaterial && skinModifier &&
        !skinModifier->GetSoftwareSkinned() &&
        bgfx::isValid(m_skinnedProgram) &&
        bgfx::isValid(m_skinnedShadowProgram);

    const NiFixedString& positionSemantic = hardwareSkinned ?
        NiCommonSemantics::POSITION_BP() : NiCommonSemantics::POSITION();
    const NiFixedString& normalSemantic = hardwareSkinned ?
        NiCommonSemantics::NORMAL_BP() : NiCommonSemantics::NORMAL();
    const NiFixedString& tangentSemantic = hardwareSkinned ?
        NiCommonSemantics::TANGENT_BP() : NiCommonSemantics::TANGENT();
    const NiFixedString& binormalSemantic = hardwareSkinned ?
        NiCommonSemantics::BINORMAL_BP() : NiCommonSemantics::BINORMAL();

    const bgfx::VertexLayout& layout = GetStandardVertexLayout();
    const unsigned int submeshCount = mesh->GetSubmeshCount();
    const bool staticGpuCacheable = IsMeshGpuCacheable(mesh);
    MeshCache* meshCache = GetOrCreateMeshCache(mesh);

    for (unsigned int submesh = 0; submesh < submeshCount; ++submesh)
    {
        NiDataStreamElementLock positionLock(mesh, positionSemantic, 0,
            NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
        if (!positionLock.IsLocked() || submesh >= positionLock.GetSubmeshCount())
            continue;

        MeshCache::Submesh* gpuSubmesh = meshCache ?
            &meshCache->m_submeshes[submesh] : nullptr;
        std::uint64_t cacheSignature = gpuSubmesh ?
            BuildMeshCacheSignature(mesh, submesh) : 0;
        if (gpuSubmesh)
        {
            const NiVertexColorProperty* cacheVertexColor =
                m_pkCurrProp ? m_pkCurrProp->GetVertexColor() : nullptr;
            if (!cacheVertexColor)
                cacheVertexColor = NiVertexColorProperty::GetDefault();
            const std::uint64_t sourceMode = cacheVertexColor ?
                static_cast<std::uint64_t>(cacheVertexColor->GetSourceMode()) : 0;
            cacheSignature ^= hardwareSkinned ? 0x9e3779b97f4a7c15ull : 0ull;
            cacheSignature ^= terrainMaterial ? 0xc2b2ae3d27d4eb4full : 0ull;
            cacheSignature ^= (sourceMode + 1ull) * 0x165667b19e3779f9ull;
            if (cacheSignature == 0)
                cacheSignature = 1;
        }
        if (gpuSubmesh && gpuSubmesh->m_signature != cacheSignature)
        {
            gpuSubmesh->Reset();
            gpuSubmesh->m_signature = cacheSignature;
        }

        const unsigned int vertexCount = positionLock.count(submesh);
        if (vertexCount == 0)
            continue;

        const std::uint64_t vertexRevision = BuildMeshDataRevision(mesh,
            static_cast<unsigned int>(NiDataStream::USAGE_VERTEX));
        const bool cachedVertex = gpuSubmesh &&
            gpuSubmesh->m_vertexCount == vertexCount &&
            gpuSubmesh->m_vertexRevision == vertexRevision &&
            (bgfx::isValid(gpuSubmesh->m_vertexBuffer) ||
                bgfx::isValid(gpuSubmesh->m_dynamicVertexBuffer));

        bgfx::TransientVertexBuffer vertexBuffer = {};
        std::vector<StandardVertex> packedVertices;
        StandardVertex* vertices = nullptr;

        if (!cachedVertex)
        {
            if (gpuSubmesh)
            {
                packedVertices.resize(vertexCount);
                vertices = packedVertices.data();
            }
            else
            {
                // bgfx::getAvailTransientVertexBuffer() is documented to
                // return either exactly the requested count or a smaller
                // count. Some bgfx revisions can unsigned-underflow when the
                // arena is exhausted and alignment moves the offset past the
                // end, producing a bogus value larger than the request.
                // Equality is therefore the only safe success test.
                if (bgfx::getAvailTransientVertexBuffer(vertexCount, layout) != vertexCount)
                    continue;
                bgfx::allocTransientVertexBuffer(&vertexBuffer, vertexCount, layout);
                vertices = reinterpret_cast<StandardVertex*>(vertexBuffer.data);
            }

            // std::vector::resize value-initializes StandardVertex, so cached
            // CPU packing is already zeroed. Transient storage is not, hence
            // only clear it explicitly for that fallback path.
            if (!gpuSubmesh)
                std::memset(vertices, 0, sizeof(StandardVertex) * vertexCount);
            for (unsigned int i = 0; i < vertexCount; ++i)
            {
                vertices[i].nz = 1.0f;
                vertices[i].tx = 1.0f;
                vertices[i].by = 1.0f;
                vertices[i].color = 0xffffffffu;
            }

            const auto copyVec3Stream = [&](const NiFixedString& semantic,
                unsigned int semanticIndex, auto setter)
            {
                NiDataStreamElementLock lock(mesh, semantic, semanticIndex,
                    NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
                if (!lock.IsLocked() || submesh >= lock.GetSubmeshCount() ||
                    lock.count(submesh) < vertexCount)
                    return false;

                const auto format = lock.GetDataStreamElement().GetFormat();
                if (format == NiDataStreamElement::F_FLOAT32_3)
                {
                    auto it = lock.begin<NiPoint3>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        setter(vertices[i], it->x, it->y, it->z);
                    return true;
                }
                if (format == NiDataStreamElement::F_FLOAT32_4)
                {
                    auto it = lock.begin<NiBgfxMath::Vec4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        setter(vertices[i], it->x, it->y, it->z);
                    return true;
                }
                if (format == NiDataStreamElement::F_FLOAT16_3)
                {
                    struct Half3 { NiFloat16 v[3]; };
                    auto it = lock.begin<Half3>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        setter(vertices[i], static_cast<float>(it->v[0]),
                            static_cast<float>(it->v[1]), static_cast<float>(it->v[2]));
                    return true;
                }
                if (format == NiDataStreamElement::F_FLOAT16_4)
                {
                    struct Half4 { NiFloat16 v[4]; };
                    auto it = lock.begin<Half4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        setter(vertices[i], static_cast<float>(it->v[0]),
                            static_cast<float>(it->v[1]), static_cast<float>(it->v[2]));
                    return true;
                }
                if (terrainMaterial && (format == NiDataStreamElement::F_FLOAT32_2 ||
                    format == NiDataStreamElement::F_FLOAT16_2))
                {
                    const bool tangent = semantic == tangentSemantic;
                    if (format == NiDataStreamElement::F_FLOAT32_2)
                    {
                        struct Float2 { float v[2]; };
                        auto it = lock.begin<Float2>(submesh);
                        for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        {
                            if (tangent) setter(vertices[i], it->v[0], 0.0f, it->v[1]);
                            else setter(vertices[i], it->v[0], it->v[1], 1.0f);
                        }
                    }
                    else
                    {
                        struct Half2 { NiFloat16 v[2]; };
                        auto it = lock.begin<Half2>(submesh);
                        for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        {
                            const float x = static_cast<float>(it->v[0]);
                            const float y = static_cast<float>(it->v[1]);
                            if (tangent) setter(vertices[i], x, 0.0f, y);
                            else setter(vertices[i], x, y, 1.0f);
                        }
                    }
                    return true;
                }
                return false;
            };

            bool validPosition = false;
            const auto posFormat = positionLock.GetDataStreamElement().GetFormat();
            if (posFormat == NiDataStreamElement::F_FLOAT32_3)
            {
                auto it = positionLock.begin<NiPoint3>(submesh);
                for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                {
                    vertices[i].x = it->x; vertices[i].y = it->y; vertices[i].z = it->z;
                }
                validPosition = true;
            }
            else if (posFormat == NiDataStreamElement::F_FLOAT32_4)
            {
                auto it = positionLock.begin<NiBgfxMath::Vec4>(submesh);
                for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                {
                    vertices[i].x = it->x; vertices[i].y = it->y; vertices[i].z = it->z;
                    if (terrainMaterial) vertices[i].skinWeights[3] = it->w;
                }
                validPosition = true;
            }
            else if (posFormat == NiDataStreamElement::F_FLOAT16_3)
            {
                struct Half3 { NiFloat16 v[3]; };
                auto it = positionLock.begin<Half3>(submesh);
                for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                {
                    vertices[i].x = static_cast<float>(it->v[0]);
                    vertices[i].y = static_cast<float>(it->v[1]);
                    vertices[i].z = static_cast<float>(it->v[2]);
                }
                validPosition = true;
            }
            else if (posFormat == NiDataStreamElement::F_FLOAT16_4)
            {
                struct Half4 { NiFloat16 v[4]; };
                auto it = positionLock.begin<Half4>(submesh);
                for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                {
                    vertices[i].x = static_cast<float>(it->v[0]);
                    vertices[i].y = static_cast<float>(it->v[1]);
                    vertices[i].z = static_cast<float>(it->v[2]);
                    if (terrainMaterial) vertices[i].skinWeights[3] = static_cast<float>(it->v[3]);
                }
                validPosition = true;
            }
            if (!validPosition)
                continue;

            copyVec3Stream(normalSemantic, 0,
                [](StandardVertex& v, float x, float y, float z)
                { v.nx = x; v.ny = y; v.nz = z; });
            copyVec3Stream(tangentSemantic, 0,
                [](StandardVertex& v, float x, float y, float z)
                { v.tx = x; v.ty = y; v.tz = z; });
            const bool copiedBinormal = copyVec3Stream(binormalSemantic, 0,
                [](StandardVertex& v, float x, float y, float z)
                { v.bx = x; v.by = y; v.bz = z; });

            if (terrainMaterial && !copiedBinormal)
            {
                // NiTerrainMaterial constructs its binormal as cross(tangent, normal).
                // Terrain meshes normally do not store a BINORMAL stream.
                for (unsigned int i = 0; i < vertexCount; ++i)
                {
                    StandardVertex& v = vertices[i];
                    v.bx = v.ty * v.nz - v.tz * v.ny;
                    v.by = v.tz * v.nx - v.tx * v.nz;
                    v.bz = v.tx * v.ny - v.ty * v.nx;
                }
            }

            if (hardwareSkinned)
            {
                NiDataStreamElementLock weightLock(mesh,
                    NiCommonSemantics::BLENDWEIGHT(), 0,
                    NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
                NiDataStreamElementLock indexLock(mesh,
                    NiCommonSemantics::BLENDINDICES(), 0,
                    NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
                if (!weightLock.IsLocked() || !indexLock.IsLocked() ||
                    submesh >= weightLock.GetSubmeshCount() ||
                    submesh >= indexLock.GetSubmeshCount() ||
                    weightLock.count(submesh) < vertexCount ||
                    indexLock.count(submesh) < vertexCount)
                {
                    continue;
                }

                bool validWeights = false;
                const auto weightFormat = weightLock.GetDataStreamElement().GetFormat();
                if (weightFormat == NiDataStreamElement::F_FLOAT32_3)
                {
                    auto it = weightLock.begin<NiPoint3>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                    {
                        vertices[i].skinWeights[0] = it->x;
                        vertices[i].skinWeights[1] = it->y;
                        vertices[i].skinWeights[2] = it->z;
                    }
                    validWeights = true;
                }
                else if (weightFormat == NiDataStreamElement::F_FLOAT32_4)
                {
                    auto it = weightLock.begin<NiBgfxMath::Vec4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        std::copy_n(it->data(), 4, vertices[i].skinWeights);
                    validWeights = true;
                }
                else if (weightFormat == NiDataStreamElement::F_FLOAT16_4)
                {
                    struct Half4 { NiFloat16 v[4]; };
                    auto it = weightLock.begin<Half4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                    {
                        for (unsigned int j = 0; j < 4; ++j)
                            vertices[i].skinWeights[j] = static_cast<float>(it->v[j]);
                    }
                    validWeights = true;
                }

                bool validIndices = false;
                const auto indexFormat = indexLock.GetDataStreamElement().GetFormat();
                if (indexFormat == NiDataStreamElement::F_UINT8_4 ||
                    indexFormat == NiDataStreamElement::F_NORMUINT8_4_BGRA)
                {
                    struct U8x4 { std::uint8_t v[4]; };
                    auto it = indexLock.begin<U8x4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                    {
                        if (indexFormat == NiDataStreamElement::F_NORMUINT8_4_BGRA)
                        {
                            vertices[i].skinIndices[0] = static_cast<float>(it->v[2]);
                            vertices[i].skinIndices[1] = static_cast<float>(it->v[1]);
                            vertices[i].skinIndices[2] = static_cast<float>(it->v[0]);
                            vertices[i].skinIndices[3] = static_cast<float>(it->v[3]);
                        }
                        else
                        {
                            for (unsigned int j = 0; j < 4; ++j)
                                vertices[i].skinIndices[j] = static_cast<float>(it->v[j]);
                        }
                    }
                    validIndices = true;
                }
                else if (indexFormat == NiDataStreamElement::F_INT16_4)
                {
                    struct I16x4 { std::int16_t v[4]; };
                    auto it = indexLock.begin<I16x4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                    {
                        for (unsigned int j = 0; j < 4; ++j)
                            vertices[i].skinIndices[j] = static_cast<float>(it->v[j]);
                    }
                    validIndices = true;
                }

                if (!validWeights || !validIndices)
                    continue;

                NiDataStreamElementLock paletteLock(mesh,
                    NiCommonSemantics::BONE_PALETTE(), 0,
                    NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
                if (!paletteLock.IsLocked() || submesh >= paletteLock.GetSubmeshCount() ||
                    paletteLock.GetDataStreamElement().GetFormat() !=
                        NiDataStreamElement::F_UINT16_1)
                {
                    continue;
                }

                const unsigned int paletteCount = paletteLock.count(submesh);
                const NiMatrix3x4* boneMatrices = skinModifier->GetBoneMatrices();
                if (!boneMatrices || paletteCount == 0 || paletteCount > MAX_SKIN_BONES)
                    continue;

                std::array<float, MAX_SKIN_BONES * 3u * 4u> skinRows = {};
                auto palette = paletteLock.begin<std::uint16_t>(submesh);
                for (unsigned int bone = 0; bone < paletteCount; ++bone, ++palette)
                {
                    const unsigned int sourceBone = *palette;
                    if (sourceBone >= skinModifier->GetBoneCount())
                        continue;
                    const NiMatrix3x4& matrix = boneMatrices[sourceBone];
                    for (unsigned int row = 0; row < 3; ++row)
                    {
                        const NiPoint4& value = matrix.m_kEntry[row];
                        const size_t offset = static_cast<size_t>(bone * 3u + row) * 4u;
                        skinRows[offset + 0] = value.X();
                        skinRows[offset + 1] = value.Y();
                        skinRows[offset + 2] = value.Z();
                        skinRows[offset + 3] = value.W();
                    }
                }
                bgfx::setUniform(m_skinBonesUniform, skinRows.data(),
                    static_cast<std::uint16_t>(paletteCount * 3u));
            }

            if (vsmBlur)
            {
                const NiTexturingProperty* texturing =
                    m_pkCurrProp ? m_pkCurrProp->GetTexturing() : nullptr;
                NiTexture* sourceTexture = texturing ?
                    texturing->GetBaseTexture() : nullptr;
                if (!sourceTexture)
                    continue;

                unsigned int kernelSize = 4;
                NiVSMShadowTechnique* vsmTechnique =
                    NiDynamicCast(NiVSMShadowTechnique, m_shadowTechnique);
                if (vsmTechnique)
                    kernelSize = vsmTechnique->GetBlurKernelSize();
                kernelSize = std::clamp(kernelSize, 2u, 16u);
                if ((kernelSize & 1u) != 0)
                    --kernelSize;

                const float blurParams[4] =
                {
                    1.0f / static_cast<float>(std::max(1u, sourceTexture->GetWidth())),
                    1.0f / static_cast<float>(std::max(1u, sourceTexture->GetHeight())),
                    static_cast<float>(kernelSize),
                    texturing->GetApplyMode() == NiTexturingProperty::APPLY_REPLACE ?
                        0.0f : 1.0f
                };
                bgfx::setUniform(m_vsmBlurParamsUniform, blurParams);
            }

            for (unsigned int uvSet = 0; uvSet < 8; ++uvSet)
            {
                NiDataStreamElementLock uvLock(mesh, NiCommonSemantics::TEXCOORD(), uvSet,
                    NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
                if (!uvLock.IsLocked() || submesh >= uvLock.GetSubmeshCount() ||
                    uvLock.count(submesh) < vertexCount)
                    continue;

                const auto uvFormat = uvLock.GetDataStreamElement().GetFormat();
                if (uvFormat == NiDataStreamElement::F_FLOAT32_2)
                {
                    struct Float2 { float v[2]; };
                    auto it = uvLock.begin<Float2>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                    {
                        vertices[i].uv[uvSet][0] = it->v[0];
                        vertices[i].uv[uvSet][1] = it->v[1];
                    }
                }
                else if (uvFormat == NiDataStreamElement::F_FLOAT16_2)
                {
                    struct Half2 { NiFloat16 v[2]; };
                    auto it = uvLock.begin<Half2>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                    {
                        vertices[i].uv[uvSet][0] = static_cast<float>(it->v[0]);
                        vertices[i].uv[uvSet][1] = static_cast<float>(it->v[1]);
                    }
                }
            }

            const NiVertexColorProperty* vertexColor =
                m_pkCurrProp ? m_pkCurrProp->GetVertexColor() : nullptr;
            if (!vertexColor)
                vertexColor = NiVertexColorProperty::GetDefault();
            const bool useVertexColors = vertexColor &&
                vertexColor->GetSourceMode() != NiVertexColorProperty::SOURCE_IGNORE;

            NiDataStreamElementLock colorLock(mesh, NiCommonSemantics::COLOR(), 0,
                NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
            if (useVertexColors && colorLock.IsLocked() &&
                submesh < colorLock.GetSubmeshCount() &&
                colorLock.count(submesh) >= vertexCount)
            {
                const auto format = colorLock.GetDataStreamElement().GetFormat();
                if (format == NiDataStreamElement::F_NORMUINT8_4 ||
                    format == NiDataStreamElement::F_UINT8_4)
                {
                    struct U8x4 { std::uint8_t v[4]; };
                    auto it = colorLock.begin<U8x4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        std::memcpy(&vertices[i].color, it->v, 4);
                }
                else if (format == NiDataStreamElement::F_NORMUINT8_4_BGRA)
                {
                    struct U8x4 { std::uint8_t v[4]; };
                    auto it = colorLock.begin<U8x4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        vertices[i].color = static_cast<std::uint32_t>(it->v[2]) |
                            (static_cast<std::uint32_t>(it->v[1]) << 8) |
                            (static_cast<std::uint32_t>(it->v[0]) << 16) |
                            (static_cast<std::uint32_t>(it->v[3]) << 24);
                }
                else if (format == NiDataStreamElement::F_FLOAT32_4)
                {
                    auto it = colorLock.begin<NiBgfxMath::Vec4>(submesh);
                    for (unsigned int i = 0; i < vertexCount; ++i, ++it)
                        vertices[i].color = PackColor(it->x, it->y, it->z, it->w);
                }
            }


            if (gpuSubmesh)
            {
                const std::uint64_t byteCount64 =
                    static_cast<std::uint64_t>(packedVertices.size()) *
                    sizeof(StandardVertex);
                if (byteCount64 > std::numeric_limits<std::uint32_t>::max())
                {
                    gpuSubmesh->Reset();
                    continue;
                }

                const bgfx::Memory* vertexMemory = bgfx::copy(
                    packedVertices.data(),
                    static_cast<std::uint32_t>(byteCount64));

                if (staticGpuCacheable)
                {
                    gpuSubmesh->m_vertexBuffer = bgfx::createVertexBuffer(
                        vertexMemory, layout);
                    if (!bgfx::isValid(gpuSubmesh->m_vertexBuffer))
                    {
                        gpuSubmesh->Reset();
                        continue;
                    }
                    bgfx::setVertexBuffer(0, gpuSubmesh->m_vertexBuffer);
                }
                else if (bgfx::isValid(gpuSubmesh->m_dynamicVertexBuffer))
                {
                    // This stream has demonstrated runtime mutation already.
                    // Keep its dynamic allocation and refresh only on revision
                    // changes.
                    bgfx::update(gpuSubmesh->m_dynamicVertexBuffer, 0,
                        vertexMemory);
                    bgfx::setVertexBuffer(0,
                        gpuSubmesh->m_dynamicVertexBuffer);
                }
                else if (bgfx::isValid(gpuSubmesh->m_vertexBuffer))
                {
                    // A mutable-marked stream changed after its initial upload.
                    // Promote it to a dynamic buffer only now. This avoids
                    // spending one of bgfx's 4096 dynamic VB handles on the
                    // many legacy meshes marked mutable that never actually
                    // change at runtime.
                    bgfx::destroy(gpuSubmesh->m_vertexBuffer);
                    gpuSubmesh->m_vertexBuffer = BGFX_INVALID_HANDLE;
                    gpuSubmesh->m_dynamicVertexBuffer =
                        bgfx::createDynamicVertexBuffer(vertexMemory, layout,
                            BGFX_BUFFER_ALLOW_RESIZE);
                    if (!bgfx::isValid(gpuSubmesh->m_dynamicVertexBuffer))
                    {
                        gpuSubmesh->Reset();
                        continue;
                    }
                    bgfx::setVertexBuffer(0,
                        gpuSubmesh->m_dynamicVertexBuffer);
                }
                else
                {
                    // First observation of a mutable-marked stream: store the
                    // current contents in an ordinary immutable buffer. If a
                    // later write changes the stream revision it will be
                    // promoted to a dynamic handle above.
                    gpuSubmesh->m_vertexBuffer = bgfx::createVertexBuffer(
                        vertexMemory, layout);
                    if (!bgfx::isValid(gpuSubmesh->m_vertexBuffer))
                    {
                        gpuSubmesh->Reset();
                        continue;
                    }
                    bgfx::setVertexBuffer(0, gpuSubmesh->m_vertexBuffer);
                }

                gpuSubmesh->m_signature = cacheSignature;
                gpuSubmesh->m_vertexCount = vertexCount;
                gpuSubmesh->m_vertexRevision = vertexRevision;
            }
            else
            {
                bgfx::setVertexBuffer(0, &vertexBuffer);
            }
        }
        else
        {
            if (bgfx::isValid(gpuSubmesh->m_dynamicVertexBuffer))
                bgfx::setVertexBuffer(0, gpuSubmesh->m_dynamicVertexBuffer);
            else
                bgfx::setVertexBuffer(0, gpuSubmesh->m_vertexBuffer);

            // Vertex weights/indices are immutable and live in the cached GPU
            // buffer, but the palette matrices are animation state and must be
            // refreshed every draw. The original cache checkpoint only uploaded
            // them on cache misses, which froze skinned meshes after frame one.
            if (hardwareSkinned)
            {
                NiDataStreamElementLock paletteLock(mesh,
                    NiCommonSemantics::BONE_PALETTE(), 0,
                    NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
                if (!paletteLock.IsLocked() ||
                    submesh >= paletteLock.GetSubmeshCount() ||
                    paletteLock.GetDataStreamElement().GetFormat() !=
                        NiDataStreamElement::F_UINT16_1)
                {
                    continue;
                }

                const unsigned int paletteCount = paletteLock.count(submesh);
                const NiMatrix3x4* boneMatrices = skinModifier->GetBoneMatrices();
                if (!boneMatrices || paletteCount == 0 ||
                    paletteCount > MAX_SKIN_BONES)
                {
                    continue;
                }

                std::array<float, MAX_SKIN_BONES * 3u * 4u> skinRows = {};
                auto palette = paletteLock.begin<std::uint16_t>(submesh);
                for (unsigned int bone = 0; bone < paletteCount; ++bone, ++palette)
                {
                    const unsigned int sourceBone = *palette;
                    if (sourceBone >= skinModifier->GetBoneCount())
                        continue;
                    const NiMatrix3x4& matrix = boneMatrices[sourceBone];
                    for (unsigned int row = 0; row < 3; ++row)
                    {
                        const NiPoint4& value = matrix.m_kEntry[row];
                        const size_t offset =
                            static_cast<size_t>(bone * 3u + row) * 4u;
                        skinRows[offset + 0] = value.X();
                        skinRows[offset + 1] = value.Y();
                        skinRows[offset + 2] = value.Z();
                        skinRows[offset + 3] = value.W();
                    }
                }
                bgfx::setUniform(m_skinBonesUniform, skinRows.data(),
                    static_cast<std::uint16_t>(paletteCount * 3u));
            }

            // VSM blur parameters are render state, not mesh state. Refresh
            // them even when the quad itself comes from the persistent cache.
            if (vsmBlur)
            {
                const NiTexturingProperty* texturing =
                    m_pkCurrProp ? m_pkCurrProp->GetTexturing() : nullptr;
                NiTexture* sourceTexture = texturing ?
                    texturing->GetBaseTexture() : nullptr;
                if (!sourceTexture)
                    continue;

                unsigned int kernelSize = 4;
                NiVSMShadowTechnique* vsmTechnique =
                    NiDynamicCast(NiVSMShadowTechnique, m_shadowTechnique);
                if (vsmTechnique)
                    kernelSize = vsmTechnique->GetBlurKernelSize();
                kernelSize = std::clamp(kernelSize, 2u, 16u);
                if ((kernelSize & 1u) != 0)
                    --kernelSize;

                const float blurParams[4] =
                {
                    1.0f / static_cast<float>(
                        std::max(1u, sourceTexture->GetWidth())),
                    1.0f / static_cast<float>(
                        std::max(1u, sourceTexture->GetHeight())),
                    static_cast<float>(kernelSize),
                    texturing->GetApplyMode() ==
                        NiTexturingProperty::APPLY_REPLACE ? 0.0f : 1.0f
                };
                bgfx::setUniform(m_vsmBlurParamsUniform, blurParams);
            }
        }

        const NiPrimitiveType::Type primitiveType = mesh->GetPrimitiveType();
        const NiWireframeProperty* wireframe =
            m_pkCurrProp ? m_pkCurrProp->GetWireframe() : nullptr;
        if (!wireframe)
            wireframe = NiWireframeProperty::GetDefault();
        const bool useWireframe = wireframe && wireframe->GetWireframe() &&
            (primitiveType == NiPrimitiveType::PRIMITIVE_TRIANGLES ||
             primitiveType == NiPrimitiveType::PRIMITIVE_TRISTRIPS);

        // bgfx does not expose a portable polygon fill-mode switch. Emulate
        // D3D wireframe exactly at the primitive level by expanding triangle
        // edges into a line-list index stream. Duplicate shared edges are
        // intentional; rasterized output matches fixed-function wireframe.
        std::vector<std::uint32_t> sourceIndices;
        std::vector<std::uint32_t> wireIndices;
        const auto appendWireTriangle = [&](std::uint32_t a, std::uint32_t b,
            std::uint32_t c)
        {
            if (a >= vertexCount || b >= vertexCount || c >= vertexCount ||
                a == b || b == c || c == a)
            {
                return;
            }
            wireIndices.push_back(a); wireIndices.push_back(b);
            wireIndices.push_back(b); wireIndices.push_back(c);
            wireIndices.push_back(c); wireIndices.push_back(a);
        };

        NiDataStreamElementLock indexLock(mesh, NiCommonSemantics::INDEX(), 0,
            NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ);
        const bool hasIndexStream = indexLock.IsLocked() &&
            submesh < indexLock.GetSubmeshCount() && indexLock.count(submesh) > 0;

        bool cachedWireBound = false;

        if (hasIndexStream)
        {
            const unsigned int indexCount = indexLock.count(submesh);
            const std::uint64_t indexRevision = BuildMeshDataRevision(mesh,
                static_cast<unsigned int>(NiDataStream::USAGE_VERTEX_INDEX));
            const auto indexFormat = indexLock.GetDataStreamElement().GetFormat();
            const bool sourceIndex32 =
                indexFormat == NiDataStreamElement::F_UINT32_1;
            if (!sourceIndex32 && indexFormat != NiDataStreamElement::F_UINT16_1)
                continue;

            if (useWireframe && gpuSubmesh &&
                gpuSubmesh->m_wireIndexRevision == indexRevision &&
                gpuSubmesh->m_wireIndexCount > 0)
            {
                if (bgfx::isValid(gpuSubmesh->m_dynamicWireIndexBuffer))
                {
                    bgfx::setIndexBuffer(
                        gpuSubmesh->m_dynamicWireIndexBuffer);
                    cachedWireBound = true;
                }
                else if (bgfx::isValid(gpuSubmesh->m_wireIndexBuffer))
                {
                    bgfx::setIndexBuffer(gpuSubmesh->m_wireIndexBuffer);
                    cachedWireBound = true;
                }
            }

            if (useWireframe)
            {
                if (!cachedWireBound)
                {
                    sourceIndices.resize(indexCount);
                    if (sourceIndex32)
                    {
                        auto src = indexLock.begin<std::uint32_t>(submesh);
                        for (unsigned int i = 0; i < indexCount; ++i, ++src)
                            sourceIndices[i] = *src;
                    }
                    else
                    {
                        auto src = indexLock.begin<std::uint16_t>(submesh);
                        for (unsigned int i = 0; i < indexCount; ++i, ++src)
                            sourceIndices[i] = *src;
                    }
                }
            }
            else if (gpuSubmesh &&
                gpuSubmesh->m_indexCount == indexCount &&
                gpuSubmesh->m_index32 == sourceIndex32 &&
                gpuSubmesh->m_indexRevision == indexRevision &&
                (bgfx::isValid(gpuSubmesh->m_indexBuffer) ||
                    bgfx::isValid(gpuSubmesh->m_dynamicIndexBuffer)))
            {
                if (bgfx::isValid(gpuSubmesh->m_dynamicIndexBuffer))
                    bgfx::setIndexBuffer(gpuSubmesh->m_dynamicIndexBuffer);
                else
                    bgfx::setIndexBuffer(gpuSubmesh->m_indexBuffer);
            }
            else if (gpuSubmesh)
            {
                const std::uint32_t stride = sourceIndex32 ? 4u : 2u;
                const std::uint64_t bytes64 =
                    static_cast<std::uint64_t>(indexCount) * stride;
                if (bytes64 > std::numeric_limits<std::uint32_t>::max())
                    continue;

                std::vector<std::uint8_t> packedIndices(
                    static_cast<size_t>(bytes64));
                if (sourceIndex32)
                {
                    auto src = indexLock.begin<std::uint32_t>(submesh);
                    auto* dst = reinterpret_cast<std::uint32_t*>(packedIndices.data());
                    for (unsigned int i = 0; i < indexCount; ++i, ++src)
                        dst[i] = *src;
                }
                else
                {
                    auto src = indexLock.begin<std::uint16_t>(submesh);
                    auto* dst = reinterpret_cast<std::uint16_t*>(packedIndices.data());
                    for (unsigned int i = 0; i < indexCount; ++i, ++src)
                        dst[i] = *src;
                }

                const bgfx::Memory* indexMemory = bgfx::copy(
                    packedIndices.data(), static_cast<std::uint32_t>(bytes64));

                if (staticGpuCacheable)
                {
                    if (bgfx::isValid(gpuSubmesh->m_indexBuffer))
                        bgfx::destroy(gpuSubmesh->m_indexBuffer);
                    gpuSubmesh->m_indexBuffer = bgfx::createIndexBuffer(
                        indexMemory,
                        sourceIndex32 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE);
                    if (!bgfx::isValid(gpuSubmesh->m_indexBuffer))
                        continue;
                    bgfx::setIndexBuffer(gpuSubmesh->m_indexBuffer);
                }
                else
                {
                    const std::uint16_t staticFlags = sourceIndex32 ?
                        BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE;

                    if (bgfx::isValid(gpuSubmesh->m_dynamicIndexBuffer))
                    {
                        if (gpuSubmesh->m_index32 != sourceIndex32)
                        {
                            bgfx::destroy(gpuSubmesh->m_dynamicIndexBuffer);
                            gpuSubmesh->m_dynamicIndexBuffer =
                                BGFX_INVALID_HANDLE;
                        }
                    }

                    if (bgfx::isValid(gpuSubmesh->m_dynamicIndexBuffer))
                    {
                        bgfx::update(gpuSubmesh->m_dynamicIndexBuffer, 0,
                            indexMemory);
                        bgfx::setIndexBuffer(
                            gpuSubmesh->m_dynamicIndexBuffer);
                    }
                    else if (bgfx::isValid(gpuSubmesh->m_indexBuffer))
                    {
                        bgfx::destroy(gpuSubmesh->m_indexBuffer);
                        gpuSubmesh->m_indexBuffer = BGFX_INVALID_HANDLE;
                        std::uint16_t flags = BGFX_BUFFER_ALLOW_RESIZE;
                        if (sourceIndex32)
                            flags |= BGFX_BUFFER_INDEX32;
                        gpuSubmesh->m_dynamicIndexBuffer =
                            bgfx::createDynamicIndexBuffer(indexMemory, flags);
                        if (!bgfx::isValid(gpuSubmesh->m_dynamicIndexBuffer))
                            continue;
                        bgfx::setIndexBuffer(
                            gpuSubmesh->m_dynamicIndexBuffer);
                    }
                    else
                    {
                        gpuSubmesh->m_indexBuffer = bgfx::createIndexBuffer(
                            indexMemory, staticFlags);
                        if (!bgfx::isValid(gpuSubmesh->m_indexBuffer))
                            continue;
                        bgfx::setIndexBuffer(gpuSubmesh->m_indexBuffer);
                    }
                }

                gpuSubmesh->m_indexCount = indexCount;
                gpuSubmesh->m_index32 = sourceIndex32;
                gpuSubmesh->m_indexRevision = indexRevision;
            }
            else
            {
                if (bgfx::getAvailTransientIndexBuffer(indexCount,
                    sourceIndex32) != indexCount)
                {
                    continue;
                }

                bgfx::TransientIndexBuffer indexBuffer;
                bgfx::allocTransientIndexBuffer(&indexBuffer, indexCount,
                    sourceIndex32);
                if (sourceIndex32)
                {
                    auto src = indexLock.begin<std::uint32_t>(submesh);
                    auto* dst = reinterpret_cast<std::uint32_t*>(indexBuffer.data);
                    for (unsigned int i = 0; i < indexCount; ++i, ++src)
                        dst[i] = *src;
                }
                else
                {
                    auto src = indexLock.begin<std::uint16_t>(submesh);
                    auto* dst = reinterpret_cast<std::uint16_t*>(indexBuffer.data);
                    for (unsigned int i = 0; i < indexCount; ++i, ++src)
                        dst[i] = *src;
                }
                bgfx::setIndexBuffer(&indexBuffer);
            }
        }

        if (useWireframe && !cachedWireBound)
        {
            if (sourceIndices.empty())
            {
                sourceIndices.resize(vertexCount);
                for (unsigned int i = 0; i < vertexCount; ++i)
                    sourceIndices[i] = i;
            }

            if (primitiveType == NiPrimitiveType::PRIMITIVE_TRIANGLES)
            {
                wireIndices.reserve((sourceIndices.size() / 3u) * 6u);
                for (size_t i = 0; i + 2 < sourceIndices.size(); i += 3)
                {
                    appendWireTriangle(sourceIndices[i], sourceIndices[i + 1],
                        sourceIndices[i + 2]);
                }
            }
            else
            {
                wireIndices.reserve(sourceIndices.size() > 2 ?
                    (sourceIndices.size() - 2u) * 6u : 0u);
                for (size_t i = 2; i < sourceIndices.size(); ++i)
                {
                    appendWireTriangle(sourceIndices[i - 2],
                        sourceIndices[i - 1], sourceIndices[i]);
                }
            }

            if (wireIndices.empty())
                continue;

            const bool wireIndex32 =
                vertexCount > std::numeric_limits<std::uint16_t>::max();
            if (gpuSubmesh)
            {
                const std::uint32_t stride = wireIndex32 ? 4u : 2u;
                const std::uint64_t bytes64 =
                    static_cast<std::uint64_t>(wireIndices.size()) * stride;
                if (bytes64 > std::numeric_limits<std::uint32_t>::max())
                    continue;

                std::vector<std::uint8_t> packedWire(
                    static_cast<size_t>(bytes64));
                if (wireIndex32)
                {
                    auto* dst = reinterpret_cast<std::uint32_t*>(packedWire.data());
                    std::copy(wireIndices.begin(), wireIndices.end(), dst);
                }
                else
                {
                    auto* dst = reinterpret_cast<std::uint16_t*>(packedWire.data());
                    for (size_t i = 0; i < wireIndices.size(); ++i)
                        dst[i] = static_cast<std::uint16_t>(wireIndices[i]);
                }

                const bgfx::Memory* wireMemory = bgfx::copy(
                    packedWire.data(), static_cast<std::uint32_t>(bytes64));

                if (staticGpuCacheable)
                {
                    if (bgfx::isValid(gpuSubmesh->m_wireIndexBuffer))
                        bgfx::destroy(gpuSubmesh->m_wireIndexBuffer);
                    gpuSubmesh->m_wireIndexBuffer = bgfx::createIndexBuffer(
                        wireMemory,
                        wireIndex32 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE);
                    if (!bgfx::isValid(gpuSubmesh->m_wireIndexBuffer))
                        continue;
                    bgfx::setIndexBuffer(gpuSubmesh->m_wireIndexBuffer);
                }
                else
                {
                    const std::uint16_t staticFlags = wireIndex32 ?
                        BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE;

                    if (bgfx::isValid(gpuSubmesh->m_dynamicWireIndexBuffer) &&
                        gpuSubmesh->m_wireIndex32 != wireIndex32)
                    {
                        bgfx::destroy(gpuSubmesh->m_dynamicWireIndexBuffer);
                        gpuSubmesh->m_dynamicWireIndexBuffer =
                            BGFX_INVALID_HANDLE;
                    }

                    if (bgfx::isValid(gpuSubmesh->m_dynamicWireIndexBuffer))
                    {
                        bgfx::update(gpuSubmesh->m_dynamicWireIndexBuffer, 0,
                            wireMemory);
                        bgfx::setIndexBuffer(
                            gpuSubmesh->m_dynamicWireIndexBuffer);
                    }
                    else if (bgfx::isValid(gpuSubmesh->m_wireIndexBuffer))
                    {
                        bgfx::destroy(gpuSubmesh->m_wireIndexBuffer);
                        gpuSubmesh->m_wireIndexBuffer = BGFX_INVALID_HANDLE;
                        std::uint16_t flags = BGFX_BUFFER_ALLOW_RESIZE;
                        if (wireIndex32)
                            flags |= BGFX_BUFFER_INDEX32;
                        gpuSubmesh->m_dynamicWireIndexBuffer =
                            bgfx::createDynamicIndexBuffer(wireMemory, flags);
                        if (!bgfx::isValid(
                            gpuSubmesh->m_dynamicWireIndexBuffer))
                        {
                            continue;
                        }
                        bgfx::setIndexBuffer(
                            gpuSubmesh->m_dynamicWireIndexBuffer);
                    }
                    else
                    {
                        gpuSubmesh->m_wireIndexBuffer = bgfx::createIndexBuffer(
                            wireMemory, staticFlags);
                        if (!bgfx::isValid(gpuSubmesh->m_wireIndexBuffer))
                            continue;
                        bgfx::setIndexBuffer(gpuSubmesh->m_wireIndexBuffer);
                    }
                }

                gpuSubmesh->m_wireIndexCount =
                    static_cast<std::uint32_t>(wireIndices.size());
                gpuSubmesh->m_wireIndex32 = wireIndex32;
                gpuSubmesh->m_wireIndexRevision = hasIndexStream ?
                    BuildMeshDataRevision(mesh, static_cast<unsigned int>(
                        NiDataStream::USAGE_VERTEX_INDEX)) : vertexRevision;
            }
            else
            {
                const std::uint32_t wireCount =
                    static_cast<std::uint32_t>(wireIndices.size());
                if (bgfx::getAvailTransientIndexBuffer(wireCount, wireIndex32) !=
                    wireCount)
                {
                    continue;
                }

                bgfx::TransientIndexBuffer wireBuffer;
                bgfx::allocTransientIndexBuffer(&wireBuffer, wireCount,
                    wireIndex32);
                if (wireIndex32)
                {
                    auto* dst = reinterpret_cast<std::uint32_t*>(wireBuffer.data);
                    std::copy(wireIndices.begin(), wireIndices.end(), dst);
                }
                else
                {
                    auto* dst = reinterpret_cast<std::uint16_t*>(wireBuffer.data);
                    for (size_t i = 0; i < wireIndices.size(); ++i)
                        dst[i] = static_cast<std::uint16_t>(wireIndices[i]);
                }
                bgfx::setIndexBuffer(&wireBuffer);
            }
        }

        uint64_t state = BuildRenderState(shadowWrite);
        if (useWireframe)
        {
            state |= BGFX_STATE_PT_LINES;
        }
        else
        {
            switch (primitiveType)
            {
            case NiPrimitiveType::PRIMITIVE_TRISTRIPS: state |= BGFX_STATE_PT_TRISTRIP; break;
            case NiPrimitiveType::PRIMITIVE_LINES: state |= BGFX_STATE_PT_LINES; break;
            case NiPrimitiveType::PRIMITIVE_LINESTRIPS: state |= BGFX_STATE_PT_LINESTRIP; break;
            case NiPrimitiveType::PRIMITIVE_POINTS: state |= BGFX_STATE_PT_POINTS; break;
            case NiPrimitiveType::PRIMITIVE_TRIANGLES: break;
            default: continue;
            }
        }

        SetModelTransform(mesh->GetWorldTransform());
        BindMaterialAndTexture(mesh);
        const bool terrainBound = terrainMaterial && !shadowWrite && !vsmBlur &&
            BindTerrainMaterial(mesh);
        const bool extendedBound = extendedMaterial && !shadowWrite && !vsmBlur &&
            !terrainBound && BindExtendedMaterial(mesh, activeMaterial);
        const bool decorationBound = decorationMaterial && !shadowWrite && !vsmBlur &&
            !terrainBound && !extendedBound && BindDecorationMaterial(mesh);
        const bool skyBound = skyMaterial && !shadowWrite && !vsmBlur &&
            !terrainBound && !extendedBound && !decorationBound && BindSkyMaterial(mesh);
        const bool softParticleFallback = isParticleSystem &&
            !hardwareSkinned && !shadowWrite && !vsmBlur && !terrainBound &&
            !extendedBound && !decorationBound && !skyBound &&
            CanUseSoftParticles();
        if (softParticleFallback)
            BindSoftParticleDepth();

        const std::uint32_t stencilState = BuildStencilState();
        bgfx::setState(state);
        bgfx::setStencil(stencilState);

        const bgfx::ProgramHandle drawProgram = softParticleFallback ?
            m_softParticleFallbackProgram : (hardwareSkinned ?
            (shadowWrite ? m_skinnedShadowProgram :
                (extendedBound ? m_extendedSkinnedProgram :
                    (decorationBound ? m_decorationSkinnedProgram : m_skinnedProgram))) :
            (shadowWrite ? m_shadowProgram :
                (vsmBlur ? m_vsmBlurProgram :
                    (terrainBound ?
                        (m_currentTerrainShadowCube &&
                            bgfx::isValid(m_terrainCubeShadowProgram) ?
                            m_terrainCubeShadowProgram : m_terrainProgram) :
                        (extendedBound ? m_extendedProgram :
                            (decorationBound ? m_decorationProgram :
                                (skyBound ? m_skyProgram : m_basicProgram)))))));
        const bgfx::ProgramHandle instancedDrawProgram = shadowWrite ?
            m_instancedShadowProgram :
            (extendedBound ? m_extendedInstancedProgram :
                (decorationBound ? m_decorationInstancedProgram : m_instancedProgram));

        const bool mirrorSoftDepth = m_softParticleDepthViewActive &&
            m_softParticlesEnabled && m_softParticleDepthClearedThisFrame &&
            !isParticleSystem && !shadowWrite && !vsmBlur && !skyBound &&
            (state & BGFX_STATE_WRITE_Z) != 0;
        const std::uint64_t softDepthState =
            (state & ~(BGFX_STATE_BLEND_MASK | BGFX_STATE_WRITE_A)) |
            BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z;
        const bgfx::ProgramHandle softDepthDrawProgram = hardwareSkinned ?
            m_softDepthSkinnedProgram :
            (terrainBound ? m_softDepthTerrainProgram : m_softDepthProgram);

        const auto submitSoftDepth = [&](bgfx::ProgramHandle program)
        {
            if (!mirrorSoftDepth || !bgfx::isValid(program))
                return;
            SetSoftParticleParams();
            bgfx::setState(softDepthState);
            bgfx::setStencil(BGFX_STENCIL_NONE);
            bgfx::submit(m_softParticleDepthViewId, program, 0,
                BGFX_DISCARD_NONE);
            bgfx::setState(state);
            bgfx::setStencil(stencilState);
        };

        bool submitted = false;
        if (!hardwareSkinned && !skyBound && mesh->GetInstanced() &&
            bgfx::isValid(instancedDrawProgram) &&
            (bgfx::getCaps()->supported & BGFX_CAPS_INSTANCING) != 0)
        {
            NiDataStreamRef* instanceRef = mesh->GetVisibleInstanceStream();
            NiDataStream* instanceStream = instanceRef ? instanceRef->GetDataStream() : nullptr;
            if (instanceRef && instanceStream &&
                instanceStream->GetStride() == sizeof(float) * 12u)
            {
                const NiDataStream::Region& region =
                    instanceRef->GetRegionForSubmesh(submesh);
                const std::uint32_t instanceCount = region.GetRange();
                if (instanceCount > 0)
                {
                    const void* rawData = instanceStream->Lock(NiDataStream::LOCK_TOOL_READ);
                    if (rawData)
                    {
                        const std::uint8_t* instanceSource =
                            static_cast<const std::uint8_t*>(rawData) +
                            static_cast<size_t>(region.GetStartIndex()) *
                            instanceStream->GetStride();

                        constexpr std::uint16_t INSTANCE_STRIDE = sizeof(float) * 12u;
                        const std::uint32_t available = bgfx::getAvailInstanceDataBuffer(
                            instanceCount, INSTANCE_STRIDE);
                        if (available == instanceCount)
                        {
                            bgfx::InstanceDataBuffer instanceBuffer;
                            bgfx::allocInstanceDataBuffer(&instanceBuffer,
                                instanceCount, INSTANCE_STRIDE);
                            std::memcpy(instanceBuffer.data, instanceSource,
                                static_cast<size_t>(instanceCount) * INSTANCE_STRIDE);
                            bgfx::setInstanceDataBuffer(&instanceBuffer);
                            submitSoftDepth(m_softDepthInstancedProgram);
                            bgfx::submit(m_viewId, instancedDrawProgram);
                            submitted = true;
                        }
                        else
                        {
                            // Transient instance space can be exhausted late in a
                            // frame. Preserve correctness by issuing ordinary draws
                            // from the same packed 3x4 transforms instead of dropping
                            // visible instances.
                            for (std::uint32_t instance = 0;
                                instance < instanceCount; ++instance)
                            {
                                const float* rows = reinterpret_cast<const float*>(
                                    instanceSource +
                                    static_cast<size_t>(instance) * INSTANCE_STRIDE);
                                // NiInstancingUtilities::PackTransform stores
                                // three matrix columns: (R00,R10,R20,Tx),
                                // (R01,R11,R21,Ty), (R02,R12,R22,Tz).
                                // bgfx::setTransform consumes the equivalent
                                // column-major 4x4 matrix directly.
                                float matrix[16] =
                                {
                                    rows[0], rows[1], rows[2],  0.0f,
                                    rows[4], rows[5], rows[6],  0.0f,
                                    rows[8], rows[9], rows[10], 0.0f,
                                    rows[3], rows[7], rows[11], 1.0f
                                };
                                bgfx::setTransform(matrix);
                                submitSoftDepth(softDepthDrawProgram);
                                const bool last = instance + 1u == instanceCount;
                                bgfx::submit(m_viewId, drawProgram, 0,
                                    last ? BGFX_DISCARD_ALL : BGFX_DISCARD_NONE);
                            }
                            submitted = true;
                        }
                        instanceStream->Unlock(NiDataStream::LOCK_TOOL_READ);
                    }
                }
            }
        }

        if (!submitted)
        {
            submitSoftDepth(softDepthDrawProgram);
            bgfx::submit(m_viewId, drawProgram);
        }
    }
}

bool BgfxRenderer::CreateParticleResources()
{
    const ParticleVertex vertices[4] =
    {
        {-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f}
    };
    const std::uint16_t indices[6] = {0, 1, 2, 2, 3, 0};

    m_particleQuadVertexBuffer = bgfx::createVertexBuffer(
        bgfx::copy(vertices, sizeof(vertices)), GetParticleVertexLayout());
    m_particleQuadIndexBuffer = bgfx::createIndexBuffer(
        bgfx::copy(indices, sizeof(indices)));

    if (!bgfx::isValid(m_particleQuadVertexBuffer) ||
        !bgfx::isValid(m_particleQuadIndexBuffer))
    {
        return false;
    }

    bgfx::setName(m_particleQuadVertexBuffer, "NiPS shared billboard quad");
    bgfx::setName(m_particleQuadIndexBuffer, "NiPS shared billboard indices");
    return true;
}

bool BgfxRenderer::UploadParticleInstances(const void* data,
    std::uint32_t count, bgfx::DynamicVertexBufferHandle& handle,
    std::uint32_t& startVertex)
{
    static_assert(sizeof(ParticleInstance) == sizeof(float) * 12u,
        "Particle instance data must stay aligned to three vec4 values.");
    handle = BGFX_INVALID_HANDLE;
    startVertex = 0;
    if (!data || count == 0)
        return false;

    ParticleInstancePage* selected = nullptr;
    for (ParticleInstancePage& page : m_particleInstancePages)
    {
        if (bgfx::isValid(page.m_handle) &&
            page.m_capacity - page.m_cursor >= count)
        {
            selected = &page;
            break;
        }
    }

    if (!selected)
    {
        ParticleInstancePage page;
        page.m_capacity = std::max(PARTICLE_INSTANCE_PAGE_SIZE, count);
        page.m_handle = bgfx::createDynamicVertexBuffer(page.m_capacity,
            GetParticleInstanceLayout());
        if (!bgfx::isValid(page.m_handle))
            return false;

        m_particleInstancePages.push_back(page);
        selected = &m_particleInstancePages.back();
    }

    const std::uint64_t byteCount64 = static_cast<std::uint64_t>(count) *
        sizeof(ParticleInstance);
    if (byteCount64 > std::numeric_limits<std::uint32_t>::max())
        return false;

    startVertex = selected->m_cursor;
    const bgfx::Memory* memory = bgfx::copy(data,
        static_cast<std::uint32_t>(byteCount64));
    bgfx::update(selected->m_handle, startVertex, memory);
    selected->m_cursor += count;
    handle = selected->m_handle;
    return true;
}

bool BgfxRenderer::TryRenderFacingQuadParticles(NiMesh* mesh,
    bool shadowWrite)
{
#if !defined(NIBGFX_ENABLE_PARTICLE_INSTANCING)
    EE_UNUSED_ARG(mesh);
    EE_UNUSED_ARG(shadowWrite);
    return false;
#else
    // Identify the supported Gamebryo particle type first so the diagnostics
    // only count systems that phase 1 could potentially instance.
    NiPSParticleSystem* particles = NiDynamicCast(NiPSParticleSystem, mesh);
    if (!particles || !NiGetModifier(NiPSFacingQuadGenerator, particles))
        return false;

    ++m_particleInstancingDebugStats.m_candidateChecks;

    if ((bgfx::getCaps()->supported & BGFX_CAPS_INSTANCING) == 0 ||
        !bgfx::isValid(m_particleProgram) ||
        !bgfx::isValid(m_particleQuadVertexBuffer) ||
        !bgfx::isValid(m_particleQuadIndexBuffer))
    {
        ++m_particleInstancingDebugStats.m_rendererUnavailable;
        return false;
    }

    // Facing quads are generated against the main culling camera. During a
    // shadow click the renderer camera is the light camera, so rebuilding the
    // billboard basis here would rotate the particles differently from the
    // legacy Gamebryo geometry. Keep the generated-quad path for shadow writes
    // in phase 1 and instance only the normal camera-facing pass.
    if (shadowWrite)
    {
        ++m_particleInstancingDebugStats.m_shadowFallbacks;
        return false;
    }

    if (particles->GetNumParticles() == 0 || mesh->GetSubmeshCount() != 1 ||
        mesh->GetInstanced())
    {
        ++m_particleInstancingDebugStats.m_layoutFallbacks;
        return false;
    }

    // HasAnimatedTextures() only means the particle system allocated the
    // per-particle variance array. NiPSFacingQuadGenerator itself always
    // creates a static shared TEXCOORD stream (0,0 / 0,1 / 1,1 / 1,0) and
    // never schedules the NiPSAlignedQuadTextureKernel. Therefore this flag
    // must not disqualify facing-quad instancing. The animated-UV path lives
    // in NiPSAlignedQuadGenerator, which is not matched by this phase-1 path.
    if (particles->HasAnimatedTextures())
        ++m_particleInstancingDebugStats.m_animatedTextureFlagsSeen;

    const NiWireframeProperty* wireframe =
        m_pkCurrProp ? m_pkCurrProp->GetWireframe() : nullptr;
    if (!wireframe)
        wireframe = NiWireframeProperty::GetDefault();
    if (wireframe && wireframe->GetWireframe())
    {
        ++m_particleInstancingDebugStats.m_wireframeFallbacks;
        return false;
    }

    const std::uint32_t count = particles->GetNumParticles();
    const NiPoint3* positions = particles->GetPositions();
    const float* initialSizes = particles->GetInitialSizes();
    const float* sizes = particles->GetSizes();
    if (!positions || !initialSizes || !sizes)
    {
        ++m_particleInstancingDebugStats.m_missingDataFallbacks;
        return false;
    }

    const NiRGBA* colors = particles->GetColors();
    const float* rotations = particles->GetRotationAngles();

    const NiMatrix3& worldRotate = particles->GetWorldRotate();
    NiPoint3 modelCameraUp = m_worldUp * worldRotate;
    NiPoint3 modelCameraRight = m_worldRight * worldRotate;
    NiPoint3 modelNormal = modelCameraRight.Cross(modelCameraUp);

    m_particleOrderScratch.resize(count);
    std::iota(m_particleOrderScratch.begin(), m_particleOrderScratch.end(), 0u);
    if (particles->IsSorted() && count > 1)
    {
        NiTransform inverseWorld;
        particles->GetWorldTransform().Invert(inverseWorld);
        const NiPoint3 modelCameraPosition = inverseWorld * m_worldLoc;

        std::stable_sort(m_particleOrderScratch.begin(),
            m_particleOrderScratch.end(),
            [&](std::uint32_t lhs, std::uint32_t rhs)
            {
                const float lhsDistance = NiAbs(
                    (positions[lhs] - modelCameraPosition).Dot(modelNormal));
                const float rhsDistance = NiAbs(
                    (positions[rhs] - modelCameraPosition).Dot(modelNormal));
                return lhsDistance > rhsDistance;
            });
    }

    m_particleInstanceScratch.resize(count);
    constexpr float inv255 = 1.0f / 255.0f;
    for (std::uint32_t dst = 0; dst < count; ++dst)
    {
        const std::uint32_t src = m_particleOrderScratch[dst];
        const NiPoint3& position = positions[src];
        ParticleInstance& instance = m_particleInstanceScratch[dst];
        instance.px = position.x;
        instance.py = position.y;
        instance.pz = position.z;
        instance.size = initialSizes[src] * sizes[src];
        instance.rotation = rotations ? rotations[src] : 0.0f;
        instance.pad0 = instance.pad1 = instance.pad2 = 0.0f;

        if (colors)
        {
            instance.r = static_cast<float>(colors[src].r()) * inv255;
            instance.g = static_cast<float>(colors[src].g()) * inv255;
            instance.b = static_cast<float>(colors[src].b()) * inv255;
            instance.a = static_cast<float>(colors[src].a()) * inv255;
        }
        else
        {
            instance.r = instance.g = instance.b = instance.a = 1.0f;
        }
    }

    bgfx::DynamicVertexBufferHandle instanceBuffer = BGFX_INVALID_HANDLE;
    std::uint32_t instanceStart = 0;
    if (!UploadParticleInstances(m_particleInstanceScratch.data(), count,
        instanceBuffer, instanceStart))
    {
        ++m_particleInstancingDebugStats.m_uploadFallbacks;
        return false;
    }

    const float cameraRight[4] =
    {
        modelCameraRight.x, modelCameraRight.y, modelCameraRight.z, 0.0f
    };
    const float cameraUp[4] =
    {
        modelCameraUp.x, modelCameraUp.y, modelCameraUp.z, 0.0f
    };
    bgfx::setUniform(m_particleCameraRightUniform, cameraRight);
    bgfx::setUniform(m_particleCameraUpUniform, cameraUp);

    SetModelTransform(mesh->GetWorldTransform());
    BindMaterialAndTexture(mesh);
    const bool useSoftParticles = CanUseSoftParticles();
    if (useSoftParticles)
        BindSoftParticleDepth();
    bgfx::setVertexBuffer(0, m_particleQuadVertexBuffer);
    bgfx::setIndexBuffer(m_particleQuadIndexBuffer);
    bgfx::setInstanceDataBuffer(instanceBuffer, instanceStart, count);
    bgfx::setState(BuildRenderState(shadowWrite));
    bgfx::setStencil(BuildStencilState());
    bgfx::submit(m_viewId, useSoftParticles ?
        m_softParticleProgram : m_particleProgram);

    ++m_particleInstancingDebugStats.m_instancedBatches;
    m_particleInstancingDebugStats.m_instancedParticles += count;

    // One unmistakable INFO line confirms that the optimized path has really
    // been used. All later activity is summarized at TRACE level.
    if (!m_particleInstancingFirstSuccessLogged)
    {
        const char* name = static_cast<const char*>(mesh->GetName());
        if (!name || !name[0])
            name = "<unnamed>";
        NiLogWriteFormat(NI_LOG_INFO, "NiBgfxRenderer", __FILE__, __LINE__,
            "[ParticleInstancing] ACTIVE: first instanced facing-quad system "
            "'%s', particles=%u, sorted=%s, instancePages=%u.",
            name, count, particles->IsSorted() ? "yes" : "no",
            static_cast<unsigned int>(m_particleInstancePages.size()));
        m_particleInstancingFirstSuccessLogged = true;
    }

    return true;
#endif
}

std::string BgfxRenderer::GetBackendShaderDirectory() const
{
    const char* backend = nullptr;
    switch (bgfx::getRendererType())
    {
    case bgfx::RendererType::Direct3D11: backend = "dx11"; break;
    // bgfx's D3D11 and D3D12 backends both consume the Windows shaderc
    // output produced from the same HLSL profile here. Keep one binary set.
    case bgfx::RendererType::Direct3D12: backend = "dx11"; break;
    case bgfx::RendererType::OpenGL: backend = "glsl"; break;
    case bgfx::RendererType::Vulkan: backend = "spirv"; break;
    default: backend = "unsupported"; break;
    }

    std::filesystem::path root = m_shaderRoot.empty() ?
        std::filesystem::path(NIBGFX_DEFAULT_SHADER_ROOT) : std::filesystem::path(m_shaderRoot);
    return (root / backend).string();
}

bgfx::ShaderHandle BgfxRenderer::LoadShader(const char* name) const
{
    if (!name || !*name)
        return BGFX_INVALID_HANDLE;

    const char* backend = nullptr;
    const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    switch (rendererType)
    {
    case bgfx::RendererType::Direct3D11:
    case bgfx::RendererType::Direct3D12: backend = "dx11"; break;
    case bgfx::RendererType::OpenGL:     backend = "glsl"; break;
    case bgfx::RendererType::Vulkan:     backend = "spirv"; break;
    default:                             backend = "unsupported"; break;
    }

    if (std::strcmp(backend, "unsupported") == 0)
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Renderer backend '%s' has no compiled NIFToolset shader set.",
            bgfx::getRendererName(rendererType));
        return BGFX_INVALID_HANDLE;
    }

    std::vector<std::filesystem::path> candidates;
    const auto appendCandidate = [&](const std::filesystem::path& root)
    {
        if (!root.empty())
            candidates.emplace_back(root / backend / name);
    };

    // An explicit application override always wins.
    if (!m_shaderRoot.empty())
        appendCandidate(std::filesystem::path(m_shaderRoot));

    // Build-tree path injected by CMake. This keeps developer builds working
    // regardless of their process working directory.
    appendCandidate(std::filesystem::path(NIBGFX_DEFAULT_SHADER_ROOT));

#if defined(EE_PLATFORM_WIN32)
    // Installed applications normally place shaders next to the executable in
    // Shaders/bgfx. Resolve from the executable itself instead of assuming the
    // process was launched with bin/ as its current working directory.
    std::array<wchar_t, 32768> modulePath = {};
    const DWORD moduleLength = GetModuleFileNameW(nullptr, modulePath.data(),
        static_cast<DWORD>(modulePath.size()));
    if (moduleLength > 0 && moduleLength < modulePath.size())
    {
        const std::filesystem::path executable(modulePath.data());
        appendCandidate(executable.parent_path() / "Shaders" / "bgfx");
    }
#endif

    // Final portable fallback for tools that intentionally set their working
    // directory to the install/bin folder.
    appendCandidate(std::filesystem::path("Shaders") / "bgfx");

    std::ifstream input;
    std::filesystem::path path;
    for (const std::filesystem::path& candidate : candidates)
    {
        input.close();
        input.clear();
        input.open(candidate, std::ios::binary | std::ios::ate);
        if (input)
        {
            path = candidate;
            break;
        }
    }
    if (!input)
    {
        std::string searched;
        for (const auto& candidate : candidates)
        {
            if (!searched.empty())
                searched += "; ";
            searched += candidate.string();
        }
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Shader '%s' was not found for backend '%s'. Searched: %s",
            name, backend, searched.empty() ? "<no paths>" : searched.c_str());
        return BGFX_INVALID_HANDLE;
    }

    const std::streamsize size = input.tellg();
    if (size <= 0 || size > static_cast<std::streamsize>(std::numeric_limits<std::uint32_t>::max() - 1))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Shader '%s' has invalid size %lld bytes at '%s'.", name,
            static_cast<long long>(size), path.string().c_str());
        return BGFX_INVALID_HANDLE;
    }
    input.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> bytes(static_cast<size_t>(size));
    if (!input.read(reinterpret_cast<char*>(bytes.data()), size))
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "Failed to read shader '%s' from '%s'.", name, path.string().c_str());
        return BGFX_INVALID_HANDLE;
    }

    const bgfx::Memory* memory = bgfx::copy(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
    bgfx::ShaderHandle shader = bgfx::createShader(memory);
    if (bgfx::isValid(shader))
    {
        bgfx::setName(shader, name);
        NiLogWriteFormat(NI_LOG_TRACE, "NiBgfxRenderer", __FILE__, __LINE__,
            "Loaded shader '%s' (%lld bytes) from '%s'.", name,
            static_cast<long long>(size), path.string().c_str());
    }
    else
    {
        NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
            "bgfx::createShader failed for '%s' loaded from '%s'. The binary may target the wrong renderer/profile.",
            name, path.string().c_str());
    }
    return shader;
}

bool BgfxRenderer::LoadBasicProgram()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_basic.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_basic.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }

    m_basicProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_basicProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the basic material program.", __FILE__, __LINE__);
    return bgfx::isValid(m_basicProgram);
}

bool BgfxRenderer::LoadInstancedProgram()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_instanced.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_basic.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }

    m_instancedProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_instancedProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the instanced material program.", __FILE__, __LINE__);
    return bgfx::isValid(m_instancedProgram);
}

bool BgfxRenderer::LoadParticlePrograms()
{
    const auto makeProgram = [this](const char* vertexName,
        const char* fragmentName, bgfx::ProgramHandle& output,
        const char* label)
    {
        bgfx::ShaderHandle vs = LoadShader(vertexName);
        bgfx::ShaderHandle fs = LoadShader(fragmentName);
        if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
        {
            if (bgfx::isValid(vs)) bgfx::destroy(vs);
            if (bgfx::isValid(fs)) bgfx::destroy(fs);
            return false;
        }

        output = bgfx::createProgram(vs, fs, true);
        if (!bgfx::isValid(output))
        {
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "bgfx::createProgram failed for %s.", label);
            return false;
        }
        return true;
    };

    if (!makeProgram("vs_ni_particle.bin", "fs_ni_basic.bin",
        m_particleProgram, "the particle billboard program"))
    {
        return false;
    }

    const bool softProgramsValid =
        makeProgram("vs_ni_particle.bin", "fs_ni_particle.bin",
            m_softParticleProgram, "the soft-particle billboard program") &&
        makeProgram("vs_ni_basic.bin", "fs_ni_particle.bin",
            m_softParticleFallbackProgram,
            "the generated-geometry soft-particle program") &&
        makeProgram("vs_ni_basic.bin", "fs_ni_soft_depth.bin",
            m_softDepthProgram, "the soft-particle depth program") &&
        makeProgram("vs_ni_instanced.bin", "fs_ni_soft_depth.bin",
            m_softDepthInstancedProgram,
            "the instanced soft-particle depth program") &&
        makeProgram("vs_ni_skinned.bin", "fs_ni_soft_depth.bin",
            m_softDepthSkinnedProgram,
            "the skinned soft-particle depth program") &&
        makeProgram("vs_ni_terrain.bin", "fs_ni_soft_depth.bin",
            m_softDepthTerrainProgram,
            "the terrain soft-particle depth program");

    if (!softProgramsValid)
    {
        bgfx::ProgramHandle* optionalPrograms[] =
        {
            &m_softParticleProgram, &m_softParticleFallbackProgram,
            &m_softDepthProgram, &m_softDepthInstancedProgram,
            &m_softDepthSkinnedProgram, &m_softDepthTerrainProgram
        };
        for (bgfx::ProgramHandle* program : optionalPrograms)
        {
            if (bgfx::isValid(*program))
                bgfx::destroy(*program);
            *program = BGFX_INVALID_HANDLE;
        }
        NiLogWrite(NI_LOG_WARNING, "NiBgfxRenderer",
            "Soft-particle shaders are unavailable. Normal particle rendering remains enabled.",
            __FILE__, __LINE__);
    }

    return true;
}

bool BgfxRenderer::LoadSkinnedPrograms()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_skinned.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_basic.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }
    m_skinnedProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_skinnedProgram))
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer",
            "bgfx::createProgram failed for the skinned material program.",
            __FILE__, __LINE__);
        return false;
    }

    bgfx::ShaderHandle shadowVs = LoadShader("vs_ni_skinned_shadow.bin");
    bgfx::ShaderHandle shadowFs = LoadShader("fs_ni_shadow.bin");
    if (!bgfx::isValid(shadowVs) || !bgfx::isValid(shadowFs))
    {
        if (bgfx::isValid(shadowVs)) bgfx::destroy(shadowVs);
        if (bgfx::isValid(shadowFs)) bgfx::destroy(shadowFs);
        return false;
    }
    m_skinnedShadowProgram = bgfx::createProgram(shadowVs, shadowFs, true);
    if (!bgfx::isValid(m_skinnedShadowProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the skinned shadow program.", __FILE__, __LINE__);
    return bgfx::isValid(m_skinnedShadowProgram);
}

bool BgfxRenderer::LoadTerrainProgram()
{
    const auto makeProgram = [this](const char* fragmentName,
        bgfx::ProgramHandle& output)
    {
        bgfx::ShaderHandle vs = LoadShader("vs_ni_terrain.bin");
        bgfx::ShaderHandle fs = LoadShader(fragmentName);
        if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
        {
            if (bgfx::isValid(vs)) bgfx::destroy(vs);
            if (bgfx::isValid(fs)) bgfx::destroy(fs);
            return false;
        }
        output = bgfx::createProgram(vs, fs, true);
        if (!bgfx::isValid(output))
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "bgfx::createProgram failed for terrain fragment shader '%s'.", fragmentName);
        return bgfx::isValid(output);
    };

    return makeProgram("fs_ni_terrain.bin", m_terrainProgram) &&
        makeProgram("fs_ni_terrain_cube.bin", m_terrainCubeShadowProgram);
}

bool BgfxRenderer::LoadExtendedPrograms()
{
    const auto makeProgram = [this](const char* vertexName,
        bgfx::ProgramHandle& output)
    {
        bgfx::ShaderHandle vs = LoadShader(vertexName);
        bgfx::ShaderHandle fs = LoadShader("fs_ni_extended.bin");
        if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
        {
            if (bgfx::isValid(vs)) bgfx::destroy(vs);
            if (bgfx::isValid(fs)) bgfx::destroy(fs);
            return false;
        }
        output = bgfx::createProgram(vs, fs, true);
        if (!bgfx::isValid(output))
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "bgfx::createProgram failed for NiExtendedMaterial vertex shader '%s'.", vertexName);
        return bgfx::isValid(output);
    };

    return makeProgram("vs_ni_basic.bin", m_extendedProgram) &&
        makeProgram("vs_ni_instanced.bin", m_extendedInstancedProgram) &&
        makeProgram("vs_ni_skinned.bin", m_extendedSkinnedProgram);
}

bool BgfxRenderer::LoadDecorationPrograms()
{
    const auto makeProgram = [this](const char* vertexName,
        bgfx::ProgramHandle& output)
    {
        bgfx::ShaderHandle vs = LoadShader(vertexName);
        bgfx::ShaderHandle fs = LoadShader("fs_ni_decoration.bin");
        if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
        {
            if (bgfx::isValid(vs)) bgfx::destroy(vs);
            if (bgfx::isValid(fs)) bgfx::destroy(fs);
            return false;
        }
        output = bgfx::createProgram(vs, fs, true);
        if (!bgfx::isValid(output))
            NiLogWriteFormat(NI_LOG_ERROR, "NiBgfxRenderer", __FILE__, __LINE__,
                "bgfx::createProgram failed for NiDecorationMaterial vertex shader '%s'.", vertexName);
        return bgfx::isValid(output);
    };

    return makeProgram("vs_ni_basic.bin", m_decorationProgram) &&
        makeProgram("vs_ni_instanced.bin", m_decorationInstancedProgram) &&
        makeProgram("vs_ni_skinned.bin", m_decorationSkinnedProgram);
}

bool BgfxRenderer::LoadSkyProgram()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_sky.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_sky.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }

    m_skyProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_skyProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for NiSkyMaterial.", __FILE__, __LINE__);
    return bgfx::isValid(m_skyProgram);
}

bool BgfxRenderer::LoadShadowPrograms()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_shadow.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_shadow.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }
    m_shadowProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_shadowProgram))
    {
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the shadow program.", __FILE__, __LINE__);
        return false;
    }

    bgfx::ShaderHandle instancedVs = LoadShader("vs_ni_instanced_shadow.bin");
    bgfx::ShaderHandle instancedFs = LoadShader("fs_ni_shadow.bin");
    if (!bgfx::isValid(instancedVs) || !bgfx::isValid(instancedFs))
    {
        if (bgfx::isValid(instancedVs)) bgfx::destroy(instancedVs);
        if (bgfx::isValid(instancedFs)) bgfx::destroy(instancedFs);
        return false;
    }
    m_instancedShadowProgram = bgfx::createProgram(instancedVs, instancedFs, true);
    if (!bgfx::isValid(m_instancedShadowProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the instanced shadow program.", __FILE__, __LINE__);
    return bgfx::isValid(m_instancedShadowProgram);
}

bool BgfxRenderer::LoadVsmBlurProgram()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_vsm_blur.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_vsm_blur.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }

    m_vsmBlurProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_vsmBlurProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the VSM blur program.", __FILE__, __LINE__);
    return bgfx::isValid(m_vsmBlurProgram);
}

bool BgfxRenderer::LoadCopyProgram()
{
    bgfx::ShaderHandle vs = LoadShader("vs_ni_copy.bin");
    bgfx::ShaderHandle fs = LoadShader("fs_ni_copy.bin");
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs))
    {
        if (bgfx::isValid(vs)) bgfx::destroy(vs);
        if (bgfx::isValid(fs)) bgfx::destroy(fs);
        return false;
    }

    m_copyProgram = bgfx::createProgram(vs, fs, true);
    if (!bgfx::isValid(m_copyProgram))
        NiLogWrite(NI_LOG_ERROR, "NiBgfxRenderer", "bgfx::createProgram failed for the framebuffer copy program.", __FILE__, __LINE__);
    return bgfx::isValid(m_copyProgram);
}
