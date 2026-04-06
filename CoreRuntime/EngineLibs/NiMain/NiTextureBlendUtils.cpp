#include "NiMainPCH.h"
#include "NiTextureBlendUtils.h"
#include "NiImageConverter.h"
#include <algorithm>
#include <vector>

//---------------------------------------------------------------------------
// 4-channel float pixel used for bilinear interpolation accumulation.
struct Pixel4f { float c[4]; };

//---------------------------------------------------------------------------
// Internal state for a single loaded-and-validated layer.
struct LayerEntry
{
    NiPointer<NiPixelData> spMask;      // RGBA32 alpha map  (R = coverage)
    NiPointer<NiPixelData> spTex;       // RGBA32 colour texture
    NiPoint2  kUVScale;                 // tile repetitions
    float     fBlendWeight;             // scales mask coverage into [0,1]
    bool      bMirrorU;                 // mirror alpha map horizontally
    bool      bMirrorV;                 // mirror alpha map vertically
};

//---------------------------------------------------------------------------
// Ensure pkTex has an RGBA32 pixel store the CPU can read channel-by-channel.
// Converts from any other format via NiImageConverter if necessary.
static NiPointer<NiPixelData> GetReadablePixels(NiSourceTexture* pkTex)
{
    if (!pkTex)
        return NULL;

    NiPixelData* pkRaw = pkTex->GetSourcePixelData();
    if (!pkRaw)
        return NULL;

    const NiPixelFormat& kFmt = pkRaw->GetPixelFormat();
    if (kFmt == NiPixelFormat::RGBA32)
        return pkRaw;

    NiImageConverter* pkConv = NiImageConverter::GetImageConverter();
    if (!pkConv)
    {
        NiOutputDebugString(
            "NiTextureBlendUtils: no NiImageConverter — cannot convert to RGBA32\n");
        return NULL;
    }

    NiPointer<NiPixelData> spConverted =
        pkConv->ConvertPixelData(*pkRaw, NiPixelFormat::RGBA32, NULL, false);

    if (!spConverted)
    {
        char buf[128];
        NiSprintf(buf, 128,
            "NiTextureBlendUtils: ConvertPixelData failed — BPP=%u compressed=%d\n",
            pkRaw->GetPixelFormat().GetBitsPerPixel(),
            (int)pkRaw->GetPixelFormat().GetCompressed());
        NiOutputDebugString(buf);
    }

    return spConverted;
}

//---------------------------------------------------------------------------
// Returns the next power of two >= x, capped at uiMax.
static NiUInt32 NextPow2Capped(NiUInt32 x, NiUInt32 uiMax)
{
    NiUInt32 p = 1u;
    while (p < x && p < uiMax)
        p <<= 1;
    return (p > uiMax) ? uiMax : p;
}

//---------------------------------------------------------------------------
// Fetch a single RGBA32 texel with clamp-to-edge addressing.
static Pixel4f FetchTexelClamp(const NiPixelData* pkData, NiInt32 iX, NiInt32 iY)
{
    const NiUInt32 uiW = pkData->GetWidth();
    const NiUInt32 uiH = pkData->GetHeight();
    const NiUInt32 uiX = (iX < 0) ? 0u : ((NiUInt32)iX >= uiW ? uiW - 1 : (NiUInt32)iX);
    const NiUInt32 uiY = (iY < 0) ? 0u : ((NiUInt32)iY >= uiH ? uiH - 1 : (NiUInt32)iY);
    const NiUInt8* p   = (*pkData)(uiX, uiY);
    Pixel4f px;
    px.c[0] = (float)p[0]; px.c[1] = (float)p[1];
    px.c[2] = (float)p[2]; px.c[3] = (float)p[3];
    return px;
}

//---------------------------------------------------------------------------
// Fetch a single RGBA32 texel with wrap addressing.
// Only ±1 OOB is possible given pixel-centre UV inputs, so a single fold suffices.
static Pixel4f FetchTexelWrap(const NiPixelData* pkData, NiInt32 iX, NiInt32 iY)
{
    const NiUInt32 uiW = pkData->GetWidth();
    const NiUInt32 uiH = pkData->GetHeight();
    const NiUInt32 uiX = (iX < 0) ? uiW - 1 : ((NiUInt32)iX >= uiW ? 0u : (NiUInt32)iX);
    const NiUInt32 uiY = (iY < 0) ? uiH - 1 : ((NiUInt32)iY >= uiH ? 0u : (NiUInt32)iY);
    const NiUInt8* p   = (*pkData)(uiX, uiY);
    Pixel4f px;
    px.c[0] = (float)p[0]; px.c[1] = (float)p[1];
    px.c[2] = (float)p[2]; px.c[3] = (float)p[3];
    return px;
}

