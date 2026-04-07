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
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#ifndef NIDECORATIONTRANSFORMMANAGER_H
#define NIDECORATIONTRANSFORMMANAGER_H

#include "NiDecorationLibType.h"
#include "NiDecorationCell.h"
#include "NiDecorationMeshInfo.h"

#include <NiMesh.h>

/// Result of a request to generate transforms
enum CellRequestGenerationResult 
{
    /// Valid transforms were generated and require uploading
    GR_GENERATED_VALID_TRANSFORMS,
    /// Invalid transforms were generated and require uploading
    GR_GENERATED_INVALID_TRANSFORMS,
    /// Invalid transforms already exist so do not require re-uploading
    GR_EXISTING_INVALID_TRANSFORMS,
    /// No transforms were generated, state of the stream is unknown so does not require uploading
    GR_EXISTING_UNKNOWN_TRANSFORMS
};

class NIDECORATION_ENTRY NiDecorationTransformProcessor : public NiMemObject
{
public:

    /// Default constructor
    NiDecorationTransformProcessor();

    /// Virtual Destructor
    virtual ~NiDecorationTransformProcessor();

    /**
        Called when the given cell needs its transforms generated
     */ 
    virtual CellRequestGenerationResult ProcessCellTransforms(NiDecorationCell* pkCell, 
        NiTransform* pkTransformStream,
        NiUInt32 uiTransformCount,
        bool bWriteInvalidTransformOnFail) = 0;

    static bool RequiresUpload(CellRequestGenerationResult eGenerationResult);
};

/**
    Manages a set of 'regions' for the given transform stream, quantized into 'cells' of a given 
    number of transforms, such that there is no internal or external fragmentation of the underlying
    shared transform stream. Thus, the regions themselves are packed tightly to each other, with all
    cells within a region packed tightly.
    
    The result is that each region (identified by a 'Region ID') has a contiguous set of transforms
    that represents all of its visible cells.

    The combined number of visible cells across all regions can never exceed the maximum visible 
    cell count for the transform manager.

    Requests for the release of cells for a specific region are processed in bulk during a call to
    ProcessRequests. 
    
    Requests for the creation of a cell is process latently; it can only be fulfilled when there is
    sufficient room in the underlying stream. Due to the constraint of keeping the underlying
    stream contigious, there may be a delay of a few calls to ProcessRequests before the creation is
    fulfilled.

    If there are more creation requests than there are free spots in the stream, they will remain
    pending until a free slot is available.
 */
class NIDECORATION_ENTRY NiDecorationTransformManager : public NiRefObject
{
public:

    /// List of NiDecorationCell pointers
    typedef efd::list<NiDecorationCell*> CellList;

    /// Set of NiDecorationCell pointers
    typedef efd::set<NiDecorationCell*> CellSet;

    /// Vector of NiDecorationCell pointers
    typedef efd::vector<NiDecorationCell*> CellVector;

    /// Identifier for a cell region
    typedef NiDecorationMeshInfo* RegionID;

    /// List of RegionID's
    typedef efd::list<RegionID> RegionIDList;

protected:
    struct CellIndex
    {
        /// Parameterized Constructor
        inline CellIndex(NiUInt32 uiX, NiUInt32 uiY);

        NiUInt32 m_uiX;
        NiUInt32 m_uiY;
    };

    typedef efd::list<CellIndex> CellIndexList;

    struct NIDECORATION_ENTRY Region
    {
        // Default constructor
        Region();

        // Destructor
        ~Region();

        CellList m_kReleasedCells;
        CellIndexList m_kRequestedCells;

        // Offset CELL in the transform stream
        NiUInt32 m_start;

        // Number of CELLS in the transform stream
        NiUInt32 m_range;

        // Index of the region within the data stream
        NiUInt32 m_uiDataStreamRegionIndex;

        NiDecorationTransformProcessor* m_pkProcessor;
    };

public:

    /// Default constructor
    NiDecorationTransformManager();

    /**
        Parameterized constructor

        @param pkInstanceStream Mesh data stream that transform manager will populate
        @param uiMaxNumCells Maximum number of cells that can be visible at once
        @param uiInstancesPerCell Number of instance transforms per cell
     */
    NiDecorationTransformManager(NiDataStream* pkInstanceStream, NiUInt32 uiMaxNumCells,
        NiUInt32 uiInstancesPerCell);

    /// Virtual Destructor
    virtual ~NiDecorationTransformManager();

