#include "NIF.Native.RenderPipeline.h"
#include "NIF.Native.Internal.h"

#include <new>

#include <NiAlphaAccumulator.h>
#include <NiAlphaSortProcessor.h>
#include <NiDefaultClickRenderStep.h>
#include <NiRenderClick.h>
#include <NiRenderListProcessor.h>
#include <NiRenderStep.h>
#include <NiRenderView.h>
#include <NiMeshCullingProcess.h>
#include <NiFixedString.h>

namespace
{
	template <typename TList>
	unsigned int NIF_GetListCount(const TList& list)
	{
		unsigned int count = 0;
		NiTListIterator pos = list.GetHeadPos();
		while (pos)
		{
			list.GetNext(pos);
			++count;
		}
		return count;
	}

	template <typename TList>
	auto NIF_GetListItemAt(const TList& list, unsigned int index) -> decltype(list.GetHead())
	{
		unsigned int currentIndex = 0;
		NiTListIterator pos = list.GetHeadPos();
		while (pos)
		{
			auto& item = list.GetNext(pos);
			if (currentIndex == index)
			{
				return item;
			}
			++currentIndex;
		}
		return list.GetHead();
	}

	NiCullingProcess* NIF_GetCullingProcess(NIF_CullingProcessHandle cullingProcess)
	{
		NIF_CullingProcessHandle_t* pHandle = static_cast<NIF_CullingProcessHandle_t*>(cullingProcess);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiMeshCullingProcess* NIF_GetMeshCullingProcess(NIF_MeshCullingProcessHandle cullingProcess)
	{
		NIF_MeshCullingProcessHandle_t* pHandle = static_cast<NIF_MeshCullingProcessHandle_t*>(cullingProcess);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiAlphaAccumulator* NIF_GetAlphaAccumulator(NIF_AlphaAccumulatorHandle accumulator)
	{
		NIF_AlphaAccumulatorHandle_t* pHandle = static_cast<NIF_AlphaAccumulatorHandle_t*>(accumulator);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRenderListProcessor* NIF_GetRenderListProcessor(NIF_RenderListProcessorHandle processor)
	{
		NIF_RenderListProcessorHandle_t* pHandle = static_cast<NIF_RenderListProcessorHandle_t*>(processor);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiAlphaSortProcessor* NIF_GetAlphaSortProcessor(NIF_AlphaSortProcessorHandle processor)
	{
		NIF_AlphaSortProcessorHandle_t* pHandle = static_cast<NIF_AlphaSortProcessorHandle_t*>(processor);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRenderView* NIF_GetRenderView(NIF_RenderViewHandle renderView)
	{
		NIF_RenderViewHandle_t* pHandle = static_cast<NIF_RenderViewHandle_t*>(renderView);
		return pHandle ? pHandle->spObject : nullptr;
	}

	Ni3DRenderView* NIF_GetRenderView3D(NIF_RenderView3DHandle renderView)
	{
		NIF_RenderView3DHandle_t* pHandle = static_cast<NIF_RenderView3DHandle_t*>(renderView);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRenderClick* NIF_GetRenderClick(NIF_RenderClickHandle renderClick)
	{
		NIF_RenderClickHandle_t* pHandle = static_cast<NIF_RenderClickHandle_t*>(renderClick);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiViewRenderClick* NIF_GetViewRenderClick(NIF_ViewRenderClickHandle renderClick)
	{
		NIF_ViewRenderClickHandle_t* pHandle = static_cast<NIF_ViewRenderClickHandle_t*>(renderClick);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiRenderStep* NIF_GetRenderStep(NIF_RenderStepHandle renderStep)
	{
		NIF_RenderStepHandle_t* pHandle = static_cast<NIF_RenderStepHandle_t*>(renderStep);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NIF_RenderStepHandle_t* NIF_GetRenderStepStorage(NIF_RenderStepHandle renderStep)
	{
		return static_cast<NIF_RenderStepHandle_t*>(renderStep);
	}

	NiRenderTargetGroup* NIF_GetRenderTargetGroup(NIF_RenderTargetGroupHandle renderTargetGroup)
	{
		NIF_RenderTargetGroupHandle_t* pHandle = static_cast<NIF_RenderTargetGroupHandle_t*>(renderTargetGroup);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiDefaultClickRenderStep* NIF_GetDefaultClickRenderStep(NIF_DefaultClickRenderStepHandle renderStep)
	{
		NIF_DefaultClickRenderStepHandle_t* pHandle = static_cast<NIF_DefaultClickRenderStepHandle_t*>(renderStep);
		return pHandle ? pHandle->spObject : nullptr;
	}

	bool NIF_RenderStepPreCallbackThunk(NiRenderStep* pkCurrentStep, void* pvCallbackData)
	{
		NIF_RenderStepHandle_t* pHandle = static_cast<NIF_RenderStepHandle_t*>(pvCallbackData);
		NIF_RenderStepCallback callback = pHandle ? pHandle->pPreCallback : nullptr;
		return callback ? (callback(static_cast<NIF_RenderStepHandle>(pHandle), pHandle->pPreCallbackUserData) != 0) : true;
	}

	bool NIF_RenderStepPostCallbackThunk(NiRenderStep* pkCurrentStep, void* pvCallbackData)
	{
		NIF_RenderStepHandle_t* pHandle = static_cast<NIF_RenderStepHandle_t*>(pvCallbackData);
		NIF_RenderStepCallback callback = pHandle ? pHandle->pPostCallback : nullptr;
		return callback ? (callback(static_cast<NIF_RenderStepHandle>(pHandle), pHandle->pPostCallbackUserData) != 0) : true;
	}
}

NIF_CullingProcessHandle NIF_CreateCullingProcessHandle(NiCullingProcess* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_CullingProcessHandle_t* pHandle = new (std::nothrow) NIF_CullingProcessHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RenderListProcessorHandle NIF_CreateRenderListProcessorHandle(NiRenderListProcessor* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderListProcessorHandle_t* pHandle = new (std::nothrow) NIF_RenderListProcessorHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_AlphaSortProcessorHandle NIF_CreateAlphaSortProcessorHandle(NiAlphaSortProcessor* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_AlphaSortProcessorHandle_t* pHandle = new (std::nothrow) NIF_AlphaSortProcessorHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_MeshCullingProcessHandle NIF_CreateMeshCullingProcessHandle(NiMeshCullingProcess* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_MeshCullingProcessHandle_t* pHandle = new (std::nothrow) NIF_MeshCullingProcessHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_AlphaAccumulatorHandle NIF_CreateAlphaAccumulatorHandle(NiAlphaAccumulator* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_AlphaAccumulatorHandle_t* pHandle = new (std::nothrow) NIF_AlphaAccumulatorHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RenderViewHandle NIF_CreateRenderViewHandle(NiRenderView* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderViewHandle_t* pHandle = new (std::nothrow) NIF_RenderViewHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RenderView3DHandle NIF_CreateRenderView3DHandle(Ni3DRenderView* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderView3DHandle_t* pHandle = new (std::nothrow) NIF_RenderView3DHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RenderClickHandle NIF_CreateRenderClickHandle(NiRenderClick* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderClickHandle_t* pHandle = new (std::nothrow) NIF_RenderClickHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_ViewRenderClickHandle NIF_CreateViewRenderClickHandle(NiViewRenderClick* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_ViewRenderClickHandle_t* pHandle = new (std::nothrow) NIF_ViewRenderClickHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_RenderStepHandle NIF_CreateRenderStepHandle(NiRenderStep* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_RenderStepHandle_t* pHandle = new (std::nothrow) NIF_RenderStepHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_DefaultClickRenderStepHandle NIF_CreateDefaultClickRenderStepHandle(NiDefaultClickRenderStep* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_DefaultClickRenderStepHandle_t* pHandle = new (std::nothrow) NIF_DefaultClickRenderStepHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

extern "C"
{

void NIF_AlphaAccumulator_Destroy(NIF_AlphaAccumulatorHandle accumulator)
{
	delete static_cast<NIF_AlphaAccumulatorHandle_t*>(accumulator);
}

NIF_AlphaAccumulatorHandle NIF_AlphaAccumulator_Create(void)
{
	NiAlphaAccumulatorPtr spAccumulator = NiNew NiAlphaAccumulator();
	return NIF_CreateAlphaAccumulatorHandle(spAccumulator);
}

void NIF_AlphaAccumulator_SetObserveNoSortHint(NIF_AlphaAccumulatorHandle accumulator, int observe)
{
	NiAlphaAccumulator* pkAccumulator = NIF_GetAlphaAccumulator(accumulator);
	if (pkAccumulator)
	{
		pkAccumulator->SetObserveNoSortHint(observe != 0);
	}
}

void NIF_RenderClick_SetRenderTargetGroup(NIF_RenderClickHandle renderClick, NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	if (pkRenderClick)
	{
		pkRenderClick->SetRenderTargetGroup(pkRenderTargetGroup);
	}
}

int NIF_RenderClick_GetRenderTargetGroup(NIF_RenderClickHandle renderClick, NIF_RenderTargetGroupHandle* outRenderTargetGroup)
{
	if (outRenderTargetGroup)
	{
		*outRenderTargetGroup = nullptr;
	}

	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (!pkRenderClick || !outRenderTargetGroup)
	{
		return 0;
	}

	*outRenderTargetGroup = NIF_CreateRenderTargetGroupHandle(pkRenderClick->GetRenderTargetGroup());
	return *outRenderTargetGroup ? 1 : 0;
}

int NIF_AlphaAccumulator_GetObserveNoSortHint(NIF_AlphaAccumulatorHandle accumulator)
{
	NiAlphaAccumulator* pkAccumulator = NIF_GetAlphaAccumulator(accumulator);
	return (pkAccumulator && pkAccumulator->GetObserveNoSortHint()) ? 1 : 0;
}

void NIF_AlphaAccumulator_SetSortByClosestPoint(NIF_AlphaAccumulatorHandle accumulator, int sortByClosestPoint)
{
	NiAlphaAccumulator* pkAccumulator = NIF_GetAlphaAccumulator(accumulator);
	if (pkAccumulator)
	{
		pkAccumulator->SetSortByClosestPoint(sortByClosestPoint != 0);
	}
}

int NIF_AlphaAccumulator_GetSortByClosestPoint(NIF_AlphaAccumulatorHandle accumulator)
{
	NiAlphaAccumulator* pkAccumulator = NIF_GetAlphaAccumulator(accumulator);
	return (pkAccumulator && pkAccumulator->GetSortByClosestPoint()) ? 1 : 0;
}

void NIF_RenderListProcessor_Destroy(NIF_RenderListProcessorHandle processor)
{
	delete static_cast<NIF_RenderListProcessorHandle_t*>(processor);
}

void NIF_AlphaSortProcessor_Destroy(NIF_AlphaSortProcessorHandle processor)
{
	delete static_cast<NIF_AlphaSortProcessorHandle_t*>(processor);
}

NIF_AlphaSortProcessorHandle NIF_AlphaSortProcessor_Create(int observeNoSortHint, int sortByClosestPoint)
{
	NiAlphaSortProcessorPtr spProcessor = NiNew NiAlphaSortProcessor(observeNoSortHint != 0, sortByClosestPoint != 0);
	return NIF_CreateAlphaSortProcessorHandle(spProcessor);
}

int NIF_AlphaSortProcessor_AsRenderListProcessor(NIF_AlphaSortProcessorHandle processor, NIF_RenderListProcessorHandle* outProcessor)
{
	if (outProcessor)
	{
		*outProcessor = nullptr;
	}

	NiAlphaSortProcessor* pkProcessor = NIF_GetAlphaSortProcessor(processor);
	if (!pkProcessor || !outProcessor)
	{
		return 0;
	}

	*outProcessor = NIF_CreateRenderListProcessorHandle(pkProcessor);
	return *outProcessor ? 1 : 0;
}

void NIF_AlphaSortProcessor_SetObserveNoSortHint(NIF_AlphaSortProcessorHandle processor, int observeNoSortHint)
{
	NiAlphaSortProcessor* pkProcessor = NIF_GetAlphaSortProcessor(processor);
	if (pkProcessor)
	{
		pkProcessor->SetObserveNoSortHint(observeNoSortHint != 0);
	}
}

int NIF_AlphaSortProcessor_GetObserveNoSortHint(NIF_AlphaSortProcessorHandle processor)
{
	NiAlphaSortProcessor* pkProcessor = NIF_GetAlphaSortProcessor(processor);
	return (pkProcessor && pkProcessor->GetObserveNoSortHint()) ? 1 : 0;
}

void NIF_CullingProcess_Destroy(NIF_CullingProcessHandle cullingProcess)
{
	delete static_cast<NIF_CullingProcessHandle_t*>(cullingProcess);
}

void NIF_MeshCullingProcess_Destroy(NIF_MeshCullingProcessHandle cullingProcess)
{
	delete static_cast<NIF_MeshCullingProcessHandle_t*>(cullingProcess);
}

NIF_MeshCullingProcessHandle NIF_MeshCullingProcess_Create(void)
{
	NiMeshCullingProcessPtr spProcess = NiNew NiMeshCullingProcess(nullptr, nullptr);
	return NIF_CreateMeshCullingProcessHandle(spProcess);
}

int NIF_MeshCullingProcess_AsCullingProcess(NIF_MeshCullingProcessHandle cullingProcess, NIF_CullingProcessHandle* outCullingProcess)
{
	if (outCullingProcess)
	{
		*outCullingProcess = nullptr;
	}

	NiMeshCullingProcess* pkCullingProcess = NIF_GetMeshCullingProcess(cullingProcess);
	if (!pkCullingProcess || !outCullingProcess)
	{
		return 0;
	}

	*outCullingProcess = NIF_CreateCullingProcessHandle(pkCullingProcess);
	return *outCullingProcess ? 1 : 0;
}

void NIF_RenderView_Destroy(NIF_RenderViewHandle renderView)
{
	delete static_cast<NIF_RenderViewHandle_t*>(renderView);
}

const char* NIF_RenderView_GetName(NIF_RenderViewHandle renderView)
{
	NiRenderView* pkRenderView = NIF_GetRenderView(renderView);
	return pkRenderView ? pkRenderView->GetName() : nullptr;
}

void NIF_RenderView_SetName(NIF_RenderViewHandle renderView, const char* name)
{
	NiRenderView* pkRenderView = NIF_GetRenderView(renderView);
	if (pkRenderView)
	{
		pkRenderView->SetName(NiFixedString(name));
	}
}

void NIF_ViewRenderClick_SetProcessor(NIF_ViewRenderClickHandle renderClick, NIF_RenderListProcessorHandle processor)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	NiRenderListProcessor* pkProcessor = NIF_GetRenderListProcessor(processor);
	if (pkRenderClick)
	{
		pkRenderClick->SetProcessor(pkProcessor);
	}
}

int NIF_ViewRenderClick_GetProcessor(NIF_ViewRenderClickHandle renderClick, NIF_RenderListProcessorHandle* outProcessor)
{
	if (outProcessor)
	{
		*outProcessor = nullptr;
	}

	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	if (!pkRenderClick || !outProcessor)
	{
		return 0;
	}

	*outProcessor = NIF_CreateRenderListProcessorHandle(pkRenderClick->GetProcessor());
	return *outProcessor ? 1 : 0;
}

void NIF_RenderView_ClearCachedPVGeometry(NIF_RenderViewHandle renderView)
{
	NiRenderView* pkRenderView = NIF_GetRenderView(renderView);
	if (pkRenderView)
	{
		pkRenderView->ClearCachedPVGeometry();
	}
}

void NIF_RenderView3D_Destroy(NIF_RenderView3DHandle renderView)
{
	delete static_cast<NIF_RenderView3DHandle_t*>(renderView);
}

NIF_RenderView3DHandle NIF_RenderView3D_Create(NIF_CameraHandle camera, NIF_CullingProcessHandle cullingProcess, int alwaysUseCameraViewport)
{
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	NiCullingProcess* pkCullingProcess = NIF_GetCullingProcess(cullingProcess);
	Ni3DRenderViewPtr spRenderView = NiNew Ni3DRenderView(pCameraHandle ? pCameraHandle->spObject : nullptr, pkCullingProcess, alwaysUseCameraViewport != 0);
	return NIF_CreateRenderView3DHandle(spRenderView);
}

int NIF_RenderView3D_AsRenderView(NIF_RenderView3DHandle renderView, NIF_RenderViewHandle* outRenderView)
{
	if (outRenderView)
	{
		*outRenderView = nullptr;
	}

	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	if (!pkRenderView || !outRenderView)
	{
		return 0;
	}

	*outRenderView = NIF_CreateRenderViewHandle(pkRenderView);
	return *outRenderView ? 1 : 0;
}

void NIF_RenderView3D_SetCamera(NIF_RenderView3DHandle renderView, NIF_CameraHandle camera)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	NIF_CameraHandle_t* pCameraHandle = static_cast<NIF_CameraHandle_t*>(camera);
	if (pkRenderView)
	{
		pkRenderView->SetCamera(pCameraHandle ? pCameraHandle->spObject : nullptr);
	}
}

NIF_CameraHandle NIF_RenderView3D_GetCamera(NIF_RenderView3DHandle renderView)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	return pkRenderView ? NIF_CreateCameraHandle(pkRenderView->GetCamera()) : nullptr;
}

void NIF_RenderView3D_SetCullingProcess(NIF_RenderView3DHandle renderView, NIF_CullingProcessHandle cullingProcess)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	NiCullingProcess* pkCullingProcess = NIF_GetCullingProcess(cullingProcess);
	if (pkRenderView)
	{
		pkRenderView->SetCullingProcess(pkCullingProcess);
	}
}

NIF_CullingProcessHandle NIF_RenderView3D_GetCullingProcess(NIF_RenderView3DHandle renderView)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	return pkRenderView ? NIF_CreateCullingProcessHandle(pkRenderView->GetCullingProcess()) : nullptr;
}

void NIF_RenderView3D_SetAlwaysUseCameraViewport(NIF_RenderView3DHandle renderView, int alwaysUseCameraViewport)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	if (pkRenderView)
	{
		pkRenderView->SetAlwaysUseCameraViewport(alwaysUseCameraViewport != 0);
	}
}

int NIF_RenderView3D_GetAlwaysUseCameraViewport(NIF_RenderView3DHandle renderView)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	return (pkRenderView && pkRenderView->GetAlwaysUseCameraViewport()) ? 1 : 0;
}

void NIF_RenderView3D_AppendScene(NIF_RenderView3DHandle renderView, NIF_AVObjectHandle scene)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	NIF_AVObjectHandle_t* pSceneHandle = static_cast<NIF_AVObjectHandle_t*>(scene);
	if (pkRenderView && pSceneHandle && pSceneHandle->spObject)
	{
		pkRenderView->AppendScene(pSceneHandle->spObject);
	}
}

void NIF_RenderView3D_PrependScene(NIF_RenderView3DHandle renderView, NIF_AVObjectHandle scene)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	NIF_AVObjectHandle_t* pSceneHandle = static_cast<NIF_AVObjectHandle_t*>(scene);
	if (pkRenderView && pSceneHandle && pSceneHandle->spObject)
	{
		pkRenderView->PrependScene(pSceneHandle->spObject);
	}
}

void NIF_RenderView3D_RemoveScene(NIF_RenderView3DHandle renderView, NIF_AVObjectHandle scene)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	NIF_AVObjectHandle_t* pSceneHandle = static_cast<NIF_AVObjectHandle_t*>(scene);
	if (pkRenderView && pSceneHandle && pSceneHandle->spObject)
	{
		pkRenderView->RemoveScene(pSceneHandle->spObject);
	}
}

void NIF_RenderView3D_RemoveAllScenes(NIF_RenderView3DHandle renderView)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	if (pkRenderView)
	{
		pkRenderView->RemoveAllScenes();
	}
}

unsigned int NIF_RenderView3D_GetSceneCount(NIF_RenderView3DHandle renderView)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	return pkRenderView ? NIF_GetListCount(pkRenderView->GetScenes()) : 0;
}

NIF_AVObjectHandle NIF_RenderView3D_GetSceneAt(NIF_RenderView3DHandle renderView, unsigned int index)
{
	Ni3DRenderView* pkRenderView = NIF_GetRenderView3D(renderView);
	NiAVObject* pkScene = pkRenderView ? NIF_GetListItemAt(pkRenderView->GetScenes(), index) : nullptr;
	return NIF_CreateAVObjectHandle(pkScene);
}

void NIF_RenderClick_Destroy(NIF_RenderClickHandle renderClick)
{
	delete static_cast<NIF_RenderClickHandle_t*>(renderClick);
}

const char* NIF_RenderClick_GetName(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return pkRenderClick ? pkRenderClick->GetName() : nullptr;
}

void NIF_RenderClick_SetName(NIF_RenderClickHandle renderClick, const char* name)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetName(NiFixedString(name));
	}
}

void NIF_RenderClick_SetClearAllBuffers(NIF_RenderClickHandle renderClick, int clearAllBuffers)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetClearAllBuffers(clearAllBuffers != 0);
	}
}

void NIF_RenderClick_SetClearColorBuffers(NIF_RenderClickHandle renderClick, int clearColorBuffers)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetClearColorBuffers(clearColorBuffers != 0);
	}
}

int NIF_RenderClick_GetClearColorBuffers(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return (pkRenderClick && pkRenderClick->GetClearColorBuffers()) ? 1 : 0;
}

void NIF_RenderClick_SetClearDepthBuffer(NIF_RenderClickHandle renderClick, int clearDepthBuffer)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetClearDepthBuffer(clearDepthBuffer != 0);
	}
}

int NIF_RenderClick_GetClearDepthBuffer(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return (pkRenderClick && pkRenderClick->GetClearDepthBuffer()) ? 1 : 0;
}

void NIF_RenderClick_SetClearStencilBuffer(NIF_RenderClickHandle renderClick, int clearStencilBuffer)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetClearStencilBuffer(clearStencilBuffer != 0);
	}
}

