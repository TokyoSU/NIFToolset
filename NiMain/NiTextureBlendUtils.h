#pragma once
#include <NiSourceTexture.h>
#include <NiPixelData.h>
#include <NiPoint2.h>

/**
    NiTextureBlendUtils provides static utility methods for blending and tiling
    textures on the CPU. These are intended for one-time use during texture
    creation, not for real-time rendering.
    The output of these methods is a new NiSourceTexture with pixel data
    created by blending or tiling the input textures according to the method
    parameters. The output texture will have an application-level backing store
    (i.e. GetSourcePixelData() will be valid) but will not be marked as static.
    The caller is responsible for managing the lifetime of the returned texture
	and its pixel data, including deletion when no longer needed.
*/
class NIMAIN_ENTRY NiTextureBlendUtils
{
public:
    /**
        Tiles pkTexA across an explicit output size, masked by pkAlphaMap.

        The alpha map and the texture are both sampled independently at the
        output resolution, so neither needs to match uiOutW/uiOutH.

        - RGB output : pkTex sampled at (uiX/uiOutW * kUVScale.x) tiled.
        - Alpha output: pkAlphaMap red channel, bilinearly scaled to output.

        @param pkAlphaMap  White/black mask — drives per-pixel opacity.
        @param pkTex       Texture to tile over the output.
        @param kUVScale    Tile repetitions across the output
                           (e.g. NiPoint2(4,4) = 4x4 tiles).
        @param uiOutW      Output texture width  in pixels (e.g. terrain sector width).
        @param uiOutH      Output texture height in pixels (e.g. terrain sector height).
    */
    static NiSourceTexture* BlendWithMask(
        NiSourceTexture* pkAlphaMap,
        NiSourceTexture* pkTex,
        const NiPoint2&  kUVScale,
        NiUInt32         uiOutW,
        NiUInt32         uiOutH);

    /**
        Blends two RGBA textures using a constant alpha weight.
        Result = lerp(pkTexA, pkTexB, fAlpha)  where fAlpha is [0..1].
        Output dimensions match pkTexA.
    */
    static NiSourceTexture* Blend(
        NiSourceTexture* pkTexA,
        NiSourceTexture* pkTexB,
        float fAlpha);
};