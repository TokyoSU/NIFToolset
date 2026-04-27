#pragma once
#include "NiAudioSystem.h"
#include "NiAudioLibType.h"
#include <string>

class NIAUDIO_ENTRY NiBASSAudioSystem : public NiAudioSystem
{
    NiDeclareRTTI;
public:
    static bool Create(HWND hWnd = NULL);
    static void Destroy();

    virtual bool Startup(const char* pcDirectoryname) override;
    virtual void Shutdown() override;
    virtual NiAudioSource* CreateSource(unsigned int uiType = NiAudioSource::TYPE_DEFAULT) override;

    virtual SpeakerType GetSpeakerType() override;
    virtual bool SetSpeakerType(unsigned int uiType = TYPE_3D_2_SPEAKER) override;
    virtual bool SetBestSpeakerTypeAvailable() override;
    virtual char* GetLastError() override;

    virtual bool GetReverbAvailable() override;
    virtual bool SetCurrentRoomReverb(unsigned int uiPreset) override;
    virtual unsigned int GetCurrentRoomReverb() override;

    virtual void Update(float fTime, bool bUpdateAll = false) override;
    virtual bool SetUnitsPerMeter(float fUnits) override;

    // BASS_MIX bus — controls volume of all TYPE_AMBIENT sources at once
    bool  GetMixerLoaded() const { return m_bMixerLoaded; }
    DWORD GetMusicMixer()  const { return m_hMusicMixer; }
    bool  SetMusicVolume(float fVolume) const;

	std::string GetLoadedPlugins() const;

protected:
    NiBASSAudioSystem();
    virtual ~NiBASSAudioSystem();

    HWND         m_hWnd;
    SpeakerType  m_eSpeakerType;
    unsigned int m_uiCurrentReverb;
    char         m_acLastError[256];
    bool         m_bBASSFXLoaded;

    // PLUGIN definition
    DWORD        m_hBASSFlac;
    DWORD        m_hBASSOpus;
    DWORD        m_hBASSAac;

    // BASS_MIX
    DWORD        m_hMusicMixer;
    bool         m_bMixerLoaded;
};