int NIF_RenderClick_GetClearStencilBuffer(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return (pkRenderClick && pkRenderClick->GetClearStencilBuffer()) ? 1 : 0;
}

void NIF_RenderClick_RequestClearAllBuffersOnce(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->RequestClearAllBuffersOnce();
	}
}

void NIF_RenderClick_RequestClearColorBuffersOnce(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->RequestClearColorBuffersOnce();
	}
}

void NIF_RenderClick_RequestClearDepthBufferOnce(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->RequestClearDepthBufferOnce();
	}
}

void NIF_RenderClick_RequestClearStencilBufferOnce(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->RequestClearStencilBufferOnce();
	}
}

void NIF_RenderClick_SetBackgroundColor(NIF_RenderClickHandle renderClick, NIF_ColorA color)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetBackgroundColor(NIF_MakeColorA(color));
	}
}

NIF_ColorA NIF_RenderClick_GetBackgroundColor(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (!pkRenderClick)
	{
		return NIF_ColorA{ 0.0f, 0.0f, 0.0f, 0.0f };
	}

	NiColorA color;
	pkRenderClick->GetBackgroundColor(color);
	return NIF_MakeColorA(color);
}

void NIF_RenderClick_SetUseRendererBackgroundColor(NIF_RenderClickHandle renderClick, int useRendererBackgroundColor)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetUseRendererBackgroundColor(useRendererBackgroundColor != 0);
	}
}