//---------------------------------------------------------------------------
static inline Pixel4f BilinearLerp(
    const Pixel4f& p00, const Pixel4f& p10,
    const Pixel4f& p01, const Pixel4f& p11,
    float fFX, float fFY)
{
    const float w00 = (1.0f - fFX) * (1.0f - fFY);
    const float w10 =          fFX  * (1.0f - fFY);
    const float w01 = (1.0f - fFX) *          fFY;
    const float w11 =          fFX  *          fFY;
    Pixel4f out;
    for (int i = 0; i < 4; ++i)
        out.c[i] = w00*p00.c[i] + w10*p10.c[i] + w01*p01.c[i] + w11*p11.c[i];
    return out;
}

//---------------------------------------------------------------------------
// Bilinear sample with clamp-to-edge + optional per-axis mirror.
// Mirror is applied in continuous texel space before floor/frac so that the
// reflection is pixel-perfect with no boundary error.
static Pixel4f SampleBilinearClamp(
    const NiPixelData* pkData,
    float fU, float fV,
    bool bMirrorU, bool bMirrorV)
{
    const NiUInt32 uiW = pkData->GetWidth();
    const NiUInt32 uiH = pkData->GetHeight();

    float fTexX = fU * (float)uiW - 0.5f;
    float fTexY = fV * (float)uiH - 0.5f;

    if (bMirrorU) fTexX = (float)(uiW - 1) - fTexX;
    if (bMirrorV) fTexY = (float)(uiH - 1) - fTexY;

    const NiInt32 iX0 = (NiInt32)NiFloor(fTexX);
    const NiInt32 iY0 = (NiInt32)NiFloor(fTexY);

    return BilinearLerp(
        FetchTexelClamp(pkData, iX0,     iY0    ),
        FetchTexelClamp(pkData, iX0 + 1, iY0    ),
        FetchTexelClamp(pkData, iX0,     iY0 + 1),
        FetchTexelClamp(pkData, iX0 + 1, iY0 + 1),
        fTexX - (float)iX0, fTexY - (float)iY0);
}

//---------------------------------------------------------------------------
// Bilinear sample with wrap addressing — seamless across tile boundaries.
static Pixel4f SampleBilinearWrap(
    const NiPixelData* pkData,
    float fU, float fV)
{
    const NiUInt32 uiW = pkData->GetWidth();
    const NiUInt32 uiH = pkData->GetHeight();

    const float fTexX = fU * (float)uiW - 0.5f;
    const float fTexY = fV * (float)uiH - 0.5f;
    const NiInt32 iX0 = (NiInt32)NiFloor(fTexX);
    const NiInt32 iY0 = (NiInt32)NiFloor(fTexY);

    return BilinearLerp(
        FetchTexelWrap(pkData, iX0,     iY0    ),
        FetchTexelWrap(pkData, iX0 + 1, iY0    ),
        FetchTexelWrap(pkData, iX0,     iY0 + 1),
        FetchTexelWrap(pkData, iX0 + 1, iY0 + 1),
        fTexX - (float)iX0, fTexY - (float)iY0);
}

//---------------------------------------------------------------------------
// Per-layer cache built once before the pixel loops.
struct LayerCache
{
    const NiUInt8* pMask;
    const NiUInt8* pTex;
    NiUInt32 uiMaskW, uiMaskH;
    NiUInt32 uiTexW, uiTexH;
    float    fBlendWeight;
    float    fUVScaleX, fUVScaleY;
    bool     bMirrorU, bMirrorV;
};

struct BilinearAxisCoord
{
    NiUInt32 uiOffset0;
    NiUInt32 uiOffset1;
    float    fWeight0;
    float    fWeight1;
};

static inline BilinearAxisCoord MakeClampAxisCoord(
    float    fTexel,
    NiUInt32 uiMaxCoord,
    NiUInt32 uiStride)
{
    const NiInt32 i0 = (NiInt32)NiFloor(fTexel);
    const float   fFrac = fTexel - (float)i0;

    auto ClampCoord = [uiMaxCoord](NiInt32 v) -> NiUInt32
    {
        return (v < 0) ? 0u : ((NiUInt32)v > uiMaxCoord ? uiMaxCoord : (NiUInt32)v);
    };

    const NiUInt32 ui0 = ClampCoord(i0);
    const NiUInt32 ui1 = ClampCoord(i0 + 1);
    BilinearAxisCoord kCoord;
    kCoord.uiOffset0 = ui0 * uiStride;
    kCoord.uiOffset1 = ui1 * uiStride;
    kCoord.fWeight0 = 1.0f - fFrac;
    kCoord.fWeight1 = fFrac;
    return kCoord;
}

