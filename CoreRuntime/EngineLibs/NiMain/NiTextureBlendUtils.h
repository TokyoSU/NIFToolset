#pragma once
#include <NiSourceTexture.h>
#include <NiPixelData.h>
#include <NiPoint2.h>
#include <string>

/**
    NiTextureBlendUtils provides static utility methods for blending and tiling
    textures on the CPU. These are intended for one-time use during texture
    creation, not for real-time rendering.

    The output of these methods is a new NiSourceTexture with pixel data
    created by blending or tiling the input textures. The output texture will
    have an application-level backing store (GetSourcePixelData() is valid)
    but will not be marked as static. The caller is responsible for managing
    the lifetime of the returned texture.
*/
class NIMAIN_ENTRY NiTextureBlendUtils
{
public:
    /**
        Describes a single splat layer for use with BuildComposite.

        Coverage at each output pixel = mask.R / 255 * fBlendWeight,
        clamped to [0,1]. The colour texture is tiled across the output
        according to kUVScale.

        uiTextureID controls compositing order: lower = drawn first (bottom),
        higher = drawn last (top). BuildComposite sorts by this field, so the
        array does not need to be pre-sorted by the caller.

        bMirrorAlphaU/V are applied in continuous texture space before coverage
        is sampled:
          bMirrorAlphaU — mirrors the alpha map left ↔ right (flips U).
          bMirrorAlphaV — mirrors the alpha map top  ↔ bottom (flips V).
    */
    struct SplatLayer
    {
        std::string pcAlphaMapPath; ///< Path to the mask texture (red channel = coverage).
        std::string pcTexturePath;  ///< Path to the tiled colour texture.
        NiPoint2    kUVScale;       ///< Tile repetitions (e.g. NiPoint2(8,8) = 8×8 tiles).
        float       fBlendWeight;   ///< [0..1] scale applied to the mask coverage.
        NiUInt32    uiTextureID;    ///< Layer order key — lower = bottom, higher = top.
        bool        bMirrorAlphaU;  ///< Flip alpha map horizontally before sampling.
        bool        bMirrorAlphaV;  ///< Flip alpha map vertically   before sampling.
    };

    /**
        Loads a texture from disk while keeping its CPU pixel data accessible.
        Forces bStatic=false so GetSourcePixelData() remains valid after the
        texture is passed to the blend utilities.

        @param pcPath  File-system path to the source texture.
        @return        New NiSourceTexture with live pixel data, or NULL on failure.
    */
    static NiSourceTexture* LoadCPUTexture(const char* pcPath);

    /**
        Builds a composite terrain texture from an array of splat layers.

        Layers are automatically sorted by uiTextureID (ascending) so the
        lowest ID becomes the base. For each output pixel, each layer
        contributes with coverage = mask.R/255 * fBlendWeight using a
        Painter's algorithm (alpha-over) composite in linear float space.
        Layers whose assets fail to load are silently skipped.

        @param pkLayers     Array of SplatLayer descriptors (need not be sorted).
        @param uiLayerCount Number of elements in pkLayers.
        @param uiOutW       Output texture width  in pixels (power-of-two recommended).
        @param uiOutH       Output texture height in pixels (power-of-two recommended).
        @return             Composited NiSourceTexture with full mip chain, or NULL.
    */
    static NiSourceTexture* BuildComposite(
        const SplatLayer* pkLayers,
        NiUInt32          uiLayerCount,
        NiUInt32          uiOutW,
        NiUInt32          uiOutH);

    /**
        Produces a masked, tiled texture at an explicit output resolution.

        - RGB output : pkTex sampled at (U * kUVScale.x, V * kUVScale.y) mod 1,
                       bilinear + wrap, independent of the output resolution.
        - Alpha output: pkAlphaMap red channel, bilinear + clamp-to-edge.

        @param pkAlphaMap   White/black mask — drives per-pixel alpha.
        @param pkTex        Colour texture to tile over the output.
        @param kUVScale     Tile repetitions across the output.
        @param uiOutW       Output texture width  in pixels.
        @param uiOutH       Output texture height in pixels.
        @param bMirrorMaskU Flip the alpha map horizontally (U = 1 − U) before sampling.
        @param bMirrorMaskV Flip the alpha map vertically   (V = 1 − V) before sampling.
        @return             New NiSourceTexture, or NULL on failure.
    */
    static NiSourceTexture* BlendWithMask(
        NiSourceTexture* pkAlphaMap,
        NiSourceTexture* pkTex,
        const NiPoint2&  kUVScale,
        NiUInt32         uiOutW,
        NiUInt32         uiOutH,
        bool             bMirrorMaskU = false,
        bool             bMirrorMaskV = false);

    /**
        Blends two textures at a constant weight.
        result = lerp(pkTexA, pkTexB, fAlpha).  Output dimensions match pkTexA.
        pkTexB is resampled bilinearly if its dimensions differ from pkTexA.

        @param pkTexA   Base texture.
        @param pkTexB   Overlay texture.
        @param fAlpha   Blend weight in [0..1].
        @return         New NiSourceTexture, or NULL on failure.
    */
    static NiSourceTexture* Blend(
        NiSourceTexture* pkTexA,
        NiSourceTexture* pkTexB,
        float            fAlpha);

    /**
        Computes the smallest power-of-two output dimensions that will render
        all layers at or above uiMinPxPerTile texels per tile, capped at uiMaxDim.

        @param pkLayers         Layer array (reads kUVScale and pcTexturePath).
        @param uiLayerCount     Number of layers.
        @param uiMinPxPerTile   Minimum output pixels per tile (quality floor).
        @param uiMaxDim         Hard cap on either dimension.
        @param uiOutW           Receives the recommended output width.
        @param uiOutH           Receives the recommended output height.
    */
    static void RecommendOutputSize(
        const SplatLayer* pkLayers,
        NiUInt32          uiLayerCount,
        NiUInt32          uiMinPxPerTile,
        NiUInt32          uiMaxDim,
        NiUInt32&         uiOutW,
        NiUInt32&         uiOutH);
};