int NIF_RenderClick_GetUseRendererBackgroundColor(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return (pkRenderClick && pkRenderClick->GetUseRendererBackgroundColor()) ? 1 : 0;
}

void NIF_RenderClick_SetPersistBackgroundColorToRenderer(NIF_RenderClickHandle renderClick, int persist)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetPersistBackgroundColorToRenderer(persist != 0);
	}
}

int NIF_RenderClick_GetPersistBackgroundColorToRenderer(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return (pkRenderClick && pkRenderClick->GetPersistBackgroundColorToRenderer()) ? 1 : 0;
}

void NIF_RenderClick_SetViewport(NIF_RenderClickHandle renderClick, NIF_Rect viewport)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetViewport(NIF_MakeRect(viewport));
	}
}

NIF_Rect NIF_RenderClick_GetViewport(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return pkRenderClick ? NIF_MakeRect(pkRenderClick->GetViewport()) : NIF_Rect{ 0.0f, 0.0f, 0.0f, 0.0f };
}

void NIF_RenderClick_SetActive(NIF_RenderClickHandle renderClick, int active)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetActive(active != 0);
	}
}

int NIF_RenderClick_GetActive(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return (pkRenderClick && pkRenderClick->GetActive()) ? 1 : 0;
}

void NIF_RenderClick_SetRenderTargetGroupDefault(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->SetRenderTargetGroup(nullptr);
	}
}

