#include "AssetLoader.h"

#include <NiAnimationSDM.h>
#include <NiDevImageConverter.h>
#include <NiFilename.h>
#include <NiImageConverter.h>
#include <NiMainSDM.h>
#include <NiNode.h>
#include <NiPath.h>
#include <NiSystemSDM.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace
{
	namespace fs = std::filesystem;

	std::string MakeError(const std::string& kPrefix, const std::string& kPath)
	{
		std::ostringstream kStream;
		kStream << kPrefix << ": " << kPath;
		return kStream.str();
	}

	const char* GetNiStreamErrorName(unsigned int uiError)
	{
		switch (uiError)
		{
		case NiStream::STREAM_OKAY:
			return "STREAM_OKAY";
		case NiStream::FILE_NOT_LOADED:
			return "FILE_NOT_LOADED";
		case NiStream::NOT_NIF_FILE:
			return "NOT_NIF_FILE";
		case NiStream::OLDER_VERSION:
			return "OLDER_VERSION";
		case NiStream::LATER_VERSION:
			return "LATER_VERSION";
		case NiStream::NO_CREATE_FUNCTION:
			return "NO_CREATE_FUNCTION";
		case NiStream::ENDIAN_MISMATCH:
			return "ENDIAN_MISMATCH";
		default:
			return "UNKNOWN_ERROR";
		}
	}

	std::string ReadFileHeader(const std::string& kPath, std::size_t uiMaxBytes)
	{
		std::ifstream kFile(kPath, std::ios::binary);
		if (!kFile)
			return std::string();

		std::string kHeader(uiMaxBytes, '\0');
		kFile.read(&kHeader[0], static_cast<std::streamsize>(uiMaxBytes));
		kHeader.resize(static_cast<std::size_t>(kFile.gcount()));

		for (char& c : kHeader)
		{
			const unsigned char uc = static_cast<unsigned char>(c);
			if (uc < 32 && c != '\t')
				c = ' ';
		}

		while (!kHeader.empty() && kHeader.back() == ' ')
			kHeader.pop_back();

		return kHeader;
	}
}

RuntimeScope::RuntimeScope() :
	m_bInitialized(false)
{
	NiSystemSDM::Init();
	NiMainSDM::Init();
	NiAnimationSDM::Init();
	NiImageConverter::SetImageConverter(NiNew NiDevImageConverter());
	m_bInitialized = true;
}

RuntimeScope::~RuntimeScope()
{
	if (!m_bInitialized)
	{
		return;
	}

	NiAnimationSDM::Shutdown();
	NiMainSDM::Shutdown();
	NiSystemSDM::Shutdown();
}

AssetLoader::AssetLoader(const ExportOptions& kOptions) :
	m_kOptions(kOptions)
{
}

bool AssetLoader::ResolveInputs(std::vector<ResolvedInputAsset>& kResolvedAssets,
	std::string& kError) const
{
	kResolvedAssets.clear();

	for (const std::string& kInputPath : m_kOptions.inputPaths)
	{
		ResolvedInputAsset kResolvedAsset;
		if (!ResolveInput(kInputPath, kResolvedAsset, kError))
		{
			return false;
		}

		kResolvedAssets.push_back(kResolvedAsset);
	}

	return true;
}

bool AssetLoader::LoadNifAsset(const std::string& kPath, LoadedNifAsset& kAsset,
	std::string& kError) const
{
	kAsset.pStream = NiNew NiStream();
	if (!kAsset.pStream)
	{
		kError = MakeError("Failed to allocate NiStream for NIF", kPath);
		return false;
	}

	kAsset.pStream->ResetLastErrorInfo();
	if (!kAsset.pStream->Load(kPath.c_str()))
	{
		const unsigned int uiErrorCode = kAsset.pStream->GetLastError();
		const char* pcErrorMessage = kAsset.pStream->GetLastErrorMessage();
		const char* pcLastLoadedRTTI = kAsset.pStream->GetLastLoadedRTTI();

		std::ostringstream kStream;
		kStream << "Failed to load NIF: " << kPath
			<< "\n    NiStream error code: " << uiErrorCode
			<< " (" << GetNiStreamErrorName(uiErrorCode) << ")"
			<< "\n    NiStream message: "
			<< ((pcErrorMessage && pcErrorMessage[0] != '\0')
				? pcErrorMessage : "<no message>");

		if (uiErrorCode == NiStream::NO_CREATE_FUNCTION &&
			pcLastLoadedRTTI && pcLastLoadedRTTI[0] != '\0')
		{
			kStream << "\n    Last/unsupported RTTI: " << pcLastLoadedRTTI;
		}

		std::error_code kFsError;
		const fs::path kFilePath(kPath);
		const bool bExists = fs::exists(kFilePath, kFsError);
		kStream << "\n    File exists: " << (bExists ? "yes" : "no");

		if (!kFsError && bExists)
		{
			const std::uintmax_t uiFileSize = fs::file_size(kFilePath, kFsError);
			if (!kFsError)
				kStream << "\n    File size: " << uiFileSize << " bytes";

			const std::string kHeader = ReadFileHeader(kPath, 128);
			if (!kHeader.empty())
				kStream << "\n    File header: " << kHeader;
		}

		if (kFsError)
			kStream << "\n    Filesystem error: " << kFsError.message();

		kError = kStream.str();
		return false;
	}

	kAsset.roots.clear();
	for (unsigned int i = 0; i < kAsset.pStream->GetObjectCount(); ++i)
	{
		NiAVObject* pkObject = NiDynamicCast(NiAVObject,
			kAsset.pStream->GetObjectAt(i));
		if (pkObject)
			kAsset.roots.push_back(pkObject);
	}

	if (kAsset.roots.empty())
	{
		kError = MakeError("No AVObject root found in NIF", kPath);
		return false;
	}

	if (kAsset.roots.size() == 1)
	{
		kAsset.root = kAsset.roots.front();
	}
	else
	{
		// Legacy/default behavior: combine all top-level stream objects into one
		// identity root and write one FBX for the input NIF.
		NiNodePtr spCombinedRoot = NiNew NiNode();
		spCombinedRoot->SetName("NIFToolset_MultiRoot");
		for (NiAVObject* pkRoot : kAsset.roots)
			spCombinedRoot->AttachChild(pkRoot);
		kAsset.root = spCombinedRoot;
	}

	// Populate world transforms and inherited property/effect states before
	// extraction. Local transforms remain unchanged and are what FBX stores.
	if (kAsset.root)
		kAsset.root->Update(0.0f, false);

	return true;
}

