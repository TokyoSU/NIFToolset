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
#ifndef NITERRAINSTORAGEPOLICY_H
#define NITERRAINSTORAGEPOLICY_H

#include "NiTerrainLibType.h"
#include "NiTerrainAssetReference.h"
#include <NiRefObject.h>
#include <NiRTTI.h>

/**
    A base class from which to derive all storage policies from in the terrain library.
    A terrain storage policy provides a common interface through which all terrain file operations
    pass through, including open and close file events. Subclasses of storage policies are also
    used to apply different file formats to the various data sources of the terrain library.
*/
class NITERRAIN_ENTRY NiTerrainStoragePolicy : public NiRefObject
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRootRTTI(NiTerrainStoragePolicy);
    /// @endcond

public:
    /// Default constructor.
    NiTerrainStoragePolicy();

    /// Destructor.
    virtual ~NiTerrainStoragePolicy();

    /**
        Send a request to the stored asset resolver object to resolve the reference given. 
        Note: This is an asynchronous operation, to be notified when the reference has been 
        processed, attach a listener to pkReference.

        @param pkReference The asset reference to request the resolution of
    */
    void ResolveAssetReference(NiTerrainAssetReference* pkReference);

    /**
        Assign a specific asset resolver to this storage policy.

        @param pkResolver The asset resolver to use when resolving references through this
            interface.
    */
    void SetAssetResolver(NiTerrainAssetResolverBase* pkResolver);

    /**
        Get the asset resolver responsible for resolving asset references on this interface.

        @return The active asset resolve on the policy.
    */
    NiTerrainAssetResolverBase* GetAssetResolver();

    /**
        An enumeration structure to expose a tri-state IO success code. This code is
        used in the closed event to identify if the file operation was a success or not. 
    */
    struct IOSuccessCode
    {
        enum Value
        {
            /// No success code was assigned to the operation (defect condition)
            UNKNOWN,
            /// The file operation failed
            FAIL,
            /// The file operation succeeded
            SUCCESS
        };
    };

    /**
        A structure used to store the set of arguments involved when the FileOpening event is 
        raised. It contains both the filename being opened, and if the file is being opened with
        write access. 
    */
    struct OpeningEventArgs
    {
        /**
            Constructor

            @param kFilename The filename of the file being opened
            @param bWrite True if the file is being opened for write access.
        */
        OpeningEventArgs(efd::utf8string kFilename, bool bWrite);

        /// The filename of the file being opened.
        efd::utf8string m_kFilename;
        /// Is the file being opened for write access.
        bool m_bWriteAccess;
    };

    /**
        A structure used to store the set of arguments involved when the FileClosed event is 
        raised. It contains both the filename being closed, and if the file operation was a 
        success or not.
    */
    struct ClosedEventArgs
    {
        /**
            Constructor

            @param kFilename The filename of the file being closed
            @param eSuccess SUCCESS if the file operation was completed successfully.
        */
        ClosedEventArgs(efd::utf8string kFilename, IOSuccessCode::Value eSuccess);

        /// The filename of the file being opened.
        efd::utf8string m_kFilename;
        /// Was the file operation a success?
        IOSuccessCode::Value m_eSuccess;
    };
    
    /// Expose the event attach/detach listener functions for the FileOpening event
    EE_TERRAINEVENT_EXPOSEEVENT_ATTACH(Opening, m_kOpeningEvent);
    /// Expose the event attach/detach listener functions for the FileClosed event
    EE_TERRAINEVENT_EXPOSEEVENT_ATTACH(Closed, m_kClosedEvent);
    /// Expose the event raise functions for the FileOpening event
    EE_TERRAINEVENT_EXPOSEEVENT_RAISE(Opening, 
        m_kOpeningEvent, 
        NiTerrainStoragePolicy, 
        OpeningEventArgs);
    /// Expose the event raise functions for the FileClosed event
    EE_TERRAINEVENT_EXPOSEEVENT_RAISE(Closed, 
        m_kClosedEvent, 
        NiTerrainStoragePolicy, 
        ClosedEventArgs);

protected:

    /// Storage for the asset resolver object used to resolve AssetID's into paths
    NiTerrainAssetResolverBasePtr m_spAssetResolver;    
    /// Opening event
    NiTerrainEvent<NiTerrainStoragePolicy, OpeningEventArgs> m_kOpeningEvent;
    /// Closing event
    NiTerrainEvent<NiTerrainStoragePolicy, ClosedEventArgs> m_kClosedEvent;
};

#endif // NITERRAINSTORAGEPOLICY_H