void NIF_RenderClick_Render(NIF_RenderClickHandle renderClick, unsigned int frameId)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->Render(frameId);
	}
}

unsigned int NIF_RenderClick_GetNumObjectsDrawn(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return pkRenderClick ? pkRenderClick->GetNumObjectsDrawn() : 0;
}

float NIF_RenderClick_GetCullTime(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return pkRenderClick ? pkRenderClick->GetCullTime() : 0.0f;
}

float NIF_RenderClick_GetRenderTime(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	return pkRenderClick ? pkRenderClick->GetRenderTime() : 0.0f;
}

void NIF_RenderClick_ReleaseCaches(NIF_RenderClickHandle renderClick)
{
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->ReleaseCaches();
	}
}

void NIF_ViewRenderClick_Destroy(NIF_ViewRenderClickHandle renderClick)
{
	delete static_cast<NIF_ViewRenderClickHandle_t*>(renderClick);
}

NIF_ViewRenderClickHandle NIF_ViewRenderClick_Create(void)
{
	NiViewRenderClickPtr spRenderClick = NiNew NiViewRenderClick();
	return NIF_CreateViewRenderClickHandle(spRenderClick);
}

int NIF_ViewRenderClick_AsRenderClick(NIF_ViewRenderClickHandle renderClick, NIF_RenderClickHandle* outRenderClick)
{
	if (outRenderClick)
	{
		*outRenderClick = nullptr;
	}

	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	if (!pkRenderClick || !outRenderClick)
	{
		return 0;
	}

	*outRenderClick = NIF_CreateRenderClickHandle(pkRenderClick);
	return *outRenderClick ? 1 : 0;
}

