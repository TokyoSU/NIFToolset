#pragma once
#include "NiAudioListener.h"
#include "NiAudioLibType.h"

class NIAUDIO_ENTRY NiBASSAudioListener : public NiAudioListener
{
    NiDeclareRTTI;
public:
    virtual void Startup() override;
    virtual void SetDirectionVector(const NiPoint3& kDir) override;
    virtual void SetUpVector(const NiPoint3& kUp) override;
    virtual void Update() override;
    virtual void UpdateAudioData() override;
    virtual NiPoint3 GetPosition() override;
    virtual NiPoint3 GetVelocity() override;
    virtual void GetOrientation(NiPoint3& kDir, NiPoint3& kUp) override;

protected:
    NiBASSAudioListener();
    virtual ~NiBASSAudioListener();
    friend class NiBASSAudioSystem;
};