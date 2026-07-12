#pragma once

#include <NiAVObject.h>

#include <cstdint>
#include <sstream>
#include <string>

// Assimp resolves bones and animation channels by node name, while Gamebryo
// stores direct AVObject pointers. Preserve valid NIF names and generate a
// deterministic name within this export run for unnamed objects so every
// reference to the same object resolves to the same Assimp node.
inline std::string GetExportNodeName(const NiAVObject* pkObject)
{
	if (pkObject)
	{
		const char* pcName = pkObject->GetName();
		if (pcName && pcName[0] != '\0')
			return pcName;
	}

	std::ostringstream kName;
	kName << "unnamed_node_" << std::hex
		<< reinterpret_cast<std::uintptr_t>(pkObject);
	return kName.str();
}
