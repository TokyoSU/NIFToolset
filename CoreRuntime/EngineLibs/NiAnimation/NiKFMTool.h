// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#pragma once
#ifndef NIKFMTOOL_H
#define NIKFMTOOL_H

#include "NiAnimationLibType.h"
#include <NiFixedString.h>
#include <NiSmartPointer.h>
#include <NiRefObject.h>
#include <NiTArray.h>
#include <NiTSet.h>
#include <NiTPointerMap.h>

namespace efd
{
    class File;
}

class NIANIMATION_ENTRY NiKFMTool : public NiRefObject
{
public:
    typedef NiUInt32 FileVersion;
    typedef NiUInt32 SequenceID;
    typedef NiUInt32 GroupID;
    typedef NiInt32 AnimIndex;
    typedef NiInt32 Priority;

    // Public constants.
    enum KFM_RC   // Return code.
    {
        KFM_SUCCESS,
        KFM_ERROR,
        KFM_ERR_SEQUENCE,
        KFM_ERR_TRANSITION,
        KFM_ERR_TRANSITION_TYPE,
        KFM_ERR_BLEND_PAIR,
        KFM_ERR_NULL_TEXT_KEYS,
        KFM_ERR_BLEND_PAIR_INDEX,
        KFM_ERR_CHAIN_SEQUENCE,
        KFM_ERR_SEQUENCE_IN_CHAIN,
        KFM_ERR_INFINITE_CHAIN,
        KFM_ERR_SEQUENCE_GROUP,
        KFM_ERR_SEQUENCE_IN_GROUP,
        KFM_ERR_FILE_IO,
        KFM_ERR_FILE_FORMAT,
        KFM_ERR_FILE_VERSION,
        KFM_ERR_ENDIAN_MISMATCH,
        KFM_ERR_SYNC_TRANS_TYPE,
        KFM_ERR_NONSYNC_TRANS_TYPE
    };
    enum TransitionType
    {
        TYPE_BLEND,
        TYPE_MORPH,
        TYPE_CROSSFADE,
        TYPE_CHAIN,
        TYPE_DEFAULT_SYNC,
        TYPE_DEFAULT_NONSYNC,
        TYPE_DEFAULT_INVALID
    };
    static const float MAX_DURATION;
    static const SequenceID SYNC_SEQUENCE_ID_NONE;

public:
    // Public internal class declarations. Friend status is given to the
    // NiKFMTool class because the streaming functions need direct access to
    // the member variables in order to use the NiStreamSave* and
    // NiStreamLoad* functions to stream them to and from disk.
    class Transition : public NiMemObject
    {
    public:
        class BlendPair : public NiMemObject
        {
        public:
            BlendPair();
            BlendPair(const NiFixedString& kStartKey,
                const NiFixedString& kTargetKey);
            ~BlendPair();

            inline const NiFixedString& GetStartKey() const;
            inline void SetStartKey(const NiFixedString& kStartKey);

            inline const NiFixedString& GetTargetKey() const;
            inline void SetTargetKey(const NiFixedString& kTargetKey);

        protected:
            NiFixedString m_kStartKey;
            NiFixedString m_kTargetKey;

            friend class NiKFMTool;
        };

        class ChainInfo : public NiMemObject
        {
        public:
            ChainInfo();
            ChainInfo(SequenceID uiSequenceID, float fDuration);

            inline SequenceID GetSequenceID() const;
            inline void SetSequenceID(SequenceID uiSequenceID);

            inline float GetDuration() const;
            inline void SetDuration(float fDuration);

        protected:
            SequenceID m_uiSequenceID;
            float m_fDuration;

            friend class NiKFMTool;
        };

        Transition();
        Transition(TransitionType eType, float fDuration);
        ~Transition();

        // Gets the translated type for the transition. Default types will
        // be resolved to their associated type.
        TransitionType GetType() const;

        // Gets and sets the stored type for the transition. Default types
        // will not be resolved.
        inline TransitionType GetStoredType() const;

