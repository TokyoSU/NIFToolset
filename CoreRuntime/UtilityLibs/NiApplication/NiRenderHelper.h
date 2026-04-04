#pragma once
#ifndef NIRENDERHELPER_H
#define NIRENDERHELPER_H

#include <NiRenderer.h>
#include <NiRenderObject.h>
#include <NiRenderView.h>
#include <NiMeshScreenElements.h>
#include <NiMesh2DRenderView.h>
#include <NiImmediateModeAdapter.h>
#include <NiImmediateModeMacro.h>
#include <NiTexturingProperty.h>
#include <NiAlphaProperty.h>
#include <NiVertexColorProperty.h>
#include <NiZBufferProperty.h>
#include <NiAVObject.h>
#include <NiFrustum.h>
#include <NiBound.h>
#include <NiDirectionalLight.h>
#include <NiPointLight.h>
#include <NiSpotLight.h>

// ---------------------------------------------------------------------------
// NiRenderHelper
//
// Header-only helpers for manual 2D and 3D rendering without NIF/KFM.
//
// ── 2D Fullscreen textured overlay ──────────────────────────────────────────
//   auto spQuad = NiRenderHelper::CreateFullscreenQuad(pkLogoTexture);
//   // In render callback (inside BeginUsingDefaultRenderTargetGroup):
//   NiRenderHelper::DrawScreenElement(spQuad, pkRenderer);
//
// ── 2D Textured HUD element (pixel offset from top-left) ───────────────────
//   auto spHUD = NiRenderHelper::CreateTexturedRect(pkHudTex, 10.f, 10.f, 256, 64);
//   NiRenderHelper::DrawScreenElement(spHUD, pkRenderer);
//
// ── 2D Batch rendering via render view ─────────────────────────────────────
//   NiMesh2DRenderViewPtr spView = NiNew NiMesh2DRenderView();
//   spView->AppendScreenElement(spQuad);
//   spView->AppendScreenElement(spHUD);
//   NiRenderHelper::DrawScreenView(spView, pkRenderer);
//
// ── 2D Colored overlays (immediate mode, normalized viewport coords [0..1]) ─
//   NiImmediateModeAdapter kAdapter;
//   NiRenderHelper::Draw2DFilledRect(kAdapter, 0.f, 0.f, 1.f, 1.f,
//       NiColorA(0.f, 0.f, 0.f, 0.5f));
//
// ── 3D Line / primitives ────────────────────────────────────────────────────
//   NiImmediateModeAdapter kAdapter;
//   kAdapter.SetCurrentCamera(pkCamera);
//   NiRenderHelper::DrawLine(kAdapter, kA, kB, NiColorA(1, 0, 0, 1));
//   NiRenderHelper::DrawWireSphere(kAdapter, kCenter, 1.0f);
//   NiRenderHelper::DrawWireBox(kAdapter, kXForm, 2.0f, 2.0f, 2.0f);
//
// ── Debug visualization ─────────────────────────────────────────────────────
//   NiRenderHelper::DrawWireFrustum(kAdapter, pkCamera);
//   NiRenderHelper::DrawAxisFrame(kAdapter, pkObject->GetWorldTransform());
//   NiRenderHelper::DrawHierarchy(kAdapter, pkSceneRoot);
// ---------------------------------------------------------------------------

namespace NiRenderHelper
{
    // -----------------------------------------------------------------------
    // 2D — Screen-element factory functions
    // -----------------------------------------------------------------------

    // Creates a fullscreen textured quad.
    // ZBufferTest = false, alpha blending = true (Gamebryo screen-element defaults).
    inline NiPointer<NiMeshScreenElements> CreateFullscreenQuad(
        NiTexture* pkTexture,
        NiTexturingProperty::ApplyMode eMode = NiTexturingProperty::APPLY_REPLACE)
    {
        if (!pkTexture)
            return nullptr;
        return NiMeshScreenElements::Create(pkTexture, eMode);
    }