bool AssetLoader::LoadKfmTool(const std::string& kPath, NiKFMToolPtr& spKfmTool,
	std::string& kError) const
{
	spKfmTool = NiNew NiKFMTool();
	if (spKfmTool->LoadFile(kPath.c_str()) != NiKFMTool::KFM_SUCCESS)
	{
		kError = MakeError("Failed to load KFM", kPath);
		spKfmTool = nullptr;
		return false;
	}

	return true;
}

bool AssetLoader::LoadKfSequences(const std::string& kPath,
	std::vector<NiSequenceDataPtr>& kSequences, std::string& kError) const
{
	NiSequenceDataPointerArray kSequenceArray;
	if (!NiSequenceData::CreateAllSequenceDatasFromFile(kPath.c_str(),
		kSequenceArray))
	{
		kError = MakeError("Failed to load KF", kPath);
		return false;
	}

	kSequences.clear();
	for (unsigned int uiIndex = 0; uiIndex < kSequenceArray.GetSize(); ++uiIndex)
	{
		if (kSequenceArray.GetAt(uiIndex))
		{
			kSequences.push_back(kSequenceArray.GetAt(uiIndex));
		}
	}

	return true;
}

bool AssetLoader::ResolveInput(const std::string& kInputPath,
	ResolvedInputAsset& kResolvedAsset, std::string& kError) const
{
	const std::string kNormalizedInput = NormalizePath(kInputPath);
	if (kNormalizedInput.empty())
	{
		kError = MakeError("Input path does not exist", kInputPath);
		return false;
	}

	fs::path kPath(kNormalizedInput);
	std::string kExtension = kPath.extension().string();
	std::transform(kExtension.begin(), kExtension.end(), kExtension.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	kResolvedAsset.inputPath = kInputPath;
	kResolvedAsset.normalizedInputPath = kNormalizedInput;
	kResolvedAsset.inputStem = kPath.stem().string();

	if (kExtension == ".kfm")
	{
		kResolvedAsset.kfmPath = kNormalizedInput;

		NiKFMToolPtr spKfmTool;
		if (!LoadKfmTool(kResolvedAsset.kfmPath, spKfmTool, kError))
		{
			return false;
		}

		kResolvedAsset.modelNifPath = NormalizePath(
			static_cast<const char*>(spKfmTool->GetFullModelPath()));
		GatherKfmSequenceFiles(*spKfmTool, kResolvedAsset.kfPaths);
	}
	else if (kExtension == ".nif")
	{
		kResolvedAsset.modelNifPath = kNormalizedInput;
		kResolvedAsset.kfmPath = FindSiblingWithExtension(kNormalizedInput,
			".kfm", m_kOptions.kfmFolder);

		if (!kResolvedAsset.kfmPath.empty())
		{
			NiKFMToolPtr spKfmTool;
			if (!LoadKfmTool(kResolvedAsset.kfmPath, spKfmTool, kError))
			{
				return false;
			}

			std::string kKfmModelPath = NormalizePath(
				static_cast<const char*>(spKfmTool->GetFullModelPath()));
			if (!kKfmModelPath.empty())
			{
				kResolvedAsset.modelNifPath = kKfmModelPath;
			}

			GatherKfmSequenceFiles(*spKfmTool, kResolvedAsset.kfPaths);
		}
		else
		{
			const fs::path kParentPath = kPath.parent_path();
			kResolvedAsset.kfPaths = FindMatchingKfFiles(kResolvedAsset.inputStem,
				m_kOptions.animFolder, kParentPath.string());
		}
	}
	else if (kExtension == ".kf")
	{
		kResolvedAsset.kfmPath = FindSiblingWithExtension(kNormalizedInput,
			".kfm", m_kOptions.kfmFolder);

		if (!kResolvedAsset.kfmPath.empty())
		{
			NiKFMToolPtr spKfmTool;
			if (!LoadKfmTool(kResolvedAsset.kfmPath, spKfmTool, kError))
			{
				return false;
			}

			kResolvedAsset.modelNifPath = NormalizePath(
				static_cast<const char*>(spKfmTool->GetFullModelPath()));
			GatherKfmSequenceFiles(*spKfmTool, kResolvedAsset.kfPaths);
		}
		else
		{
			kResolvedAsset.kfPaths.push_back(kNormalizedInput);
			kResolvedAsset.modelNifPath = FindSiblingWithExtension(
				kNormalizedInput, ".nif", m_kOptions.nifFolder);
		}
	}
	else
	{
		kError = MakeError("Unsupported input extension", kNormalizedInput);
		return false;
	}

	if (kResolvedAsset.modelNifPath.empty())
	{
		kError = MakeError("Unable to resolve model NIF for input",
			kNormalizedInput);
		return false;
	}

	return true;
}

std::string AssetLoader::NormalizePath(const std::string& kPath) const
{
	if (kPath.empty())
	{
		return std::string();
	}

	try
	{
		if (!fs::exists(kPath))
		{
			return std::string();
		}

		fs::path kCanonicalPath = fs::weakly_canonical(fs::path(kPath));
		return kCanonicalPath.make_preferred().string();
	}
	catch (...)
	{
		return std::string();
	}
}

std::string AssetLoader::FindSiblingWithExtension(const std::string& kPath,
	const char* pcExtension, const std::string& kSearchFolder) const
{
	const fs::path kInputPath(kPath);
	std::vector<fs::path> kSearchRoots;

	if (!kSearchFolder.empty())
	{
		kSearchRoots.push_back(fs::path(kSearchFolder));
	}

	if (kInputPath.has_parent_path())
	{
		kSearchRoots.push_back(kInputPath.parent_path());
	}

	for (const fs::path& kSearchRoot : kSearchRoots)
	{
		if (kSearchRoot.empty() || !fs::exists(kSearchRoot))
		{
			continue;
		}

		fs::path kCandidate = kSearchRoot / kInputPath.stem();
		kCandidate.replace_extension(pcExtension);
		std::string kResolved = NormalizePath(kCandidate.string());
		if (!kResolved.empty())
		{
			return kResolved;
		}
	}

	return std::string();
}

std::vector<std::string> AssetLoader::FindMatchingKfFiles(const std::string& kStem,
	const std::string& kPrimaryFolder, const std::string& kFallbackFolder) const
{
	std::vector<std::string> kResults;
	std::set<std::string> kSeenPaths;

	const fs::path kPrimaryPath(kPrimaryFolder);
	const fs::path kFallbackPath(kFallbackFolder);
	const fs::path kTargetName = fs::path(kStem).replace_extension(".kf");

	auto kCollectMatches = [&](const fs::path& kDirectory)
	{
		if (kDirectory.empty() || !fs::exists(kDirectory))
		{
			return;
		}

		for (const fs::directory_entry& kEntry : fs::directory_iterator(kDirectory))
		{
			if (!kEntry.is_regular_file())
			{
				continue;
			}

			fs::path kFilename = kEntry.path().filename();
			if (kFilename != kTargetName)
			{
				continue;
			}

			std::string kNormalized = NormalizePath(kEntry.path().string());
			if (!kNormalized.empty() && kSeenPaths.insert(kNormalized).second)
			{
				kResults.push_back(kNormalized);
			}
		}
	};

	kCollectMatches(kPrimaryPath);
	kCollectMatches(kFallbackPath);
	return kResults;
}

void AssetLoader::GatherKfmSequenceFiles(NiKFMTool& kKfmTool,
	std::vector<std::string>& kKfPaths) const
{
	kKfPaths.clear();

	NiKFMTool::SequenceID* puiSequenceIDs = nullptr;
	NiUInt32 uiSequenceCount = 0;
	kKfmTool.GetSequenceIDs(puiSequenceIDs, uiSequenceCount);

	std::set<std::string> kSeenPaths;
	for (NiUInt32 uiIndex = 0; uiIndex < uiSequenceCount; ++uiIndex)
	{
		const NiFixedString kFullKfPath = kKfmTool.GetFullKFFilename(
			puiSequenceIDs[uiIndex]);
		if (!kFullKfPath.Exists())
		{
			continue;
		}

		std::string kNormalizedPath = NormalizePath(
			static_cast<const char*>(kFullKfPath));
		if (!kNormalizedPath.empty() && kSeenPaths.insert(kNormalizedPath).second)
		{
			kKfPaths.push_back(kNormalizedPath);
		}
	}

	NiFree(puiSequenceIDs);
}