        inline float GetDuration() const;
        inline void SetDuration(float fDuration);

        typedef NiTPrimitiveSet<BlendPair*> BlendPairSet;
        typedef NiTObjectSet<ChainInfo> ChainInfoSet;

        BlendPairSet& GetBlendPairs();
        ChainInfoSet& GetChainInfo();

        inline void ClearBlendPairs();
        inline void ClearChainInfo();

    protected:
        TransitionType m_eType;
        float m_fDuration;
        BlendPairSet m_aBlendPairs;
        ChainInfoSet m_aChainInfo;

        // This member is only used by default transition instances.
        TransitionType m_eDefaultType;

        friend class NiKFMTool;
    };

    class Sequence : public NiMemObject
    {
    public:
        Sequence();
        ~Sequence();

        inline SequenceID GetSequenceID() const;
        inline void SetSequenceID(SequenceID uiSequenceID);

        inline const NiFixedString& GetFilename() const;
        inline void SetFilename(const NiFixedString& kFilename);

        inline AnimIndex GetAnimIndex() const;
        inline void SetAnimIndex(AnimIndex iAnimIndex);

        inline const NiFixedString& GetSequenceName() const;
        inline void SetSequenceName(const NiFixedString& kSequenceName);

        inline NiTPointerMap<SequenceID, Transition*>& GetTransitions();

    protected:
        SequenceID m_uiSequenceID;
        NiFixedString m_kFilename;
        AnimIndex m_iAnimIndex; // deprecated, required only for loading old assets
        NiFixedString m_kSequenceName;
        NiTPointerMap<SequenceID, Transition*> m_mapTransitions;

        friend class NiKFMTool;
    };

    class SequenceGroup : public NiMemObject
    {
    public:
        class SequenceInfo : public NiMemObject
        {
        public:
            SequenceInfo();
            SequenceInfo(SequenceID uiSequenceID, Priority iPriority,
                float fWeight, float fEaseInTime, float fEaseOutTime,
                SequenceID uiSynchronizeSequenceID = SYNC_SEQUENCE_ID_NONE,
                bool bAdditive = false);

            inline SequenceID GetSequenceID() const;
            inline void SetSequenceID(SequenceID uiSequenceID);

            inline Priority GetPriority() const;
            inline void SetPriority(Priority iPriority);

            inline float GetWeight() const;
            inline void SetWeight(float fWeight);

            inline float GetEaseInTime() const;
            inline void SetEaseInTime(float fEaseInTime);

            inline float GetEaseOutTime() const;
            inline void SetEaseOutTime(float fEaseOutTime);

            inline SequenceID GetSynchronizeSequenceID() const;
            inline void SetSynchronizeSequenceID(
                SequenceID uiSynchronizeSequenceID);

            inline bool GetAdditive() const;
            inline void SetAdditive(bool bAdditive);

        protected:
            SequenceID m_uiSequenceID;
            Priority m_iPriority;
            float m_fWeight;
            float m_fEaseInTime;
            float m_fEaseOutTime;
            SequenceID m_uiSynchronizeSequenceID;
            bool m_bAdditive;

            friend class NiKFMTool;
        };

        SequenceGroup();
        SequenceGroup(GroupID uiGroupID, const NiFixedString& kName);
        ~SequenceGroup();

        inline GroupID GetGroupID() const;
        inline void SetGroupID(GroupID uiGroupID);

        inline const NiFixedString& GetName() const;
        inline void SetName(const NiFixedString& kName);


        typedef NiTObjectSet<SequenceInfo> SequenceInfoSet;

        SequenceInfoSet& GetSequenceInfo();

    protected:
        GroupID m_uiGroupID;
        NiFixedString m_kName;
        SequenceInfoSet m_aSequenceInfo;

        friend class NiKFMTool;
    };

    // Constructor and destructor.
    NiKFMTool(const NiFixedString& kBaseKFMPath = NULL);
    ~NiKFMTool();

