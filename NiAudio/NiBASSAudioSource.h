#pragma once
#include "NiAudioSource.h"
#include "NiAudioLibType.h"

class NIAUDIO_ENTRY NiBASSAudioSource : public NiAudioSource
{
    NiDeclareRTTI;
public:
    NiBASSAudioSource(unsigned int uiType = TYPE_DEFAULT);
    virtual ~NiBASSAudioSource();

    virtual bool Load() override;
    virtual bool Unload() override;

    virtual bool  SetConeData(float fAngle1Deg, float fAngle2Deg, float fGain) override;
    virtual void  GetConeData(float& fAngle1Deg, float& fAngle2Deg, float& fGain) override;
    virtual bool  SetMinMaxDistance(float fMin, float fMax) override;
    virtual void  GetMinMaxDistance(float& fMin, float& fMax) override;
    virtual bool  SetGain(float fGain) override;
    virtual float GetGain() override;
    virtual bool  SetPlaybackRate(long lRate) override;
    virtual long  GetPlaybackRate() override;
    virtual bool  Play() override;
    virtual bool  Stop() override;
    virtual void  Rewind() override;
    virtual Status GetStatus() override;

    virtual bool         SetPlayTime(float fTime) override;
    virtual float        GetPlayTime() override;
    virtual bool         GetPlayLength(float& fTime) override;
    virtual bool         SetPlayPosition(unsigned int uiPos) override;
    virtual unsigned int GetPlayPosition() override;

    virtual bool  SetRoomEffectLevel(float fLevel) override;
    virtual float GetRoomEffectLevel() override;
    virtual bool  SetOcclusionFactor(float fLevel) override;
    virtual float GetOcclusionFactor() override;
    virtual bool  SetObstructionFactor(float fLevel) override;
    virtual float GetObstructionFactor() override;

    virtual NiPoint3 GetPosition() override;
    virtual void     GetOrientation(NiPoint3& kDir, NiPoint3& kUp) override;
    virtual void     UpdateAudioData(float fTime) override;

protected:
    // BASS handles are DWORD — avoid including bass.h in the header
    unsigned int m_uiStream;  // HSTREAM or HCHANNEL
    unsigned int m_uiSample;  // HSAMPLE (non-zero for non-streamed sources only)
    bool         m_bIs3D;
};