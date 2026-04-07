#pragma once
#ifndef NIMESHHELPER_H
#define NIMESHHELPER_H

#include <NiMesh.h>
#include <NiNode.h>
#include <NiPrimitiveType.h>
#include <NiDataStream.h>
#include <NiDataStreamElement.h>
#include <NiCommonSemantics.h>
#include <NiTexturingProperty.h>
#include <NiAlphaProperty.h>
#include <NiVertexColorProperty.h>
#include <NiZBufferProperty.h>
#include <NiSourceTexture.h>
#include <NiBound.h>
#include <NiPoint2.h>
#include <NiPoint3.h>
#include <NiColor.h>
#include <NiStream.h>
#include <NiMatrix3.h>
#include <NiTextureTransform.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// NiMeshDesc — geometry descriptor for NiMeshHelper::Create
// ---------------------------------------------------------------------------
// All arrays must remain valid until after Create() returns. Create() copies
// data into GPU-resident NiDataStreams before returning.
//
// Minimal required fields: pkPositions, uiVertexCount, pkIndices, uiIndexCount.
//
// Example — flat triangle (no texture, no normals):
//   NiPoint3 kPos[3] = { {-1,0,0}, {1,0,0}, {0,1,0} };
//   NiUInt32 kIdx[3] = { 0, 1, 2 };
//   NiMeshDesc kDesc;
//   kDesc.pkPositions   = kPos; kDesc.uiVertexCount = 3;
//   kDesc.pkIndices     = kIdx; kDesc.uiIndexCount  = 3;
//   auto spMesh = NiMeshHelper::Create(kDesc);
//
// Example — heightmap terrain from GFTerrainReader:
//   GFTerrainReader kReader; kReader.LoadFromFile("Data\\World.fsm");
//   auto spMesh = NiMeshHelper::CreateFromHeightmap(kReader, 0, 1.0f);
//   auto spMeshInv = NiMeshHelper::CreateFromHeightmap(kReader, 0, 1.0f, nullptr, true);
//   pkScene->AttachChild(spMesh);
//   pkScene->Update(0.0f);
// ---------------------------------------------------------------------------

struct NiMeshDesc
{
    // Required — triangle list vertices and indices.
    const NiPoint3* pkPositions   = nullptr;
    NiUInt32        uiVertexCount = 0;
    const NiUInt32* pkIndices     = nullptr;
    NiUInt32        uiIndexCount  = 0;

    // Optional streams — nullptr skips the stream entirely.
    const NiPoint3* pkNormals     = nullptr; // NORMAL semantic, F_FLOAT32_3
    const NiPoint2* pkUVs         = nullptr; // TEXCOORD 0,  F_FLOAT32_2
    const NiColorA* pkColors      = nullptr; // COLOR semantic, F_FLOAT32_4 [0..1]

    // Optional property helpers.
    NiTexture*      pkBaseTexture = nullptr; // attaches NiTexturingProperty (MODULATE)
    bool            bAlphaBlend   = false;   // attaches NiAlphaProperty (SrcAlpha)
    bool            bVertexColors = false;   // attaches NiVertexColorProperty
    bool            bZBufferTest  = true;    // attaches NiZBufferProperty
};

// ---------------------------------------------------------------------------
// NiMeshHelper namespace
// ---------------------------------------------------------------------------
namespace NiMeshHelper
{
    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------
    namespace Detail
    {
        // Computes a tight bounding sphere from an AABB extents sweep.
        inline NiBound ComputeBound(const NiPoint3* pkPos, NiUInt32 uiCount)
        {
            NiPoint3 kMin = pkPos[0];
            NiPoint3 kMax = pkPos[0];

            for (NiUInt32 i = 1; i < uiCount; ++i)
            {
                kMin.x = std::min(kMin.x, pkPos[i].x);
                kMin.y = std::min(kMin.y, pkPos[i].y);
                kMin.z = std::min(kMin.z, pkPos[i].z);
                kMax.x = std::max(kMax.x, pkPos[i].x);
                kMax.y = std::max(kMax.y, pkPos[i].y);
                kMax.z = std::max(kMax.z, pkPos[i].z);
            }

            const NiPoint3 kCenter = (kMin + kMax) * 0.5f;
            const float    fRadius = (kMax - kCenter).Length();

            NiBound kBound;
            kBound.SetCenterAndRadius(kCenter, fRadius);
            return kBound;
        }

