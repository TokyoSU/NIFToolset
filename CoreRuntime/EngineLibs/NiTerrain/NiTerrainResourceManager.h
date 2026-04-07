// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2008 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net

#ifndef NITERRAINRESOURCEMANAGER_H
#define NITERRAINRESOURCEMANAGER_H

#include "NiTerrainLibType.h"
#include <NiSourceTexture.h>
#include <NiRenderedTexture.h>
#include <NiDataStream.h>
#include "NiTerrainEvents.h"

class NiTerrain;

/**
    This abstract class is used throughout the NiTerrain library to handle the creation of relevant 
    resources to store it's data in. Resources are tracked in a very basic way and if further 
    tracking is required then users are encouraged to derive their own resource managers.
*/
class NITERRAIN_ENTRY NiTerrainResourceManager: public NiRefObject
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:
    
    /**
        Defines the different texture type the manager can affect
    */
    struct TextureType
    {
        enum Value
        {
            /// Texture blend mask
            BLEND_MASK,
            /// Low detail diffuse texture
            LOWDETAIL_DIFFUSE,
            /// Low detail normal map
            LOWDETAIL_NORMAL,
            /// The terrain heighmap
            TERRAIN_HEIGHTMAP,
            /// The maximum number of texture type
            NUM_TEXTURE_TYPES
        };
    };

    /**
        Defines the different stream type the manager can affect
    */
    struct StreamType
    {
        enum Value
        {
            /// Position stream
            POSITION,
            /// Normal / tangent stream
            NORMAL_TANGENT,
            /// Texture coordinates stream
            TEXTURE_COORD,
            /// Index stream
            INDEX,
            /// Maximum static stream types
            NUM_STREAM_TYPES,
            /// Dynamic stream
            DYNAMIC = 0x80000000
        };
    };

    /**
        This listener provides a simple mechanism for tracking allocations and 
        deallocations within the terrain library.
    */
    class NITERRAIN_ENTRY Listener
    {
    public:
        /// TextureType enum
        typedef NiTerrainResourceManager::TextureType TextureType;
        /// StreamType enum
        typedef NiTerrainResourceManager::StreamType StreamType;

        /// Default constructor
        Listener();

        /// Destructor
        ~Listener();

        /// Event args for resource allocation or destruction events
        struct AllocationEventArgs
        {
            enum Type
            {
                TEXTURE_ALLOC,
                TEXTURE_RELEASE,
                STREAM_ALLOC,
                STREAM_RELEASE,
                BUFFER_ALLOC,
                BUFFER_RELEASE
            } m_eventType;
            
            /// Valid for texture alloc and release
            struct TextureArgs
            {
                TextureType::Value m_ePurpose;
                const NiTexture* m_pkTexture;
            };

            /// Valid for stream alloc ONLY
            struct StreamAllocArgs
            {
                StreamType::Value m_ePurpose;
                efd::UInt32 m_uiLOD;
                const NiDataStream* m_pkStream;
            };

            /// Valid for stream release ONLY
            struct StreamReleaseArgs
            {
                StreamType::Value m_ePurpose;
                const NiDataStream* m_pkStream;
            };

            /// Valid for buffer alloc ONLY
            struct BufferAllocArgs
            {
                efd::UInt32 m_uiBufferSize;
                void* m_pvBuffer;
            };

            /// Valid for buffer release ONLY
            struct BufferReleaseArgs
            {
                void* m_pvBuffer;
            };

            union
            {
                TextureArgs m_textureArgs;
                StreamAllocArgs m_streamAllocArgs;
                StreamReleaseArgs m_streamReleaseArgs;
                BufferAllocArgs m_bufferAllocArgs;
                BufferReleaseArgs m_bufferReleaseArgs;
            };
        };
        typedef NiTerrainEvent<NiTerrain, const AllocationEventArgs&> AllocationEvent;

        /// Event args for registration and de-registration events
        struct RegistrationEventArgs
        {
            enum Type
            {
                OBJECT_REGISTERED,
                OBJECT_DEREGISTERED
            } m_eventType;

            const NiRefObject* m_pkObject;
        };
        typedef NiTerrainEvent<NiTerrain, RegistrationEventArgs> RegistrationEvent;

        /// Report the allocation or deallocation of a resource
        AllocationEvent m_kAllocationEvent;

        /// Report the registration or destruction of a specific resource for tracking
        RegistrationEvent m_kRegistrationEvent;
    };

    /// Virtual destructor to allow derivation
    virtual ~NiTerrainResourceManager();

    /**
        Returns the total number of active objects that this allocator is 
        responsible for. If this value is greater than 0, then a terrain
        may not adjust it's configuration.
    */
    inline NiUInt32 GetNumActiveObjects();

    /**
        Returns the terrain object that owns this resource manager
    */
    inline NiTerrain* GetTerrain();
    
    
    /**
        Creates a texture appropriate for the owning terrain according to the type of texture.
    */
    virtual NiSourceTexture* CreateTexture(TextureType::Value eType, NiPixelData* pkPixelData);
    
    /**
        Creates a texture appropriate for the owning terrain according to the type of texture.
    */
    virtual NiRenderedTexture* CreateRenderedTexture(TextureType::Value eType);

    /**
        Release the use of the texture.
        NULL textures are ignored.
    */
    virtual void ReleaseTexture(TextureType::Value eType, NiTexture* pkTexture);

    /**
        Create a stream of the requested type.   
    */
    virtual NiDataStream* CreateStream(StreamType::Value eType, NiUInt32 uiLODLevel);

    /**
        Release the use of a stream. 
    */
    virtual void ReleaseStream(StreamType::Value eType, NiDataStream* pkStream);

    /**
        Allocate a buffer of a particular size
    */
    template<typename T> T* CreateBuffer(efd::UInt32 uiCount);

    /**
        Release a buffer of a particular size
    */
    virtual void ReleaseBuffer(void* pvBuffer);

    /**
        Release all the unused buffers that have been allocated by the manager
    */
    virtual void ReleaseAllUnusedBuffers();

    /**
        Set the buffer memory usage threshold. The resource manager will attempt to stay 
        below this threshold of allocated buffer memory whenever it can by pruning unused buffers
        from it's pool if it needs to. 

        It should be noted that this memory usage threshold only applies to 'Buffers' allocated
        in the resource manager, not textures, or streams. It should also be known that 
        this threshold may be breached if the terrain requires the use of more memory at any time. 
    */
    virtual void SetBufferMemoryUsageThreshold(efd::UInt64 uiThreshold);

    /**
        Set the listener object to listen to the workings of this manager
    */
    inline void SetListener(Listener* pkListener);

