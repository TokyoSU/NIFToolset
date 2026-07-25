#include "NIF.Native.Animation.h"
#include "NIF.Native.Internal.h"

#include <cstddef>
#include <new>

static_assert(offsetof(NIF_TextKeyDesc, text) == (sizeof(void*) == 8 ? 8u : 4u), "Unexpected text-key layout");
static_assert(sizeof(NIF_TextKeyDesc) == (sizeof(void*) == 8 ? 16u : 8u), "Unexpected text-key size");
static_assert(sizeof(NIF_KFMSequenceGroupEntryDesc) == 28u, "Unexpected sequence-group entry size");
static_assert(sizeof(NIF_KFMBlendPairDesc) == sizeof(void*) * 2u, "Unexpected blend-pair size");
static_assert(sizeof(NIF_KFMChainEntryDesc) == 8u, "Unexpected chain-entry size");

#include <NiActorManager.h>
#include <NiAnimState.h>
#include <NiAnimationConstants.h>
#include <NiControllerSequence.h>
#include <NiFixedString.h>
#include <NiKFMTool.h>
#include <NiSequenceData.h>
#include <NiTextKey.h>
#include <NiTextKeyExtraData.h>
#include <NiTimeController.h>

namespace
{
	NiControllerSequence* NIF_GetSequence(NIF_ControllerSequenceHandle sequence)
	{
		NIF_ControllerSequenceHandle_t* pHandle = static_cast<NIF_ControllerSequenceHandle_t*>(sequence);
		return pHandle ? NiDynamicCast(NiControllerSequence, pHandle->spObject) : nullptr;
	}

	NiTextKeyExtraData* NIF_GetTextKeys(NIF_TextKeyExtraDataHandle textKeys)
	{
		NIF_TextKeyExtraDataHandle_t* pHandle = static_cast<NIF_TextKeyExtraDataHandle_t*>(textKeys);
		return pHandle ? NiDynamicCast(NiTextKeyExtraData, pHandle->spObject) : nullptr;
	}

	NiSequenceData* NIF_GetSequenceData(NIF_SequenceDataHandle sequenceData)
	{
		NIF_SequenceDataHandle_t* pHandle = static_cast<NIF_SequenceDataHandle_t*>(sequenceData);
		return pHandle ? NiDynamicCast(NiSequenceData, pHandle->spObject) : nullptr;
	}

	NiKFMTool* NIF_GetKFMTool(NIF_KFMToolHandle kfmTool)
	{
		NIF_KFMToolHandle_t* pHandle = static_cast<NIF_KFMToolHandle_t*>(kfmTool);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiActorManager* NIF_GetActorManager(NIF_ActorManagerHandle actorManager)
	{
		NIF_ActorManagerHandle_t* pHandle = static_cast<NIF_ActorManagerHandle_t*>(actorManager);
		return pHandle ? pHandle->spObject : nullptr;
	}

	NiAVObject* NIF_GetAVObject(NIF_AVObjectHandle avObject)
	{
		NIF_AVObjectHandle_t* pHandle = static_cast<NIF_AVObjectHandle_t*>(avObject);
		return pHandle ? pHandle->spObject : nullptr;
	}
}

NIF_KFMToolHandle NIF_CreateKFMToolHandle(NiKFMTool* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_KFMToolHandle_t* pHandle = new (std::nothrow) NIF_KFMToolHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_ActorManagerHandle NIF_CreateActorManagerHandle(NiActorManager* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_ActorManagerHandle_t* pHandle = new (std::nothrow) NIF_ActorManagerHandle_t();
	if (!pHandle)
	{
		NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate native handle");
		return nullptr;
	}
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_SequenceDataHandle NIF_CreateSequenceDataHandle(NiSequenceData* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_SequenceDataHandle_t* pHandle = new (std::nothrow) NIF_SequenceDataHandle_t();
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

void NIF_Animation_KFM_Destroy(NIF_KFMToolHandle kfmTool)
{
	delete static_cast<NIF_KFMToolHandle_t*>(kfmTool);
}

NIF_KFMToolHandle NIF_Animation_KFM_Create(void)
{
	try
	{
		NiKFMToolPtr spTool = NiNew NiKFMTool();
		if (!spTool)
		{
			NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate NiKFMTool");
			return nullptr;
		}
		return NIF_CreateKFMToolHandle(spTool);
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Animation_KFM_Create");
		return nullptr;
	}
}

NIF_KFMToolHandle NIF_Animation_KFM_Load(const char* filename)
{
	if (!filename || !filename[0])
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "KFM filename is null or empty");
		return nullptr;
	}

	try
	{
		NiKFMToolPtr spTool = NiNew NiKFMTool();
		if (!spTool)
		{
			NIF_SetLastError(NIF_RESULT_OUT_OF_MEMORY, "Failed to allocate NiKFMTool");
			return nullptr;
		}
		if (spTool->LoadFile(filename) != NiKFMTool::KFM_SUCCESS)
		{
			NIF_SetLastError(NIF_RESULT_ENGINE_ERROR, "NiKFMTool failed to load the KFM file");
			return nullptr;
		}
		return NIF_CreateKFMToolHandle(spTool);
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Animation_KFM_Load");
		return nullptr;
	}
}

int NIF_Animation_KFM_LoadFile(NIF_KFMToolHandle kfmTool, const char* filename)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool)
	{
		NIF_SetLastError(NIF_RESULT_INVALID_HANDLE, "Invalid KFM tool handle");
		return 0;
	}
	if (!filename || !filename[0])
	{
		NIF_SetLastError(NIF_RESULT_INVALID_ARGUMENT, "KFM filename is null or empty");
		return 0;
	}

