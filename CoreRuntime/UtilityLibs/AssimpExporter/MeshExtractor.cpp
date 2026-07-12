#include "MeshExtractor.h"
#include "ExportNaming.h"

#include <NiNode.h>
#include <NiMesh.h>
#include <NiGeometry.h>
#include <NiTriShape.h>
#include <NiTriStrips.h>
#include <NiTriShapeData.h>
#include <NiTriStripsData.h>
#include <NiTriBasedGeomData.h>
#include <NiSkinInstance.h>
#include <NiSkinData.h>
#include <NiSkinPartition.h>
#include <NiSkinningMeshModifier.h>
#include <NiTexturingProperty.h>
#include <NiSourceTexture.h>
#include <NiDataStreamElementLock.h>
#include <NiCommonSemantics.h>
#include <NiFloat16.h>

#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	using Index4 = std::array<unsigned int, 4>;
	using Weight4 = std::array<float, 4>;

	struct Float2
	{
		float v[2];
	};

	struct Float3
	{
		float v[3];
	};

	struct Float4
	{
		float v[4];
	};

	struct Half2
	{
		NiFloat16 v[2];
	};

	struct Half3
	{
		NiFloat16 v[3];
	};

	struct Half4
	{
		NiFloat16 v[4];
	};

	struct UInt8x4
	{
		NiUInt8 v[4];
	};

	struct Int8x4
	{
		NiInt8 v[4];
	};

	struct UInt16x4
	{
		NiUInt16 v[4];
	};

	struct Int16x4
	{
		NiInt16 v[4];
	};

	//--------------------------------------------------------------------------------------------------
	bool ReadVec3Stream(NiMesh* pkMesh, const NiFixedString& kSemantic,
		unsigned int uiSubmesh, std::vector<aiVector3D>& kOut)
	{
		kOut.clear();

		NiDataStreamElementLock kLock(pkMesh, kSemantic, 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const NiUInt32 uiCount = kLock.count(uiSubmesh);
		kOut.reserve(uiCount);

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_FLOAT32_3:
		{
			auto it = kLock.begin<Float3>(uiSubmesh);
			auto itEnd = kLock.end<Float3>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back(it->v[0], it->v[1], it->v[2]);
			break;
		}
		case NiDataStreamElement::F_FLOAT32_4:
		{
			auto it = kLock.begin<Float4>(uiSubmesh);
			auto itEnd = kLock.end<Float4>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back(it->v[0], it->v[1], it->v[2]);
			break;
		}
		case NiDataStreamElement::F_FLOAT16_3:
		{
			auto it = kLock.begin<Half3>(uiSubmesh);
			auto itEnd = kLock.end<Half3>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back((float)it->v[0], (float)it->v[1], (float)it->v[2]);
			break;
		}
		case NiDataStreamElement::F_FLOAT16_4:
		{
			auto it = kLock.begin<Half4>(uiSubmesh);
			auto itEnd = kLock.end<Half4>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back((float)it->v[0], (float)it->v[1], (float)it->v[2]);
			break;
		}
		default:
			return false;
		}

		return kOut.size() == uiCount;
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadVec2Stream(NiMesh* pkMesh, const NiFixedString& kSemantic,
		unsigned int uiSubmesh, std::vector<aiVector2D>& kOut)
	{
		kOut.clear();

		NiDataStreamElementLock kLock(pkMesh, kSemantic, 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const NiUInt32 uiCount = kLock.count(uiSubmesh);
		kOut.reserve(uiCount);

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_FLOAT32_2:
		{
			auto it = kLock.begin<Float2>(uiSubmesh);
			auto itEnd = kLock.end<Float2>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back(it->v[0], it->v[1]);
			break;
		}
		case NiDataStreamElement::F_FLOAT16_2:
		{
			auto it = kLock.begin<Half2>(uiSubmesh);
			auto itEnd = kLock.end<Half2>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.emplace_back((float)it->v[0], (float)it->v[1]);
			break;
		}
		default:
			return false;
		}

		return kOut.size() == uiCount;
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadBlendIndices(NiMesh* pkMesh, unsigned int uiSubmesh,
		unsigned int uiVertexCount, std::vector<Index4>& kOut)
	{
		kOut.assign(uiVertexCount, Index4{0, 0, 0, 0});

		NiDataStreamElementLock kLock(pkMesh, NiCommonSemantics::BLENDINDICES(), 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const unsigned int uiCount = std::min<unsigned int>(
			uiVertexCount, kLock.count(uiSubmesh));

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_UINT8_4:
		case NiDataStreamElement::F_NORMUINT8_4:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k];
			return true;
		}
		case NiDataStreamElement::F_NORMUINT8_4_BGRA:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = it->v[2];
				kOut[i][1] = it->v[1];
				kOut[i][2] = it->v[0];
				kOut[i][3] = it->v[3];
			}
			return true;
		}
		case NiDataStreamElement::F_INT8_4:
		case NiDataStreamElement::F_NORMINT8_4:
		{
			auto it = kLock.begin<Int8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = static_cast<unsigned int>(std::max<int>(0, it->v[k]));
			return true;
		}
		case NiDataStreamElement::F_UINT16_4:
		{
			auto it = kLock.begin<UInt16x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k];
			return true;
		}
		case NiDataStreamElement::F_INT16_4:
		{
			auto it = kLock.begin<Int16x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = static_cast<unsigned int>(std::max<int>(0, it->v[k]));
			return true;
		}
		default:
			return false;
		}
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadBlendWeights(NiMesh* pkMesh, unsigned int uiSubmesh,
		unsigned int uiVertexCount, std::vector<Weight4>& kOut)
	{
		kOut.assign(uiVertexCount, Weight4{0.0f, 0.0f, 0.0f, 0.0f});

		NiDataStreamElementLock kLock(pkMesh, NiCommonSemantics::BLENDWEIGHT(), 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const unsigned int uiCount = std::min<unsigned int>(
			uiVertexCount, kLock.count(uiSubmesh));

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_FLOAT32_3:
		{
			auto it = kLock.begin<Float3>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = it->v[0];
				kOut[i][1] = it->v[1];
				kOut[i][2] = it->v[2];
				kOut[i][3] = std::max(0.0f, 1.0f - it->v[0] - it->v[1] - it->v[2]);
			}
			break;
		}
		case NiDataStreamElement::F_FLOAT32_4:
		{
			auto it = kLock.begin<Float4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k];
			break;
		}
		case NiDataStreamElement::F_FLOAT16_3:
		{
			auto it = kLock.begin<Half3>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = (float)it->v[0];
				kOut[i][1] = (float)it->v[1];
				kOut[i][2] = (float)it->v[2];
				kOut[i][3] = std::max(0.0f,
					1.0f - kOut[i][0] - kOut[i][1] - kOut[i][2]);
			}
			break;
		}
		case NiDataStreamElement::F_FLOAT16_4:
		{
			auto it = kLock.begin<Half4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = (float)it->v[k];
			break;
		}
		case NiDataStreamElement::F_NORMUINT8_4:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k] / 255.0f;
			break;
		}
		case NiDataStreamElement::F_NORMUINT8_4_BGRA:
		{
			auto it = kLock.begin<UInt8x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
			{
				kOut[i][0] = it->v[2] / 255.0f;
				kOut[i][1] = it->v[1] / 255.0f;
				kOut[i][2] = it->v[0] / 255.0f;
				kOut[i][3] = it->v[3] / 255.0f;
			}
			break;
		}
		case NiDataStreamElement::F_NORMUINT16_4:
		{
			auto it = kLock.begin<UInt16x4>(uiSubmesh);
			for (unsigned int i = 0; i < uiCount; ++i, ++it)
				for (unsigned int k = 0; k < 4; ++k)
					kOut[i][k] = it->v[k] / 65535.0f;
			break;
		}
		default:
			return false;
		}

		for (Weight4& kWeights : kOut)
		{
			float fTotal = 0.0f;
			for (float& fWeight : kWeights)
			{
				fWeight = std::max(0.0f, fWeight);
				fTotal += fWeight;
			}

			if (fTotal > 0.000001f)
			{
				const float fInvTotal = 1.0f / fTotal;
				for (float& fWeight : kWeights)
					fWeight *= fInvTotal;
			}
		}

		return true;
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadBonePalette(NiMesh* pkMesh, unsigned int uiSubmesh,
		std::vector<unsigned int>& kOut)
	{
		kOut.clear();

		NiDataStreamElementLock kLock(pkMesh, NiCommonSemantics::BONE_PALETTE(), 0,
			NiDataStreamElement::F_UNKNOWN, NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);
		if (!kLock.IsLocked() || uiSubmesh >= kLock.GetSubmeshCount())
			return false;

		const unsigned int uiCount = kLock.count(uiSubmesh);
		kOut.reserve(uiCount);

		switch (kLock.GetDataStreamElement().GetFormat())
		{
		case NiDataStreamElement::F_UINT8_1:
		{
			auto it = kLock.begin<NiUInt8>(uiSubmesh);
			auto itEnd = kLock.end<NiUInt8>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.push_back(*it);
			break;
		}
		case NiDataStreamElement::F_UINT16_1:
		{
			auto it = kLock.begin<NiUInt16>(uiSubmesh);
			auto itEnd = kLock.end<NiUInt16>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.push_back(*it);
			break;
		}
		case NiDataStreamElement::F_UINT32_1:
		{
			auto it = kLock.begin<NiUInt32>(uiSubmesh);
			auto itEnd = kLock.end<NiUInt32>(uiSubmesh);
			for (; it != itEnd; ++it)
				kOut.push_back(*it);
			break;
		}
		default:
			return false;
		}

		return !kOut.empty();
	}

	//--------------------------------------------------------------------------------------------------
	bool ReadTriangleIndices(NiMesh* pkMesh, unsigned int uiSubmesh,
		unsigned int uiVertexCount, std::vector<unsigned int>& kOut)
	{
		kOut.clear();
		if (!pkMesh || uiVertexCount == 0)
			return false;

		std::vector<unsigned int> kRawIndices;
		unsigned int uiRestartIndex = 0xFFFFFFFFu;

		NiDataStreamElementLock kIndexLock(pkMesh, NiCommonSemantics::INDEX(), 0,
			NiDataStreamElement::F_UNKNOWN,
			NiDataStream::LOCK_READ | NiDataStream::LOCK_TOOL_READ);

		if (kIndexLock.IsLocked())
		{
			if (uiSubmesh >= kIndexLock.GetSubmeshCount())
				return false;

			NiDataStreamRef* pkIndexRef = kIndexLock.GetDataStreamRef();
			NiDataStream* pkIndexStream = kIndexLock.GetDataStream();
			if (!pkIndexRef || !pkIndexStream)
				return false;

			// NiDataStreamElementLock reports the mesh submesh count, not the
			// number of mappings actually stored on this particular stream ref.
			// Validate the remap before calling GetRegionForSubmesh()/count().
			if (uiSubmesh >= pkIndexRef->GetSubmeshRemapCount())
			{
				std::cerr << "    Submesh " << uiSubmesh
					<< " has no index-stream region mapping." << std::endl;
				return false;
			}

			const unsigned int uiRegionIndex =
				pkIndexRef->GetRegionIndexForSubmesh(uiSubmesh);
			if (uiRegionIndex >= pkIndexStream->GetRegionCount())
			{
				std::cerr << "    Submesh " << uiSubmesh
					<< " maps to invalid index region " << uiRegionIndex
					<< " (region count=" << pkIndexStream->GetRegionCount()
					<< ")." << std::endl;
				return false;
			}

			const NiDataStreamElement& kElement =
				kIndexLock.GetDataStreamElement();
			const NiDataStream::Region& kRegion =
				pkIndexStream->GetRegion(uiRegionIndex);

			const unsigned int uiIndexCount = kRegion.GetRange();
			const unsigned int uiStride = pkIndexRef->GetStride();
			const unsigned int uiElementOffset = kElement.GetOffset();
			const size_t stElementSize = kElement.SizeOf();
			const size_t stStreamSize = pkIndexRef->GetSize();

			// NiDataStreamPrimitiveLock builds its end iterator from the raw region
			// end. For a triangle list whose region count is not exactly divisible
			// by three, incrementing by three can never equal that end iterator and
			// eventually reads past the buffer. It also assumes a tightly packed
			// index element. Validate the metadata and read a bounded number of
			// elements through the stride-aware element iterator instead.
			const std::uint64_t uiRegionStart =
				static_cast<std::uint64_t>(kRegion.GetStartIndex()) * uiStride;
			const std::uint64_t uiLastElementEnd = uiIndexCount == 0 ? uiRegionStart :
				uiRegionStart + static_cast<std::uint64_t>(uiIndexCount - 1) * uiStride +
				uiElementOffset + stElementSize;

			if (uiIndexCount == 0 || uiStride == 0 || stElementSize == 0 ||
				uiElementOffset + stElementSize > uiStride ||
				uiLastElementEnd > stStreamSize)
			{
				std::cerr << "    Submesh " << uiSubmesh
					<< " has an invalid index region: count=" << uiIndexCount
					<< " start=" << kRegion.GetStartIndex()
					<< " stride=" << uiStride
					<< " elementOffset=" << uiElementOffset
					<< " elementSize=" << stElementSize
					<< " streamSize=" << stStreamSize << std::endl;
				return false;
			}

			kRawIndices.reserve(uiIndexCount);
			switch (kElement.GetFormat())
			{
			case NiDataStreamElement::F_UINT8_1:
			{
				uiRestartIndex = 0xFFu;
				auto it = kIndexLock.begin<NiUInt8>(uiSubmesh);
				for (unsigned int i = 0; i < uiIndexCount; ++i, ++it)
					kRawIndices.push_back(static_cast<unsigned int>(*it));
				break;
			}
			case NiDataStreamElement::F_UINT16_1:
			{
				uiRestartIndex = 0xFFFFu;
				auto it = kIndexLock.begin<NiUInt16>(uiSubmesh);
				for (unsigned int i = 0; i < uiIndexCount; ++i, ++it)
					kRawIndices.push_back(static_cast<unsigned int>(*it));
				break;
			}
			case NiDataStreamElement::F_UINT32_1:
			{
				uiRestartIndex = 0xFFFFFFFFu;
				auto it = kIndexLock.begin<NiUInt32>(uiSubmesh);
				for (unsigned int i = 0; i < uiIndexCount; ++i, ++it)
					kRawIndices.push_back(static_cast<unsigned int>(*it));
				break;
			}
			default:
				std::cerr << "    Submesh " << uiSubmesh
					<< " uses unsupported index format "
					<< static_cast<unsigned int>(kElement.GetFormat()) << std::endl;
				return false;
			}

			std::cerr << "    Submesh " << uiSubmesh
				<< " index stream: count=" << uiIndexCount
				<< " stride=" << uiStride
				<< " elementSize=" << stElementSize << std::endl;
		}
		else
		{
			// Non-indexed NiMesh: the vertex region is the primitive stream.
			kRawIndices.resize(uiVertexCount);
			for (unsigned int i = 0; i < uiVertexCount; ++i)
				kRawIndices[i] = i;
		}

		auto AppendTriangle = [&](unsigned int i0, unsigned int i1,
			unsigned int i2)
		{
			if (i0 == uiRestartIndex || i1 == uiRestartIndex ||
				i2 == uiRestartIndex)
			{
				return;
			}
			if (i0 >= uiVertexCount || i1 >= uiVertexCount ||
				i2 >= uiVertexCount)
			{
				return;
			}
			if (i0 == i1 || i0 == i2 || i1 == i2)
				return;

			kOut.push_back(i0);
			kOut.push_back(i1);
			kOut.push_back(i2);
		};

		switch (pkMesh->GetPrimitiveType())
		{
		case NiPrimitiveType::PRIMITIVE_TRIANGLES:
			if ((kRawIndices.size() % 3) != 0)
			{
				std::cerr << "    Submesh " << uiSubmesh << " has "
					<< (kRawIndices.size() % 3)
					<< " trailing triangle-list index value(s); ignoring them."
					<< std::endl;
			}
			for (size_t i = 0; i + 2 < kRawIndices.size(); i += 3)
				AppendTriangle(kRawIndices[i], kRawIndices[i + 1],
					kRawIndices[i + 2]);
			break;

		case NiPrimitiveType::PRIMITIVE_TRISTRIPS:
		{
			unsigned int uiStripVertexCount = 0;
			unsigned int i0 = 0;
			unsigned int i1 = 0;
			bool bOddTriangle = false;

			for (unsigned int i2 : kRawIndices)
			{
				if (i2 == uiRestartIndex)
				{
					uiStripVertexCount = 0;
					bOddTriangle = false;
					continue;
				}

				if (uiStripVertexCount == 0)
				{
					i0 = i2;
					uiStripVertexCount = 1;
					continue;
				}
				if (uiStripVertexCount == 1)
				{
					i1 = i2;
					uiStripVertexCount = 2;
					continue;
				}

				if (bOddTriangle)
					AppendTriangle(i1, i0, i2);
				else
					AppendTriangle(i0, i1, i2);

				// Degenerate triangles still consume a strip step and therefore
				// still flip the winding of the following primitive.
				bOddTriangle = !bOddTriangle;
				i0 = i1;
				i1 = i2;
				++uiStripVertexCount;
			}
			break;
		}

		default:
			return false;
		}

		return !kOut.empty();
	}

	//--------------------------------------------------------------------------------------------------
	void NormalizeLegacyWeights(IntermediateMesh& kMesh)
	{
		if (kMesh.boneWeights.size() != kMesh.positions.size() ||
			kMesh.boneNames.empty())
		{
			return;
		}

		unsigned int uiFallbackVertices = 0;
		for (std::vector<VertexBoneWeight>& kWeights : kMesh.boneWeights)
		{
			kWeights.erase(std::remove_if(kWeights.begin(), kWeights.end(),
				[&](const VertexBoneWeight& kWeight)
				{
					return kWeight.boneIndex >= kMesh.boneNames.size() ||
						kWeight.weight <= 0.0f;
				}), kWeights.end());

			// A partition palette can legally map more than one local slot to
			// the same global bone. Merge those entries before enforcing UE's
			// four-influence limit so a duplicate does not consume a slot.
			std::vector<VertexBoneWeight> kMergedWeights;
			for (const VertexBoneWeight& kWeight : kWeights)
			{
				auto kExisting = std::find_if(kMergedWeights.begin(),
					kMergedWeights.end(), [&](const VertexBoneWeight& kMerged)
					{
						return kMerged.boneIndex == kWeight.boneIndex;
					});
				if (kExisting != kMergedWeights.end())
					kExisting->weight += kWeight.weight;
				else
					kMergedWeights.push_back(kWeight);
			}
			kWeights.swap(kMergedWeights);

			std::stable_sort(kWeights.begin(), kWeights.end(),
				[](const VertexBoneWeight& kLeft, const VertexBoneWeight& kRight)
				{
					return kLeft.weight > kRight.weight;
				});
			if (kWeights.size() > 4)
				kWeights.resize(4);

			float fTotal = 0.0f;
			for (const VertexBoneWeight& kWeight : kWeights)
				fTotal += kWeight.weight;

			if (fTotal <= 0.000001f)
			{
				kWeights = {{0u, 1.0f}};
				++uiFallbackVertices;
				continue;
			}

			const float fInvTotal = 1.0f / fTotal;
			for (VertexBoneWeight& kWeight : kWeights)
				kWeight.weight *= fInvTotal;
		}

		if (uiFallbackVertices > 0)
		{
			std::cerr << "    Warning: assigned " << uiFallbackVertices
				<< " unweighted vertices to bone 0 in mesh '" << kMesh.name << "'."
				<< std::endl;
		}
	}
}