static inline BilinearAxisCoord MakeWrapAxisCoord(
    float    fTexel,
    NiUInt32 uiMaxCoord,
    NiUInt32 uiStride)
{
    const NiInt32 i0 = (NiInt32)NiFloor(fTexel);
    const float   fFrac = fTexel - (float)i0;

    auto WrapCoord = [uiMaxCoord](NiInt32 v) -> NiUInt32
    {
        return (v < 0) ? uiMaxCoord : ((NiUInt32)v > uiMaxCoord ? 0u : (NiUInt32)v);
    };

    const NiUInt32 ui0 = WrapCoord(i0);
    const NiUInt32 ui1 = WrapCoord(i0 + 1);
    BilinearAxisCoord kCoord;
    kCoord.uiOffset0 = ui0 * uiStride;
    kCoord.uiOffset1 = ui1 * uiStride;
    kCoord.fWeight0 = 1.0f - fFrac;
    kCoord.fWeight1 = fFrac;
    return kCoord;
}

static std::vector<BilinearAxisCoord> BuildClampAxisCoords(
    NiUInt32 uiOutSize,
    NiUInt32 uiSrcSize,
    NiUInt32 uiStride,
    bool     bMirror)
{
    std::vector<BilinearAxisCoord> kCoords(uiOutSize);
    const float fInvOut = 1.0f / (float)uiOutSize;
    const NiUInt32 uiMaxCoord = uiSrcSize - 1u;

    for (NiUInt32 i = 0; i < uiOutSize; ++i)
    {
        float fTexel = (((float)i) + 0.5f) * fInvOut * (float)uiSrcSize - 0.5f;
        if (bMirror)
            fTexel = (float)uiMaxCoord - fTexel;
        kCoords[i] = MakeClampAxisCoord(fTexel, uiMaxCoord, uiStride);
    }

    return kCoords;
}

static std::vector<BilinearAxisCoord> BuildWrapAxisCoords(
    NiUInt32 uiOutSize,
    NiUInt32 uiSrcSize,
    NiUInt32 uiStride,
    float    fScale)
{
    std::vector<BilinearAxisCoord> kCoords(uiOutSize);
    const float fInvOut = 1.0f / (float)uiOutSize;
    const NiUInt32 uiMaxCoord = uiSrcSize - 1u;

    for (NiUInt32 i = 0; i < uiOutSize; ++i)
    {
        const float fUV = (((float)i) + 0.5f) * fInvOut;
        const float fTiled = NiFmod(fUV * fScale, 1.0f);
        const float fTexel = fTiled * (float)uiSrcSize - 0.5f;
        kCoords[i] = MakeWrapAxisCoord(fTexel, uiMaxCoord, uiStride);
    }

    return kCoords;
}

static inline float SampleRedBilinear(
    const NiUInt8*             pBase,
    const BilinearAxisCoord&   kX,
    const BilinearAxisCoord&   kY)
{
    const float fRow0 =
        kX.fWeight0 * (float)pBase[kY.uiOffset0 + kX.uiOffset0] +
        kX.fWeight1 * (float)pBase[kY.uiOffset0 + kX.uiOffset1];
    const float fRow1 =
        kX.fWeight0 * (float)pBase[kY.uiOffset1 + kX.uiOffset0] +
        kX.fWeight1 * (float)pBase[kY.uiOffset1 + kX.uiOffset1];
    return kY.fWeight0 * fRow0 + kY.fWeight1 * fRow1;
}

static inline void SampleRGBBilinear(
    const NiUInt8*             pBase,
    const BilinearAxisCoord&   kX,
    const BilinearAxisCoord&   kY,
    float&                     fR,
    float&                     fG,
    float&                     fB)
{
    const NiUInt8* p00 = pBase + kY.uiOffset0 + kX.uiOffset0;
    const NiUInt8* p10 = pBase + kY.uiOffset0 + kX.uiOffset1;
    const NiUInt8* p01 = pBase + kY.uiOffset1 + kX.uiOffset0;
    const NiUInt8* p11 = pBase + kY.uiOffset1 + kX.uiOffset1;

    const float fR0 = kX.fWeight0 * (float)p00[0] + kX.fWeight1 * (float)p10[0];
    const float fG0 = kX.fWeight0 * (float)p00[1] + kX.fWeight1 * (float)p10[1];
    const float fB0 = kX.fWeight0 * (float)p00[2] + kX.fWeight1 * (float)p10[2];
    const float fR1 = kX.fWeight0 * (float)p01[0] + kX.fWeight1 * (float)p11[0];
    const float fG1 = kX.fWeight0 * (float)p01[1] + kX.fWeight1 * (float)p11[1];
    const float fB1 = kX.fWeight0 * (float)p01[2] + kX.fWeight1 * (float)p11[2];

    fR = kY.fWeight0 * fR0 + kY.fWeight1 * fR1;
    fG = kY.fWeight0 * fG0 + kY.fWeight1 * fG1;
    fB = kY.fWeight0 * fB0 + kY.fWeight1 * fB1;
}