    // Creates a textured quad at a pixel offset (fX, fY) from eCorner,
    // with pixel dimensions uiW × uiH.
    // Properties are identical to CreateFullscreenQuad.
    inline NiPointer<NiMeshScreenElements> CreateTexturedRect(
        NiTexture* pkTexture,
        float fX, float fY,
        unsigned int uiW, unsigned int uiH,
        NiRenderer::DisplayCorner eCorner = NiRenderer::CORNER_TOP_LEFT,
        bool bSafeZone = false)
    {
        NiRenderer* pkRenderer = NiRenderer::GetRenderer();
        if (!pkRenderer || !pkTexture)
            return nullptr;

        NiPointer<NiMeshScreenElements> spElem =
            NiMeshScreenElements::Create(pkRenderer, fX, fY, uiW, uiH, eCorner, bSafeZone);
        if (!spElem)
            return nullptr;

        NiTexturingProperty* pkTP = NiNew NiTexturingProperty();
        pkTP->SetBaseTexture(pkTexture);
        pkTP->SetBaseFilterMode(NiTexturingProperty::FILTER_NEAREST);
        pkTP->SetApplyMode(NiTexturingProperty::APPLY_REPLACE);
        pkTP->SetBaseClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);

        NiAlphaProperty* pkAP = NiNew NiAlphaProperty();
        pkAP->SetAlphaBlending(true);

        NiVertexColorProperty* pkVCP = NiNew NiVertexColorProperty();
        pkVCP->SetSourceMode(NiVertexColorProperty::SOURCE_EMISSIVE);
        pkVCP->SetLightingMode(NiVertexColorProperty::LIGHTING_E);

        NiZBufferProperty* pkZP = NiNew NiZBufferProperty();
        pkZP->SetZBufferTest(false);
        pkZP->SetZBufferWrite(true);