	try
	{
		if (pkKFMTool->LoadFile(filename) != NiKFMTool::KFM_SUCCESS)
		{
			NIF_SetLastError(NIF_RESULT_ENGINE_ERROR, "NiKFMTool failed to load the KFM file");
			return 0;
		}
		return 1;
	}
	catch (...)
	{
		NIF_SetLastErrorFromCurrentException("NIF_Animation_KFM_LoadFile");
		return 0;
	}
}

const char* NIF_Animation_KFM_GetModelPath(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<const char*>(pkKFMTool->GetModelPath()) : nullptr;
}

void NIF_Animation_KFM_SetModelPath(NIF_KFMToolHandle kfmTool, const char* modelPath)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (pkKFMTool && modelPath)
	{
		pkKFMTool->SetModelPath(NiFixedString(modelPath));
	}
}

const char* NIF_Animation_KFM_GetModelRoot(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<const char*>(pkKFMTool->GetModelRoot()) : nullptr;
}

void NIF_Animation_KFM_SetModelRoot(NIF_KFMToolHandle kfmTool, const char* modelRoot)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (pkKFMTool && modelRoot)
	{
		pkKFMTool->SetModelRoot(NiFixedString(modelRoot));
	}
}

const char* NIF_Animation_KFM_GetBaseKFMPath(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<const char*>(pkKFMTool->GetBaseKFMPath()) : nullptr;
}

void NIF_Animation_KFM_SetBaseKFMPath(NIF_KFMToolHandle kfmTool, const char* baseKfmPath)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (pkKFMTool && baseKfmPath)
	{
		pkKFMTool->SetBaseKFMPath(NiFixedString(baseKfmPath));
	}
}

unsigned int NIF_Animation_KFM_GetSequenceCount(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool)
	{
		return 0;
	}

	NiKFMTool::SequenceID* pSequenceIds = nullptr;
	unsigned int uiCount = 0;
	pkKFMTool->GetSequenceIDs(pSequenceIds, uiCount);
	NiFree(pSequenceIds);
	return uiCount;
}

int NIF_Animation_KFM_GetSequenceIdAt(NIF_KFMToolHandle kfmTool, unsigned int index, unsigned int* outSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool || !outSequenceId)
	{
		return 0;
	}

	NiKFMTool::SequenceID* pSequenceIds = nullptr;
	unsigned int uiCount = 0;
	pkKFMTool->GetSequenceIDs(pSequenceIds, uiCount);
	if (!pSequenceIds || index >= uiCount)
	{
		NiFree(pSequenceIds);
		return 0;
	}

	*outSequenceId = pSequenceIds[index];
	NiFree(pSequenceIds);
	return 1;
}

unsigned int NIF_Animation_KFM_FindSequenceId(NIF_KFMToolHandle kfmTool, const char* sequenceName)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool || !sequenceName)
	{
		return NiActorManager::INVALID_SEQUENCE_ID;
	}

	NiKFMTool::SequenceID* pSequenceIds = nullptr;
	unsigned int uiCount = 0;
	pkKFMTool->GetSequenceIDs(pSequenceIds, uiCount);
	for (unsigned int ui = 0; ui < uiCount; ++ui)
	{
		NiKFMTool::Sequence* pkSequence = pkKFMTool->GetSequence(pSequenceIds[ui]);
		if (pkSequence && NiStricmp(static_cast<const char*>(pkSequence->GetSequenceName()), sequenceName) == 0)
		{
			unsigned int uiSequenceId = pSequenceIds[ui];
			NiFree(pSequenceIds);
			return uiSequenceId;
		}
	}

	NiFree(pSequenceIds);
	return NiActorManager::INVALID_SEQUENCE_ID;
}