//--------------------------------------------------------------------------------------------------
MeshExtractor::MeshExtractor(const std::string& kTextureOutputFolder,
	bool bConvertTexturesToPng)
	: m_kTextureOutputFolder(kTextureOutputFolder)
	, m_bConvertTexturesToPng(bConvertTexturesToPng)
{
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::Extract(NiAVObject* pkRoot,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials,
	NodeIndexMap& kNodeIndexMap) const
{
	if (!pkRoot)
		return;

	TraverseNode(pkRoot, kMeshes, kMaterials, kNodeIndexMap);
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::TraverseNode(NiAVObject* pkObject,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials,
	NodeIndexMap& kNodeIndexMap) const
{
	if (!pkObject)
		return;

	const unsigned int uiIndex = static_cast<unsigned int>(kNodeIndexMap.size());
	kNodeIndexMap[pkObject] = uiIndex;

	const char* pcObjName = pkObject->GetName();
	std::cerr << "  Visiting: " << (pcObjName ? pcObjName : "<unnamed>")
		<< " (" << pkObject->GetRTTI()->GetName() << ")" << std::endl;

	if (NiIsKindOf(NiMesh, pkObject))
	{
		NiMesh* pkMesh = NiStaticCast(NiMesh, pkObject);
		const bool bOk = ExtractNiMesh(pkMesh, kMeshes, kMaterials);
		std::cerr << "    NiMesh extraction " << (bOk ? "succeeded" : "failed")
			<< std::endl;
	}
	else if (NiIsKindOf(NiGeometry, pkObject))
	{
		NiGeometry* pkGeom = NiStaticCast(NiGeometry, pkObject);
		const bool bOk = ExtractNiGeometry(pkGeom, kMeshes, kMaterials);
		std::cerr << "    NiGeometry extraction " << (bOk ? "succeeded" : "failed")
			<< std::endl;
	}

	if (NiIsKindOf(NiNode, pkObject))
	{
		NiNode* pkNode = NiStaticCast(NiNode, pkObject);
		std::cerr << "    Node child count: " << pkNode->GetArrayCount() << std::endl;
		for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
		{
			NiAVObject* pkChild = pkNode->GetAt(i);
			if (pkChild)
				TraverseNode(pkChild, kMeshes, kMaterials, kNodeIndexMap);
		}
	}
}

//--------------------------------------------------------------------------------------------------
std::string MeshExtractor::ResolveTexturePath(NiTexturingProperty* pkTexProp) const
{
	if (!pkTexProp)
		return std::string();

	NiTexture* pkTex = pkTexProp->GetBaseTexture();
	if (!pkTex)
		return std::string();

	if (NiIsKindOf(NiSourceTexture, pkTex))
	{
		NiSourceTexture* pkSrc = NiStaticCast(NiSourceTexture, pkTex);
		const NiFixedString& kFilename = pkSrc->GetFilename();
		if ((const char*)kFilename)
			return std::string((const char*)kFilename);
	}
	return std::string();
}

//--------------------------------------------------------------------------------------------------
unsigned int MeshExtractor::FindOrAddMaterial(const std::string& kTexturePath,
	const std::string& kMeshName,
	std::vector<IntermediateMaterial>& kMaterials) const
{
	for (unsigned int i = 0; i < static_cast<unsigned int>(kMaterials.size()); ++i)
	{
		if (kMaterials[i].diffuseTexturePath == kTexturePath)
			return i;
	}

	IntermediateMaterial kMaterial;
	kMaterial.diffuseTexturePath = kTexturePath;
	if (!kTexturePath.empty())
		kMaterial.name = fs::path(kTexturePath).stem().string();
	else
		kMaterial.name = kMeshName + "_mat";
	kMaterials.push_back(std::move(kMaterial));
	return static_cast<unsigned int>(kMaterials.size()) - 1;
}

//--------------------------------------------------------------------------------------------------
aiMatrix4x4 MeshExtractor::MakeAiMatrix(const NiTransform& kT) const
{
	const float fScale = kT.m_fScale;
	const NiMatrix3& kRotate = kT.m_Rotate;
	const NiPoint3& kTranslate = kT.m_Translate;

	return aiMatrix4x4(
		fScale * kRotate.GetEntry(0, 0), fScale * kRotate.GetEntry(0, 1),
		fScale * kRotate.GetEntry(0, 2), kTranslate.x,
		fScale * kRotate.GetEntry(1, 0), fScale * kRotate.GetEntry(1, 1),
		fScale * kRotate.GetEntry(1, 2), kTranslate.y,
		fScale * kRotate.GetEntry(2, 0), fScale * kRotate.GetEntry(2, 1),
		fScale * kRotate.GetEntry(2, 2), kTranslate.z,
		0.0f, 0.0f, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------------------------
bool MeshExtractor::ExtractNiMesh(NiMesh* pkMesh,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials) const
{
	if (!pkMesh)
		return false;

	const NiPrimitiveType::Type ePrimitive = pkMesh->GetPrimitiveType();
	const unsigned int uiSubmeshCount = pkMesh->GetSubmeshCount();
	std::cerr << "    NiMesh primitive type: " << static_cast<int>(ePrimitive)
		<< ", submesh count: " << uiSubmeshCount
		<< ", modifier count: " << pkMesh->GetModifierCount() << std::endl;

	if (ePrimitive != NiPrimitiveType::PRIMITIVE_TRIANGLES &&
		ePrimitive != NiPrimitiveType::PRIMITIVE_TRISTRIPS)
	{
		std::cerr << "    Skipping: unsupported primitive type" << std::endl;
		return false;
	}

	if (uiSubmeshCount == 0)
	{
		std::cerr << "    Skipping: mesh has no submeshes" << std::endl;
		return false;
	}

	IntermediateMesh kOut;
	const char* pcName = pkMesh->GetName();
	kOut.name = pcName ? std::string(pcName) : "mesh";
	kOut.sourceObject = pkMesh;

	NiTexturingProperty* pkTexProp = NiStaticCast(NiTexturingProperty,
		pkMesh->GetProperty(NiProperty::TEXTURING));
	const std::string kTexPath = ResolveTexturePath(pkTexProp);
	kOut.materialIndex = FindOrAddMaterial(kTexPath, kOut.name, kMaterials);

	NiSkinningMeshModifier* pkSkinModifier = nullptr;
	for (NiUInt32 m = 0; m < pkMesh->GetModifierCount(); ++m)
	{
		NiMeshModifier* pkModifier = pkMesh->GetModifierAt(m);
		if (pkModifier && NiIsKindOf(NiSkinningMeshModifier, pkModifier))
		{
			pkSkinModifier = NiStaticCast(NiSkinningMeshModifier, pkModifier);
			break;
		}
	}

	if (pkSkinModifier)
		InitializeSkinningFromNiMesh(pkSkinModifier, kOut);

	bool bAllNormals = true;
	bool bAllUvs = true;
	unsigned int uiExportedSubmeshes = 0;

	for (unsigned int uiSubmesh = 0; uiSubmesh < uiSubmeshCount; ++uiSubmesh)
	{
		std::vector<aiVector3D> kPositions;
		std::vector<aiVector3D> kNormals;
		std::vector<aiVector2D> kUvs;
		std::vector<unsigned int> kIndices;

		bool bGotPositions = false;
		if (pkSkinModifier)
		{
			bGotPositions = ReadVec3Stream(pkMesh,
				NiCommonSemantics::POSITION_BP(), uiSubmesh, kPositions);
		}
		if (!bGotPositions)
		{
			bGotPositions = ReadVec3Stream(pkMesh,
				NiCommonSemantics::POSITION(), uiSubmesh, kPositions);
		}

		if (!bGotPositions || kPositions.empty())
		{
			std::cerr << "    Submesh " << uiSubmesh
				<< " skipped: no readable position stream." << std::endl;
			continue;
		}

		bool bGotNormals = false;
		if (pkSkinModifier)
		{
			bGotNormals = ReadVec3Stream(pkMesh,
				NiCommonSemantics::NORMAL_BP(), uiSubmesh, kNormals);
		}
		if (!bGotNormals)
		{
			bGotNormals = ReadVec3Stream(pkMesh,
				NiCommonSemantics::NORMAL(), uiSubmesh, kNormals);
		}
		if (!bGotNormals || kNormals.size() != kPositions.size())
		{
			bAllNormals = false;
			kNormals.assign(kPositions.size(), aiVector3D());
		}

		const bool bGotUvs = ReadVec2Stream(pkMesh,
			NiCommonSemantics::TEXCOORD(), uiSubmesh, kUvs);
		if (!bGotUvs || kUvs.size() != kPositions.size())
		{
			bAllUvs = false;
			kUvs.assign(kPositions.size(), aiVector2D());
		}

		if (!ReadTriangleIndices(pkMesh, uiSubmesh,
			static_cast<unsigned int>(kPositions.size()), kIndices))
		{
			std::cerr << "    Submesh " << uiSubmesh
				<< " skipped: no readable triangles." << std::endl;
			continue;
		}

		const unsigned int uiVertexBase = static_cast<unsigned int>(kOut.positions.size());
		kOut.positions.insert(kOut.positions.end(), kPositions.begin(), kPositions.end());
		kOut.normals.insert(kOut.normals.end(), kNormals.begin(), kNormals.end());
		kOut.uvs.insert(kOut.uvs.end(), kUvs.begin(), kUvs.end());

		kOut.indices.reserve(kOut.indices.size() + kIndices.size());
		for (unsigned int uiIndex : kIndices)
			kOut.indices.push_back(uiVertexBase + uiIndex);

		if (kOut.isSkinned)
		{
			AppendSkinningFromNiMeshSubmesh(pkMesh, pkSkinModifier, uiSubmesh,
				static_cast<unsigned int>(kPositions.size()), kOut);
		}

		++uiExportedSubmeshes;
	}

	if (kOut.positions.empty() || kOut.indices.empty())
	{
		std::cerr << "    Skipping: no complete submesh could be extracted." << std::endl;
		return false;
	}

	if (!bAllNormals)
		kOut.normals.clear();
	if (!bAllUvs)
		kOut.uvs.clear();

	if (kOut.isSkinned)
		NormalizeLegacyWeights(kOut);

	std::cerr << "    Combined " << uiExportedSubmeshes << "/" << uiSubmeshCount
		<< " submeshes: vertices=" << kOut.positions.size()
		<< " triangles=" << (kOut.indices.size() / 3)
		<< " bones=" << kOut.boneNames.size() << std::endl;

	kMeshes.push_back(std::move(kOut));
	return true;
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::InitializeSkinningFromNiMesh(
	NiSkinningMeshModifier* pkModifier, IntermediateMesh& kOut) const
{
	if (!pkModifier)
		return;

	const NiUInt32 uiBoneCount = pkModifier->GetBoneCount();
	if (uiBoneCount == 0)
		return;

	NiAVObject** ppkBones = pkModifier->GetBones();
	NiTransform* pkSkinToBone = pkModifier->GetSkinToBoneTransforms();

	kOut.isSkinned = true;
	kOut.boneNames.resize(uiBoneCount);
	kOut.boneOffsetMatrices.resize(uiBoneCount);

	for (NiUInt32 b = 0; b < uiBoneCount; ++b)
	{
		NiAVObject* pkBone = ppkBones ? ppkBones[b] : nullptr;
		kOut.boneNames[b] = pkBone
			? GetExportNodeName(pkBone)
			: "missing_bone_" + std::to_string(b);

		kOut.boneOffsetMatrices[b] = pkSkinToBone
			? MakeAiMatrix(pkSkinToBone[b]) : aiMatrix4x4();
	}
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::AppendSkinningFromNiMeshSubmesh(NiMesh* pkMesh,
	NiSkinningMeshModifier* pkModifier, unsigned int uiSubmesh,
	unsigned int uiVertexCount, IntermediateMesh& kOut) const
{
	if (!pkMesh || !pkModifier || kOut.boneNames.empty())
		return;

	std::vector<Index4> kBlendIndices;
	std::vector<Weight4> kBlendWeights;
	std::vector<unsigned int> kBonePalette;

	const bool bHasIndices = ReadBlendIndices(pkMesh, uiSubmesh,
		uiVertexCount, kBlendIndices);
	const bool bHasWeights = ReadBlendWeights(pkMesh, uiSubmesh,
		uiVertexCount, kBlendWeights);
	const bool bHasPalette = ReadBonePalette(pkMesh, uiSubmesh, kBonePalette);

	const size_t stOldSize = kOut.boneWeights.size();
	kOut.boneWeights.resize(stOldSize + uiVertexCount);

	unsigned int uiFallbackVertices = 0;
	for (unsigned int v = 0; v < uiVertexCount; ++v)
	{
		std::vector<VertexBoneWeight>& kVertexWeights = kOut.boneWeights[stOldSize + v];

		if (bHasIndices && bHasWeights)
		{
			for (unsigned int k = 0; k < 4; ++k)
			{
				const float fWeight = kBlendWeights[v][k];
				if (fWeight <= 0.000001f)
					continue;

				unsigned int uiBoneIndex = kBlendIndices[v][k];
				if (bHasPalette)
				{
					if (uiBoneIndex >= kBonePalette.size())
						continue;
					uiBoneIndex = kBonePalette[uiBoneIndex];
				}

				if (uiBoneIndex < kOut.boneNames.size())
					kVertexWeights.push_back({uiBoneIndex, fWeight});
			}
		}

		if (kVertexWeights.empty())
		{
			// Unreal rejects or drops vertices without a valid influence. Bone 0
			// is the least destructive fallback and keeps the complete section.
			kVertexWeights.push_back({0u, 1.0f});
			++uiFallbackVertices;
		}
	}

	if (uiFallbackVertices > 0)
	{
		std::cerr << "    Warning: submesh " << uiSubmesh << " had "
			<< uiFallbackVertices << " vertices without readable skin weights; "
			<< "assigned them to bone 0." << std::endl;
	}
}

//--------------------------------------------------------------------------------------------------
bool MeshExtractor::ExtractNiGeometry(NiGeometry* pkGeom,
	std::vector<IntermediateMesh>& kMeshes,
	std::vector<IntermediateMaterial>& kMaterials) const
{
	if (!pkGeom)
		return false;

	NiGeometryData* pkData = pkGeom->GetModelData();
	if (!pkData)
	{
		std::cerr << "  Skipped geometry without model data: " << pkGeom->GetName()
			<< std::endl;
		return false;
	}

	const bool bIsShape = NiIsKindOf(NiTriShape, pkGeom);
	const bool bIsStrips = NiIsKindOf(NiTriStrips, pkGeom);
	if (!bIsShape && !bIsStrips)
	{
		std::cerr << "  Skipped unsupported legacy geometry: " << pkGeom->GetName()
			<< std::endl;
		return false;
	}

	IntermediateMesh kOut;
	const char* pcName = pkGeom->GetName();
	kOut.name = pcName ? std::string(pcName) : "legacy_mesh";
	kOut.sourceObject = pkGeom;

	NiTexturingProperty* pkTexProp = NiStaticCast(NiTexturingProperty,
		pkGeom->GetProperty(NiProperty::TEXTURING));
	const std::string kTexPath = ResolveTexturePath(pkTexProp);
	kOut.materialIndex = FindOrAddMaterial(kTexPath, kOut.name, kMaterials);

	const unsigned short usVertexCount = pkData->GetVertexCount();
	if (usVertexCount == 0)
		return false;

	const NiPoint3* pkVertices = pkData->GetVertices();
	if (!pkVertices)
		return false;

	kOut.positions.reserve(usVertexCount);
	for (unsigned short v = 0; v < usVertexCount; ++v)
		kOut.positions.emplace_back(pkVertices[v].x, pkVertices[v].y, pkVertices[v].z);

	const NiPoint3* pkNormals = pkData->GetNormals();
	if (pkNormals)
	{
		kOut.normals.reserve(usVertexCount);
		for (unsigned short v = 0; v < usVertexCount; ++v)
			kOut.normals.emplace_back(pkNormals[v].x, pkNormals[v].y, pkNormals[v].z);
	}

	const NiPoint2* pkUvs = pkData->GetTextures();
	if (pkUvs)
	{
		kOut.uvs.reserve(usVertexCount);
		for (unsigned short v = 0; v < usVertexCount; ++v)
			kOut.uvs.emplace_back(pkUvs[v].x, pkUvs[v].y);
	}

	NiTriBasedGeomData* pkTriData = NiStaticCast(NiTriBasedGeomData, pkData);
	const unsigned short usTriangleCount = pkTriData->GetTriangleCount();
	kOut.indices.reserve(usTriangleCount * 3u);

	for (unsigned short t = 0; t < usTriangleCount; ++t)
	{
		unsigned short i0 = 0;
		unsigned short i1 = 0;
		unsigned short i2 = 0;
		pkTriData->GetTriangleIndices(t, i0, i1, i2);
		if (i0 >= usVertexCount || i1 >= usVertexCount || i2 >= usVertexCount)
			continue;
		if (i0 == i1 || i0 == i2 || i1 == i2)
			continue;

		kOut.indices.push_back(i0);
		kOut.indices.push_back(i1);
		kOut.indices.push_back(i2);
	}

	if (kOut.indices.empty())
		return false;

	if (pkGeom->GetSkinInstance())
	{
		ExtractSkinningFromNiGeometry(pkGeom, kOut);
		NormalizeLegacyWeights(kOut);
	}

	kMeshes.push_back(std::move(kOut));
	return true;
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::ExtractSkinningFromNiGeometry(NiGeometry* pkGeom,
	IntermediateMesh& kOut) const
{
	NiSkinInstance* pkSkin = pkGeom->GetSkinInstance();
	if (!pkSkin)
		return;

	NiSkinData* pkSkinData = pkSkin->GetSkinData();
	if (!pkSkinData)
		return;

	const unsigned int uiBoneCount = pkSkinData->GetBoneCount();
	NiSkinData::BoneData* pkBoneData = pkSkinData->GetBoneData();
	if (uiBoneCount == 0 || !pkBoneData)
		return;

	const NiAVObject* const* ppkBones = pkSkin->GetBones();

	kOut.isSkinned = true;
	kOut.boneNames.resize(uiBoneCount);
	kOut.boneOffsetMatrices.resize(uiBoneCount);

	for (unsigned int b = 0; b < uiBoneCount; ++b)
	{
		const NiAVObject* pkBone = ppkBones ? ppkBones[b] : nullptr;
		kOut.boneNames[b] = pkBone
			? GetExportNodeName(pkBone)
			: "missing_bone_" + std::to_string(b);

		kOut.boneOffsetMatrices[b] = MakeAiMatrix(pkBoneData[b].m_kSkinToBone);
	}

	const unsigned int uiVertexCount = static_cast<unsigned int>(kOut.positions.size());
	kOut.boneWeights.resize(uiVertexCount);

	bool bHasAnyBoneVertexData = false;
	for (unsigned int b = 0; b < uiBoneCount; ++b)
	{
		NiSkinData::BoneData& kBone = pkBoneData[b];
		if (kBone.m_usVerts == 0 || !kBone.m_pkBoneVertData)
			continue;

		bHasAnyBoneVertexData = true;
		for (unsigned short vi = 0; vi < kBone.m_usVerts; ++vi)
		{
			const unsigned int uiVertex = kBone.m_pkBoneVertData[vi].m_usVert;
			const float fWeight = kBone.m_pkBoneVertData[vi].m_fWeight;
			if (uiVertex < uiVertexCount && fWeight > 0.0f)
				kOut.boneWeights[uiVertex].push_back({b, fWeight});
		}
	}

	if (!bHasAnyBoneVertexData)
		ExtractSkinningFromPartition(pkSkin, pkSkinData, kOut, uiVertexCount);
}

//--------------------------------------------------------------------------------------------------
void MeshExtractor::ExtractSkinningFromPartition(NiSkinInstance* pkSkin,
	NiSkinData* pkSkinData, IntermediateMesh& kOut,
	unsigned int uiVertexCount) const
{
	// Since Gamebryo 1.2 the authoritative partition is stored on
	// NiSkinInstance. Older files may still keep it on NiSkinData.
	NiSkinPartition* pkPartition = pkSkin ? pkSkin->GetSkinPartition() : nullptr;
	if (!pkPartition && pkSkinData)
		pkPartition = pkSkinData->GetSkinPartition(true);
	if (!pkPartition)
		return;

	const unsigned int uiPartitionCount = pkPartition->GetPartitionCount();
	NiSkinPartition::Partition* pkParts = pkPartition->GetPartitions();
	if (!pkParts)
		return;

	for (unsigned int p = 0; p < uiPartitionCount; ++p)
	{
		const NiSkinPartition::Partition& kPart = pkParts[p];
		if (!kPart.m_pusVertexMap || !kPart.m_pusBones || !kPart.m_pfWeights)
			continue;

		for (unsigned short vLocal = 0; vLocal < kPart.m_usVertices; ++vLocal)
		{
			const unsigned int uiFullVertex = kPart.m_pusVertexMap[vLocal];
			if (uiFullVertex >= uiVertexCount)
				continue;

			for (unsigned short k = 0; k < kPart.m_usBonesPerVertex; ++k)
			{
				const unsigned int uiInfluence =
					vLocal * kPart.m_usBonesPerVertex + k;
				const float fWeight = kPart.m_pfWeights[uiInfluence];
				if (fWeight <= 0.0f)
					continue;

				const unsigned int uiLocalBone = kPart.m_pucBonePalette
					? kPart.m_pucBonePalette[uiInfluence]
					: static_cast<unsigned int>(k);
				if (uiLocalBone >= kPart.m_usBones)
					continue;

				const unsigned int uiGlobalBone = kPart.m_pusBones[uiLocalBone];
				if (uiGlobalBone < kOut.boneNames.size())
					kOut.boneWeights[uiFullVertex].push_back({uiGlobalBone, fWeight});
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------
aiNode* MeshExtractor::BuildNodeRecursive(NiAVObject* pkObject,
	const MeshNodeAssignmentMap& kMeshNodeAssignments) const
{
	if (!pkObject)
		return nullptr;

	aiNode* pkAiNode = new aiNode();
	pkAiNode->mName = GetExportNodeName(pkObject).c_str();
	pkAiNode->mTransformation = MakeAiMatrix(pkObject->GetLocalTransform());

	auto kAssignment = kMeshNodeAssignments.find(pkObject);
	if (kAssignment != kMeshNodeAssignments.end() && !kAssignment->second.empty())
	{
		pkAiNode->mNumMeshes = static_cast<unsigned int>(kAssignment->second.size());
		pkAiNode->mMeshes = new unsigned int[pkAiNode->mNumMeshes];
		for (unsigned int i = 0; i < pkAiNode->mNumMeshes; ++i)
			pkAiNode->mMeshes[i] = kAssignment->second[i];
	}

	if (NiIsKindOf(NiNode, pkObject))
	{
		NiNode* pkNode = NiStaticCast(NiNode, pkObject);
		std::vector<aiNode*> kChildren;
		kChildren.reserve(pkNode->GetArrayCount());

		for (unsigned int i = 0; i < pkNode->GetArrayCount(); ++i)
		{
			NiAVObject* pkChild = pkNode->GetAt(i);
			if (!pkChild)
				continue;

			aiNode* pkChildNode = BuildNodeRecursive(pkChild, kMeshNodeAssignments);
			if (pkChildNode)
			{
				pkChildNode->mParent = pkAiNode;
				kChildren.push_back(pkChildNode);
			}
		}

		pkAiNode->mNumChildren = static_cast<unsigned int>(kChildren.size());
		if (!kChildren.empty())
		{
			pkAiNode->mChildren = new aiNode*[kChildren.size()];
			for (unsigned int i = 0; i < kChildren.size(); ++i)
				pkAiNode->mChildren[i] = kChildren[i];
		}
	}

	return pkAiNode;
}

//--------------------------------------------------------------------------------------------------
aiNode* MeshExtractor::BuildNodeHierarchy(NiAVObject* pkRoot,
	const MeshNodeAssignmentMap& kMeshNodeAssignments) const
{
	return BuildNodeRecursive(pkRoot, kMeshNodeAssignments);
}
