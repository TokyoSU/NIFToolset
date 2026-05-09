#include "NiAudioPCH.h"
#include "NiBASSAudioSource.h"
#include "NiBASSAudioSystem.h"
#include <bass.h>
#include <bassmix.h>

namespace
{
    float GetEffectiveGain(float fGain, float fOcclusionFactor)
    {
        float fEffectiveGain = fGain * (1.0f - fOcclusionFactor);
        if (fEffectiveGain < 0.0f)
            return 0.0f;
        if (fEffectiveGain > 1.0f)
            return 1.0f;
        return fEffectiveGain;
    }
}

NiImplementRTTI(NiBASSAudioSource, NiAudioSource);

//---------------------------------------------------------------------------
NiBASSAudioSource::NiBASSAudioSource(unsigned int uiType)
    : NiAudioSource(uiType)
    , m_uiStream(0)
    , m_uiSample(0)
    , m_bIs3D(uiType == TYPE_3D)
{ /* */ }
//---------------------------------------------------------------------------
NiBASSAudioSource::~NiBASSAudioSource()
{
    Unload();
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::Load()
{
    if (GetLoaded())
        return true;
    if (!m_pcFilename)
        return false;

    DWORD dwFlags = 0;
    if (m_bIs3D)
        dwFlags |= BASS_SAMPLE_3D | BASS_SAMPLE_MONO | BASS_SAMPLE_MUTEMAX;
    if (m_iLoopCount == LOOP_INFINITE)
        dwFlags |= BASS_SAMPLE_LOOP;

    if (GetStreamed())
    {
        m_uiStream = BASS_StreamCreateFile(FALSE, m_pcFilename, 0, 0, dwFlags);
    }
    else
    {
        m_uiSample = BASS_SampleLoad(FALSE, m_pcFilename, 0, 0, 1, dwFlags);
        if (m_uiSample)
            m_uiStream = BASS_SampleGetChannel(m_uiSample, FALSE);
    }

    if (!m_uiStream)
        return false;

    BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_VOL,
        GetEffectiveGain(m_fGain, m_fOcclusionFactor));
    if (m_lPlaybackRate > 0)
        BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_FREQ, (float)m_lPlaybackRate);

    if (m_bIs3D)
    {
        BASS_ChannelFlags(m_uiStream, BASS_SAMPLE_MUTEMAX, BASS_SAMPLE_MUTEMAX);
        BASS_ChannelSet3DAttributes(m_uiStream,
            BASS_3DMODE_NORMAL,
            m_fMinDistance,
            m_fMaxDistance,
            GetCone() ? (int)m_fConeAngle1Deg : -1,
            GetCone() ? (int)m_fConeAngle2Deg : -1,
            GetCone() ? m_fConeGain : -1.0f);

        UpdateAudioData(0.0f);
    }

    SetLoaded(true);
    return true;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::Unload()
{
    if (!GetLoaded())
        return true;
    Stop();
    if (m_uiSample)
    {
        BASS_SampleFree(m_uiSample);
        m_uiSample = 0;
    }
    else if (m_uiStream)
    {
        BASS_StreamFree(m_uiStream);
    }
    m_uiStream = 0;
    SetLoaded(false);
    return true;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::PrepareStreamedAudio(unsigned int uiSampleRate,
    unsigned int uiChannelCount, bool bFloatSamples)
{
    if (GetType() != TYPE_VIDEO || uiSampleRate == 0 || uiChannelCount == 0)
        return false;

    Unload();

    DWORD dwFlags = 0;
    if (bFloatSamples)
        dwFlags |= BASS_SAMPLE_FLOAT;
    if (m_bIs3D)
        dwFlags |= BASS_SAMPLE_3D | BASS_SAMPLE_MONO;

    m_uiSample = 0;
    m_uiStream = BASS_StreamCreate(uiSampleRate, uiChannelCount, dwFlags,
        STREAMPROC_PUSH, NULL);
    if (!m_uiStream)
        return false;

    SetStreamed(true);
    SetLoaded(true);

    BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_VOL,
        GetEffectiveGain(m_fGain, m_fOcclusionFactor));
    if (m_lPlaybackRate > 0)
        BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_FREQ, (float)m_lPlaybackRate);

    if (m_bIs3D)
    {
        BASS_ChannelFlags(m_uiStream, BASS_SAMPLE_MUTEMAX, BASS_SAMPLE_MUTEMAX);
        BASS_ChannelSet3DAttributes(m_uiStream,
            BASS_3DMODE_NORMAL,
            m_fMinDistance,
            m_fMaxDistance,
            GetCone() ? (int)m_fConeAngle1Deg : -1,
            GetCone() ? (int)m_fConeAngle2Deg : -1,
            GetCone() ? m_fConeGain : -1.0f);

        UpdateAudioData(0.0f);
    }

    return true;
}
//---------------------------------------------------------------------------
unsigned int NiBASSAudioSource::PushAudioData(const void* pvBuffer,
    unsigned int uiByteCount)
{
    if (!m_uiStream || !pvBuffer || uiByteCount == 0)
        return 0;

    return BASS_StreamPutData(m_uiStream, pvBuffer, uiByteCount);
}
//---------------------------------------------------------------------------
void NiBASSAudioSource::EndAudioData()
{
    if (m_uiStream)
        BASS_StreamPutData(m_uiStream, NULL, BASS_STREAMPROC_END);
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::Play()
{
    if (!m_uiStream) return false;

    if (!m_bIs3D)
    {
        NiBASSAudioSystem* pkSys = NiDynamicCast(NiBASSAudioSystem, NiAudioSystem::GetAudioSystem());
        if (pkSys && pkSys->GetMixerLoaded())
        {
            DWORD hMixer = pkSys->GetMusicMixer();
            // Add to mixer only if not already in it
            if (BASS_Mixer_ChannelGetMixer(m_uiStream) != hMixer)
                BASS_Mixer_StreamAddChannel(hMixer, m_uiStream, BASS_MIXER_CHAN_NORAMPIN);
        }
    }

    return BASS_ChannelPlay(m_uiStream, FALSE) != 0;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::Stop()
{
    if (!m_uiStream) return false;

    if (!m_bIs3D)
    {
        NiBASSAudioSystem* pkSys = NiDynamicCast(NiBASSAudioSystem,
            NiAudioSystem::GetAudioSystem());
        if (pkSys && pkSys->GetMixerLoaded())
            BASS_Mixer_ChannelRemove(m_uiStream); // detach before stopping
    }

    return BASS_ChannelStop(m_uiStream) != 0;
}
//---------------------------------------------------------------------------
void NiBASSAudioSource::Rewind()
{
    if (m_uiStream)
        BASS_ChannelSetPosition(m_uiStream, 0, BASS_POS_BYTE);
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetGain(float fGain)
{
    m_fGain = fGain;
    if (!m_uiStream) return false;
    return BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_VOL,
        GetEffectiveGain(m_fGain, m_fOcclusionFactor)) != 0;
}
//---------------------------------------------------------------------------
float NiBASSAudioSource::GetGain()
{
    float fGain = m_fGain;
    if (m_uiStream)
        BASS_ChannelGetAttribute(m_uiStream, BASS_ATTRIB_VOL, &fGain);
    return fGain;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetPlaybackRate(long lRate)
{
    m_lPlaybackRate = lRate;
    if (!m_uiStream) return false;
    return BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_FREQ, (float)lRate) != 0;
}
//---------------------------------------------------------------------------
long NiBASSAudioSource::GetPlaybackRate()
{
    float fFreq = (float)m_lPlaybackRate;
    if (m_uiStream)
        BASS_ChannelGetAttribute(m_uiStream, BASS_ATTRIB_FREQ, &fFreq);
    return (long)fFreq;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetMinMaxDistance(float fMin, float fMax)
{
    m_fMinDistance = fMin < 0.0f ? 0.0f : fMin;
    m_fMaxDistance = fMax < m_fMinDistance ? m_fMinDistance : fMax;
    if (!m_uiStream) return false;
    if (m_bIs3D)
        BASS_ChannelFlags(m_uiStream, BASS_SAMPLE_MUTEMAX, BASS_SAMPLE_MUTEMAX);
    return BASS_ChannelSet3DAttributes(m_uiStream, m_bIs3D ? BASS_3DMODE_NORMAL : BASS_3DMODE_OFF, m_fMinDistance, m_fMaxDistance, -1, -1, -1.0f) != 0;
}
//---------------------------------------------------------------------------
void NiBASSAudioSource::GetMinMaxDistance(float& fMin, float& fMax)
{
    fMin = m_fMinDistance;
    fMax = m_fMaxDistance;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetConeData(float fAngle1Deg, float fAngle2Deg, float fGain)
{
    m_fConeAngle1Deg = fAngle1Deg;
    m_fConeAngle2Deg = fAngle2Deg;
    m_fConeGain      = fGain;
    SetCone(true);
    if (!m_uiStream) return false;
    return BASS_ChannelSet3DAttributes(m_uiStream,
        BASS_3DMODE_NORMAL, m_fMinDistance, m_fMaxDistance,
        (int)fAngle1Deg, (int)fAngle2Deg, fGain) != 0;
}
//---------------------------------------------------------------------------
void NiBASSAudioSource::GetConeData(float& fAngle1Deg, float& fAngle2Deg, float& fGain)
{
    fAngle1Deg = m_fConeAngle1Deg;
    fAngle2Deg = m_fConeAngle2Deg;
    fGain      = m_fConeGain;
}
//---------------------------------------------------------------------------
NiAudioSource::Status NiBASSAudioSource::GetStatus()
{
    if (!m_uiStream || !GetLoaded()) return NOT_SET;
    switch (BASS_ChannelIsActive(m_uiStream))
    {
    case BASS_ACTIVE_PLAYING: return PLAYING;
    case BASS_ACTIVE_STOPPED: return STOPPED;
    case BASS_ACTIVE_PAUSED:  return STOPPED;
    case BASS_ACTIVE_STALLED: return PLAYING;
    default:                  return FREE;
    }
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetPlayTime(float fTime)
{
    m_fPlayTime = fTime;
    if (!m_uiStream) return false;
    QWORD qwPos = BASS_ChannelSeconds2Bytes(m_uiStream, (double)fTime);
    return BASS_ChannelSetPosition(m_uiStream, qwPos, BASS_POS_BYTE) != 0;
}
//---------------------------------------------------------------------------
float NiBASSAudioSource::GetPlayTime()
{
    if (!m_uiStream) return m_fPlayTime;
    QWORD qwPos = BASS_ChannelGetPosition(m_uiStream, BASS_POS_BYTE);
    return (float)BASS_ChannelBytes2Seconds(m_uiStream, qwPos);
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::GetPlayLength(float& fTime)
{
    if (!m_uiStream) return false;
    QWORD qwLen = BASS_ChannelGetLength(m_uiStream, BASS_POS_BYTE);
    if (qwLen == (QWORD)-1) return false;
    fTime = (float)BASS_ChannelBytes2Seconds(m_uiStream, qwLen);
    return true;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetPlayPosition(unsigned int uiPos)
{
    m_uiPlayPosition = uiPos;
    if (!m_uiStream) return false;
    return BASS_ChannelSetPosition(m_uiStream, (QWORD)uiPos, BASS_POS_BYTE) != 0;
}
//---------------------------------------------------------------------------
unsigned int NiBASSAudioSource::GetPlayPosition()
{
    if (!m_uiStream) return m_uiPlayPosition;
    return (unsigned int)BASS_ChannelGetPosition(m_uiStream, BASS_POS_BYTE);
}
//---------------------------------------------------------------------------
// Occlusion: BASS has no native equivalent — approximate as gain attenuation
bool NiBASSAudioSource::SetOcclusionFactor(float fLevel)
{
    m_fOcclusionFactor = fLevel;
    if (m_uiStream)
        BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_VOL,
            GetEffectiveGain(m_fGain, m_fOcclusionFactor));
    return true;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetObstructionFactor(float fLevel)
{
    m_fObstructionFactor = fLevel;
    return false; // No native BASS equivalent
}
//---------------------------------------------------------------------------
bool NiBASSAudioSource::SetRoomEffectLevel(float fLevel)
{
    m_fRoomEffectLevel = fLevel;
    return false; // Requires BASS_FX plugin
}
//---------------------------------------------------------------------------
float NiBASSAudioSource::GetRoomEffectLevel()    { return m_fRoomEffectLevel; }
float NiBASSAudioSource::GetOcclusionFactor()    { return m_fOcclusionFactor; }
float NiBASSAudioSource::GetObstructionFactor()  { return m_fObstructionFactor; }
//---------------------------------------------------------------------------
NiPoint3 NiBASSAudioSource::GetPosition() { return GetWorldTranslate(); }
void NiBASSAudioSource::GetOrientation(NiPoint3& kDir, NiPoint3& kUp)
{
    kDir = m_kDirection;
    kUp  = m_kUp;
}
//---------------------------------------------------------------------------
void NiBASSAudioSource::UpdateAudioData(float /*fTime*/)
{
    if (!m_uiStream || !m_bIs3D)
        return;

    NiPoint3 kPos = GetWorldTranslate();
    BASS_3DVECTOR pos    = { kPos.x, kPos.y, kPos.z };
    BASS_3DVECTOR vel    = { m_kLocalVelocity.x, m_kLocalVelocity.y, m_kLocalVelocity.z };
    BASS_3DVECTOR orient = { m_kDirection.x, m_kDirection.y, m_kDirection.z };
    BASS_ChannelSet3DPosition(m_uiStream, &pos,
        GetCone() ? &orient : NULL, &vel);

    float fVolume = GetEffectiveGain(m_fGain, m_fOcclusionFactor);
    if (m_fMaxDistance > 0.0f)
    {
        NiAudioSystem* pkAudioSystem = NiAudioSystem::GetAudioSystem();
        NiAudioListener* pkListener = pkAudioSystem ? pkAudioSystem->GetListener() : NULL;
        if (pkListener)
        {
            NiPoint3 kDelta = kPos - pkListener->GetWorldTranslate();
            float fDistanceSqr = kDelta.SqrLength();
            float fMaxDistanceSqr = m_fMaxDistance * m_fMaxDistance;
            if (fDistanceSqr > fMaxDistanceSqr)
                fVolume = 0.0f;
        }
    }

    BASS_ChannelSetAttribute(m_uiStream, BASS_ATTRIB_VOL, fVolume);
}