//---------------------------------------------------------------------------
// Single-pass terrain splat compositor.
//
// Layers must already be sorted bottom-to-top (ascending uiTextureID).
// For every output pixel the compositor walks all layers in order and
// accumulates colour using the Painter's algorithm in linear float space:
//
//   fCoverage  = clamp(mask.R / 255 * fBlendWeight, 0, 1)
//   colour.rgb += fCoverage * (layerTex.rgb - colour.rgb)   // alpha-over
//
// Each mask is sampled with bilinear + clamp-to-edge (+ optional mirror).
// Each colour texture is sampled with bilinear + wrap for seamless tiling.
// Pixel-centre UVs ((x+0.5)/W) are used throughout to avoid the half-texel
// offset that appears at the top-left corner with edge UVs.
static NiPointer<NiPixelData> CompositeLayersPD(
    const LayerEntry* pkLayers,
    NiUInt32          uiLayerCount,
    NiUInt32          uiOutW,
    NiUInt32          uiOutH)
{
    NIASSERT(pkLayers && uiLayerCount > 0 && uiOutW > 0 && uiOutH > 0);

    NiPixelData* pkResult = NiNew NiPixelData(uiOutW, uiOutH, NiPixelFormat::RGBA32);

    struct PreparedLayer
    {
        LayerCache kCache;
        std::vector<BilinearAxisCoord> kMaskX;
        std::vector<BilinearAxisCoord> kMaskY;
        std::vector<BilinearAxisCoord> kTexX;
        std::vector<BilinearAxisCoord> kTexY;
        float fCoverageScale;
    };

    // Precompute raw pointers, dimensions, and bilinear sample coordinates once.
    std::vector<PreparedLayer> kPrepared(uiLayerCount);
    for (NiUInt32 i = 0; i < uiLayerCount; ++i)
    {
        const LayerEntry& kL = pkLayers[i];
        PreparedLayer& kP = kPrepared[i];
        LayerCache& kC = kP.kCache;
        kC.pMask        = kL.spMask->GetPixels(0);
        kC.pTex         = kL.spTex ->GetPixels(0);
        kC.uiMaskW      = kL.spMask->GetWidth();
        kC.uiMaskH      = kL.spMask->GetHeight();
        kC.uiTexW       = kL.spTex ->GetWidth();
        kC.uiTexH       = kL.spTex ->GetHeight();
        kC.fBlendWeight = kL.fBlendWeight;
        kC.fUVScaleX    = kL.kUVScale.x;
        kC.fUVScaleY    = kL.kUVScale.y;
        kC.bMirrorU     = kL.bMirrorU;
        kC.bMirrorV     = kL.bMirrorV;
        kP.kMaskX       = BuildClampAxisCoords(uiOutW, kC.uiMaskW, 4u, kC.bMirrorU);
        kP.kMaskY       = BuildClampAxisCoords(uiOutH, kC.uiMaskH, kC.uiMaskW * 4u, kC.bMirrorV);
        kP.kTexX        = BuildWrapAxisCoords(uiOutW, kC.uiTexW, 4u, kC.fUVScaleX);
        kP.kTexY        = BuildWrapAxisCoords(uiOutH, kC.uiTexH, kC.uiTexW * 4u, kC.fUVScaleY);
        kP.fCoverageScale = kC.fBlendWeight / 255.0f;
    }
    NiUInt8* pOutBase = pkResult->GetPixels(0);

    for (NiUInt32 uiY = 0; uiY < uiOutH; ++uiY)
    {
        NiUInt8* pOutRow = pOutBase + uiY * uiOutW * 4u;

        for (NiUInt32 uiX = 0; uiX < uiOutW; ++uiX)
        {
            float fR = 0.0f, fG = 0.0f, fB = 0.0f;

            for (NiUInt32 i = 0; i < uiLayerCount; ++i)
            {
                const PreparedLayer& kP = kPrepared[i];
                const float fMaskR = SampleRedBilinear(
                    kP.kCache.pMask,
                    kP.kMaskX[uiX],
                    kP.kMaskY[uiY]);
                const float fCoverage = NiClamp(fMaskR * kP.fCoverageScale, 0.0f, 1.0f);
                if (fCoverage < (1.0f / 255.0f))
                    continue;

                float kTexR, kTexG, kTexB;
                SampleRGBBilinear(
                    kP.kCache.pTex,
                    kP.kTexX[uiX],
                    kP.kTexY[uiY],
                    kTexR, kTexG, kTexB);

                // Painter's algorithm (alpha-over) in linear float space.
                fR += fCoverage * (kTexR - fR);
                fG += fCoverage * (kTexG - fG);
                fB += fCoverage * (kTexB - fB);
            }

            NiUInt8* pOut = pOutRow + uiX * 4u;
            pOut[0] = (NiUInt8)NiMin((NiUInt32)(fR + 0.5f), 255u);
            pOut[1] = (NiUInt8)NiMin((NiUInt32)(fG + 0.5f), 255u);
            pOut[2] = (NiUInt8)NiMin((NiUInt32)(fB + 0.5f), 255u);
            pOut[3] = 255u;
        }
    }

    return NiPointer<NiPixelData>(pkResult);
}

