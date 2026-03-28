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

// Disable warning C4251.  Template classes cannot be exported for the obvious
// reason that the code is not generated until an instance of the class is
// declared.  With this warning enabled, you get thousands of complaints about
// class data members that are of template type.  For example, a member such
// as 'NiTPrimitiveArray<NiAVObject*> m_array' generates the warning.
//
// When maintaining the DLL code itself, you might want to enable the warning
// to check for cases where you might have failed to put the
// NIAPPLICATION_ENTRY after the class keyword.  In particular, nested classes
// must have NIAPPLICATION_ENTRY.  Also, friend functions must be tagged with
// NIAPPLICATION_ENTRY.

#ifdef WIN32
#pragma warning( disable : 4251 )
#endif

#endif
