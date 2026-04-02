// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2010 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#ifndef NIDECORATIONLIBTYPE_H
#define NIDECORATIONLIBTYPE_H

#ifndef __SPU__

    #if defined(NIDECORATION_EXPORT)
        // building DLL library uses this
        #define NIDECORATION_ENTRY __declspec(dllexport)
    #elif defined(NIDECORATION_IMPORT)
        // something importing a DLL uses this
        #define NIDECORATION_ENTRY __declspec(dllimport)
    #else
        // static library project uses this
        #define NIDECORATION_ENTRY
    #endif
#else
#define NIDECORATION_ENTRY 
#endif // __SPU__

#endif