    // Functions for adding components.
    KFM_RC AddSequence(SequenceID uiSequenceID,
        const NiFixedString& kFilename, const NiFixedString& kSequenceName);
    KFM_RC AddTransition(SequenceID uiSrcID, SequenceID uiDesID,
        TransitionType eType, float fDuration);
    KFM_RC AddBlendPair(SequenceID uiSrcID, SequenceID uiDesID,
        const NiFixedString& kStartKey, const NiFixedString& kTargetKey);
    KFM_RC AddSequenceToChain(SequenceID uiSrcID, SequenceID uiDesID,
        SequenceID uiSequenceID, float fDuration);
    KFM_RC AddSequenceGroup(GroupID uiGroupID,
        const NiFixedString& kName);
    KFM_RC AddSequenceToGroup(GroupID uiGroupID,
        SequenceID uiSequenceID, Priority iPriority, float fWeight,
        float fEaseInTime, float fEaseOutTime,
        SequenceID uiSynchronizeToSequence = SYNC_SEQUENCE_ID_NONE);

    // Functions for updating components.
    KFM_RC UpdateSequence(SequenceID uiSequenceID,
        const NiFixedString& kFilename, const NiFixedString& kSequenceName);
    KFM_RC UpdateTransition(SequenceID uiSrcID, SequenceID uiDesID,
        TransitionType eType, float fDuration);
    KFM_RC UpdateSequenceID(SequenceID uiOldID, SequenceID uiNewID);
    KFM_RC UpdateGroupID(GroupID uiOldID, GroupID uiNewID);

    // Functions for removing components.
    KFM_RC RemoveSequence(SequenceID uiSequenceID);
    KFM_RC RemoveTransition(SequenceID uiSrcID, SequenceID uiDesID);
    KFM_RC RemoveBlendPair(SequenceID uiSrcID, SequenceID uiDesID,
        const NiFixedString& kStartKey, const NiFixedString& kTargetKey);
    KFM_RC RemoveAllBlendPairs(SequenceID uiSrcID, SequenceID uiDesID);
    KFM_RC RemoveSequenceFromChain(SequenceID uiSrcID, SequenceID uiDesID,
        SequenceID uiSequenceID);
    KFM_RC RemoveAllSequencesFromChain(SequenceID uiSrcID,
        SequenceID uiDesID);
    KFM_RC RemoveSequenceGroup(GroupID uiGroupID);
    KFM_RC RemoveSequenceFromGroup(GroupID uiGroupID,
        SequenceID uiSequenceID);
    KFM_RC RemoveAllSequencesFromGroup(GroupID uiGroupID);

    // Functions for retrieving components.
    Sequence* GetSequence(SequenceID uiSequenceID) const;
    Transition* GetTransition(SequenceID uiSrcID, SequenceID uiDesID)
        const;
    SequenceGroup* GetSequenceGroup(GroupID uiGroupID) const;

    // Functions for retrieving identifier codes.
    void GetSequenceIDs(SequenceID*& puiSequenceIDs,
        NiUInt32& uiNumIDs) const;
    void GetGroupIDs(GroupID*& puiGroupIDs, NiUInt32& uiNumIDs)
        const;
    SequenceID FindUnusedSequenceID() const;
    GroupID FindUnusedGroupID() const;

    // Functions for accessing model data.
    inline const NiFixedString& GetModelPath() const;
    inline void SetModelPath(const NiFixedString& kModelPath);
    inline const NiFixedString& GetModelRoot() const;
    inline void SetModelRoot(const NiFixedString& kModelRoot);

    // Functions for accessing default transition information.
    TransitionType GetDefaultSyncTransType() const;
    KFM_RC SetDefaultSyncTransType(TransitionType eType);
    TransitionType GetDefaultNonSyncTransType() const;
    KFM_RC SetDefaultNonSyncTransType(TransitionType eType);
    inline float GetDefaultSyncTransDuration() const;
    inline void SetDefaultSyncTransDuration(float fDuration);
    inline float GetDefaultNonSyncTransDuration() const;
    inline void SetDefaultNonSyncTransDuration(float fDuration);