        // Attaches standard rendering properties to pkMesh according to desc flags.
        inline void AttachProperties(NiMesh*    pkMesh,
                                     NiTexture* pkBase,
                                     bool       bAlphaBlend,
                                     bool       bVertexColors,
                                     bool       bZBufferTest)
        {
            if (pkBase)
            {
                NiTexturingProperty* pkTP = NiNew NiTexturingProperty();
                pkTP->SetApplyMode(NiTexturingProperty::APPLY_MODULATE);
                pkTP->SetBaseTexture(pkBase);
                pkTP->SetBaseFilterMode(NiTexturingProperty::FILTER_TRILERP);
                pkMesh->AttachProperty(pkTP);
            }

            if (bAlphaBlend)
            {
                NiAlphaProperty* pkAP = NiNew NiAlphaProperty();
                pkAP->SetAlphaBlending(true);
                pkAP->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);
                pkAP->SetDestBlendMode(NiAlphaProperty::ALPHA_INVSRCALPHA);
                pkMesh->AttachProperty(pkAP);
            }

            if (bVertexColors)
            {
                NiVertexColorProperty* pkVC = NiNew NiVertexColorProperty();
                pkVC->SetSourceMode(NiVertexColorProperty::SOURCE_AMB_DIFF);
                pkVC->SetLightingMode(NiVertexColorProperty::LIGHTING_E_A_D);
                pkMesh->AttachProperty(pkVC);
            }

            {
                NiZBufferProperty* pkZP = NiNew NiZBufferProperty();
                pkZP->SetZBufferTest(bZBufferTest);
                pkZP->SetZBufferWrite(true);
                pkMesh->AttachProperty(pkZP);
            }
        }
    } // namespace Detail

    // -----------------------------------------------------------------------
    // ComputeSmoothNormals — accumulates weighted face normals into per-vertex
    //                        normals for use as NiMeshDesc::pkNormals.
    //
    // kNormalsOut is resized to uiVertexCount and normalised on output.
    // -----------------------------------------------------------------------
    inline void ComputeSmoothNormals(
        const NiPoint3*        pkPositions,
        NiUInt32               uiVertexCount,
        const NiUInt32*        pkIndices,
        NiUInt32               uiIndexCount,
        std::vector<NiPoint3>& kNormalsOut)
    {
        kNormalsOut.assign(uiVertexCount, NiPoint3(0.f, 0.f, 0.f));

        for (NiUInt32 i = 0; i + 2 < uiIndexCount; i += 3)
        {
            const NiUInt32 i0 = pkIndices[i + 0];
            const NiUInt32 i1 = pkIndices[i + 1];
            const NiUInt32 i2 = pkIndices[i + 2];

            const NiPoint3 kE1    = pkPositions[i1] - pkPositions[i0];
            const NiPoint3 kE2    = pkPositions[i2] - pkPositions[i0];
            const NiPoint3 kFaceN = kE1.Cross(kE2); // weighted by triangle area

            kNormalsOut[i0] += kFaceN;
            kNormalsOut[i1] += kFaceN;
            kNormalsOut[i2] += kFaceN;
        }

        for (NiUInt32 i = 0; i < uiVertexCount; ++i)
            kNormalsOut[i].Unitize();
    }

    // -----------------------------------------------------------------------
    // Create — builds an NiMesh from a NiMeshDesc descriptor.
    //
    // All streams are written once (ACCESS_CPU_WRITE_STATIC | ACCESS_GPU_READ).
    // Returns nullptr if required fields (positions / indices) are missing.
    // -----------------------------------------------------------------------
    inline NiPointer<NiMesh> Create(const NiMeshDesc& kDesc)
    {
        if (!kDesc.pkPositions || kDesc.uiVertexCount == 0 ||
            !kDesc.pkIndices   || kDesc.uiIndexCount  == 0)
            return nullptr;

        NiPointer<NiMesh> spMesh = NiNew NiMesh();
        spMesh->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);
        spMesh->SetSubmeshCount(1);

        const NiUInt8 uiVA = static_cast<NiUInt8>(
            NiDataStream::ACCESS_GPU_READ | NiDataStream::ACCESS_CPU_WRITE_STATIC);

        // Position — required
        spMesh->AddStream(
            NiCommonSemantics::POSITION(), 0,
            NiDataStreamElement::F_FLOAT32_3,
            kDesc.uiVertexCount, uiVA,
            NiDataStream::USAGE_VERTEX,
            kDesc.pkPositions);

        // Index — required
        spMesh->AddStream(
            NiCommonSemantics::INDEX(), 0,
            NiDataStreamElement::F_UINT32_1,
            kDesc.uiIndexCount, uiVA,
            NiDataStream::USAGE_VERTEX_INDEX,
            kDesc.pkIndices);

        // Normal — optional
        if (kDesc.pkNormals)
        {
            spMesh->AddStream(
                NiCommonSemantics::NORMAL(), 0,
                NiDataStreamElement::F_FLOAT32_3,
                kDesc.uiVertexCount, uiVA,
                NiDataStream::USAGE_VERTEX,
                kDesc.pkNormals);
        }

        // UV / TEXCOORD 0 — optional
        if (kDesc.pkUVs)
        {
            spMesh->AddStream(
                NiCommonSemantics::TEXCOORD(), 0,
                NiDataStreamElement::F_FLOAT32_2,
                kDesc.uiVertexCount, uiVA,
                NiDataStream::USAGE_VERTEX,
                kDesc.pkUVs);
        }

        // Vertex colour — optional
        if (kDesc.pkColors)
        {
            spMesh->AddStream(
                NiCommonSemantics::COLOR(), 0,
                NiDataStreamElement::F_FLOAT32_4,
                kDesc.uiVertexCount, uiVA,
                NiDataStream::USAGE_VERTEX,
                kDesc.pkColors);
        }

        spMesh->SetModelBound(
            Detail::ComputeBound(kDesc.pkPositions, kDesc.uiVertexCount));

        Detail::AttachProperties(spMesh,
            kDesc.pkBaseTexture,
            kDesc.bAlphaBlend,
            kDesc.bVertexColors,
            kDesc.bZBufferTest);

        return spMesh;
    }
} // namespace NiMeshHelper