void NIF_ViewRenderClick_AppendRenderView(NIF_ViewRenderClickHandle renderClick, NIF_RenderViewHandle renderView)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	NiRenderView* pkRenderView = NIF_GetRenderView(renderView);
	if (pkRenderClick && pkRenderView)
	{
		pkRenderClick->AppendRenderView(pkRenderView);
	}
}

void NIF_ViewRenderClick_PrependRenderView(NIF_ViewRenderClickHandle renderClick, NIF_RenderViewHandle renderView)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	NiRenderView* pkRenderView = NIF_GetRenderView(renderView);
	if (pkRenderClick && pkRenderView)
	{
		pkRenderClick->PrependRenderView(pkRenderView);
	}
}

void NIF_ViewRenderClick_RemoveRenderView(NIF_ViewRenderClickHandle renderClick, NIF_RenderViewHandle renderView)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	NiRenderView* pkRenderView = NIF_GetRenderView(renderView);
	if (pkRenderClick && pkRenderView)
	{
		pkRenderClick->RemoveRenderView(pkRenderView);
	}
}

void NIF_ViewRenderClick_RemoveAllRenderViews(NIF_ViewRenderClickHandle renderClick)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	if (pkRenderClick)
	{
		pkRenderClick->RemoveAllRenderViews();
	}
}

