#pragma once
#include <NiRefObject.h>
#include <NiSmartPointer.h>
#include <NiEntityComponentInterface.h>
#include <NiTransformationComponent.h>
#include <NiPoint3.h>
#include <NiMatrix3.h>

NiSmartPointer(NiEntitySyncComponent);

/// Attach to any NiGeneralEntity to give it replicated network state.
/// On the client: receives server snapshots and interpolates.
/// On the server: broadcasts dirty state each tick.
class NiEntitySyncComponent : public NiRefObject,
    public NiEntityComponentInterface
{
public:
    static NiFixedString ms_kClassName;
    static NiFixedString ms_kComponentName;

    using EntityNetID = unsigned int;

    NiEntitySyncComponent();
    virtual ~NiEntitySyncComponent();

    // --- NiEntityComponentInterface ---
    virtual void Update(NiEntityPropertyInterface* pkParent,
        float fTime, NiEntityErrorInterface* pkErrors,
        NiExternalAssetManager* pkAssetManager) override;
    virtual void BuildVisibleSet(NiEntityRenderingContext*,
        NiEntityErrorInterface*) override { /* no geometry */ }
    virtual NiFixedString GetClassName() const override;
    virtual NiFixedString GetName()      const override;

    // --- Server ---
    void        SetNetID(EntityNetID uiID)  { m_uiNetID = uiID; }
    EntityNetID GetNetID()  const           { return m_uiNetID; }
    bool        IsDirty()   const           { return m_bDirty; }
    void        ClearDirty()                { m_bDirty = false; }
    void        MarkDirty()                 { m_bDirty = true; }

    // --- Client ---
    void PushSnapshot(float fServerTime, const NiPoint3& kPos,
                      const NiMatrix3& kRot, unsigned short usSequenceID);

    void SetInterpolationDelay(float fSeconds) { m_fDelay = fSeconds; }
    
	// *** begin Emergent internal use only ***

    static void _SDMInit();
	static void _SDMShutdown();

	// *** end Emergent internal use only ***

private:
    struct Snapshot
    {
        float          fTime;
        NiPoint3       kPos;
        NiMatrix3      kRot;
        unsigned short usSeqID;
    };

    // pkParent is passed through so SetPropertyData can route to the
    // correct sibling component (NiTransformationComponent, NiActorComponent)
    void InterpolateTowardsLatest(NiEntityPropertyInterface* pkParent,
                                  float fRenderTime);

    EntityNetID  m_uiNetID         = 0;
    bool         m_bDirty          = false;
    float        m_fDelay          = 0.1f;

    static constexpr unsigned int kSnapshotCount = 8;
    Snapshot     m_kSnapshots[kSnapshotCount];
    unsigned int m_uiSnapshotHead  = 0;
    unsigned int m_uiSnapshotCount = 0;
};