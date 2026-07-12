#pragma once

#include "ExportOptions.h"

#include <NiAVObject.h>
#include <NiKFMTool.h>
#include <NiSequenceData.h>
#include <NiStream.h>

#include <string>
#include <vector>

struct ResolvedInputAsset
{
	std::string inputPath;
	std::string normalizedInputPath;
	std::string inputStem;
	std::string modelNifPath;
	std::string kfmPath;
	std::vector<std::string> kfPaths;
};

struct LoadedNifAsset
{
	NiStream* pStream = nullptr;
	NiAVObjectPtr root;

	~LoadedNifAsset()
	{
		if (pStream)
		{
			NiDelete pStream;
			pStream = nullptr;
		}
	}
};

class RuntimeScope
{
public:
	RuntimeScope();
	~RuntimeScope();

private:
	bool m_bInitialized;
};

class AssetLoader
{
public:
	explicit AssetLoader(const ExportOptions& kOptions);

	bool ResolveInputs(std::vector<ResolvedInputAsset>& kResolvedAssets,
		std::string& kError) const;
	bool LoadNifAsset(const std::string& kPath, LoadedNifAsset& kAsset,
		std::string& kError) const;
	bool LoadKfmTool(const std::string& kPath, NiKFMToolPtr& spKfmTool,
		std::string& kError) const;
	bool LoadKfSequences(const std::string& kPath,
		std::vector<NiSequenceDataPtr>& kSequences, std::string& kError) const;

private:
	bool ResolveInput(const std::string& kInputPath,
		ResolvedInputAsset& kResolvedAsset, std::string& kError) const;
	std::string NormalizePath(const std::string& kPath) const;
	std::string FindSiblingWithExtension(const std::string& kPath,
		const char* pcExtension, const std::string& kSearchFolder) const;
	std::vector<std::string> FindMatchingKfFiles(const std::string& kStem,
		const std::string& kPrimaryFolder, const std::string& kFallbackFolder) const;
	void GatherKfmSequenceFiles(NiKFMTool& kKfmTool,
		std::vector<std::string>& kKfPaths) const;

	const ExportOptions& m_kOptions;
};
