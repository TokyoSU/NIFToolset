// Forwarding header — allows win32 subdirectory sources to use #include "efdPCH.h"
// without requiring Foundation/efd/efd as a bare include path (which shadows <string.h>
// with efd/String.h on case-insensitive Windows filesystems).
#pragma once
#include <efd/efdPCH.h>
