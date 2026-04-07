// GAMEBASE USA LLC PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Gamebase USA LLC and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2011 Gamebase USA LLC.
//      All Rights Reserved.
//
// Gamebase USA LLC, Research Triangle Park, NC 27709
// http://www.gamebryo.com

#pragma once
#ifndef NiLightPrePassLIBTYPE_H
#define NiLightPrePassLIBTYPE_H

#ifndef __SPU__
#ifdef NILIGHTPREPASS_EXPORT
    // DLL library project uses this
    #define NILIGHTPREPASS_ENTRY __declspec(dllexport)
#else
#ifdef NILIGHTPREPASS_IMPORT
    // client of DLL uses this
    #define NILIGHTPREPASS_ENTRY __declspec(dllimport)
#else
    // static library project uses this
    #define NILIGHTPREPASS_ENTRY
#endif
#endif
#else
#define NILIGHTPREPASS_ENTRY
#endif // __SPU__

// Disable warning C4251.  Template classes cannot be exported for the obvious
// reason that the code is not generated until an instance of the class is
// declared.  With this warning enabled, you get thousands of complaints about
// class data members that are of template type.  For example, a member such
// as 'NiTPrimitiveArray<AVObject*> m_array' generates the warning.
//
// When maintaining the DLL code itself, you might want to enable the warning
// to check for cases where you might have failed to put the NILIGHTPREPASS_ENTRY after
// the class keyword.  In particular, nested classes must have NILIGHTPREPASS_ENTRY.
// Also, friend functions must be tagged with NILIGHTPREPASS_ENTRY.

#if defined(WIN32) || defined(WIN64)
    #pragma warning(disable : 4251)
#endif

#endif
