#include "NiAudioPCH.h"
#include "NiBASSAudioSystem.h"
#include "NiBASSAudioSource.h"
#include "NiBASSAudioListener.h"
#include <bass.h>
#include <bass_fx.h>
#include <bassmix.h>

namespace
{
    DWORD NiAudioVolumeToBASSConfig(float fVolume)
    {
        if (fVolume < 0.0f)
            fVolume = 0.0f;
        else if (fVolume > 1.0f)
            fVolume = 1.0f;

        return (DWORD)(fVolume * 10000.0f + 0.5f);
    }
}

NiImplementRTTI(NiBASSAudioSystem, NiAudioSystem, NiTypeMask::NiBASSAudioSystem);

//---------------------------------------------------------------------------
NiBASSAudioSystem::NiBASSAudioSystem()
    : m_hWnd(NULL)
    , m_eSpeakerType(TYPE_3D_2_SPEAKER)
    , m_uiCurrentReverb(ENVIRONMENT_GENERIC)
    , m_bBASSFXLoaded(false)
    , m_hMusicMixer(0)
    , m_bMixerLoaded(false)
	, m_hBASSFlac(0)
	, m_hBASSOpus(0)
	, m_hBASSAac(0)
{
    m_acLastError[0] = '\0';
    ms_pAudioSystem = this;
}
//---------------------------------------------------------------------------
NiBASSAudioSystem::~NiBASSAudioSystem()
{
    ms_pAudioSystem = NULL;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::Create(HWND hWnd)
{
    NIASSERT(ms_pAudioSystem == NULL);
    NiBASSAudioSystem* pkSystem = NiNew NiBASSAudioSystem;
    pkSystem->m_hWnd = hWnd;
    return true;
}
//---------------------------------------------------------------------------
void NiBASSAudioSystem::Destroy()
{
    NiDelete ms_pAudioSystem;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::Startup(const char* /*pcDirectoryname*/)
{
    if (!BASS_Init(-1, 44100, BASS_DEVICE_3D, m_hWnd, NULL))
    {
        NiSprintf(m_acLastError, sizeof(m_acLastError), "BASS_Init failed (error %d)", BASS_ErrorGetCode());
        return false;
    }

    // Enable explicit speaker assignment via BASS_SPEAKER_* flags
    BASS_SetConfig(BASS_CONFIG_VISTA_SPEAKERS, TRUE);

    // Optional format plugins — silent if DLL is absent
    m_hBASSFlac = BASS_PluginLoad("bassflac.dll", NULL);
    m_hBASSOpus = BASS_PluginLoad("bassopus.dll", NULL);
    m_hBASSAac  = BASS_PluginLoad("bassaac.dll",  NULL);

    // BASS_FX — reverb/DSP effects (linked via bass_fx.lib)
    m_bBASSFXLoaded = (BASS_FX_GetVersion() != 0);

    // BASS_MIX — ambient/music bus (linked via bassmix.lib)
    m_bMixerLoaded = (BASS_Mixer_GetVersion() != 0);
    if (m_bMixerLoaded)
    {
        m_hMusicMixer = BASS_Mixer_StreamCreate(44100, 2, BASS_MIXER_END);
        if (m_hMusicMixer)
            BASS_ChannelPlay(m_hMusicMixer, FALSE);
        else
            m_bMixerLoaded = false;
    }

    BASS_Set3DFactors(1.0f / m_fUnitsPerMeter, 1.0f, 1.0f);

    // Auto-detect best speaker layout from device
    SetBestSpeakerTypeAvailable();

    m_spListener = NiNew NiBASSAudioListener;
    m_spListener->Startup();
    SetMasterVolume(m_fMasterVolume);
    return true;
}
//---------------------------------------------------------------------------
void NiBASSAudioSystem::Shutdown()
{
    if (m_hMusicMixer)
    {
        BASS_ChannelStop(m_hMusicMixer);
        BASS_StreamFree(m_hMusicMixer);
        m_hMusicMixer = 0;
    }
    NiAudioSystem::Shutdown();
	if (m_hBASSFlac) BASS_PluginFree(m_hBASSFlac);
	if (m_hBASSOpus) BASS_PluginFree(m_hBASSOpus);
	if (m_hBASSAac)  BASS_PluginFree(m_hBASSAac);
    BASS_Free();
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::SetMusicVolume(float fVolume) const
{
    if (!m_hMusicMixer) return false;
    return BASS_ChannelSetAttribute(m_hMusicMixer, BASS_ATTRIB_VOL, fVolume) != 0;
}

//---------------------------------------------------------------------------
bool NiBASSAudioSystem::SetMasterVolume(float fVolume)
{
    if (!NiAudioSystem::SetMasterVolume(fVolume))
        return false;

    if (!m_spListener)
        return true;

    const DWORD uiVolume = NiAudioVolumeToBASSConfig(m_fMasterVolume);
    const bool bStreams = BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, uiVolume) != 0;
    const bool bSamples = BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, uiVolume) != 0;
    const bool bMusic = BASS_SetConfig(BASS_CONFIG_GVOL_MUSIC, uiVolume) != 0;
    return bStreams && bSamples && bMusic;
}
//---------------------------------------------------------------------------
std::string NiBASSAudioSystem::GetLoadedPlugins() const
{
    std::string result = "Loaded plugins:\n";
	result.append(m_hBASSFlac ? " - BASSFLAC\n" : " - BASSFLAC (not loaded)\n");
	result.append(m_hBASSOpus ? " - BASSOPUS\n" : " - BASSOPUS (not loaded)\n");
	result.append(m_hBASSAac ? " - BASSAAC\n" : " - BASSAAC (not loaded)\n");
	result.append(m_bBASSFXLoaded ? " - BASS_FX\n" : " - BASS_FX (not loaded)\n");
	result.append(m_bMixerLoaded ? " - BASS_MIX\n" : " - BASS_MIX (not loaded)");
	return result;
}
//---------------------------------------------------------------------------
NiAudioSource* NiBASSAudioSystem::CreateSource(unsigned int uiType)
{
    return NiNew NiBASSAudioSource(uiType);
}
//---------------------------------------------------------------------------
NiAudioSystem::SpeakerType NiBASSAudioSystem::GetSpeakerType()
{
    return m_eSpeakerType;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::SetSpeakerType(unsigned int uiType)
{
    if (uiType >= (unsigned int)TYPE_3D_SPEAKER_TYPE_COUNT)
        return false;

    // Validate the requested type against what the device actually has
    BASS_INFO kInfo;
    if (BASS_GetInfo(&kInfo))
    {
        static const DWORD kRequiredSpeakers[] = { 2, 2, 4, 4, 6, 8 };
        if (kInfo.speakers < kRequiredSpeakers[uiType])
            return false;
    }

    m_eSpeakerType = (SpeakerType)uiType;
    // Enable Vista-style per-speaker channel assignment
    BASS_SetConfig(BASS_CONFIG_VISTA_SPEAKERS, TRUE);
    return true;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::SetBestSpeakerTypeAvailable()
{
    BASS_INFO kInfo;
    if (!BASS_GetInfo(&kInfo))
        return false;

    // Map device speaker count to the best matching NiAudioSystem type
    if      (kInfo.speakers >= 8) m_eSpeakerType = TYPE_3D_7_1_SPEAKER;
    else if (kInfo.speakers >= 6) m_eSpeakerType = TYPE_3D_5_1_SPEAKER;
    else if (kInfo.speakers >= 4) m_eSpeakerType = TYPE_3D_4_SPEAKER;
    else                          m_eSpeakerType = TYPE_3D_2_SPEAKER;

    BASS_SetConfig(BASS_CONFIG_VISTA_SPEAKERS, TRUE);
    return true;
}
//---------------------------------------------------------------------------
char* NiBASSAudioSystem::GetLastError()
{
    int iErr = BASS_ErrorGetCode();
    if (iErr == BASS_OK)
        m_acLastError[0] = '\0';
    else
        NiSprintf(m_acLastError, sizeof(m_acLastError), "BASS error %d", iErr);
    return m_acLastError;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::GetReverbAvailable()
{
    return m_bBASSFXLoaded;
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::SetCurrentRoomReverb(unsigned int uiPreset)
{
    if (!GetReverbAvailable())
        return false;
    m_uiCurrentReverb = uiPreset;
    return true;
}
//---------------------------------------------------------------------------
unsigned int NiBASSAudioSystem::GetCurrentRoomReverb()
{
    return m_uiCurrentReverb;
}
//---------------------------------------------------------------------------
void NiBASSAudioSystem::Update(float fTime, bool bUpdateAll)
{
    NiAudioSystem::Update(fTime, bUpdateAll);
    BASS_Apply3D();
}
//---------------------------------------------------------------------------
bool NiBASSAudioSystem::SetUnitsPerMeter(float fUnits)
{
    if (!NiAudioSystem::SetUnitsPerMeter(fUnits))
        return false;
    BASS_Set3DFactors(1.0f / m_fUnitsPerMeter, 1.0f, 1.0f);
    return true;
}