unsigned int NIF_ViewRenderClick_GetRenderViewCount(NIF_ViewRenderClickHandle renderClick)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	return pkRenderClick ? NIF_GetListCount(pkRenderClick->GetRenderViews()) : 0;
}

NIF_RenderViewHandle NIF_ViewRenderClick_GetRenderViewAt(NIF_ViewRenderClickHandle renderClick, unsigned int index)
{
	NiViewRenderClick* pkRenderClick = NIF_GetViewRenderClick(renderClick);
	NiRenderView* pkRenderView = pkRenderClick ? NIF_GetListItemAt(pkRenderClick->GetRenderViews(), index) : nullptr;
	return NIF_CreateRenderViewHandle(pkRenderView);
}

void NIF_RenderStep_Destroy(NIF_RenderStepHandle renderStep)
{
	NIF_RenderStepHandle_t* pHandle = static_cast<NIF_RenderStepHandle_t*>(renderStep);
	if (!pHandle)
	{
		return;
	}

	NiRenderStep* pkRenderStep = pHandle->spObject;
	if (pkRenderStep)
	{
		if (pkRenderStep->GetPreProcessingCallbackFunc() == &NIF_RenderStepPreCallbackThunk &&
			pkRenderStep->GetPreProcessingCallbackFuncData() == pHandle)
		{
			pkRenderStep->SetPreProcessingCallbackFunc(nullptr, nullptr);
		}
		if (pkRenderStep->GetPostProcessingCallbackFunc() == &NIF_RenderStepPostCallbackThunk &&
			pkRenderStep->GetPostProcessingCallbackFuncData() == pHandle)
		{
			pkRenderStep->SetPostProcessingCallbackFunc(nullptr, nullptr);
		}
	}
	delete pHandle;
}

