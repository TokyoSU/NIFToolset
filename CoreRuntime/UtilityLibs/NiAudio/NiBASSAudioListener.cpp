#include "NiAudioPCH.h"
#include "NiBASSAudioListener.h"
#include <bass.h>

NiImplementRTTI(NiBASSAudioListener, NiAudioListener);

//---------------------------------------------------------------------------
NiBASSAudioListener::NiBASSAudioListener() { /* */ }
NiBASSAudioListener::~NiBASSAudioListener() { /* */ }
//---------------------------------------------------------------------------
void NiBASSAudioListener::Startup()
{
    BASS_3DVECTOR pos   = { 0.0f,  0.0f,  0.0f };
    BASS_3DVECTOR vel   = { 0.0f,  0.0f,  0.0f };
    BASS_3DVECTOR front = { 0.0f,  0.0f, -1.0f };
    BASS_3DVECTOR top   = { 0.0f,  1.0f,  0.0f };
    BASS_Set3DPosition(&pos, &vel, &front, &top);
    BASS_Apply3D();
}
//---------------------------------------------------------------------------
void NiBASSAudioListener::SetDirectionVector(const NiPoint3& kDir)
{
    m_kDirection = kDir;
    UpdateAudioData();
}
//---------------------------------------------------------------------------
void NiBASSAudioListener::SetUpVector(const NiPoint3& kUp)
{
    m_kUp = kUp;
    UpdateAudioData();
}
//---------------------------------------------------------------------------
void NiBASSAudioListener::Update()
{
    UpdateAudioData();
}
//---------------------------------------------------------------------------
void NiBASSAudioListener::UpdateAudioData()
{
    NiPoint3 kPos = GetWorldTranslate();
    BASS_3DVECTOR pos   = { kPos.x, kPos.y, kPos.z };
    BASS_3DVECTOR vel   = { m_kLocalVelocity.x, m_kLocalVelocity.y, m_kLocalVelocity.z };
    BASS_3DVECTOR front = { m_kDirection.x, m_kDirection.y, m_kDirection.z };
    BASS_3DVECTOR top   = { m_kUp.x, m_kUp.y, m_kUp.z };
    BASS_Set3DPosition(&pos, &vel, &front, &top);
    BASS_Apply3D();
}
//---------------------------------------------------------------------------
NiPoint3 NiBASSAudioListener::GetPosition()   { return GetWorldTranslate(); }
NiPoint3 NiBASSAudioListener::GetVelocity()   { return m_kLocalVelocity; }
void NiBASSAudioListener::GetOrientation(NiPoint3& kDir, NiPoint3& kUp)
{
    kDir = m_kDirection;
    kUp  = m_kUp;
}