//---------------------------------------------------------------------------
// Box-filter mip-map generator.
// Produces a new NiPixelData containing the complete mip chain for pkBase
// (which must be RGBA32). Each level is a 2×2 average of the level above.
static NiPointer<NiPixelData> GenerateMipChain(NiPixelData* pkBase)
{
    NIASSERT(pkBase);

    const NiUInt32 uiW = pkBase->GetWidth();
    const NiUInt32 uiH = pkBase->GetHeight();

    NiUInt32 uiNumMips = 1;
    { NiUInt32 w = uiW, h = uiH; while (w > 1 || h > 1) { w = NiMax(1u, w >> 1); h = NiMax(1u, h >> 1); ++uiNumMips; } }

    NiPixelData* pkResult = NiNew NiPixelData(uiW, uiH, NiPixelFormat::RGBA32, uiNumMips);
    NiMemcpy(pkResult->GetPixels(0), pkBase->GetPixels(0), uiW * uiH * 4u);

    for (NiUInt32 uiMip = 1; uiMip < uiNumMips; ++uiMip)
    {
        const NiUInt32 uiSrcW = pkResult->GetWidth(uiMip - 1);
        const NiUInt32 uiSrcH = pkResult->GetHeight(uiMip - 1);
        const NiUInt32 uiDstW = pkResult->GetWidth(uiMip);
        const NiUInt32 uiDstH = pkResult->GetHeight(uiMip);

        const NiUInt8* pSrc = pkResult->GetPixels(uiMip - 1);
        NiUInt8*       pDst = pkResult->GetPixels(uiMip);

        for (NiUInt32 uiDY = 0; uiDY < uiDstH; ++uiDY)
        {
            const NiUInt32 sy0       = uiDY * 2;
            const NiUInt32 sy1       = NiMin(sy0 + 1, uiSrcH - 1);
            const NiUInt32 uiRow0    = sy0 * uiSrcW;   // ← precomputed once per row
            const NiUInt32 uiRow1    = sy1 * uiSrcW;   // ← precomputed once per row

            for (NiUInt32 uiDX = 0; uiDX < uiDstW; ++uiDX)
            {
                const NiUInt32 sx0 = uiDX * 2;
                const NiUInt32 sx1 = NiMin(sx0 + 1, uiSrcW - 1);

                // Process all 4 channels as a single uint32 accumulation.
                const NiUInt8* p00 = pSrc + (uiRow0 + sx0) * 4;
                const NiUInt8* p10 = pSrc + (uiRow0 + sx1) * 4;
                const NiUInt8* p01 = pSrc + (uiRow1 + sx0) * 4;
                const NiUInt8* p11 = pSrc + (uiRow1 + sx1) * 4;
                NiUInt8* pD = pDst + (uiDY * uiDstW + uiDX) * 4;

                pD[0] = (NiUInt8)(((NiUInt32)p00[0] + p10[0] + p01[0] + p11[0]) >> 2);
                pD[1] = (NiUInt8)(((NiUInt32)p00[1] + p10[1] + p01[1] + p11[1]) >> 2);
                pD[2] = (NiUInt8)(((NiUInt32)p00[2] + p10[2] + p01[2] + p11[2]) >> 2);
                pD[3] = (NiUInt8)(((NiUInt32)p00[3] + p10[3] + p01[3] + p11[3]) >> 2);
            }
        }
    }

    return NiPointer<NiPixelData>(pkResult);
}