protected:

    /// @name Listener event modifiers
    //@{
    /**
        Reports a texture allocation to the listener.
    */
    inline void NotifyAllocTexture(TextureType::Value ePurpose, const NiTexture* pkTexture);
    
    /**
        Reports a texture release to the listener.
    */
    inline void NotifyReleaseTexture(TextureType::Value ePurpose, const NiTexture* pkTexture);
    
    /**
        Reports a stream allocation to the listener.
    */
    inline void NotifyAllocStream(StreamType::Value ePurpose, const NiDataStream* pkStream, 
        efd::UInt32 uiLODLevel);
    
    /**
        Reports a stream release to the listener.
    */
    inline void NotifyReleaseStream(StreamType::Value ePurpose, const NiDataStream* pkStream);
    
    /**
        Reports a buffer allocation to the listener.
    */
    inline void NotifyAllocBuffer(efd::UInt32 uiBufferSize, void* pvBuffer);
    
    /**
        Reports a buffer release to the listener.
    */
    inline void NotifyReleaseBuffer(void* pvBuffer);
    
    /**
        Reports the registration of an object to the listener.
    */
    inline void NotifyRegister(const NiRefObject* pkObject);
    
    /**
        Reports the deregistration of an object to the listener.
    */
    inline void NotifyDeregister(const NiRefObject* pkObject);
    //@}

    /**
        Standard constructor. Each allocator is assigned to a specific terrain
        object as different configurations will require different allocators
        to handle the different object parameters.
    */
    NiTerrainResourceManager(NiTerrain* pkOwner);

    /**
        Register a generic resource with the allocator. These functions manage
        the active object count, and also perform basic error checking to flag situations 
        involving multiple releases and objects that have already been destroyed.
    */
    inline void RegisterResource(NiRefObject* pkObject);

    /**
        Deregister a generic resource with the allocator. These functions manage
        the active object count, and also perform basic error checking to flag situations 
        involving multiple releases and objects that have already been destroyed.
    */
    inline void DeregisterResource(NiRefObject* pkObject);

    /**
        Allocate a buffer of bytes of a given size
    */
    virtual void* CreateBuffer(efd::UInt32 uiNumBytes);

    /// Mutex to protect the active object counts
    efd::CriticalSection m_kMutex;

private:

    /// The terrain that is using this allocator
    NiTerrain* m_pkTerrain;

    /// Total number of objects active that have been created by the allocator
    NiUInt32 m_uiActiveObjects;

    /// A smart pointer to a listener object that may listen to resource allocations
    Listener m_kListener;
};

/**
    This class represents the default standard resource manager to be used. It supports creation
    of all the relevant resources and pooling of regular buffers that the terrain uses. 
*/
class NITERRAIN_ENTRY NiTerrainStandardResourceManager: public NiTerrainResourceManager
{
    /// @cond EMERGENT_INTERNAL
    NiDeclareRTTI;
    /// @endcond

public:
    /// Constructor
    NiTerrainStandardResourceManager(NiTerrain* pkOwner);

    /// Destructor for error checking
    ~NiTerrainStandardResourceManager();

