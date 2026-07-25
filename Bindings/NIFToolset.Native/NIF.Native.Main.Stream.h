#pragma once
#ifndef NIF_NATIVE_MAIN_STREAM_H
#define NIF_NATIVE_MAIN_STREAM_H

#include "NIF.Native.Common.h"

#ifdef __cplusplus
extern "C"
{
#endif

NIFTOOLSET_NATIVE_ENTRY NIF_StreamHandle NIF_Stream_Create(void);
NIFTOOLSET_NATIVE_ENTRY void NIF_Stream_Destroy(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY int NIF_Stream_LoadFromFile(NIF_StreamHandle stream, const char* path);
NIFTOOLSET_NATIVE_ENTRY int NIF_Stream_SaveToFile(NIF_StreamHandle stream, const char* path);
NIFTOOLSET_NATIVE_ENTRY int NIF_Stream_InsertObject(NIF_StreamHandle stream, NIF_ObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY int NIF_Stream_RemoveObject(NIF_StreamHandle stream, NIF_ObjectHandle object);
NIFTOOLSET_NATIVE_ENTRY void NIF_Stream_Clear(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Stream_GetObjectCount(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Stream_GetLastErrorCode(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY const char* NIF_Stream_GetLastErrorMessage(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY size_t NIF_Stream_CopyLastErrorMessage(NIF_StreamHandle stream, char* destination, size_t destinationSize);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Stream_GetFileVersion(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY unsigned int NIF_Stream_GetUserVersion(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY int NIF_Stream_GetSaveAsLittleEndian(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY void NIF_Stream_SetSaveAsLittleEndian(NIF_StreamHandle stream, int littleEndian);
NIFTOOLSET_NATIVE_ENTRY int NIF_Stream_GetSourceIsLittleEndian(NIF_StreamHandle stream);
NIFTOOLSET_NATIVE_ENTRY NIF_ObjectHandle NIF_Stream_GetObjectAt(NIF_StreamHandle stream, unsigned int index);

#ifdef __cplusplus
}
#endif

#endif // NIF_NATIVE_MAIN_STREAM_H
