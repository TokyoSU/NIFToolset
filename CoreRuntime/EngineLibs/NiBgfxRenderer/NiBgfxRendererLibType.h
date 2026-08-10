#pragma once
#ifndef NIBGFXRENDERERLIBTYPE_H
#define NIBGFXRENDERERLIBTYPE_H

#if defined(_WIN32)
#   if defined(NIBGFXRENDERER_EXPORT)
#       define NIBGFXRENDERER_ENTRY __declspec(dllexport)
#   elif defined(NIBGFXRENDERER_IMPORT)
#       define NIBGFXRENDERER_ENTRY __declspec(dllimport)
#   else
#       define NIBGFXRENDERER_ENTRY
#   endif
#else
#   define NIBGFXRENDERER_ENTRY
#endif

#endif