//---------------------------------------------------------------------------
// Generates a full CPU mip chain for spPixelData and uploads it as an
// NiSourceTexture with TRUE_COLOR_32 + SMOOTH alpha + mip-mapping enabled.
static NiSourceTexture* WrapAsTexture(NiPointer<NiPixelData> spPixelData)
{
    if (!spPixelData)
        return NULL;

    // Build the full mip chain on the CPU so every GPU mip level has correct
    // box-filtered data. Without this, D3D11 GenerateMips (which requires
    // MISC_GENERATE_MIPS) may leave mips 1+ uninitialized (black), producing
    // visible trilinear blur artefacts at distance.
    NiPointer<NiPixelData> spMipped = GenerateMipChain(spPixelData);

    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eAlphaFmt    = NiTexture::FormatPrefs::SMOOTH;
    kPrefs.m_eMipMapped   = NiTexture::FormatPrefs::YES;

    return NiSourceTexture::Create(
        spMipped ? spMipped : spPixelData, kPrefs, false);
}

//---------------------------------------------------------------------------
NiSourceTexture* NiTextureBlendUtils::LoadCPUTexture(const char* pcPath)
{
    NIASSERT(pcPath);

    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eAlphaFmt    = NiTexture::FormatPrefs::SMOOTH;
    kPrefs.m_eMipMapped   = NiTexture::FormatPrefs::NO;

    return NiSourceTexture::Create(pcPath, kPrefs, false, false, false);
}

//---------------------------------------------------------------------------
NiSourceTexture* NiTextureBlendUtils::BuildComposite(
    const SplatLayer* pkLayers,
    NiUInt32          uiLayerCount,
    NiUInt32          uiOutW,
    NiUInt32          uiOutH)
{
    NIASSERT(pkLayers);
    NIASSERT(uiLayerCount > 0 && uiOutW > 0 && uiOutH > 0);

    // Build a sorted index array by uiTextureID ascending (bottom → top).
    std::vector<NiUInt32> kOrder(uiLayerCount);
    for (NiUInt32 i = 0; i < uiLayerCount; ++i)
        kOrder[i] = i;
    std::sort(kOrder.begin(), kOrder.end(),
        [pkLayers](NiUInt32 a, NiUInt32 b)
        { return pkLayers[a].uiTextureID < pkLayers[b].uiTextureID; });

    // Load and validate each layer in sorted order.
    std::vector<LayerEntry> kEntries;
    kEntries.reserve(uiLayerCount);

    for (NiUInt32 uiSortedIdx = 0; uiSortedIdx < uiLayerCount; ++uiSortedIdx)
    {
        const NiUInt32    uiIdx = kOrder[uiSortedIdx];
        const SplatLayer& kL    = pkLayers[uiIdx];

        if (kL.fBlendWeight <= 0.0f)
            continue;

        NiPointer<NiSourceTexture> spAlpha(LoadCPUTexture(kL.pcAlphaMapPath.c_str()));
        NiPointer<NiSourceTexture> spTex  (LoadCPUTexture(kL.pcTexturePath.c_str()));

        if (!spAlpha || !spTex)
        {
            char buf[512];
            NiSprintf(buf, sizeof(buf),
                "NiTextureBlendUtils: layer tid=%u skipped (load failed)\n"
                "  alpha='%s'\n  tex='%s'\n",
                kL.uiTextureID,
                kL.pcAlphaMapPath.c_str(),
                kL.pcTexturePath.c_str());
            NiOutputDebugString(buf);
            continue;
        }

        NiPointer<NiPixelData> spMaskPD(GetReadablePixels(spAlpha));
        NiPointer<NiPixelData> spTexPD (GetReadablePixels(spTex));

        if (!spMaskPD || !spTexPD)
        {
            NiOutputDebugString(
                "NiTextureBlendUtils: layer skipped (pixel data unavailable)\n");
            continue;
        }

        {
            char buf[256];
            NiSprintf(buf, 256,
                "NiTextureBlendUtils: tid=%u — tex %ux%u  mask %ux%u  "
                "uvScale=(%.2f,%.2f)  weight=%.3f\n",
                kL.uiTextureID,
                spTexPD->GetWidth(),  spTexPD->GetHeight(),
                spMaskPD->GetWidth(), spMaskPD->GetHeight(),
                kL.kUVScale.x, kL.kUVScale.y,
                kL.fBlendWeight);
            NiOutputDebugString(buf);
        }

        LayerEntry kE;
        kE.spMask       = spMaskPD;
        kE.spTex        = spTexPD;
        kE.kUVScale     = kL.kUVScale;
        kE.fBlendWeight = kL.fBlendWeight;
        kE.bMirrorU     = kL.bMirrorAlphaU;
        kE.bMirrorV     = kL.bMirrorAlphaV;
        kEntries.push_back(std::move(kE));
    }

    if (kEntries.empty())
    {
        NiOutputDebugString("NiTextureBlendUtils::BuildComposite — no valid layers\n");
        return NULL;
    }

    return WrapAsTexture(
        CompositeLayersPD(kEntries.data(), (NiUInt32)kEntries.size(), uiOutW, uiOutH));
}

