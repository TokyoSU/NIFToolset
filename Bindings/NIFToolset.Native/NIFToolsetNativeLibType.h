#pragma once
#ifndef NIFTOOLSETNATIVELIBTYPE_H
#define NIFTOOLSETNATIVELIBTYPE_H

#if defined(_WIN32)
    #define NIFTOOLSET_NATIVE_CALL __cdecl

    #if defined(NIFTOOLSET_NATIVE_STATIC)
        #define NIFTOOLSET_NATIVE_ENTRY
    #elif defined(NIFTOOLSET_NATIVE_EXPORT)
        #define NIFTOOLSET_NATIVE_ENTRY __declspec(dllexport)
    #elif defined(NIFTOOLSET_NATIVE_IMPORT)
        #define NIFTOOLSET_NATIVE_ENTRY __declspec(dllimport)
    #else
        #define NIFTOOLSET_NATIVE_ENTRY
    #endif
#else
    #define NIFTOOLSET_NATIVE_CALL

    #if defined(NIFTOOLSET_NATIVE_EXPORT)
        #define NIFTOOLSET_NATIVE_ENTRY __attribute__((visibility("default")))
    #else
        #define NIFTOOLSET_NATIVE_ENTRY
    #endif
#endif

#if defined(_WIN32)
    #pragma warning(disable : 4251)
#endif

#endif // NIFTOOLSETNATIVELIBTYPE_H