int NIF_Animation_KFM_AddSequence(NIF_KFMToolHandle kfmTool, unsigned int sequenceId, const char* filename, const char* sequenceName)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return (pkKFMTool && filename && sequenceName) ? static_cast<int>(pkKFMTool->AddSequence(sequenceId, NiFixedString(filename), NiFixedString(sequenceName))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_UpdateSequence(NIF_KFMToolHandle kfmTool, unsigned int sequenceId, const char* filename, const char* sequenceName)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return (pkKFMTool && filename && sequenceName) ? static_cast<int>(pkKFMTool->UpdateSequence(sequenceId, NiFixedString(filename), NiFixedString(sequenceName))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveSequence(NIF_KFMToolHandle kfmTool, unsigned int sequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveSequence(sequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_AddTransition(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, int transitionType, float duration)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->AddTransition(sourceSequenceId, destinationSequenceId, static_cast<NiKFMTool::TransitionType>(transitionType), duration)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_UpdateTransition(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, int transitionType, float duration)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->UpdateTransition(sourceSequenceId, destinationSequenceId, static_cast<NiKFMTool::TransitionType>(transitionType), duration)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveTransition(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveTransition(sourceSequenceId, destinationSequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_IsTransitionAllowed(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, int* outAllowed)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool || !outAllowed)
	{
		return static_cast<int>(NiKFMTool::KFM_ERROR);
	}

	bool bAllowed = false;
	NiKFMTool::KFM_RC eResult = pkKFMTool->IsTransitionAllowed(sourceSequenceId, destinationSequenceId, bAllowed);
	if (eResult == NiKFMTool::KFM_SUCCESS)
	{
		*outAllowed = bAllowed ? 1 : 0;
	}
	return static_cast<int>(eResult);
}

int NIF_Animation_KFM_AddSequenceGroup(NIF_KFMToolHandle kfmTool, unsigned int groupId, const char* groupName)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return (pkKFMTool && groupName) ? static_cast<int>(pkKFMTool->AddSequenceGroup(groupId, NiFixedString(groupName))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveSequenceGroup(NIF_KFMToolHandle kfmTool, unsigned int groupId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveSequenceGroup(groupId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

unsigned int NIF_Animation_KFM_GetGroupCount(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool)
	{
		return 0;
	}

	NiKFMTool::GroupID* pGroupIds = nullptr;
	unsigned int uiCount = 0;
	pkKFMTool->GetGroupIDs(pGroupIds, uiCount);
	NiFree(pGroupIds);
	return uiCount;
}

int NIF_Animation_KFM_GetGroupIdAt(NIF_KFMToolHandle kfmTool, unsigned int index, unsigned int* outGroupId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (!pkKFMTool || !outGroupId)
	{
		return 0;
	}

	NiKFMTool::GroupID* pGroupIds = nullptr;
	unsigned int uiCount = 0;
	pkKFMTool->GetGroupIDs(pGroupIds, uiCount);
	if (!pGroupIds || index >= uiCount)
	{
		NiFree(pGroupIds);
		return 0;
	}

	*outGroupId = pGroupIds[index];
	NiFree(pGroupIds);
	return 1;
}

int NIF_Animation_KFM_AddSequenceToGroup(NIF_KFMToolHandle kfmTool, unsigned int groupId, unsigned int sequenceId, int priority, float weight, float easeInTime, float easeOutTime, unsigned int synchronizeSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->AddSequenceToGroup(groupId, sequenceId, priority, weight, easeInTime, easeOutTime, synchronizeSequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveSequenceFromGroup(NIF_KFMToolHandle kfmTool, unsigned int groupId, unsigned int sequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveSequenceFromGroup(groupId, sequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveAllSequencesFromGroup(NIF_KFMToolHandle kfmTool, unsigned int groupId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveAllSequencesFromGroup(groupId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

unsigned int NIF_Animation_KFM_GetSequenceGroupEntryCount(NIF_KFMToolHandle kfmTool, unsigned int groupId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	NiKFMTool::SequenceGroup* pkGroup = pkKFMTool ? pkKFMTool->GetSequenceGroup(groupId) : nullptr;
	return pkGroup ? pkGroup->GetSequenceInfo().GetSize() : 0;
}

int NIF_Animation_KFM_GetSequenceGroupEntryAt(NIF_KFMToolHandle kfmTool, unsigned int groupId, unsigned int index, NIF_KFMSequenceGroupEntryDesc* outEntry)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	NiKFMTool::SequenceGroup* pkGroup = pkKFMTool ? pkKFMTool->GetSequenceGroup(groupId) : nullptr;
	if (!pkGroup || !outEntry || index >= pkGroup->GetSequenceInfo().GetSize())
	{
		return 0;
	}

	NiKFMTool::SequenceGroup::SequenceInfo& kEntry = pkGroup->GetSequenceInfo().GetAt(index);
	outEntry->sequenceId = kEntry.GetSequenceID();
	outEntry->priority = kEntry.GetPriority();
	outEntry->weight = kEntry.GetWeight();
	outEntry->easeInTime = kEntry.GetEaseInTime();
	outEntry->easeOutTime = kEntry.GetEaseOutTime();
	outEntry->synchronizeSequenceId = kEntry.GetSynchronizeSequenceID();
	outEntry->additive = kEntry.GetAdditive() ? 1 : 0;
	return 1;
}

int NIF_Animation_KFM_AddBlendPair(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, const char* startKey, const char* targetKey)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return (pkKFMTool && (startKey || targetKey)) ? static_cast<int>(pkKFMTool->AddBlendPair(sourceSequenceId, destinationSequenceId, NiFixedString(startKey), NiFixedString(targetKey))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveBlendPair(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, const char* startKey, const char* targetKey)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return (pkKFMTool && (startKey || targetKey)) ? static_cast<int>(pkKFMTool->RemoveBlendPair(sourceSequenceId, destinationSequenceId, NiFixedString(startKey), NiFixedString(targetKey))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveAllBlendPairs(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveAllBlendPairs(sourceSequenceId, destinationSequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

unsigned int NIF_Animation_KFM_GetBlendPairCount(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	NiKFMTool::Transition* pkTransition = pkKFMTool ? pkKFMTool->GetTransition(sourceSequenceId, destinationSequenceId) : nullptr;
	return pkTransition ? pkTransition->GetBlendPairs().GetSize() : 0;
}

int NIF_Animation_KFM_GetBlendPairAt(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, unsigned int index, NIF_KFMBlendPairDesc* outBlendPair)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	NiKFMTool::Transition* pkTransition = pkKFMTool ? pkKFMTool->GetTransition(sourceSequenceId, destinationSequenceId) : nullptr;
	if (!pkTransition || !outBlendPair || index >= pkTransition->GetBlendPairs().GetSize())
	{
		return 0;
	}

	NiKFMTool::Transition::BlendPair* pkBlendPair = pkTransition->GetBlendPairs().GetAt(index);
	if (!pkBlendPair)
	{
		return 0;
	}

	outBlendPair->startKey = pkBlendPair->GetStartKey();
	outBlendPair->targetKey = pkBlendPair->GetTargetKey();
	return 1;
}

int NIF_Animation_KFM_AddSequenceToChain(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, unsigned int sequenceId, float duration)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->AddSequenceToChain(sourceSequenceId, destinationSequenceId, sequenceId, duration)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveSequenceFromChain(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, unsigned int sequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveSequenceFromChain(sourceSequenceId, destinationSequenceId, sequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_RemoveAllSequencesFromChain(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->RemoveAllSequencesFromChain(sourceSequenceId, destinationSequenceId)) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

unsigned int NIF_Animation_KFM_GetChainEntryCount(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	NiKFMTool::Transition* pkTransition = pkKFMTool ? pkKFMTool->GetTransition(sourceSequenceId, destinationSequenceId) : nullptr;
	return pkTransition ? pkTransition->GetChainInfo().GetSize() : 0;
}

int NIF_Animation_KFM_GetChainEntryAt(NIF_KFMToolHandle kfmTool, unsigned int sourceSequenceId, unsigned int destinationSequenceId, unsigned int index, NIF_KFMChainEntryDesc* outChainEntry)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	NiKFMTool::Transition* pkTransition = pkKFMTool ? pkKFMTool->GetTransition(sourceSequenceId, destinationSequenceId) : nullptr;
	if (!pkTransition || !outChainEntry || index >= pkTransition->GetChainInfo().GetSize())
	{
		return 0;
	}

	NiKFMTool::Transition::ChainInfo& kChainEntry = pkTransition->GetChainInfo().GetAt(index);
	outChainEntry->sequenceId = kChainEntry.GetSequenceID();
	outChainEntry->duration = kChainEntry.GetDuration();
	return 1;
}

int NIF_Animation_KFM_GetDefaultSyncTransitionType(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->GetDefaultSyncTransType()) : static_cast<int>(NiKFMTool::TYPE_DEFAULT_INVALID);
}

int NIF_Animation_KFM_SetDefaultSyncTransitionType(NIF_KFMToolHandle kfmTool, int transitionType)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->SetDefaultSyncTransType(static_cast<NiKFMTool::TransitionType>(transitionType))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

int NIF_Animation_KFM_GetDefaultNonSyncTransitionType(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->GetDefaultNonSyncTransType()) : static_cast<int>(NiKFMTool::TYPE_DEFAULT_INVALID);
}

int NIF_Animation_KFM_SetDefaultNonSyncTransitionType(NIF_KFMToolHandle kfmTool, int transitionType)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? static_cast<int>(pkKFMTool->SetDefaultNonSyncTransType(static_cast<NiKFMTool::TransitionType>(transitionType))) : static_cast<int>(NiKFMTool::KFM_ERROR);
}

float NIF_Animation_KFM_GetDefaultSyncTransitionDuration(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? pkKFMTool->GetDefaultSyncTransDuration() : 0.0f;
}

void NIF_Animation_KFM_SetDefaultSyncTransitionDuration(NIF_KFMToolHandle kfmTool, float duration)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (pkKFMTool)
	{
		pkKFMTool->SetDefaultSyncTransDuration(duration);
	}
}

float NIF_Animation_KFM_GetDefaultNonSyncTransitionDuration(NIF_KFMToolHandle kfmTool)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return pkKFMTool ? pkKFMTool->GetDefaultNonSyncTransDuration() : 0.0f;
}

void NIF_Animation_KFM_SetDefaultNonSyncTransitionDuration(NIF_KFMToolHandle kfmTool, float duration)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	if (pkKFMTool)
	{
		pkKFMTool->SetDefaultNonSyncTransDuration(duration);
	}
}

const char* NIF_Animation_KFM_LookupReturnCode(int returnCode)
{
	return NiKFMTool::LookupReturnCode(static_cast<NiKFMTool::KFM_RC>(returnCode));
}

void NIF_Animation_ActorManager_Destroy(NIF_ActorManagerHandle actorManager)
{
	delete static_cast<NIF_ActorManagerHandle_t*>(actorManager);
}

NIF_ActorManagerHandle NIF_Animation_ActorManager_CreateFromKFM(NIF_KFMToolHandle kfmTool, const char* kfmFilePath, int cumulativeAnimations, int loadFilesFromDisk)
{
	NiKFMTool* pkKFMTool = NIF_GetKFMTool(kfmTool);
	return (pkKFMTool && kfmFilePath) ? NIF_CreateActorManagerHandle(NiActorManager::Create(pkKFMTool, kfmFilePath, cumulativeAnimations != 0, loadFilesFromDisk != 0)) : nullptr;
}

NIF_ActorManagerHandle NIF_Animation_ActorManager_CreateFromFile(const char* kfmFilename, int cumulativeAnimations, int loadFilesFromDisk)
{
	return kfmFilename ? NIF_CreateActorManagerHandle(NiActorManager::Create(kfmFilename, cumulativeAnimations != 0, loadFilesFromDisk != 0)) : nullptr;
}

int NIF_Animation_ActorManager_ReloadNIFFile(NIF_ActorManagerHandle actorManager, int loadNifFile)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && pkActorManager->ReloadNIFFile(nullptr, loadNifFile != 0, nullptr)) ? 1 : 0;
}

int NIF_Animation_ActorManager_ReloadKFFile(NIF_ActorManagerHandle actorManager, const char* filename)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && filename && pkActorManager->ReloadKFFile(filename)) ? 1 : 0;
}

int NIF_Animation_ActorManager_ChangeNIFRoot(NIF_ActorManagerHandle actorManager, NIF_AVObjectHandle nifRoot)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	NiAVObject* pkNifRoot = NIF_GetAVObject(nifRoot);
	return (pkActorManager && pkNifRoot && pkActorManager->ChangeNIFRoot(pkNifRoot, nullptr)) ? 1 : 0;
}

int NIF_Animation_ActorManager_LoadSequenceData(NIF_ActorManagerHandle actorManager, unsigned int sequenceId, int loadKfFile)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && pkActorManager->LoadSequenceData(sequenceId, loadKfFile != 0, nullptr)) ? 1 : 0;
}

int NIF_Animation_ActorManager_LoadAllSequenceData(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && pkActorManager->LoadAllSequenceData()) ? 1 : 0;
}

void NIF_Animation_ActorManager_UnloadSequenceData(NIF_ActorManagerHandle actorManager, unsigned int sequenceId)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->UnloadSequenceData(sequenceId);
	}
}