//---------------------------------------------------------------------------
NiSourceTexture* NiTextureBlendUtils::BlendWithMask(
    NiSourceTexture* pkAlphaMap,
    NiSourceTexture* pkTex,
    const NiPoint2&  kUVScale,
    NiUInt32         uiOutW,
    NiUInt32         uiOutH,
    bool             bMirrorMaskU,
    bool             bMirrorMaskV)
{
    NIASSERT(uiOutW > 0 && uiOutH > 0);
    NIASSERT(kUVScale.x > 0.0f && kUVScale.y > 0.0f);

    NiPointer<NiPixelData> spMask(GetReadablePixels(pkAlphaMap));
    NiPointer<NiPixelData> spTexPD(GetReadablePixels(pkTex));

    if (!spMask || !spTexPD)
    {
        NiOutputDebugString(
            "NiTextureBlendUtils::BlendWithMask — readable pixel data unavailable\n");
        return NULL;
    }

    // Output: RGB = tiled colour texture, A = mask red channel.
    // This preserves the coverage information so the caller can use it with Blend().
    NiPixelData* pkResult = NiNew NiPixelData(uiOutW, uiOutH, NiPixelFormat::RGBA32);

    const std::vector<BilinearAxisCoord> kMaskX =
        BuildClampAxisCoords(uiOutW, spMask->GetWidth(), 4u, bMirrorMaskU);
    const std::vector<BilinearAxisCoord> kMaskY =
        BuildClampAxisCoords(uiOutH, spMask->GetHeight(), spMask->GetWidth() * 4u, bMirrorMaskV);
    const std::vector<BilinearAxisCoord> kTexX =
        BuildWrapAxisCoords(uiOutW, spTexPD->GetWidth(), 4u, kUVScale.x);
    const std::vector<BilinearAxisCoord> kTexY =
        BuildWrapAxisCoords(uiOutH, spTexPD->GetHeight(), spTexPD->GetWidth() * 4u, kUVScale.y);

    const NiUInt8* pMaskBase = spMask->GetPixels(0);
    const NiUInt8* pTexBase = spTexPD->GetPixels(0);
    NiUInt8* pOutBase = pkResult->GetPixels(0);

    for (NiUInt32 uiY = 0; uiY < uiOutH; ++uiY)
    {
        NiUInt8* pOutRow = pOutBase + uiY * uiOutW * 4u;
        for (NiUInt32 uiX = 0; uiX < uiOutW; ++uiX)
        {
            float fTexR, fTexG, fTexB;
            SampleRGBBilinear(pTexBase, kTexX[uiX], kTexY[uiY], fTexR, fTexG, fTexB);
            const float fMaskR = SampleRedBilinear(pMaskBase, kMaskX[uiX], kMaskY[uiY]);

            NiUInt8* pOut = pOutRow + uiX * 4u;
            pOut[0] = (NiUInt8)NiMin((NiUInt32)(fTexR + 0.5f), 255u);
            pOut[1] = (NiUInt8)NiMin((NiUInt32)(fTexG + 0.5f), 255u);
            pOut[2] = (NiUInt8)NiMin((NiUInt32)(fTexB + 0.5f), 255u);
            pOut[3] = (NiUInt8)NiMin((NiUInt32)(fMaskR + 0.5f), 255u);
        }
    }

    return WrapAsTexture(NiPointer<NiPixelData>(pkResult));
}