        spElem->AttachProperty(pkTP);
        spElem->AttachProperty(pkAP);
        spElem->AttachProperty(pkVCP);
        spElem->AttachProperty(pkZP);
        spElem->UpdateProperties();
        return spElem;
    }

    // -----------------------------------------------------------------------
    // 2D — Rendering (must be called inside BeginUsingDefaultRenderTargetGroup)
    // -----------------------------------------------------------------------

    // Sets screen-space camera mode and renders a single NiMeshScreenElements.
    // Call pkRenderer->SetCameraData(pkCamera) afterwards to resume 3D rendering.
    inline void DrawScreenElement(NiMeshScreenElements* pkElem,
        NiRenderer* pkRenderer)
    {
        if (!pkElem || !pkRenderer)
            return;
        pkRenderer->SetScreenSpaceCameraData();
        pkElem->RenderImmediate(pkRenderer);
    }

    // Sets screen-space camera mode and renders all elements in a NiMesh2DRenderView.
    // kViewport defaults to the full-screen normalized rect (left=0, right=1,
    // top=1, bottom=0).
    inline void DrawScreenView(NiMesh2DRenderView* pkView,
        NiRenderer* pkRenderer,
        const NiRect<float>& kViewport = NiRect<float>(0.0f, 1.0f, 1.0f, 0.0f))
    {
        if (!pkView || !pkRenderer)
            return;

        pkView->SetCameraData(kViewport);
        const NiVisibleArray& kVisible =
            pkView->GetPVGeometry(NiRenderView::FORCE_PV_GEOMETRY_UPDATE);

        const unsigned int uiCount = kVisible.GetCount();
        for (unsigned int ui = 0; ui < uiCount; ++ui)
        {
            reinterpret_cast<NiRenderObject*>(
                &kVisible.GetAt(ui))->RenderImmediate(pkRenderer);
        }
    }

    // -----------------------------------------------------------------------
    // 2D — Immediate mode colored overlays (NiImmediateModeAdapter)
    //
    // Coordinates are in normalized viewport space: (0,0) = top-left,
    // (1,1) = bottom-right, matching the full-screen NiRect<float>(0,1,1,0).
    // These helpers call SetScreenSpaceCameraData internally; pass pkViewport
    // to restrict them to a sub-region.
    // -----------------------------------------------------------------------

    // Filled solid-color rectangle.
    inline void Draw2DFilledRect(NiImmediateModeAdapter& kAdapter,
        float fLeft, float fTop, float fRight, float fBottom,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        const NiRect<float>* pkViewport = nullptr)
    {
        kAdapter.SetScreenSpaceCameraData(pkViewport);
        kAdapter.SetZBufferProperty(false, false);
        kAdapter.SetCurrentColor(kColor);

        const NiPoint3 kVerts[6] = {
            { fLeft,  fTop,    0.0f },
            { fRight, fTop,    0.0f },
            { fLeft,  fBottom, 0.0f },
            { fRight, fTop,    0.0f },
            { fRight, fBottom, 0.0f },
            { fLeft,  fBottom, 0.0f },
        };
        kAdapter.BeginDrawing(NiPrimitiveType::PRIMITIVE_TRIANGLES, false);
        kAdapter.Append(6, kVerts);
        kAdapter.EndDrawing();
    }

    // Wireframe rectangle outline.
    inline void Draw2DWireRect(NiImmediateModeAdapter& kAdapter,
        float fLeft, float fTop, float fRight, float fBottom,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        const NiRect<float>* pkViewport = nullptr)
    {
        kAdapter.SetScreenSpaceCameraData(pkViewport);
        kAdapter.SetZBufferProperty(false, false);
        kAdapter.SetCurrentColor(kColor);

        const NiPoint3 kVerts[8] = {
            { fLeft,  fTop,    0.0f }, { fRight, fTop,    0.0f },
            { fRight, fTop,    0.0f }, { fRight, fBottom, 0.0f },
            { fRight, fBottom, 0.0f }, { fLeft,  fBottom, 0.0f },
            { fLeft,  fBottom, 0.0f }, { fLeft,  fTop,    0.0f },
        };
        kAdapter.BeginDrawing(NiPrimitiveType::PRIMITIVE_LINES, false);
        kAdapter.Append(8, kVerts);
        kAdapter.EndDrawing();
    }

    // Single screen-space line between two normalized points.
    inline void Draw2DLine(NiImmediateModeAdapter& kAdapter,
        float fX0, float fY0, float fX1, float fY1,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        const NiRect<float>* pkViewport = nullptr)
    {
        kAdapter.SetScreenSpaceCameraData(pkViewport);
        kAdapter.SetZBufferProperty(false, false);
        kAdapter.SetCurrentColor(kColor);

        const NiPoint3 kP0 = { fX0, fY0, 0.0f };
        const NiPoint3 kP1 = { fX1, fY1, 0.0f };
        kAdapter.BeginDrawing(NiPrimitiveType::PRIMITIVE_LINES, false);
        kAdapter.Append(&kP0, &kP1);
        kAdapter.EndDrawing();
    }

    // -----------------------------------------------------------------------
    // 3D — Immediate mode primitives
    // Call kAdapter.SetCurrentCamera(pkCamera) before any of these functions.
    // -----------------------------------------------------------------------

    // Single line segment between two 3D world-space points.
    inline void DrawLine(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kP0, const NiPoint3& kP1,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f))
    {
        kAdapter.SetCurrentColor(kColor);
        kAdapter.BeginDrawing(NiPrimitiveType::PRIMITIVE_LINES);
        kAdapter.Append(&kP0, &kP1);
        kAdapter.EndDrawing();
    }

    // Sequence of line segments from an array. uiCount must be even.
    inline void DrawLines(NiImmediateModeAdapter& kAdapter,
        unsigned int uiCount, const NiPoint3* pkPoints,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f))
    {
        if (!pkPoints || uiCount < 2)
            return;
        kAdapter.SetCurrentColor(kColor);
        kAdapter.BeginDrawing(NiPrimitiveType::PRIMITIVE_LINES);
        kAdapter.Append(uiCount, pkPoints);
        kAdapter.EndDrawing();
    }

    // Solid triangle.
    inline void DrawTriangle(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kP0, const NiPoint3& kP1, const NiPoint3& kP2,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f))
    {
        kAdapter.SetCurrentColor(kColor);
        kAdapter.BeginDrawing(NiPrimitiveType::PRIMITIVE_TRIANGLES);
        kAdapter.Append(&kP0, &kP1, &kP2);
        kAdapter.EndDrawing();
    }

    // Wireframe oriented bounding box (fSizeX/Y/Z are full extents, not half-extents).
    inline void DrawWireBox(NiImmediateModeAdapter& kAdapter,
        const NiTransform& kTransform,
        float fSizeX, float fSizeY, float fSizeZ,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f))
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireBox(kTransform, fSizeX, fSizeY, fSizeZ);
    }

    // Solid oriented bounding box.
    inline void DrawSolidBox(NiImmediateModeAdapter& kAdapter,
        const NiTransform& kTransform,
        float fSizeX, float fSizeY, float fSizeZ,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f))
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.SolidBox(kTransform, fSizeX, fSizeY, fSizeZ);
    }

    // Wireframe axis-aligned bounding box from min/max world-space points.
    inline void DrawWireAABB(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kMin, const NiPoint3& kMax,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f))
    {
        NiTransform kXForm;
        kXForm.m_fScale    = 1.0f;
        kXForm.m_Translate = (kMin + kMax) * 0.5f;
        kXForm.m_Rotate.MakeIdentity();
        const NiPoint3 kSize = kMax - kMin;
        DrawWireBox(kAdapter, kXForm, kSize.x, kSize.y, kSize.z, kColor);
    }

    // Wireframe sphere.
    inline void DrawWireSphere(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kCenter, float fRadius,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 16, unsigned int uiStacks = 8)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireSphere(kCenter, fRadius, uiSlices, uiStacks);
    }

    // Solid sphere.
    inline void DrawSolidSphere(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kCenter, float fRadius,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 16, unsigned int uiStacks = 8)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.SolidSphere(kCenter, fRadius, uiSlices, uiStacks);
    }

    // Wireframe bounding sphere displayed as three axis-aligned circles.
    inline void DrawWireBounds(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kCenter, float fRadius,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 32)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireBounds(kCenter, fRadius, uiSlices);
    }

    // Wireframe bounding sphere from an NiBound.
    inline void DrawWireBounds(NiImmediateModeAdapter& kAdapter,
        const NiBound& kBound,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 32)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireBounds(kBound, uiSlices);
    }

    // Wireframe circle in the plane spanned by kAxis1 and kAxis2.
    inline void DrawWireCircle(NiImmediateModeAdapter& kAdapter,
        const NiPoint3& kCenter, float fRadius,
        const NiPoint3& kAxis1, const NiPoint3& kAxis2,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 32)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireCircle(kCenter, fRadius, kAxis1, kAxis2, uiSlices);
    }

    // Wireframe cone. Tip at the local origin, base along the local +X axis.
    inline void DrawWireCone(NiImmediateModeAdapter& kAdapter,
        const NiTransform& kTransform,
        float fLength, float fRadius,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 16)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireCone(kTransform, fLength, fRadius, uiSlices);
    }

    // Solid cone.
    inline void DrawSolidCone(NiImmediateModeAdapter& kAdapter,
        const NiTransform& kTransform,
        float fLength, float fRadius,
        const NiColorA& kColor = NiColorA(1.0f, 1.0f, 1.0f, 1.0f),
        unsigned int uiSlices = 16)
    {
        kAdapter.SetCurrentColor(kColor);
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.SolidCone(kTransform, fLength, fRadius, uiSlices);
    }

    // -----------------------------------------------------------------------
    // Debug — Scene visualization helpers (NiImmediateModeMacro)
    // Call kAdapter.SetCurrentCamera(pkCamera) before any of these functions.
    // -----------------------------------------------------------------------

    // Draw the camera frustum as a wireframe volume.
    inline void DrawWireFrustum(NiImmediateModeAdapter& kAdapter,
        NiCamera* pkCamera)
    {
        if (!pkCamera)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireFrustum(pkCamera);
    }

    // Draw a wireframe frustum from explicit frustum/transform data.
    inline void DrawWireFrustum(NiImmediateModeAdapter& kAdapter,
        const NiFrustum& kFrustum, const NiTransform& kTransform)
    {
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireFrustum(kFrustum, kTransform);
    }

    // Draw a wireframe camera icon (axes + frustum outline).
    inline void DrawWireCamera(NiImmediateModeAdapter& kAdapter,
        NiCamera* pkCamera, float fScaleMult = 1.0f)
    {
        if (!pkCamera)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireCamera(pkCamera, fScaleMult);
    }

    // Draw a rotational frame (X/Y/Z axes). kTransform is copied so both
    // const and non-const sources are accepted.
    // Use GetScreenScaleFactor() to size it consistently on screen.
    inline void DrawAxisFrame(NiImmediateModeAdapter& kAdapter,
        NiTransform kTransform, float fScaleMult = 1.0f)
    {
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.RotationalFrame(kTransform, fScaleMult);
    }

    // Draw the full scene-graph hierarchy of pkRoot as wireframe lines.
    // bBones: draw bone objects; bNodes: draw non-bone nodes;
    // bConnections: draw parent→child connector lines.
    inline void DrawHierarchy(NiImmediateModeAdapter& kAdapter,
        NiAVObject* pkRoot,
        float fScaleMult = 1.0f,
        bool bBones = true,
        bool bNodes = true,
        bool bConnections = true)
    {
        if (!pkRoot)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireHierarchy(pkRoot, fScaleMult, bBones, bNodes, bConnections);
    }

    // Draw the skinning bone hierarchy embedded in pkMesh.
    inline void DrawBoneHierarchy(NiImmediateModeAdapter& kAdapter,
        NiMesh* pkMesh, float fScaleMult = 1.0f)
    {
        if (!pkMesh)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireBoneHierarchy(pkMesh, fScaleMult);
    }

    // Draw a wireframe point-light icon.
    inline void DrawWirePointLight(NiImmediateModeAdapter& kAdapter,
        NiPointLight* pkLight, float fScaleMult = 1.0f)
    {
        if (!pkLight)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WirePointLight(pkLight, fScaleMult);
    }

    // Draw a wireframe spot-light cone icon.
    inline void DrawWireSpotLight(NiImmediateModeAdapter& kAdapter,
        NiSpotLight* pkLight, float fScaleMult = 1.0f)
    {
        if (!pkLight)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireSpotLight(pkLight, fScaleMult);
    }

    // Draw a wireframe directional-light arrow icon.
    inline void DrawWireDirectionalLight(NiImmediateModeAdapter& kAdapter,
        NiDirectionalLight* pkLight, float fScaleMult = 1.0f)
    {
        if (!pkLight)
            return;
        NiImmediateModeMacro kMacro(kAdapter);
        kMacro.WireDirectionalLight(pkLight, fScaleMult);
    }

    // Returns a scale factor that makes an icon appear the same screen size
    // regardless of pkObj's distance from the adapter's current camera.
    inline float GetScreenScaleFactor(const NiAVObject* pkObj,
        const NiImmediateModeAdapter* pkAdapter)
    {
        return NiImmediateModeMacro::GetScreenScaleFactor(pkObj, pkAdapter);
    }
}

#endif // NIRENDERHELPER_H