void NIF_Animation_ActorManager_Update(NIF_ActorManagerHandle actorManager, float time)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->Update(time);
	}
}

float NIF_Animation_ActorManager_GetLastUpdateTime(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? pkActorManager->GetLastUpdateTime() : NiActorManager::INVALID_TIME;
}

unsigned int NIF_Animation_ActorManager_GetTargetAnimation(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? pkActorManager->GetTargetAnimation() : NiActorManager::INVALID_SEQUENCE_ID;
}

int NIF_Animation_ActorManager_SetTargetAnimation(NIF_ActorManagerHandle actorManager, unsigned int sequenceId)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && pkActorManager->SetTargetAnimation(sequenceId)) ? 1 : 0;
}

void NIF_Animation_ActorManager_Reset(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->Reset();
	}
}

unsigned int NIF_Animation_ActorManager_GetCurrentAnimation(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? pkActorManager->GetCurAnimation() : NiActorManager::INVALID_SEQUENCE_ID;
}

int NIF_Animation_ActorManager_GetTransitionState(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? static_cast<int>(pkActorManager->GetTransitionState()) : static_cast<int>(NiActorManager::NO_TRANSITION);
}

unsigned int NIF_Animation_ActorManager_GetNextAnimation(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? pkActorManager->GetNextAnimation() : NiActorManager::INVALID_SEQUENCE_ID;
}

