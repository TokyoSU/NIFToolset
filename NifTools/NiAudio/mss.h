#pragma once
#include <cstdint>

// Type definitions for Miles Sound System handles and structures
typedef void* HDIGDRIVER;
typedef void* HSAMPLE;
typedef void* HSTREAM;
typedef int S32;
typedef unsigned int U32;
typedef float F32;

// Dummy AILSOUNDINFO struct
struct AILSOUNDINFO {
    int samples;
    int channels;
    int rate;
};

// Dummy enums/constants for sample status
#define SMP_DONE 0
#define SMP_PLAYING 1
#define SMP_STOPPED 2
#define SMP_FREE 3
#define SMP_PLAYINGBUTRELEASED 4

// Dummy file type constants
#define AILFILETYPE_PCM_WAV 1
#define AILFILETYPE_ADPCM_WAV 2
#define AILFILETYPE_MPEG_L3_AUDIO 3

// Dummy sample stage
typedef int SAMPLESTAGE;
#define DP_OUTPUT 0
#define SP_OUTPUT 1

// Function stubs for all Miles API used in NiMilesSource.cpp
inline S32 AIL_file_size(const char*) { return 0; }
inline void* AIL_file_read(const char*, void*) { return nullptr; }
inline S32 AIL_file_type(void*, S32) { return 0; }
inline void AIL_WAV_info(void*, AILSOUNDINFO*) {}
inline void AIL_decompress_ADPCM(AILSOUNDINFO*, void**, int) {}
inline void AIL_mem_free_lock(void*) {}
inline void AIL_decompress_ASI(void*, S32, const char*, void**, int, int) {}
inline void* AIL_open_stream(HDIGDRIVER, const char*, int) { return nullptr; }
inline void AIL_close_stream(HSTREAM) {}
inline void AIL_pause_stream(HSTREAM, int) {}
inline void AIL_set_stream_loop_count(HSTREAM, int) {}
inline void AIL_start_stream(HSTREAM) {}
inline HSAMPLE AIL_stream_sample_handle(HSTREAM) { return nullptr; }
inline void AIL_set_sample_volume_levels(HSAMPLE, float, float) {}
inline void AIL_sample_volume_levels(HSAMPLE, float* fLeftGain, float* fRightGain) { if (fLeftGain) *fLeftGain = 1.0f; if (fRightGain) *fRightGain = 1.0f; }
inline void AIL_set_sample_playback_rate(HSAMPLE, S32) {}
inline S32 AIL_sample_playback_rate(HSAMPLE) { return 0; }
inline void AIL_set_sample_3D_cone(HSAMPLE, float, float, float) {}
inline void AIL_sample_3D_cone(HSAMPLE, float*, float*, float*) {}
inline void AIL_set_sample_3D_distances(HSAMPLE, float, float, int) {}
inline void AIL_sample_3D_distances(HSAMPLE, float*, float*, void*) {}
inline void AIL_set_sample_reverb_levels(HSAMPLE, float, float) {}
inline void AIL_sample_reverb_levels(HSAMPLE, int, F32* fLevel) { if (fLevel) *fLevel = 0.0f; }
inline void AIL_set_sample_occlusion(HSAMPLE, float) {}
inline F32 AIL_sample_occlusion(HSAMPLE) { return 0.0f; }
inline void AIL_set_sample_obstruction(HSAMPLE, float) {}
inline F32 AIL_sample_obstruction(HSAMPLE) { return 0.0f; }
inline void AIL_set_sample_3D_position(HSAMPLE, float, float, float) {}
inline void AIL_sample_3D_position(HSAMPLE, float* x, float* y, float* z) { if (x) *x = 0.0f; if (y) *y = 0.0f; if (z) *z = 0.0f; }
inline void AIL_set_sample_3D_velocity_vector(HSAMPLE, float, float, float) {}
inline void AIL_sample_3D_velocity(HSAMPLE, float* x, float* y, float* z) { if (x) *x = 0.0f; if (y) *y = 0.0f; if (z) *z = 0.0f; }
inline void AIL_set_sample_3D_orientation(HSAMPLE, float, float, float, float, float, float) {}
inline void AIL_sample_3D_orientation(HSAMPLE, float*, float*, float*, float*, float*, float*) {}
inline void* AIL_allocate_sample_handle(HDIGDRIVER) { return nullptr; }
inline void AIL_release_sample_handle(HSAMPLE) {}
inline int AIL_set_sample_file(HSAMPLE, void*, int) { return 1; }
inline void AIL_set_sample_loop_count(HSAMPLE, int) {}
inline void AIL_set_sample_position(HSAMPLE, unsigned int) {}
inline void AIL_resume_sample(HSAMPLE) {}
inline void AIL_start_sample(HSAMPLE) {}
inline void AIL_stop_sample(HSAMPLE) {}
inline U32 AIL_stream_status(HSTREAM) { return SMP_DONE; }
inline U32 AIL_sample_status(HSAMPLE) { return SMP_DONE; }
inline void AIL_set_stream_ms_position(HSTREAM, S32) {}
inline void AIL_set_sample_ms_position(HSAMPLE, S32) {}
inline void AIL_stream_ms_position(HSTREAM, S32* offset, S32*) { if (offset) *offset = 0; }
inline void AIL_sample_ms_position(HSAMPLE, S32*, S32* offset) { if (offset) *offset = 0; }
inline void AIL_set_stream_position(HSTREAM, unsigned int) {}
inline void AIL_set_sample_stage_property(HSAMPLE, SAMPLESTAGE, char*, int, void*, int) {}
inline void AIL_sample_stage_property(HSAMPLE, SAMPLESTAGE, char*, void*, void*, void*) {}
inline U32 AIL_stream_position(HSTREAM) { return 0; }
inline U32 AIL_sample_position(HSAMPLE) { return 0; }

