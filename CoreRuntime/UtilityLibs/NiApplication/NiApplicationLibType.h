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
#ifndef NIAPPLICATIONLIBTYPE_H
#define NIAPPLICATIONLIBTYPE_H

#ifdef NIAPPLICATION_EXPORT
    // DLL library project uses this
    #define NIAPPLICATION_ENTRY __declspec(dllexport)
#else
#ifdef NIAPPLICATION_IMPORT
    // client of DLL uses this
    #define NIAPPLICATION_ENTRY __declspec(dllimport)
#else
    // static library project uses this
    #define NIAPPLICATION_ENTRY
#endif
#endif

// Suppress C4251 (std/template members inside exported classes).
#ifdef WIN32
    #pragma warning(disable : 4251)
#endif

#endif // NIAPPLICATIONLIBTYPE_H
