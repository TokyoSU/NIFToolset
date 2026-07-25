#include "NIF.Native.Main.Stream.h"
#include "NIF.Native.Internal.h"

#include <new>

namespace
{
	bool NIF_StreamContainsObject(NiStream* stream, NiObject* object)
	{
		if (!stream || !object)
		{
			return false;
		}
		for (unsigned int index = 0; index < stream->GetObjectCount(); ++index)
		{
			if (stream->GetObjectAt(index) == object)
			{
				return true;
			}
		}
		return false;
	}

	void NIF_SetStreamFailure(NiStream* stream, const char* fallback)
	{
		const char* message = stream ? stream->GetLastErrorMessage() : nullptr;
		NIF_SetLastError(NIF_RESULT_ENGINE_ERROR, message && message[0] ? message : fallback);
	}
}

extern "C"
{

NIF_StreamHandle NIF_Stream_Create(void)
{
	try
	{
		NIF_StreamHandle_t* handle = new (std::nothrow) NIF_StreamHandle_t();
		if (!handle)
		{
			NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate stream handle");
			return nullptr;
		}
		handle->pStream = NiNew NiStream();
		if (!handle->pStream)
		{
			delete handle;
			NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate NiStream");
			return nullptr;
		}
		return static_cast<NIF_StreamHandle>(handle);
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Stream_Create");
		return nullptr;
	}
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
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid stream handle");
		return 0;
	}
	if (!path || !path[0])
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "path must not be empty");
		return 0;
	}

	try
	{
		if (!pStreamHandle->pStream->Load(path))
		{
			NIF_SetStreamFailure(pStreamHandle->pStream, "Failed to load NIF file");
			return 0;
		}
		return 1;
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Stream_LoadFromFile");
		return 0;
	}
}

int NIF_Stream_SaveToFile(NIF_StreamHandle stream, const char* path)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid stream handle");
		return 0;
	}
	if (!path || !path[0])
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "path must not be empty");
		return 0;
	}

	try
	{
		if (!pStreamHandle->pStream->Save(path))
		{
			NIF_SetStreamFailure(pStreamHandle->pStream, "Failed to save NIF file");
			return 0;
		}
		return 1;
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Stream_SaveToFile");
		return 0;
	}
}

int NIF_Stream_InsertObject(NIF_StreamHandle stream, NIF_ObjectHandle object)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pStreamHandle || !pStreamHandle->pStream || !pObjectHandle || !pObjectHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid stream or object handle");
		return 0;
	}
	if (NIF_StreamContainsObject(pStreamHandle->pStream, pObjectHandle->spObject))
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "Object is already present in the stream");
		return 0;
	}
	try
	{
		pStreamHandle->pStream->InsertObject(pObjectHandle->spObject);
		return 1;
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Stream_InsertObject");
		return 0;
	}
}

int NIF_Stream_RemoveObject(NIF_StreamHandle stream, NIF_ObjectHandle object)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	NIF_ObjectHandle_t* pObjectHandle = static_cast<NIF_ObjectHandle_t*>(object);
	if (!pStreamHandle || !pStreamHandle->pStream || !pObjectHandle || !pObjectHandle->spObject)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid stream or object handle");
		return 0;
	}
	if (!NIF_StreamContainsObject(pStreamHandle->pStream, pObjectHandle->spObject))
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "Object is not present in the stream");
		return 0;
	}
	try
	{
		pStreamHandle->pStream->RemoveObject(pObjectHandle->spObject);
		return 1;
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Stream_RemoveObject");
		return 0;
	}
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

size_t NIF_Stream_CopyLastErrorMessage(NIF_StreamHandle stream, char* destination, size_t destinationSize)
{
	return NIF_CopyStringInternal(NIF_Stream_GetLastErrorMessage(stream), destination, destinationSize);
}

unsigned int NIF_Stream_GetFileVersion(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	return (pStreamHandle && pStreamHandle->pStream) ? pStreamHandle->pStream->GetFileVersion() : 0;
}

unsigned int NIF_Stream_GetUserVersion(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	return (pStreamHandle && pStreamHandle->pStream) ? pStreamHandle->pStream->GetFileUserDefinedVersion() : 0;
}

int NIF_Stream_GetSaveAsLittleEndian(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	return (pStreamHandle && pStreamHandle->pStream && pStreamHandle->pStream->GetSaveAsLittleEndian()) ? 1 : 0;
}

void NIF_Stream_SetSaveAsLittleEndian(NIF_StreamHandle stream, int littleEndian)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid stream handle");
		return;
	}
	pStreamHandle->pStream->SetSaveAsLittleEndian(littleEndian != 0);
}

int NIF_Stream_GetSourceIsLittleEndian(NIF_StreamHandle stream)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	return (pStreamHandle && pStreamHandle->pStream && pStreamHandle->pStream->GetSourceIsLittleEndian()) ? 1 : 0;
}

NIF_ObjectHandle NIF_Stream_GetObjectAt(NIF_StreamHandle stream, unsigned int index)
{
	NIF_StreamHandle_t* pStreamHandle = static_cast<NIF_StreamHandle_t*>(stream);
	if (!pStreamHandle || !pStreamHandle->pStream)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid stream handle");
		return nullptr;
	}
	if (index >= pStreamHandle->pStream->GetObjectCount())
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_RANGE, "stream object index is out of range");
		return nullptr;
	}

	return NIF_CreateObjectHandle(pStreamHandle->pStream->GetObjectAt(index));
}

}