typedef void* HDIGDRIVER;

#define AILCALLBACK __stdcall

inline void AIL_startup() {}
inline void AIL_shutdown() {}
inline void AIL_close_digital_driver(HDIGDRIVER) {}

// Stubs for memory management
inline void AIL_mem_use_malloc(void*) {}
inline void AIL_mem_use_free(void*) {}

// Stubs for startup / shutdown
inline bool AIL_quick_startup(int, int, int, int, int) { return false; }
inline void AIL_quick_handles(HDIGDRIVER*, void*, void*) {}
inline void AIL_quick_shutdown() {}

// Stubs for digital driver
inline HDIGDRIVER AIL_set_redist_directory(const char*) { return nullptr; }
inline int MSS_MC_USE_SYSTEM_CONFIG = 0;

// Stubs for 3D / room type
inline void AIL_set_3D_distance_factor(HDIGDRIVER, float) {}
inline void AIL_output_filter_driver_property(HDIGDRIVER, char*, void*, void*, void*) {}
inline unsigned int AIL_room_type(HDIGDRIVER, int) { return 0; }
inline void AIL_set_room_type(HDIGDRIVER, int) {}

// Stubs for sample management
inline unsigned int AIL_active_sample_count(HDIGDRIVER) { return 0; }

// Error handling
inline char* AIL_last_error() { return 0; }

// Callback placeholder
inline void AIL_set_callback(unsigned int, void*, void*) {}

// --------------------
// 3D listener functions
// --------------------
inline void AIL_set_listener_3D_position(HDIGDRIVER, float x, float y, float z) {}
inline void AIL_set_listener_3D_orientation(HDIGDRIVER, float fx, float fy, float fz, float ux, float uy, float uz) {}
inline void AIL_set_listener_3D_velocity_vector(HDIGDRIVER, float x, float y, float z) {}
inline void AIL_listener_3D_position(HDIGDRIVER, float* x, float* y, float* z) { if (x)*x = 0; if (y)*y = 0; if (z)*z = 0; }
inline void AIL_listener_3D_velocity(HDIGDRIVER, float* x, float* y, float* z) { if (x)*x = 0; if (y)*y = 0; if (z)*z = 0; }
inline void AIL_listener_3D_orientation(HDIGDRIVER, float* fx, float* fy, float* fz, float* ux, float* uy, float* uz) {
    if (fx)*fx = 0; if (fy)*fy = 0; if (fz)*fz = 0;
    if (ux)*ux = 0; if (uy)*uy = 0; if (uz)*uz = 0;
}