int NIF_Animation_ActorManager_IsPaused(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && pkActorManager->IsPaused()) ? 1 : 0;
}

void NIF_Animation_ActorManager_SetPaused(NIF_ActorManagerHandle actorManager, int paused)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->SetPaused(paused != 0);
	}
}

void NIF_Animation_ActorManager_RefreshControllerManager(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->RefreshControllerManager();
	}
}

NIF_ActorManagerHandle NIF_Animation_ActorManager_Clone(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateActorManagerHandle(pkActorManager->Clone()) : nullptr;
}

NIF_ActorManagerHandle NIF_Animation_ActorManager_CloneOnlyAnimation(NIF_ActorManagerHandle actorManager, NIF_AVObjectHandle avObject, int cumulativeAnimations)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	NiAVObject* pkAVObject = NIF_GetAVObject(avObject);
	return pkActorManager ? NIF_CreateActorManagerHandle(pkActorManager->CloneOnlyAnimation(pkAVObject, cumulativeAnimations != 0)) : nullptr;
}

NIF_KFMToolHandle NIF_Animation_ActorManager_GetKFMTool(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateKFMToolHandle(pkActorManager->GetKFMTool()) : nullptr;
}

NIF_AVObjectHandle NIF_Animation_ActorManager_GetNIFRoot(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateAVObjectHandle(pkActorManager->GetNIFRoot()) : nullptr;
}

NIF_AVObjectHandle NIF_Animation_ActorManager_GetActorRoot(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateAVObjectHandle(pkActorManager->GetActorRoot()) : nullptr;
}

NIF_AVObjectHandle NIF_Animation_ActorManager_GetAccumRoot(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateAVObjectHandle(pkActorManager->GetAccumRoot()) : nullptr;
}

unsigned int NIF_Animation_ActorManager_FindSequenceId(NIF_ActorManagerHandle actorManager, const char* sequenceName)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && sequenceName) ? pkActorManager->FindSequenceID(sequenceName) : NiActorManager::INVALID_SEQUENCE_ID;
}

