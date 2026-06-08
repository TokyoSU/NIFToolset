#include "NIF.Native.Main.Stream.h"
#include "NIF.Native.Internal.h"

extern "C"
{

NIF_StreamHandle NIF_Stream_Create(void)
{
	NIF_StreamHandle_t* handle = new NIF_StreamHandle_t();
	handle->pStream = NiNew NiStream();
	return static_cast<NIF_StreamHandle>(handle);
}

void NIF_Stream_Destroy(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle)
	{
		return;
	}

	NiDelete pStreamHandle->pStream;
	delete pStreamHandle;
}

int NIF_Stream_LoadFromFile(NIF_StreamHandle stream, const char* path)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream || !path)
	{
		return 0;
	}

	return pStreamHandle->pStream->Load(path) ? 1 : 0;
}

void NIF_Stream_Clear(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		return;
	}

	pStreamHandle->pStream->RemoveAllObjects();
	pStreamHandle->pStream->ResetLastErrorInfo();
}

unsigned int NIF_Stream_GetObjectCount(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		return 0;
	}

	return pStreamHandle->pStream->GetObjectCount();
}

unsigned int NIF_Stream_GetLastErrorCode(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		return NiStream::FILE_NOT_LOADED;
	}

	return pStreamHandle->pStream->GetLastError();
}

const char* NIF_Stream_GetLastErrorMessage(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		return nullptr;
	}

	return pStreamHandle->pStream->GetLastErrorMessage();
}

NIF_ObjectHandle NIF_Stream_GetObjectAt(NIF_StreamHandle stream, unsigned int index)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream || index >= pStreamHandle->pStream->GetObjectCount())
	{
		return nullptr;
	}

	return NIF_CreateObjectHandle(pStreamHandle->pStream->GetObjectAt(index));
}

}