// ---------------------------------------------------------------------------
// NiScrollMaterial — runtime state for one animated material layer
//
// Supports two independent effects that can be combined freely:
//   • UV scroll  — continuously translates the base map's NiTextureTransform
//                  at (fSpeedU, fSpeedV) UV units per second.
//   • Texture flip — cycles through kFrames, swapping the base map's texture
//                  every fFrameDuration seconds.
//
// Typical water / lava setup:
//   NiScrollMaterial kWater;
//   NiMaterialAnimHelper::Enable(kWater, spWaterMesh, 0.05f, 0.02f);
//   NiMaterialAnimHelper::AddFrame(kWater, spWaterTex0);
//   NiMaterialAnimHelper::AddFrame(kWater, spWaterTex1);
//   NiMaterialAnimHelper::AddFrame(kWater, spWaterTex2);
//   kWater.fFrameDuration = 0.08f;   // ≈ 12 fps
//
//   // In game loop:
//   NiMaterialAnimHelper::Tick(kWater, fDeltaTime);
//
//   // Teardown:
//   NiMaterialAnimHelper::Disable(kWater);
// ---------------------------------------------------------------------------
struct NiScrollMaterial
{
    NiPointer<NiTexturingProperty> spTP;          // keeps the property alive
    NiTextureTransform*            pkTransform    = nullptr; // owned if bOwnsTransform
    bool                           bOwnsTransform = false;

    // UV scroll state
    float    fSpeedU = 0.f;                       // UV units / second along U
    float    fSpeedV = 0.f;                       // UV units / second along V
    NiPoint2 kOffset = NiPoint2::ZERO;            // accumulated UV offset

    // Texture flip state — leave kFrames empty for scroll-only
    std::vector<NiPointer<NiSourceTexture>> kFrames;
    float    fFrameDuration = 0.1f;               // seconds per frame
    float    fFrameAccum    = 0.f;
    NiUInt32 uiFrame        = 0;

    [[nodiscard]] bool IsValid()    const { return spTP != nullptr && pkTransform != nullptr; }
    [[nodiscard]] bool HasScroll()  const { return fSpeedU != 0.f || fSpeedV != 0.f; }
    [[nodiscard]] bool HasFlip()    const { return kFrames.size() > 1; }
};

// -----------------------------------------------------------------------
// NiMaterialAnimHelper — UV scroll and texture flip for NiMesh materials
// -----------------------------------------------------------------------
namespace NiMaterialAnimHelper
{
    // -------------------------------------------------------------------
    // Enable — attaches scroll+flip state to pkMesh's base map.
    //
    // fSpeedU / fSpeedV  UV units per second. Pass 0 for a flip-only setup.
    //
    // The mesh must already have an NiTexturingProperty attached (from
    // NiMeshHelper::Create with pkBaseTexture, or loaded from a NIF).
    // If the base map has no NiTextureTransform one is created; otherwise
    // the existing one is reused without deleting it.
    //
    // Returns false if pkMesh has no NiTexturingProperty.
    // -------------------------------------------------------------------
    inline bool Enable(
        NiScrollMaterial& kOut,
        NiMesh*           pkMesh,
        float             fSpeedU = 0.f,
        float             fSpeedV = 0.f)
    {
        if (!pkMesh)
            return false;

        NiTexturingProperty* pkTP = NiDynamicCast(NiTexturingProperty,
            pkMesh->GetProperty(NiProperty::TEXTURING));
        if (!pkTP)
            return false;

        // Get or create the base map slot.
        NiTexturingProperty::Map* pkBaseMap = pkTP->GetBaseMap();
        if (!pkBaseMap)
        {
            pkBaseMap = NiNew NiTexturingProperty::Map();
            pkTP->SetBaseMap(pkBaseMap);
        }

        // Reuse an existing NiTextureTransform or create a fresh one.
        NiTextureTransform* pkExisting = pkBaseMap->GetTextureTransform();
        if (pkExisting)
        {
            kOut.pkTransform    = pkExisting;
            kOut.bOwnsTransform = false;
        }
        else
        {
            kOut.pkTransform    = NiNew NiTextureTransform();
            kOut.bOwnsTransform = true;
            pkBaseMap->SetTextureTransform(kOut.pkTransform);
        }

        kOut.spTP    = pkTP;
        kOut.fSpeedU = fSpeedU;
        kOut.fSpeedV = fSpeedV;
        kOut.kOffset = NiPoint2::ZERO;
        return true;
    }