//---------------------------------------------------------------------------
NiSourceTexture* NiTextureBlendUtils::Blend(
    NiSourceTexture* pkTexA,
    NiSourceTexture* pkTexB,
    float            fAlpha)
{
    NiPointer<NiPixelData> spA(GetReadablePixels(pkTexA));
    NiPointer<NiPixelData> spB(GetReadablePixels(pkTexB));

    if (!spA || !spB)
    {
        NiOutputDebugString("NiTextureBlendUtils::Blend — readable pixel data unavailable\n");
        return NULL;
    }

    const NiUInt32 uiW             = spA->GetWidth();
    const NiUInt32 uiH             = spA->GetHeight();
    const float    fClampedAlpha   = NiClamp(fAlpha, 0.0f, 1.0f);

    NiPixelData* pkResult = NiNew NiPixelData(uiW, uiH, NiPixelFormat::RGBA32);

    const NiUInt8* pABase = spA->GetPixels(0);
    const NiUInt8* pBBase = spB->GetPixels(0);
    NiUInt8* pOutBase = pkResult->GetPixels(0);

    if (spB->GetWidth() == uiW && spB->GetHeight() == uiH)
    {
        const NiUInt32 uiPixelCount = uiW * uiH;
        for (NiUInt32 i = 0; i < uiPixelCount; ++i)
        {
            const NiUInt8* pA = pABase + i * 4u;
            const NiUInt8* pB = pBBase + i * 4u;
            NiUInt8* pOut = pOutBase + i * 4u;
            for (int c = 0; c < 3; ++c)
            {
                const float fA = (float)pA[c];
                pOut[c] = (NiUInt8)NiMin(
                    (NiUInt32)(fA + fClampedAlpha * ((float)pB[c] - fA) + 0.5f), 255u);
            }
            pOut[3] = 255u;
        }

        return WrapAsTexture(NiPointer<NiPixelData>(pkResult));
    }

    const std::vector<BilinearAxisCoord> kBX =
        BuildClampAxisCoords(uiW, spB->GetWidth(), 4u, false);
    const std::vector<BilinearAxisCoord> kBY =
        BuildClampAxisCoords(uiH, spB->GetHeight(), spB->GetWidth() * 4u, false);

    for (NiUInt32 uiY = 0; uiY < uiH; ++uiY)
    {
        const NiUInt8* pARow = pABase + uiY * uiW * 4u;
        NiUInt8* pOutRow = pOutBase + uiY * uiW * 4u;
        for (NiUInt32 uiX = 0; uiX < uiW; ++uiX)
        {
            const NiUInt8* pA = pARow + uiX * 4u;
            float fBR, fBG, fBB;
            SampleRGBBilinear(pBBase, kBX[uiX], kBY[uiY], fBR, fBG, fBB);

            NiUInt8* pOut = pOutRow + uiX * 4u;
            for (int c = 0; c < 3; ++c)
            {
                const float fA = (float)pA[c];
                const float fB = (c == 0) ? fBR : (c == 1) ? fBG : fBB;
                pOut[c] = (NiUInt8)NiMin(
                    (NiUInt32)(fA + fClampedAlpha * (fB - fA) + 0.5f), 255u);
            }
            pOut[3] = 255u;
        }
    }

    return WrapAsTexture(NiPointer<NiPixelData>(pkResult));
}

//---------------------------------------------------------------------------
void NiTextureBlendUtils::RecommendOutputSize(
    const SplatLayer* pkLayers,
    NiUInt32          uiLayerCount,
    NiUInt32          uiMinPxPerTile,
    NiUInt32          uiMaxDim,
    NiUInt32&         uiOutW,
    NiUInt32&         uiOutH)
{
    NIASSERT(pkLayers && uiLayerCount > 0);
    NIASSERT(uiMinPxPerTile > 0 && uiMaxDim > 0);

    NiUInt32 uiBestW = 1u;
    NiUInt32 uiBestH = 1u;

    for (NiUInt32 i = 0; i < uiLayerCount; ++i)
    {
        const SplatLayer& kL = pkLayers[i];

        NiPointer<NiSourceTexture> spTex(LoadCPUTexture(kL.pcTexturePath.c_str()));
        if (!spTex)
            continue;

        NiPointer<NiPixelData> spPD(GetReadablePixels(spTex));
        if (!spPD)
            continue;

        // Ideal output = source texel size × tile count.
        // uiMinPxPerTile guards against a gigantic UV scale with a tiny texture.
        const NiUInt32 uiPxPerTileW = NiMax(spPD->GetWidth(),  uiMinPxPerTile);
        const NiUInt32 uiPxPerTileH = NiMax(spPD->GetHeight(), uiMinPxPerTile);

        const NiUInt32 uiNeedW = (NiUInt32)(kL.kUVScale.x * (float)uiPxPerTileW + 0.5f);
        const NiUInt32 uiNeedH = (NiUInt32)(kL.kUVScale.y * (float)uiPxPerTileH + 0.5f);

        if (uiNeedW > uiBestW) uiBestW = uiNeedW;
        if (uiNeedH > uiBestH) uiBestH = uiNeedH;
    }

    uiOutW = NextPow2Capped(uiBestW, uiMaxDim);
    uiOutH = NextPow2Capped(uiBestH, uiMaxDim);
}
