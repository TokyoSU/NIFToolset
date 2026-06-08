#pragma once
#ifndef NIFTOOLSETNATIVELIBTYPE_H
#define NIFTOOLSETNATIVELIBTYPE_H

#ifdef NIFTOOLSET_NATIVE_EXPORT
	#define NIFTOOLSET_NATIVE_ENTRY __declspec(dllexport)
#else
#ifdef NIFTOOLSET_NATIVE_IMPORT
	#define NIFTOOLSET_NATIVE_ENTRY __declspec(dllimport)
#else
	#define NIFTOOLSET_NATIVE_ENTRY
#endif
#endif

#ifdef WIN32
	#pragma warning(disable : 4251)
#endif

#endif // NIFTOOLSETNATIVELIBTYPE_H