    /**
        Sets the stream that the manager will take care of. The manager must be in a 'reset' state;
        the visible cell count must equal zero.

        @param pkInstanceStream Mesh data stream that transform manager will populate
        @param uiMaxNumCells Maximum number of cells that can be visible at once
        @param uiInstancesPerCell Number of instance transforms per cell
     */
    virtual void SetInstanceStream(NiDataStream* pkInstanceStream, NiUInt32 uiMaxNumCells,
        NiUInt32 uiInstancesPerCell);

    /**
        Registers a new region under the given ID. The given processor will be used to request 
        calculation of transforms for any visible cells.

        @note The transform manager will take ownership of the given processor, deleting it when
            the RegionID is released.
     */
    virtual void CreateInstanceRegion(RegionID kRegionID, NiDecorationTransformProcessor* pkProcessor);

    /**
        Releases the region with the given ID. A region can only be released if it is empty: that is 
        there are no visible cells.
     */
    virtual void ReleaseInstanceRegion(RegionID kRegionID);

    /**
        @return List of the ID's of all registered instance regions.
     */
    inline const RegionIDList& GetInstanceRegions() const;

    /// @return The maximum number of cells that can be visible at any one time, across all regions.
    inline NiUInt32 GetNumMaxCells() const;

    /// @return The number of instance transforms per cell.
    inline NiUInt32 GetInstancesPerCell() const;

    /// @return The CPU read/writable NiTransform stream
    inline NiTransform* GetTransformStream();

    /// @return The number of transforms contained in the transform stream
    inline NiUInt32 GetTransformCount();

    /**
        Request that a cell is allocated to the instance region corresponding to the given regionID. 
        The cell index has no impact on the cell allocation, it is passed to the processor once the
        cell has been created.
        
        @note The cell may not be created until a few calls to ProcessRequests have passed.

        @param kRegionID The identifier of the region to which the requested cell will be given
        @param uiCellIndexX X-Coordinate of the requested cell
        @param uiCellIndexY Y-Coordinate of the requested cell
     */
    void RequestCells(RegionID kRegionID, NiUInt32 uiCellIndexX, NiUInt32 uiCellIndexY);

    /**
        Request that the given cell (belonging to the given RegionID) is released. The cell will
        not be released until a call to ProcessRequests is made.

        @param kRegionID The identifier of the region containing the cell to be released.
        @param pkCell The cell which is to be released.
     */
    void ReleaseCell(RegionID kRegionID, NiDecorationCell* pkCell);

    /**
        Mark all cells for release and cancel any pending cell creation requests within the given 
        region.

        @param kRegionID Identifier of the region to clear.
     */
    void ClearRegion(RegionID kRegionID);

    /**
        @return The number of cells that are ready for rendering within the given region.
     */
    inline NiUInt32 GetVisibleCellCount(RegionID kRegionID);

    /**
        Get the index of the first quantization cell that belongs to the given region. 

        @return true if the given region has at least one cell, false otherwise.        
     */
    inline bool GetFirstCellIndex(RegionID kRegionID, NiUInt32& uiFirstCellIndex);

    // processing
    void ProcessRequests();

    // uploading
    inline const CellSet& GetChangedCells() const;
    inline void MarkAllCellsUploaded();

    NiTransform* LockStream(NiDataStream::LockType lockType);
    void Unlock(NiDataStream::LockType lockType);

protected:

    /// Debugging helpers to make sure we are all working ok.
    //@{
    inline void AssertCellConsistency();
    inline void AssertCellDebugTransforms(NiDecorationCell* pkCell);
    void DoAssertCellConsistency();
    void DoAssertCellDebugTransforms(NiDecorationCell* pkCell);
    //@}

    /// Helper to lock only a specific region
    NiTransform* LockRegion(Region& kRegion, NiDataStream::LockType lockType);

    /// Safely retrieve a region struct.
    Region& GetRegion(RegionID kRegionID);

    /// Safely retrieve mesh info object from a region identifier
    NiDecorationMeshInfo* GetMeshInfo(RegionID kRegion);

    typedef efd::map<RegionID, Region> RegionIdToStreamRegionMap;
    RegionIdToStreamRegionMap m_kStreamRegions;
    RegionIDList m_kRegions;

    /// List of pre-allocated NiDecorationCell objects that are not in use
    CellList m_kUnusedCells;

    /// In-order array of all cells that have been allocated.
    CellVector m_kCells;

    /// Set of cells which need to be uploaded.
    CellSet m_kCellsToUpload;

    NiUInt32 m_uiMaxCellCount;
    NiUInt32 m_uiInstancesPerCell;

    NiUInt32 m_uiTransformCount;
    NiTransform* m_pkTransforms;
    NiDataStream* m_pkInstanceStream;

};

#include "NiDecorationTransformManager.inl"

NiSmartPointer(NiDecorationTransformManager);

#endif // NIDECORATIONTRANSFORMMANAGER_H