NIF_ObjectHandle NIF_Animation_ActorManager_GetSequenceData(NIF_ActorManagerHandle actorManager, unsigned int sequenceId)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateObjectHandle(pkActorManager->GetSequenceData(sequenceId)) : nullptr;
}

NIF_ControllerSequenceHandle NIF_Animation_ActorManager_GetActiveSequence(NIF_ActorManagerHandle actorManager, unsigned int sequenceId, int checkExtraSequences, int checkStateSequences)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateControllerSequenceHandle(pkActorManager->GetActiveSequence(sequenceId, checkExtraSequences != 0, checkStateSequences != 0)) : nullptr;
}

unsigned int NIF_Animation_ActorManager_GetSequenceId(NIF_ActorManagerHandle actorManager, NIF_ControllerSequenceHandle sequence, int checkExtraSequences, int checkStateSequences)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return (pkActorManager && pkSequence) ? pkActorManager->GetSequenceID(pkSequence, checkExtraSequences != 0, checkStateSequences != 0) : NiActorManager::INVALID_SEQUENCE_ID;
}

NIF_ControllerSequenceHandle NIF_Animation_ActorManager_ActivateSequence(NIF_ActorManagerHandle actorManager, unsigned int sequenceId, int priority, float weight, float easeInTime, unsigned int timeSyncSequenceId, float frequency, float startFrame, int additiveBlend, float additiveRefFrame)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? NIF_CreateControllerSequenceHandle(pkActorManager->ActivateSequence(sequenceId, priority, weight, easeInTime, timeSyncSequenceId, frequency, startFrame, additiveBlend != 0, additiveRefFrame)) : nullptr;
}

int NIF_Animation_ActorManager_DeactivateSequence(NIF_ActorManagerHandle actorManager, unsigned int sequenceId, float easeOutTime)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && pkActorManager->DeactivateSequence(sequenceId, easeOutTime)) ? 1 : 0;
}

int NIF_Animation_ActorManager_RegisterTextKeyCallback(NIF_ActorManagerHandle actorManager, int eventType, unsigned int sequenceId, const char* textKey)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && textKey) ? (pkActorManager->RegisterCallback(static_cast<NiActorManager::EventType>(eventType), sequenceId, NiFixedString(textKey)) ? 1 : 0) : 0;
}

void NIF_Animation_ActorManager_UnregisterTextKeyCallback(NIF_ActorManagerHandle actorManager, int eventType, unsigned int sequenceId, const char* textKey)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager && textKey)
	{
		pkActorManager->UnregisterCallback(static_cast<NiActorManager::EventType>(eventType), sequenceId, NiFixedString(textKey));
	}
}

int NIF_Animation_ActorManager_RegisterCallback(NIF_ActorManagerHandle actorManager, int eventType, unsigned int sequenceId)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? (pkActorManager->RegisterCallback(static_cast<NiActorManager::EventType>(eventType), sequenceId, static_cast<NiTextKeyMatch*>(nullptr)) ? 1 : 0) : 0;
}

void NIF_Animation_ActorManager_UnregisterCallback(NIF_ActorManagerHandle actorManager, int eventType, unsigned int sequenceId)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->UnregisterCallback(static_cast<NiActorManager::EventType>(eventType), sequenceId, static_cast<NiTextKeyMatch*>(nullptr));
	}
}

void NIF_Animation_ActorManager_ClearAllRegisteredCallbacks(NIF_ActorManagerHandle actorManager)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	if (pkActorManager)
	{
		pkActorManager->ClearAllRegisteredCallbacks();
	}
}

float NIF_Animation_ActorManager_GetNextTextKeyEventTime(NIF_ActorManagerHandle actorManager, unsigned int sequenceId, const char* textKey)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return (pkActorManager && textKey) ? pkActorManager->GetNextEventTime(NiActorManager::TEXT_KEY_EVENT, sequenceId, NiFixedString(textKey)) : NiActorManager::INVALID_TIME;
}

float NIF_Animation_ActorManager_GetNextEndOfSequenceTime(NIF_ActorManagerHandle actorManager, unsigned int sequenceId)
{
	NiActorManager* pkActorManager = NIF_GetActorManager(actorManager);
	return pkActorManager ? pkActorManager->GetNextEventTime(NiActorManager::END_OF_SEQUENCE, sequenceId) : NiActorManager::INVALID_TIME;
}

const char* NIF_Animation_GetStartTextKey(void)
{
	return static_cast<const char*>(NiAnimationConstants::GetStartTextKey());
}

const char* NIF_Animation_GetEndTextKey(void)
{
	return static_cast<const char*>(NiAnimationConstants::GetEndTextKey());
}

const char* NIF_Animation_GetMorphTextKey(void)
{
	return static_cast<const char*>(NiAnimationConstants::GetMorphTextKey());
}

void NIF_Animation_SequenceData_Destroy(NIF_SequenceDataHandle sequenceData)
{
	delete static_cast<NIF_SequenceDataHandle_t*>(sequenceData);
}

