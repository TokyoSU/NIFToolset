#include "NIF.Native.Floodgate.h"
#include "NIF.Native.Internal.h"

#include <NiDataStream.h>
#include <NiSPStream.h>
#include <NiSPTask.h>
#include <NiSPWorkflow.h>
#include <NiStreamProcessor.h>

struct NIF_FloodgateTaskHandle_t
{
	NiSPTaskPtr spTask;
};

struct NIF_FloodgateWorkflowHandle_t
{
	NiSPWorkflowPtr spWorkflow;
};

struct NIF_FloodgateStreamHandle_t
{
	NiSPStream* pStream = nullptr;
};

extern "C"
{

void NIF_Floodgate_Task_Destroy(NIF_FloodgateTaskHandle task)
{
	delete static_cast<NIF_FloodgateTaskHandle_t*>(task);
}

NIF_FloodgateTaskHandle NIF_Floodgate_Task_Create(unsigned short inputCount, unsigned short outputCount)
{
	NiSPTaskPtr spTask = NiSPTask::GetNewTask(inputCount, outputCount);
	if (!spTask)
	{
		return nullptr;
	}

	NIF_FloodgateTaskHandle_t* pHandle = new NIF_FloodgateTaskHandle_t();
	pHandle->spTask = spTask;
	return pHandle;
}

void NIF_Floodgate_Task_Clear(NIF_FloodgateTaskHandle task, int ignoreCaching)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	if (!pHandle || !pHandle->spTask)
	{
		return;
	}

	pHandle->spTask->Clear(ignoreCaching != 0);
}

unsigned int NIF_Floodgate_Task_GetInputCount(NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	return (pHandle && pHandle->spTask) ? pHandle->spTask->GetInputCount() : 0;
}

unsigned int NIF_Floodgate_Task_GetOutputCount(NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	return (pHandle && pHandle->spTask) ? pHandle->spTask->GetOutputCount() : 0;
}

void NIF_Floodgate_Task_SetBlockCount(NIF_FloodgateTaskHandle task, unsigned int blockCount)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	if (!pHandle || !pHandle->spTask)
	{
		return;
	}

	pHandle->spTask->SetOptimalBlockCount(blockCount);
}

unsigned int NIF_Floodgate_Task_GetBlockCount(NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	return (pHandle && pHandle->spTask) ? pHandle->spTask->GetOptimalBlockCount() : 0;
}

void NIF_Floodgate_Task_SetDataDecompositionEnabled(NIF_FloodgateTaskHandle task, int enabled)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	if (!pHandle || !pHandle->spTask)
	{
		return;
	}

	pHandle->spTask->SetIsDataDecompositionEnabled(enabled != 0);
}

int NIF_Floodgate_Task_IsDataDecompositionEnabled(NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	return (pHandle && pHandle->spTask && pHandle->spTask->IsDataDecompositionEnabled()) ? 1 : 0;
}

void NIF_Floodgate_Task_SetCacheable(NIF_FloodgateTaskHandle task, int cacheable)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	if (!pHandle || !pHandle->spTask)
	{
		return;
	}

	pHandle->spTask->SetIsCacheable(cacheable != 0);
}

int NIF_Floodgate_Task_IsCacheable(NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	return (pHandle && pHandle->spTask && pHandle->spTask->IsCacheable()) ? 1 : 0;
}

void NIF_Floodgate_Task_SetCompacted(NIF_FloodgateTaskHandle task, int compacted)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	if (!pHandle || !pHandle->spTask)
	{
		return;
	}

	pHandle->spTask->SetIsCompacted(compacted != 0);
}

int NIF_Floodgate_Task_IsCompacted(NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateTaskHandle_t* pHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	return (pHandle && pHandle->spTask && pHandle->spTask->IsCompacted()) ? 1 : 0;
}

int NIF_Floodgate_Task_AddInput(NIF_FloodgateTaskHandle task, NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateTaskHandle_t* pTaskHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	NIF_FloodgateStreamHandle_t* pStreamHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pTaskHandle || !pTaskHandle->spTask || !pStreamHandle || !pStreamHandle->pStream)
	{
		return 0;
	}

	pTaskHandle->spTask->AddInput(pStreamHandle->pStream);
	return 1;
}

int NIF_Floodgate_Task_AddOutput(NIF_FloodgateTaskHandle task, NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateTaskHandle_t* pTaskHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	NIF_FloodgateStreamHandle_t* pStreamHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pTaskHandle || !pTaskHandle->spTask || !pStreamHandle || !pStreamHandle->pStream)
	{
		return 0;
	}

	pTaskHandle->spTask->AddOutput(pStreamHandle->pStream);
	return 1;
}

int NIF_Floodgate_Task_RemoveInput(NIF_FloodgateTaskHandle task, NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateTaskHandle_t* pTaskHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	NIF_FloodgateStreamHandle_t* pStreamHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pTaskHandle || !pTaskHandle->spTask || !pStreamHandle || !pStreamHandle->pStream)
	{
		return 0;
	}

	pTaskHandle->spTask->RemoveInput(pStreamHandle->pStream);
	return 1;
}

