#pragma once
#ifndef NILIGHTHELPER_H
#define NILIGHTHELPER_H

// -----------------------------------------------------------------------
// NiLightHelper.h
//
// Header-only helpers for creating and attaching the four Gamebryo light
// types (Ambient, Directional, Point, Spot) onto a scene graph node.
//
// Usage:
//   NiLightHelper::DirectionalDesc kDesc;
//   kDesc.m_kDiffuse  = NiColor(1.f, 0.95f, 0.85f);
//   kDesc.m_fPitchDeg = -45.f;
//   NiDirectionalLightPtr spLight = NiLightHelper::CreateDirectional(kDesc, g_spScene);
//   g_spScene->UpdateEffects();
// -----------------------------------------------------------------------

#include <NiAmbientLight.h>
#include <NiDirectionalLight.h>
#include <NiPointLight.h>
#include <NiSpotLight.h>
#include <NiNode.h>
#include <NiMath.h>

namespace NiLightHelper
{
    // -------------------------------------------------------------------
    // Shared base descriptor — applies to every light type
    // -------------------------------------------------------------------
    struct BaseDesc
    {
        NiColor m_kAmbient  = NiColor(0.0f, 0.0f, 0.0f);
        NiColor m_kDiffuse  = NiColor(1.0f, 1.0f, 1.0f);
        NiColor m_kSpecular = NiColor(1.0f, 1.0f, 1.0f);
        float   m_fDimmer   = 1.0f;   // MUST be > 0 — defaults to 0 in NiLight ctor
        bool    m_bEnabled  = true;
    };

    // -------------------------------------------------------------------
    // Ambient light — color only, no position or direction
    // -------------------------------------------------------------------
    struct AmbientDesc : BaseDesc {};

    // -------------------------------------------------------------------
    // Directional light — infinite, orientation-driven
    // Model space direction is (1,0,0); pitch/yaw rotate it into world space.
    // -------------------------------------------------------------------
    struct DirectionalDesc : BaseDesc
    {
        float m_fPitchDeg = -45.0f;  // rotation around X (negative = downward)
        float m_fYawDeg   =  0.0f;   // rotation around Z
    };

    // -------------------------------------------------------------------
    // Point light — positional, falls off over range
    // Attenuation in this version of the engine is controlled by
    // SetRange (hard cutoff) and SetFalloff (power of distance falloff).
    // -------------------------------------------------------------------
    struct PointDesc : BaseDesc
    {
        NiPoint3 m_kPosition  = NiPoint3(0.0f, 0.0f, 0.0f);
        float    m_fRange     = 500.0f;  // world-unit cutoff radius; <=0 = infinite
        float    m_fMaxRange  = 500.0f;  // engine-extended soft limit
        float    m_fFalloff   = 1.0f;    // exponent: 1=linear, 2=quadratic
    };

    // -------------------------------------------------------------------
    // Spot light — positional + cone, extends PointDesc
    // Model space direction is (1,0,0); pitch/yaw rotate it.
    // -------------------------------------------------------------------
    struct SpotDesc : PointDesc
    {
        float m_fPitchDeg       = 0.0f;   // aim pitch in degrees
        float m_fYawDeg         = 0.0f;   // aim yaw in degrees
        float m_fOuterAngleDeg  = 30.0f;  // cone outer half-angle in degrees
        float m_fInnerAngleDeg  = 20.0f;  // cone inner (full-bright) half-angle
        float m_fSpotExponent   = 1.0f;   // falloff from inner to outer edge
    };

    // -------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------
    namespace Detail
    {
        inline void ApplyBase(NiLight* pkLight, const BaseDesc& kDesc)
        {
            pkLight->SetAmbientColor(kDesc.m_kAmbient);
            pkLight->SetDiffuseColor(kDesc.m_kDiffuse);
            pkLight->SetSpecularColor(kDesc.m_kSpecular);
            pkLight->SetDimmer(kDesc.m_fDimmer);
            pkLight->SetSwitch(kDesc.m_bEnabled);
        }

        // Build a rotation matrix from independent pitch (X) and yaw (Z) angles
        inline NiMatrix3 MakePitchYaw(float fPitchDeg, float fYawDeg)
        {
            NiMatrix3 kPitch, kYaw;
            kPitch.MakeXRotation(NiDegToRad(fPitchDeg));
            kYaw.MakeZRotation(NiDegToRad(fYawDeg));
            return kYaw * kPitch;
        }

        // Attach the light to the scene: child for transform updates,
        // effect for illumination.  pkScene may be nullptr (caller attaches manually).
        inline void AttachToScene(NiLight* pkLight, NiNode* pkScene)
        {
            if (!pkScene)
                return;
            pkScene->AttachChild(pkLight);
            pkScene->AttachEffect(pkLight);
        }
    }

    // -------------------------------------------------------------------
    // Factory functions
    // All functions optionally attach to pkScene (pass nullptr to skip).
    // Remember to call pkScene->UpdateEffects() after all lights are added.
    // -------------------------------------------------------------------

    [[nodiscard]] inline NiAmbientLightPtr CreateAmbient(
        const AmbientDesc& kDesc,
        NiNode* pkScene = nullptr)
    {
        NiAmbientLightPtr spLight = NiNew NiAmbientLight();
        Detail::ApplyBase(spLight, kDesc);
        Detail::AttachToScene(spLight, pkScene);
        return spLight;
    }

    [[nodiscard]] inline NiDirectionalLightPtr CreateDirectional(
        const DirectionalDesc& kDesc,
        NiNode* pkScene = nullptr)
    {
        NiDirectionalLightPtr spLight = NiNew NiDirectionalLight();
        Detail::ApplyBase(spLight, kDesc);
        spLight->SetRotate(Detail::MakePitchYaw(kDesc.m_fPitchDeg, kDesc.m_fYawDeg));
        Detail::AttachToScene(spLight, pkScene);
        return spLight;
    }

    [[nodiscard]] inline NiPointLightPtr CreatePoint(
        const PointDesc& kDesc,
        NiNode* pkScene = nullptr)
    {
        NiPointLightPtr spLight = NiNew NiPointLight();
        Detail::ApplyBase(spLight, kDesc);
        spLight->SetTranslate(kDesc.m_kPosition);
        spLight->SetRange(kDesc.m_fRange);
        spLight->SetMaxRange(kDesc.m_fMaxRange);
        spLight->SetFalloff(kDesc.m_fFalloff);
        Detail::AttachToScene(spLight, pkScene);
        return spLight;
    }

    [[nodiscard]] inline NiSpotLightPtr CreateSpot(
        const SpotDesc& kDesc,
        NiNode* pkScene = nullptr)
    {
        NiSpotLightPtr spLight = NiNew NiSpotLight();
        Detail::ApplyBase(spLight, kDesc);
        spLight->SetTranslate(kDesc.m_kPosition);
        spLight->SetRange(kDesc.m_fRange);
        spLight->SetMaxRange(kDesc.m_fMaxRange);
        spLight->SetFalloff(kDesc.m_fFalloff);
        spLight->SetRotate(Detail::MakePitchYaw(kDesc.m_fPitchDeg, kDesc.m_fYawDeg));
        spLight->SetSpotAngle(kDesc.m_fOuterAngleDeg);
        spLight->SetInnerSpotAngle(kDesc.m_fInnerAngleDeg);
        spLight->SetSpotExponent(kDesc.m_fSpotExponent);
        Detail::AttachToScene(spLight, pkScene);
        return spLight;
    }

} // namespace NiLightHelper

#endif // NILIGHTHELPER_H