    /**
        Creates a blend mask texture appropriate for the owning terrain.
    */
    virtual NiSourceTexture* CreateTexture(TextureType::Value eType, 
        NiPixelData* pkPixelData);
    
    /**
        Creates a blend mask texture appropriate for the owning terrain.
    */
    virtual NiRenderedTexture* CreateRenderedTexture(TextureType::Value eType);

    /**
        Release the use of the blend textures
    */
    virtual void ReleaseTexture(TextureType::Value eType, 
        NiTexture* pkTexture);

    /**
        Create a stream of the requested type.
    */
    virtual NiDataStream* CreateStream(StreamType::Value eType, NiUInt32 uiLODLevel);

    /**
        Release the use of a stream. 
    */
    virtual void ReleaseStream(StreamType::Value eType, NiDataStream* pkStream);

    /**
        Release the use of a buffer. 
    */
    virtual void ReleaseBuffer(void* pvBuffer);

    /**
        Release the use of all unused buffers. 
    */
    virtual void ReleaseAllUnusedBuffers();

    /**
        Sets the buffer memory threshold.
    */
    virtual void SetBufferMemoryUsageThreshold(efd::UInt64 uiThreshold);

protected:

    /**
        A class used to describe a block of memory allocated in this manager. 
    */
    class BufferData : public NiRefObject
    {
    public:
        BufferData(efd::UInt32 uiSize);
        ~BufferData();

        /// Get the buffer to use for storing data
        inline void* GetBuffer();
        /// Get the size of the buffer
        inline efd::UInt32 GetSize();
        /// Fetch the buffer data object based on the buffer pointer
        static inline BufferData* FetchBufferData(void* pvBuffer);  

        //-- Least recently used list --
        /// Get the buffer that was more recently used
        BufferData* GetMoreRecentlyUsed();
        /// Get the buffer that was less recently used
        BufferData* GetLessRecentlyUsed();
        /// Set the more recently used buffer 
        void SetMoreRecentlyUsed(BufferData* pkNext);
        /// Remove this buffer from the least recently used list
        void BeginUsing();

        // -- Size linked list --
        /// Get the next free buffer after this one
        inline BufferData* GetNextFree();
        /// Set the next free buffer after this one
        inline void SetNextFree(BufferData* pkNext);

    private:

        /**
            This data is stored at the beginning of the allocated buffer but is hidden from
            the user, as the user is given a pointer to the memory after this structure. It allows
            the buffer data object to be fetched from the user's pointer immediately without 
            lookups.
        */
        struct InlineBufferData
        {
            BufferData* m_pkBufferData;
        };
        /// Pointer to the next buffer that is free
        BufferData* m_pkNextFree;
        /// Pointer to the next buffer that is recently used
        BufferData* m_pkMoreRecentlyUsed;
        /// Pointer to the next buffer that is recently used
        BufferData* m_pkLessRecentlyUsed;
        /// The size of the buffer
        efd::UInt32 m_uiSize;
        /// Pointer to the allocated buffer
        efd::UInt8* m_pucBuffer;
    };

    /// Calculate the format of the position stream for a particular LOD
    NiDataStreamElement::Format GetPositionStreamFormat(NiUInt32 uiLODLevel);
    /// Calculate the format of the normal stream for a particular LOD
    NiDataStreamElement::Format GetNormalStreamFormat(NiUInt32 uiLODLevel);
    /// Calculate the format of the tangent stream for a particular LOD
    NiDataStreamElement::Format GetTangentStreamFormat(NiUInt32 uiLODLevel);

    /// Create a buffer for use of a specific size
    virtual void* CreateBuffer(efd::UInt32 uiNumBytes);
    /// Clear the pool of buffers stored in the manager
    void EmptyBufferPool();
    /// Enforce the buffer usage threshold if possible
    void EnforceBufferUsage();

    
    void BeginUsingBuffer(BufferData* pkBuffer);
    void FinishUsingBuffer(BufferData* pkBuffer);

    // Free buffer pool
    typedef efd::map<efd::UInt32, BufferData*> FreePoolType;
    FreePoolType m_kFreeBufferPool;
    /// Least recently used buffer list
    BufferData* m_pkLeastRecentlyUsedBuffer;
    /// Most recently used buffer list
    BufferData* m_pkMostRecentlyUsedBuffer;
    /// The threshold of buffers to pool up to
    efd::UInt64 m_uiBufferUsageThreshold;
    /// The number of bytes allocated for buffers in the pools
    efd::UInt64 m_uiBufferUsage;

    /// Texture formats for each type of texture
    NiTexture::FormatPrefs m_akTextureFormats[TextureType::NUM_TEXTURE_TYPES];
    bool m_abTextureIsStatic[TextureType::NUM_TEXTURE_TYPES];
};

NiSmartPointer(NiTerrainResourceManager);

#include "NiTerrainResourceManager.inl"

#endif