const char* NIF_RenderStep_GetName(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	return pkRenderStep ? pkRenderStep->GetName() : nullptr;
}

void NIF_RenderStep_SetName(NIF_RenderStepHandle renderStep, const char* name)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	if (pkRenderStep)
	{
		pkRenderStep->SetName(NiFixedString(name));
	}
}

void NIF_RenderStep_Render(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	if (pkRenderStep)
	{
		pkRenderStep->Render();
	}
}

void NIF_RenderStep_SetActive(NIF_RenderStepHandle renderStep, int active)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	if (pkRenderStep)
	{
		pkRenderStep->SetActive(active != 0);
	}
}

int NIF_RenderStep_GetActive(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	return (pkRenderStep && pkRenderStep->GetActive()) ? 1 : 0;
}

void NIF_RenderStep_SetPreCallback(NIF_RenderStepHandle renderStep, NIF_RenderStepCallback callback, void* userData)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	NIF_RenderStepHandle_t* pHandle = NIF_GetRenderStepStorage(renderStep);
	if (!pkRenderStep || !pHandle)
	{
		return;
	}

	pHandle->pPreCallback = callback;
	pHandle->pPreCallbackUserData = userData;
	pkRenderStep->SetPreProcessingCallbackFunc(callback ? &NIF_RenderStepPreCallbackThunk : nullptr, callback ? pHandle : nullptr);
}

void NIF_RenderStep_SetPostCallback(NIF_RenderStepHandle renderStep, NIF_RenderStepCallback callback, void* userData)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	NIF_RenderStepHandle_t* pHandle = NIF_GetRenderStepStorage(renderStep);
	if (!pkRenderStep || !pHandle)
	{
		return;
	}

	pHandle->pPostCallback = callback;
	pHandle->pPostCallbackUserData = userData;
	pkRenderStep->SetPostProcessingCallbackFunc(callback ? &NIF_RenderStepPostCallbackThunk : nullptr, callback ? pHandle : nullptr);
}

void NIF_RenderStep_ClearCallbacks(NIF_RenderStepHandle renderStep)
{
	NIF_RenderStepHandle_t* pHandle = NIF_GetRenderStepStorage(renderStep);
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	if (!pHandle || !pkRenderStep)
	{
		return;
	}

	if (pkRenderStep->GetPreProcessingCallbackFunc() == &NIF_RenderStepPreCallbackThunk &&
		pkRenderStep->GetPreProcessingCallbackFuncData() == pHandle)
	{
		pkRenderStep->SetPreProcessingCallbackFunc(nullptr, nullptr);
	}
	if (pkRenderStep->GetPostProcessingCallbackFunc() == &NIF_RenderStepPostCallbackThunk &&
		pkRenderStep->GetPostProcessingCallbackFuncData() == pHandle)
	{
		pkRenderStep->SetPostProcessingCallbackFunc(nullptr, nullptr);
	}
	pHandle->pPreCallback = nullptr;
	pHandle->pPostCallback = nullptr;
	pHandle->pPreCallbackUserData = nullptr;
	pHandle->pPostCallbackUserData = nullptr;
}

unsigned int NIF_RenderStep_GetNumObjectsDrawn(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	return pkRenderStep ? pkRenderStep->GetNumObjectsDrawn() : 0;
}