    // -------------------------------------------------------------------
    // AddFrame — appends a texture to the flip sequence.
    //
    // Frames are displayed in the order they are added and loop back to
    // the first frame after the last. At least two frames are required
    // for flipping to occur; a single frame is treated as a static texture.
    // Call Enable() before AddFrame().
    // -------------------------------------------------------------------
    inline void AddFrame(NiScrollMaterial& kMat, NiSourceTexture* pkTex)
    {
        if (!kMat.IsValid() || !pkTex)
            return;

        // Set the first frame as the initial base texture.
        if (kMat.kFrames.empty())
        {
            NiTexturingProperty::Map* pkBaseMap = kMat.spTP->GetBaseMap();
            if (pkBaseMap)
                pkBaseMap->SetTexture(pkTex);
        }

        kMat.kFrames.emplace_back(pkTex);
    }

    // -------------------------------------------------------------------
    // Tick — advances UV scroll and texture flip by fDeltaTime seconds.
    //
    // Call once per game frame. fDeltaTime is the elapsed time since the
    // last Tick call, in seconds (e.g. from NiApplication::GetFrameTime()).
    // -------------------------------------------------------------------
    inline void Tick(NiScrollMaterial& kMat, float fDeltaTime)
    {
        if (!kMat.IsValid() || fDeltaTime <= 0.f)
            return;

        NiTexturingProperty::Map* pkBaseMap = kMat.spTP->GetBaseMap();
        if (!pkBaseMap)
            return;

        // ── UV scroll ──────────────────────────────────────────────────
        if (kMat.HasScroll())
        {
            kMat.kOffset.x += kMat.fSpeedU * fDeltaTime;
            kMat.kOffset.y += kMat.fSpeedV * fDeltaTime;

            // Wrap to [0, 1) to preserve floating-point precision over time.
            kMat.kOffset.x -= std::floor(kMat.kOffset.x);
            kMat.kOffset.y -= std::floor(kMat.kOffset.y);

            kMat.pkTransform->SetTranslate(kMat.kOffset);
        }

        // ── Texture flip ───────────────────────────────────────────────
        if (kMat.HasFlip())
        {
            kMat.fFrameAccum += fDeltaTime;
            if (kMat.fFrameAccum >= kMat.fFrameDuration)
            {
                kMat.fFrameAccum -= kMat.fFrameDuration;
                kMat.uiFrame =
                    (kMat.uiFrame + 1) %
                    static_cast<NiUInt32>(kMat.kFrames.size());

                pkBaseMap->SetTexture(kMat.kFrames[kMat.uiFrame]);
            }
        }
    }

    // -------------------------------------------------------------------
    // Disable — removes animation state and restores the original texture.
    //
    // If Enable() created the NiTextureTransform, it is deleted and the
    // base map's transform pointer is set to nullptr. If the transform was
    // pre-existing its translation is reset to (0, 0) only.
    // -------------------------------------------------------------------
    inline void Disable(NiScrollMaterial& kMat)
    {
        if (!kMat.spTP)
            return;

        NiTexturingProperty::Map* pkBaseMap = kMat.spTP->GetBaseMap();
        if (pkBaseMap)
        {
            // Restore the first frame as the static texture.
            if (!kMat.kFrames.empty())
                pkBaseMap->SetTexture(kMat.kFrames.front());

            // Clean up or reset the NiTextureTransform.
            if (kMat.bOwnsTransform && kMat.pkTransform)
            {
                pkBaseMap->SetTextureTransform(nullptr);
                NiDelete kMat.pkTransform;
            }
            else if (kMat.pkTransform)
            {
                kMat.pkTransform->SetTranslate(NiPoint2::ZERO);
            }
        }

        kMat = NiScrollMaterial{};
    }

} // namespace NiMaterialAnimHelper

#endif // NIMESHHELPER_H