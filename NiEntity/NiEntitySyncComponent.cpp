#include "NiEntityPCH.h"
#include "NiEntitySyncComponent.h"
#include "NiEntityInterface.h"
#include <NiActorComponent.h>
#include <NiQuaternion.h>

//---------------------------------------------------------------------------
NiFixedString NiEntitySyncComponent::ms_kClassName;
NiFixedString NiEntitySyncComponent::ms_kComponentName;
//---------------------------------------------------------------------------
NiEntitySyncComponent::NiEntitySyncComponent() { /* */ }
NiEntitySyncComponent::~NiEntitySyncComponent() { /* */ }
//---------------------------------------------------------------------------
void NiEntitySyncComponent::PushSnapshot(float fServerTime,
    const NiPoint3& kPos, const NiMatrix3& kRot, unsigned short usSeqID)
{
    Snapshot& kSnap = m_kSnapshots[m_uiSnapshotHead % kSnapshotCount];
    kSnap.fTime   = fServerTime;
    kSnap.kPos    = kPos;
    kSnap.kRot    = kRot;
    kSnap.usSeqID = usSeqID;
    ++m_uiSnapshotHead;
    if (m_uiSnapshotCount < kSnapshotCount) ++m_uiSnapshotCount;
}
//---------------------------------------------------------------------------
void NiEntitySyncComponent::_SDMInit()
{
    ms_kClassName = "NiEntitySyncComponent";
    ms_kComponentName = "Sync";
}
//---------------------------------------------------------------------------
void NiEntitySyncComponent::_SDMShutdown()
{
    ms_kClassName = NULL;
    ms_kComponentName = NULL;
}
//---------------------------------------------------------------------------
void NiEntitySyncComponent::Update(NiEntityPropertyInterface* pkParent,
    float fTime, NiEntityErrorInterface*, NiExternalAssetManager*)
{
    if (m_uiSnapshotCount < 2 || !pkParent) return;

    InterpolateTowardsLatest(pkParent, fTime - m_fDelay);
}
//---------------------------------------------------------------------------
NiFixedString NiEntitySyncComponent::GetClassName() const
{
    return ms_kClassName;
}
//---------------------------------------------------------------------------
NiFixedString NiEntitySyncComponent::GetName() const
{
    return ms_kComponentName;
}
//---------------------------------------------------------------------------
void NiEntitySyncComponent::InterpolateTowardsLatest(
    NiEntityPropertyInterface* pkParent, float fRenderTime)
{
    // Find the two snapshots that bracket fRenderTime
    const Snapshot* pkA = nullptr;
    const Snapshot* pkB = nullptr;

    for (unsigned int i = 0; i + 1 < m_uiSnapshotCount; ++i)
    {
        unsigned int idxA = (m_uiSnapshotHead - m_uiSnapshotCount + i)
                            % kSnapshotCount;
        unsigned int idxB = (idxA + 1) % kSnapshotCount;
        if (m_kSnapshots[idxA].fTime <= fRenderTime &&
            fRenderTime <= m_kSnapshots[idxB].fTime)
        {
            pkA = &m_kSnapshots[idxA];
            pkB = &m_kSnapshots[idxB];
            break;
        }
    }

    if (!pkA || !pkB) return;

    float fRange = pkB->fTime - pkA->fTime;
    float t      = (fRange > 0.0f)
                 ? (fRenderTime - pkA->fTime) / fRange
                 : 1.0f;
    t = NiClamp(t, 0.0f, 1.0f);

    // --- Position: use public PROP_TRANSLATION() accessor ---
    const NiPoint3 kPos = NiLerp(t, pkA->kPos, pkB->kPos);
    pkParent->SetPropertyData(NiTransformationComponent::PROP_TRANSLATION(), kPos, 0);

    // --- Rotation: quaternion slerp, use public PROP_ROTATION() accessor ---
    NiQuaternion qA, qB;
    qA.FromRotation(pkA->kRot);
    qB.FromRotation(pkB->kRot);
    const NiQuaternion qOut = NiQuaternion::Slerp(t, qA, qB);
    NiMatrix3 kRot;
    qOut.ToRotation(kRot);
    pkParent->SetPropertyData(NiTransformationComponent::PROP_ROTATION(), kRot, 0);

    // --- Animation: ms_kActiveSequenceIDName is protected; find the
    //     NiActorComponent via the entity interface and call its public
    //     SetActiveSequenceID() directly instead. ---
    if (pkB->usSeqID != pkA->usSeqID)
    {
        // pkParent is always the parent entity in Update() — safe to cast
        NiEntityInterface* pkEntity = static_cast<NiEntityInterface*>(pkParent);
        for (unsigned int i = 0; i < pkEntity->GetComponentCount(); ++i)
        {
            NiEntityComponentInterface* pkComp = pkEntity->GetComponentAt(i);
            if (pkComp && pkComp->GetClassName() == "NiActorComponent")
            {
                static_cast<NiActorComponent*>(pkComp)->SetActiveSequenceID(pkB->usSeqID);
                break;
            }
        }
    }
}