int NIF_Floodgate_Task_RemoveOutput(NIF_FloodgateTaskHandle task, NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateTaskHandle_t* pTaskHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	NIF_FloodgateStreamHandle_t* pStreamHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pTaskHandle || !pTaskHandle->spTask || !pStreamHandle || !pStreamHandle->pStream)
	{
		return 0;
	}

	pTaskHandle->spTask->RemoveOutput(pStreamHandle->pStream);
	return 1;
}

void NIF_Floodgate_Workflow_Destroy(NIF_FloodgateWorkflowHandle workflow)
{
	delete static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
}

NIF_FloodgateWorkflowHandle NIF_Floodgate_Workflow_Create(void)
{
	NiSPWorkflowPtr spWorkflow = NiSPWorkflow::GetFreeWorkflow();
	if (!spWorkflow)
	{
		return nullptr;
	}

	NIF_FloodgateWorkflowHandle_t* pHandle = new NIF_FloodgateWorkflowHandle_t();
	pHandle->spWorkflow = spWorkflow;
	return pHandle;
}

void NIF_Floodgate_Workflow_Clear(NIF_FloodgateWorkflowHandle workflow)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	if (!pHandle || !pHandle->spWorkflow)
	{
		return;
	}

	pHandle->spWorkflow->Clear();
}

void NIF_Floodgate_Workflow_Reset(NIF_FloodgateWorkflowHandle workflow)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	if (!pHandle || !pHandle->spWorkflow)
	{
		return;
	}

	pHandle->spWorkflow->Reset();
}

unsigned int NIF_Floodgate_Workflow_GetTaskCount(NIF_FloodgateWorkflowHandle workflow)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	return (pHandle && pHandle->spWorkflow) ? pHandle->spWorkflow->GetSize() : 0;
}

int NIF_Floodgate_Workflow_AddTask(NIF_FloodgateWorkflowHandle workflow, NIF_FloodgateTaskHandle task)
{
	NIF_FloodgateWorkflowHandle_t* pWorkflowHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	NIF_FloodgateTaskHandle_t* pTaskHandle = static_cast<NIF_FloodgateTaskHandle_t*>(task);
	if (!pWorkflowHandle || !pWorkflowHandle->spWorkflow || !pTaskHandle || !pTaskHandle->spTask)
	{
		return 0;
	}

	pWorkflowHandle->spWorkflow->Add(pTaskHandle->spTask);
	return 1;
}

NIF_FloodgateTaskHandle NIF_Floodgate_Workflow_AddNewTask(NIF_FloodgateWorkflowHandle workflow, unsigned short inputCount, unsigned short outputCount)
{
	NIF_FloodgateWorkflowHandle_t* pWorkflowHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	if (!pWorkflowHandle || !pWorkflowHandle->spWorkflow)
	{
		return nullptr;
	}

	NiSPTask* pkTask = pWorkflowHandle->spWorkflow->AddNewTask(inputCount, outputCount, false);
	if (!pkTask)
	{
		return nullptr;
	}

	NIF_FloodgateTaskHandle_t* pHandle = new NIF_FloodgateTaskHandle_t();
	pHandle->spTask = pkTask;
	return pHandle;
}

int NIF_Floodgate_Workflow_GetStatus(NIF_FloodgateWorkflowHandle workflow)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	return (pHandle && pHandle->spWorkflow) ? static_cast<int>(pHandle->spWorkflow->GetStatus()) : 0;
}

int NIF_Floodgate_Processor_IsAvailable(void)
{
	return NiStreamProcessor::Get() ? 1 : 0;
}

unsigned int NIF_Floodgate_Processor_GetWorkerThreadCount(void)
{
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	return pkProcessor ? pkProcessor->GetWorkerThreadCount() : 0;
}

int NIF_Floodgate_Processor_SetWorkerThreadCount(unsigned int workerThreadCount)
{
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	return (pkProcessor && pkProcessor->SetWorkerThreadCount(workerThreadCount)) ? 1 : 0;
}

int NIF_Floodgate_Processor_GetParallelExecution(void)
{
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	return (pkProcessor && pkProcessor->GetParallelExecution()) ? 1 : 0;
}

int NIF_Floodgate_Processor_SetParallelExecution(int parallelExecution)
{
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	return (pkProcessor && pkProcessor->SetParallelExecution(parallelExecution != 0)) ? 1 : 0;
}

int NIF_Floodgate_Processor_Submit(NIF_FloodgateWorkflowHandle workflow, int priority)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	if (!pHandle || !pHandle->spWorkflow || !pkProcessor)
	{
		return 0;
	}

	return pkProcessor->Submit(pHandle->spWorkflow, static_cast<NiStreamProcessor::Priority>(priority)) ? 1 : 0;
}

int NIF_Floodgate_Processor_Poll(NIF_FloodgateWorkflowHandle workflow)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	if (!pHandle || !pHandle->spWorkflow || !pkProcessor)
	{
		return 0;
	}

	return pkProcessor->Poll(pHandle->spWorkflow) ? 1 : 0;
}