float NIF_RenderStep_GetCullTime(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	return pkRenderStep ? pkRenderStep->GetCullTime() : 0.0f;
}

float NIF_RenderStep_GetRenderTime(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	return pkRenderStep ? pkRenderStep->GetRenderTime() : 0.0f;
}

void NIF_RenderStep_ReleaseCaches(NIF_RenderStepHandle renderStep)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	if (pkRenderStep)
	{
		pkRenderStep->ReleaseCaches();
	}
}

int NIF_RenderStep_SetOutputRenderTargetGroup(NIF_RenderStepHandle renderStep, NIF_RenderTargetGroupHandle renderTargetGroup)
{
	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	NiRenderTargetGroup* pkRenderTargetGroup = NIF_GetRenderTargetGroup(renderTargetGroup);
	return (pkRenderStep && pkRenderStep->SetOutputRenderTargetGroup(pkRenderTargetGroup)) ? 1 : 0;
}

int NIF_RenderStep_GetOutputRenderTargetGroup(NIF_RenderStepHandle renderStep, NIF_RenderTargetGroupHandle* outRenderTargetGroup)
{
	if (outRenderTargetGroup)
	{
		*outRenderTargetGroup = nullptr;
	}

	NiRenderStep* pkRenderStep = NIF_GetRenderStep(renderStep);
	if (!pkRenderStep || !outRenderTargetGroup)
	{
		return 0;
	}

	*outRenderTargetGroup = NIF_CreateRenderTargetGroupHandle(pkRenderStep->GetOutputRenderTargetGroup());
	return *outRenderTargetGroup ? 1 : 0;
}

void NIF_DefaultClickRenderStep_Destroy(NIF_DefaultClickRenderStepHandle renderStep)
{
	delete static_cast<NIF_DefaultClickRenderStepHandle_t*>(renderStep);
}

NIF_DefaultClickRenderStepHandle NIF_DefaultClickRenderStep_Create(void)
{
	NiDefaultClickRenderStepPtr spRenderStep = NiNew NiDefaultClickRenderStep();
	return NIF_CreateDefaultClickRenderStepHandle(spRenderStep);
}

int NIF_DefaultClickRenderStep_AsRenderStep(NIF_DefaultClickRenderStepHandle renderStep, NIF_RenderStepHandle* outRenderStep)
{
	if (outRenderStep)
	{
		*outRenderStep = nullptr;
	}

	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	if (!pkRenderStep || !outRenderStep)
	{
		return 0;
	}

	*outRenderStep = NIF_CreateRenderStepHandle(pkRenderStep);
	return *outRenderStep ? 1 : 0;
}

void NIF_DefaultClickRenderStep_AppendRenderClick(NIF_DefaultClickRenderStepHandle renderStep, NIF_RenderClickHandle renderClick)
{
	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderStep && pkRenderClick)
	{
		pkRenderStep->AppendRenderClick(pkRenderClick);
	}
}

void NIF_DefaultClickRenderStep_PrependRenderClick(NIF_DefaultClickRenderStepHandle renderStep, NIF_RenderClickHandle renderClick)
{
	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderStep && pkRenderClick)
	{
		pkRenderStep->PrependRenderClick(pkRenderClick);
	}
}

void NIF_DefaultClickRenderStep_RemoveRenderClick(NIF_DefaultClickRenderStepHandle renderStep, NIF_RenderClickHandle renderClick)
{
	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	NiRenderClick* pkRenderClick = NIF_GetRenderClick(renderClick);
	if (pkRenderStep && pkRenderClick)
	{
		pkRenderStep->RemoveRenderClick(pkRenderClick);
	}
}

void NIF_DefaultClickRenderStep_RemoveAllRenderClicks(NIF_DefaultClickRenderStepHandle renderStep)
{
	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	if (pkRenderStep)
	{
		pkRenderStep->RemoveAllRenderClicks();
	}
}

unsigned int NIF_DefaultClickRenderStep_GetRenderClickCount(NIF_DefaultClickRenderStepHandle renderStep)
{
	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	return pkRenderStep ? NIF_GetListCount(pkRenderStep->GetRenderClickList()) : 0;
}

NIF_RenderClickHandle NIF_DefaultClickRenderStep_GetRenderClickAt(NIF_DefaultClickRenderStepHandle renderStep, unsigned int index)
{
	NiDefaultClickRenderStep* pkRenderStep = NIF_GetDefaultClickRenderStep(renderStep);
	NiRenderClick* pkRenderClick = pkRenderStep ? NIF_GetListItemAt(pkRenderStep->GetRenderClickList(), index) : nullptr;
	return NIF_CreateRenderClickHandle(pkRenderClick);
}

}
