#include "NiMainPCH.h"
#include "NiTextureBlendUtils.h"

//---------------------------------------------------------------------------
static NiPixelData* GetRawPixels(NiSourceTexture* pkTex)
{
    // Access the app-level pixel data (only valid before GPU upload)
    return pkTex ? pkTex->GetSourcePixelData() : NULL;
}

//---------------------------------------------------------------------------
// Nearest-neighbour sample of pkData at normalized UV [0..1)
static const NiUInt8* SampleNearest(const NiPixelData* pkData, float fU, float fV)
{
    const NiUInt32 uiX = (NiUInt32)(fU * (float)(pkData->GetWidth()  - 1));
    const NiUInt32 uiY = (NiUInt32)(fV * (float)(pkData->GetHeight() - 1));
    return (*pkData)(uiX, uiY);
}

//---------------------------------------------------------------------------
NiSourceTexture* NiTextureBlendUtils::BlendWithMask(
    NiSourceTexture* pkAlphaMap,
    NiSourceTexture* pkTexA,
    const NiPoint2&  kUVScale,
    NiUInt32         uiOutW,
    NiUInt32         uiOutH)
{
    NiPixelData* pkMask = GetRawPixels(pkAlphaMap);
    NiPixelData* pkTex  = GetRawPixels(pkTexA);

    NIASSERT(pkMask && pkTex);
    NIASSERT(uiOutW > 0 && uiOutH > 0);

    NiPixelData* pkResult = NiNew NiPixelData(uiOutW, uiOutH, NiPixelFormat::RGBA32);

    const float fInvOutW = 1.0f / (float)uiOutW;
    const float fInvOutH = 1.0f / (float)uiOutH;

    for (NiUInt32 uiY = 0; uiY < uiOutH; ++uiY)
    {
        // Normalized [0..1) position within the output
        const float fNormV = (float)uiY * fInvOutH;

        for (NiUInt32 uiX = 0; uiX < uiOutW; ++uiX)
        {
            const float fNormU = (float)uiX * fInvOutW;

            // Sample alpha map — scaled to fill the entire output
            const NiUInt8* pM = SampleNearest(pkMask, fNormU, fNormV);
            const float fCoverage = pM[0] / 255.0f;

            // Sample pkTexA — tiled kUVScale times across the output
            const float fTiledU = NiFmod(fNormU * kUVScale.x, 1.0f);
            const float fTiledV = NiFmod(fNormV * kUVScale.y, 1.0f);
            const NiUInt8* pTex = SampleNearest(pkTex, fTiledU, fTiledV);

            NiUInt8* pOut = (*pkResult)(uiX, uiY);

            // RGB from the tiled texture, alpha driven by the mask coverage
            pOut[0] = pTex[0];
            pOut[1] = pTex[1];
            pOut[2] = pTex[2];
            pOut[3] = (NiUInt8)(fCoverage * 255.0f);
        }
    }

    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eAlphaFmt    = NiTexture::FormatPrefs::SMOOTH;
    kPrefs.m_eMipMapped   = NiTexture::FormatPrefs::YES;

    return NiSourceTexture::Create(pkResult, kPrefs);
}

//---------------------------------------------------------------------------
NiSourceTexture* NiTextureBlendUtils::Blend(
    NiSourceTexture* pkTexA,
    NiSourceTexture* pkTexB,
    float fAlpha)
{
    NiPixelData* pkA = GetRawPixels(pkTexA);
    NiPixelData* pkB = GetRawPixels(pkTexB);

    NIASSERT(pkA && pkB);

    const NiUInt32 uiW = pkA->GetWidth();
    const NiUInt32 uiH = pkA->GetHeight();

    NiPixelData* pkResult = NiNew NiPixelData(uiW, uiH, NiPixelFormat::RGBA32);

    for (NiUInt32 uiY = 0; uiY < uiH; ++uiY)
    {
        for (NiUInt32 uiX = 0; uiX < uiW; ++uiX)
        {
            const NiUInt8* pA = (*pkA)(uiX, uiY);
            const NiUInt8* pB = (*pkB)(uiX, uiY);
            NiUInt8* pOut     = (*pkResult)(uiX, uiY);

            pOut[0] = (NiUInt8)(pA[0] + fAlpha * (pB[0] - pA[0]));
            pOut[1] = (NiUInt8)(pA[1] + fAlpha * (pB[1] - pA[1]));
            pOut[2] = (NiUInt8)(pA[2] + fAlpha * (pB[2] - pA[2]));
            pOut[3] = (NiUInt8)(pA[3] + fAlpha * (pB[3] - pA[3]));
        }
    }

    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eAlphaFmt    = NiTexture::FormatPrefs::SMOOTH;
    kPrefs.m_eMipMapped   = NiTexture::FormatPrefs::YES;

    return NiSourceTexture::Create(pkResult, kPrefs);
}