    // Functions for performing lookups on components or component data.
    KFM_RC IsTransitionAllowed(SequenceID uiSrcID, SequenceID uiDesID,
        bool& bAllowed) const;
    static const char* LookupReturnCode(KFM_RC eReturnCode);
    bool IsValidChainTransition(SequenceID uiSrcID, SequenceID uiDesID,
        Transition* pkTransition);

    // Functions for getting fully qualified paths.
    inline const NiFixedString& GetBaseKFMPath() const;
    inline void SetBaseKFMPath(const NiFixedString& kBaseKFMPath);
    const NiFixedString& GetFullModelPath();
    NiFixedString GetFullKFFilename(SequenceID uiSequenceID);

    // Functions for streaming data to a file.
    KFM_RC LoadFile(const char* pcFilename);
    KFM_RC SaveFile(const char* pcFilename, bool bUseBinary = true,
        bool bLittleEndian = true);

    KFM_RC LoadFromStream(efd::BinaryStream* pkStream, const char* pcFilename);

protected:
    void UpdateTransitionsContainingSequence(SequenceID uiOldID,
        SequenceID uiNewID);
    void UpdateSequenceGroupsContainingSequence(SequenceID uiOldID,
        SequenceID uiNewID);
    void RemoveTransitionsContainingSequence(SequenceID uiSequenceID);
    void RemoveSequenceFromSequenceGroups(SequenceID uiSequenceID);

    KFM_RC AddSequence(SequenceID uiSequenceID,
        const NiFixedString& kFilename, AnimIndex iAnimIndex);

    inline Sequence* GetSequenceFromID(SequenceID uiSequenceID) const;
    inline Transition* GetTransitionFromID(SequenceID uiSequenceID,
        Sequence* pkSequence) const;
    inline SequenceGroup* GetSequenceGroupFromID(
        GroupID uiGroupID) const;

    void GatherChainIDs(SequenceID uiSrcID, SequenceID uiDesID,
        Transition* pkTransition, NiUnsignedIntSet& kChainIDs);
    void HandleDelayedBlendsInChains();

    void ConvertRelativePaths(const char* pcNewBaseKFMPath);

    // Streaming functions.
    KFM_RC WriteBinary(efd::BinaryStream& kFile);
    KFM_RC WriteAscii(efd::BinaryStream& kFile);
    KFM_RC ReadBinary(efd::BinaryStream& kFile, FileVersion uiVersion);
    KFM_RC ReadAscii(efd::BinaryStream& kFile, FileVersion uiVersion);
    KFM_RC ReadOldVersionAscii(efd::BinaryStream& kFile, FileVersion uiVersion);

    // Streaming helper functions.
    void SaveCString(efd::BinaryStream& kFile, const char* pcString);
    void LoadCString(efd::BinaryStream& kFile, char*& pcString);
    void SaveFixedString(efd::BinaryStream& kFile, const NiFixedString& kString);
    void LoadFixedString(efd::BinaryStream& kFile, NiFixedString& kString);
    void LoadCStringAsFixedString(efd::BinaryStream& kFile, NiFixedString& kString);

    // Protected variables.
    NiFixedString m_kBaseKFMPath;
    NiFixedString m_kFullPathBuffer;
    NiFixedString m_kModelPath;
    NiFixedString m_kModelRoot;
    NiTPointerMap<SequenceID, Sequence*> m_mapSequences;
    NiTPointerMap<GroupID, SequenceGroup*> m_mapSequenceGroups;

    // Default transition settings.
    Transition* m_pkDefaultSyncTrans;
    Transition* m_pkDefaultNonSyncTrans;
};

NiSmartPointer(NiKFMTool);

#include "NiKFMTool.inl"

#endif // #ifndef NIKFMTOOL_H