int NIF_Floodgate_Processor_Wait(NIF_FloodgateWorkflowHandle workflow, unsigned int timeout)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	if (!pHandle || !pHandle->spWorkflow || !pkProcessor)
	{
		return 0;
	}

	return pkProcessor->Wait(pHandle->spWorkflow, timeout) ? 1 : 0;
}

void NIF_Floodgate_Processor_Clear(NIF_FloodgateWorkflowHandle workflow)
{
	NIF_FloodgateWorkflowHandle_t* pHandle = static_cast<NIF_FloodgateWorkflowHandle_t*>(workflow);
	NiStreamProcessor* pkProcessor = NiStreamProcessor::Get();
	if (!pHandle || !pHandle->spWorkflow || !pkProcessor)
	{
		return;
	}

	pkProcessor->Clear(pHandle->spWorkflow);
}

void NIF_Floodgate_Stream_Destroy(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle)
	{
		return;
	}

	delete pHandle->pStream;
	delete pHandle;
}

NIF_FloodgateStreamHandle NIF_Floodgate_Stream_Create(void* data, unsigned int stride, unsigned int blockCount, int fixedInput)
{
	NIF_FloodgateStreamHandle_t* pHandle = new NIF_FloodgateStreamHandle_t();
	pHandle->pStream = NiNew NiSPStream(data, stride, blockCount, fixedInput != 0);
	return pHandle;
}

void* NIF_Floodgate_Stream_GetData(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetData() : nullptr;
}

void NIF_Floodgate_Stream_SetData(NIF_FloodgateStreamHandle stream, void* data)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->SetData(data);
}

unsigned int NIF_Floodgate_Stream_GetStride(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetStride() : 0;
}

void NIF_Floodgate_Stream_SetStride(NIF_FloodgateStreamHandle stream, unsigned int stride)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->SetStride(static_cast<NiUInt16>(stride));
}

unsigned int NIF_Floodgate_Stream_GetBlockCount(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetBlockCount() : 0;
}

void NIF_Floodgate_Stream_SetBlockCount(NIF_FloodgateStreamHandle stream, unsigned int blockCount)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->SetBlockCount(blockCount);
}

int NIF_Floodgate_Stream_IsFixedInput(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream && pHandle->pStream->IsFixedInput()) ? 1 : 0;
}

int NIF_Floodgate_Stream_GetAutoSetBlockCount(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream && pHandle->pStream->GetAutoSetBlockCount()) ? 1 : 0;
}

void NIF_Floodgate_Stream_SetAutoSetBlockCount(NIF_FloodgateStreamHandle stream, int autoSetBlockCount)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->SetAutoSetBlockCount(autoSetBlockCount != 0);
}

unsigned int NIF_Floodgate_Stream_GetRegionIndex(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetRegionIdx() : 0;
}

void NIF_Floodgate_Stream_SetRegionIndex(NIF_FloodgateStreamHandle stream, unsigned int regionIndex)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->SetRegionIdx(regionIndex);
}

unsigned int NIF_Floodgate_Stream_GetElementOffset(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetElementOffset() : 0;
}

void NIF_Floodgate_Stream_SetElementOffset(NIF_FloodgateStreamHandle stream, unsigned int elementOffset)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->SetElementOffset(elementOffset);
}

void NIF_Floodgate_Stream_SetDataSource(NIF_FloodgateStreamHandle stream, NIF_DataStreamHandle dataStream, int autoSetBlockCount, unsigned int regionIndex, unsigned int elementOffset)
{
	NIF_FloodgateStreamHandle_t* pStreamHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	NIF_DataStreamHandle_t* pDataStreamHandle = static_cast<NIF_DataStreamHandle_t*>(dataStream);
	if (!pStreamHandle || !pStreamHandle->pStream || !pDataStreamHandle || !pDataStreamHandle->spObject)
	{
		return;
	}

	pStreamHandle->pStream->SetDataSource(pDataStreamHandle->spObject, autoSetBlockCount != 0, regionIndex, elementOffset);
}

NIF_DataStreamHandle NIF_Floodgate_Stream_GetDataSource(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? NIF_CreateDataStreamHandle(static_cast<NiDataStream*>(pHandle->pStream->GetDataSource())) : nullptr;
}

unsigned int NIF_Floodgate_Stream_GetInputCount(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetEffectiveInputSize() : 0;
}

unsigned int NIF_Floodgate_Stream_GetOutputCount(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetEffectiveOutputSize() : 0;
}

unsigned int NIF_Floodgate_Stream_GetDataSize(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	return (pHandle && pHandle->pStream) ? pHandle->pStream->GetDataSize() : 0;
}

void NIF_Floodgate_Stream_Prepare(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->Prepare();
}

void NIF_Floodgate_Stream_ClearTaskArrays(NIF_FloodgateStreamHandle stream)
{
	NIF_FloodgateStreamHandle_t* pHandle = static_cast<NIF_FloodgateStreamHandle_t*>(stream);
	if (!pHandle || !pHandle->pStream)
	{
		return;
	}

	pHandle->pStream->ClearTaskArrays();
}

}