NIF_SequenceDataHandle NIF_Animation_SequenceData_CreateFromFileByName(const char* filename, const char* sequenceName)
{
	return (filename && sequenceName) ? NIF_CreateSequenceDataHandle(NiSequenceData::CreateSequenceDataFromFile(filename, NiFixedString(sequenceName))) : nullptr;
}

NIF_SequenceDataHandle NIF_Animation_SequenceData_CreateFromFileByIndex(const char* filename, unsigned int index)
{
	return filename ? NIF_CreateSequenceDataHandle(NiSequenceData::CreateSequenceDataFromFile(filename, index)) : nullptr;
}

const char* NIF_Animation_SequenceData_GetName(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? static_cast<const char*>(pkSequenceData->GetName()) : nullptr;
}

void NIF_Animation_SequenceData_SetName(NIF_SequenceDataHandle sequenceData, const char* name)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	if (pkSequenceData && name)
	{
		pkSequenceData->SetName(NiFixedString(name));
	}
}

NIF_TextKeyExtraDataHandle NIF_Animation_SequenceData_GetTextKeys(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? NIF_CreateTextKeyExtraDataHandle(pkSequenceData->GetTextKeys()) : nullptr;
}

void NIF_Animation_SequenceData_SetTextKeys(NIF_SequenceDataHandle sequenceData, NIF_TextKeyExtraDataHandle textKeys)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	NiTextKeyExtraData* pkTextKeys = NIF_GetTextKeys(textKeys);
	if (pkSequenceData)
	{
		pkSequenceData->SetTextKeys(pkTextKeys);
	}
}

unsigned int NIF_Animation_SequenceData_GetEvaluatorCount(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? pkSequenceData->GetNumEvaluators() : 0;
}

float NIF_Animation_SequenceData_GetKeyTime(NIF_SequenceDataHandle sequenceData, const char* textKey)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return (pkSequenceData && textKey) ? pkSequenceData->GetKeyTimeAt(NiFixedString(textKey)) : NiSequenceData::INVALID_TIME;
}

float NIF_Animation_SequenceData_TimeDivFreq(NIF_SequenceDataHandle sequenceData, float time)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? pkSequenceData->TimeDivFreq(time) : 0.0f;
}

float NIF_Animation_SequenceData_TimeMultFreq(NIF_SequenceDataHandle sequenceData, float time)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? pkSequenceData->TimeMultFreq(time) : 0.0f;
}

float NIF_Animation_SequenceData_GetDuration(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? pkSequenceData->GetDuration() : 0.0f;
}

float NIF_Animation_SequenceData_GetDurationDivFreq(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? pkSequenceData->GetDurationDivFreq() : 0.0f;
}

void NIF_Animation_SequenceData_SetDuration(NIF_SequenceDataHandle sequenceData, float duration)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	if (pkSequenceData)
	{
		pkSequenceData->SetDuration(duration);
	}
}

int NIF_Animation_SequenceData_GetCycleType(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? static_cast<int>(pkSequenceData->GetCycleType()) : static_cast<int>(NiTimeController::LOOP);
}

int NIF_Animation_SequenceData_SetCycleType(NIF_SequenceDataHandle sequenceData, int cycleType)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return (pkSequenceData && pkSequenceData->SetCycleType(static_cast<NiTimeController::CycleType>(cycleType))) ? 1 : 0;
}

float NIF_Animation_SequenceData_GetFrequency(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? pkSequenceData->GetFrequency() : 0.0f;
}

void NIF_Animation_SequenceData_SetFrequency(NIF_SequenceDataHandle sequenceData, float frequency)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	if (pkSequenceData)
	{
		pkSequenceData->SetFrequency(frequency);
	}
}

NIF_ObjectHandle NIF_Animation_SequenceData_AsObject(NIF_SequenceDataHandle sequenceData)
{
	NiSequenceData* pkSequenceData = NIF_GetSequenceData(sequenceData);
	return pkSequenceData ? NIF_CreateObjectHandle(pkSequenceData) : nullptr;
}

void NIF_Animation_Sequence_Destroy(NIF_ControllerSequenceHandle sequence)
{
	delete static_cast<NIF_ControllerSequenceHandle_t*>(sequence);
}

const char* NIF_Animation_Sequence_GetName(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? static_cast<const char*>(pkSequence->GetName()) : nullptr;
}

unsigned int NIF_Animation_Sequence_GetActivationId(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetActivationID() : 0;
}

int NIF_Animation_Sequence_GetState(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? static_cast<int>(pkSequence->GetState()) : static_cast<int>(NiAnimState::INACTIVE);
}

int NIF_Animation_Sequence_IsAdditiveBlend(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return (pkSequence && pkSequence->IsAdditiveBlend()) ? 1 : 0;
}

float NIF_Animation_Sequence_GetOffset(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetOffset() : 0.0f;
}

void NIF_Animation_Sequence_SetOffset(NIF_ControllerSequenceHandle sequence, float offset)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	if (pkSequence)
	{
		pkSequence->SetOffset(offset);
	}
}

int NIF_Animation_Sequence_GetPriority(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetPriority() : 0;
}

float NIF_Animation_Sequence_GetWeight(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetWeight() : 0.0f;
}

void NIF_Animation_Sequence_SetWeight(NIF_ControllerSequenceHandle sequence, float weight)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	if (pkSequence)
	{
		pkSequence->SetWeight(weight);
	}
}

float NIF_Animation_Sequence_GetDuration(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetDuration() : 0.0f;
}

float NIF_Animation_Sequence_GetDurationDivFreq(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetDurationDivFreq() : 0.0f;
}

int NIF_Animation_Sequence_GetCycleType(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? static_cast<int>(pkSequence->GetCycleType()) : static_cast<int>(NiTimeController::LOOP);
}

int NIF_Animation_Sequence_SetCycleType(NIF_ControllerSequenceHandle sequence, int cycleType)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return (pkSequence && pkSequence->SetCycleType(static_cast<NiTimeController::CycleType>(cycleType))) ? 1 : 0;
}

float NIF_Animation_Sequence_GetFrequency(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetFrequency() : 0.0f;
}

void NIF_Animation_Sequence_SetFrequency(NIF_ControllerSequenceHandle sequence, float frequency)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	if (pkSequence)
	{
		pkSequence->SetFrequency(frequency);
	}
}

float NIF_Animation_Sequence_TimeDivFreq(NIF_ControllerSequenceHandle sequence, float time)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->TimeDivFreq(time) : 0.0f;
}

float NIF_Animation_Sequence_TimeMultFreq(NIF_ControllerSequenceHandle sequence, float time)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->TimeMultFreq(time) : 0.0f;
}

float NIF_Animation_Sequence_GetLastTime(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetLastTime() : 0.0f;
}

float NIF_Animation_Sequence_GetLastScaledTime(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetLastScaledTime() : 0.0f;
}

float NIF_Animation_Sequence_GetSpinnerScaledWeight(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetSpinnerScaledWeight() : 0.0f;
}

float NIF_Animation_Sequence_GetEaseSpinner(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetEaseSpinner() : 0.0f;
}

float NIF_Animation_Sequence_GetEaseEndTime(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetEaseEndTime() : 0.0f;
}

float NIF_Animation_Sequence_GetDestFrame(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetDestFrame() : 0.0f;
}

void NIF_Animation_Sequence_Reset(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	if (pkSequence)
	{
		pkSequence->ResetSequence();
	}
}

float NIF_Animation_Sequence_GetKeyTime(NIF_ControllerSequenceHandle sequence, const char* textKey)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return (pkSequence && textKey) ? pkSequence->GetKeyTimeAt(NiFixedString(textKey)) : NiControllerSequence::INVALID_TIME;
}

float NIF_Animation_Sequence_GetTimeAt(NIF_ControllerSequenceHandle sequence, const char* textKey, float currentTime)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return (pkSequence && textKey) ? pkSequence->GetTimeAt(NiFixedString(textKey), currentTime) : NiControllerSequence::INVALID_TIME;
}

unsigned int NIF_Animation_Sequence_GetEvaluatorCount(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? pkSequence->GetNumEvaluators() : 0;
}

NIF_TextKeyExtraDataHandle NIF_Animation_Sequence_GetTextKeys(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? NIF_CreateTextKeyExtraDataHandle(pkSequence->GetTextKeys()) : nullptr;
}

NIF_ControllerSequenceHandle NIF_Animation_Sequence_GetTimeSyncSequence(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? NIF_CreateControllerSequenceHandle(pkSequence->GetTimeSyncSequence()) : nullptr;
}

NIF_ObjectHandle NIF_Animation_Sequence_AsObject(NIF_ControllerSequenceHandle sequence)
{
	NiControllerSequence* pkSequence = NIF_GetSequence(sequence);
	return pkSequence ? NIF_CreateObjectHandle(pkSequence) : nullptr;
}

void NIF_Animation_TextKeys_Destroy(NIF_TextKeyExtraDataHandle textKeys)
{
	delete static_cast<NIF_TextKeyExtraDataHandle_t*>(textKeys);
}

unsigned int NIF_Animation_TextKeys_GetCount(NIF_TextKeyExtraDataHandle textKeys)
{
	NiTextKeyExtraData* pkTextKeys = NIF_GetTextKeys(textKeys);
	unsigned int uiNumKeys = 0;
	if (pkTextKeys)
	{
		pkTextKeys->GetKeys(uiNumKeys);
	}
	return uiNumKeys;
}

int NIF_Animation_TextKeys_GetAt(NIF_TextKeyExtraDataHandle textKeys, unsigned int index, NIF_TextKeyDesc* outKey)
{
	NiTextKeyExtraData* pkTextKeys = NIF_GetTextKeys(textKeys);
	if (!pkTextKeys || !outKey)
	{
		return 0;
	}

	unsigned int uiNumKeys = 0;
	NiTextKey* pkKeys = pkTextKeys->GetKeys(uiNumKeys);
	if (!pkKeys || index >= uiNumKeys)
	{
		return 0;
	}

	outKey->time = pkKeys[index].GetTime();
	outKey->text = static_cast<const char*>(pkKeys[index].GetText());
	return 1;
}

NIF_ObjectHandle NIF_Animation_TextKeys_AsObject(NIF_TextKeyExtraDataHandle textKeys)
{
	NiTextKeyExtraData* pkTextKeys = NIF_GetTextKeys(textKeys);
	return pkTextKeys ? NIF_CreateObjectHandle(pkTextKeys) : nullptr;
}

}
