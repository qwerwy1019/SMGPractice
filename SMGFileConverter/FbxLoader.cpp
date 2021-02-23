#include "FbxLoader.h"

#include <filesystem>
#include <set>

#include "SMGEngine/SkinnedData.h"
#include "SMGEngine/MathHelper.h"
#include "SMGEngine/D3DUtil.h"
#include "SMGEngine/FileHelper.h"


void FbxLoader::loadFbxPolygons(FbxMesh* mesh, std::vector<FbxPolygonVertexInfo>& polygonVertices) const
{
	const int polygonCount = mesh->GetPolygonCount();
	if (polygonCount <= 0)
	{
		ThrowErrCode(ErrCode::InvalidVertexInfo, "mesh ��忡 vertex�� ���� �� ������?");
	}
	polygonVertices.reserve(polygonCount * VERTEX_PER_POLYGON);

	// ��Ƽ���̾ ������� �ʱ�� �Ѵ�. [12/31/2020 qwerw]
	const FbxLayerElementMaterial* layerMaterials = mesh->GetLayer(0)->GetMaterials();
	if (layerMaterials == nullptr)
	{
		ThrowErrCode(ErrCode::MaterialNotFound, "material�� �Ⱦ��� �޽��� �ֳ�...?");
	}

	const FbxLayerElementUV* layerUvs = mesh->GetLayer(0)->GetUVs(FbxLayerElement::eTextureDiffuse);
	if (layerUvs == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidTextureInfo, "TextureUV �����Ͱ� �̻��մϴ�.");
	}
	const IndexMappingType uvIndexType = getIndexMappingType(layerUvs->GetReferenceMode(), layerUvs->GetMappingMode());

	const FbxLayerElementNormal* layerNomrals = mesh->GetLayer(0)->GetNormals();
	if (layerNomrals == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidNormalData, "Normal �����Ͱ� �̻��մϴ�.");
	}
	const IndexMappingType normalIndexType = getIndexMappingType(layerNomrals->GetReferenceMode(), layerNomrals->GetMappingMode());

	const FbxLayerElementVertexColor* layerColors = mesh->GetLayer(0)->GetVertexColors();
	const IndexMappingType colorIndexType =
		(layerColors != nullptr) ? getIndexMappingType(layerColors->GetReferenceMode(), layerColors->GetMappingMode()) : IndexMappingType::Undefined;

	for (int p = 0; p < polygonCount; ++p)
	{
		FbxPolygonVertexInfo polygonVertexInfo;

		polygonVertexInfo._materialIndex = layerMaterials->GetIndexArray().GetAt(p);

		for (int v = 0; v < VERTEX_PER_POLYGON; ++v)
		{
			polygonVertexInfo._controlPointIndex = mesh->GetPolygonVertex(p, v);

			switch (uvIndexType)
			{
				case IndexMappingType::ControlPointIndex:
					polygonVertexInfo._uvIndex = polygonVertexInfo._controlPointIndex;
					break;
				case IndexMappingType::IndexArrayAtControlPoint:
					polygonVertexInfo._uvIndex = layerUvs->GetIndexArray().GetAt(polygonVertexInfo._controlPointIndex);
					break;
				case IndexMappingType::PolygonVertexIndex:
				case IndexMappingType::IndexArrayAtPolygonVertex:
					polygonVertexInfo._uvIndex = mesh->GetTextureUVIndex(p, v);
					break;;
				case IndexMappingType::Undefined:
				default:
					ThrowErrCode(ErrCode::UndefinedType, "Ÿ���� �߰��Ǿ���?");
					static_assert(static_cast<int>(IndexMappingType::Undefined) == 4);
			}

			switch (normalIndexType)
			{
				case IndexMappingType::ControlPointIndex:
					polygonVertexInfo._normalIndex = polygonVertexInfo._controlPointIndex;
					break;
				case IndexMappingType::IndexArrayAtControlPoint:
					polygonVertexInfo._normalIndex = layerNomrals->GetIndexArray().GetAt(polygonVertexInfo._controlPointIndex);
					break;
				case IndexMappingType::PolygonVertexIndex:
					polygonVertexInfo._normalIndex = polygonVertices.size();
					break;
				case IndexMappingType::IndexArrayAtPolygonVertex:
					polygonVertexInfo._normalIndex = layerNomrals->GetIndexArray().GetAt(polygonVertices.size());
					break;
				case IndexMappingType::Undefined:
				default:
					ThrowErrCode(ErrCode::UndefinedType, "Ÿ���� �߰��Ǿ���?");
					static_assert(static_cast<int>(IndexMappingType::Undefined) == 4);
			}

			switch (colorIndexType)
			{
				case IndexMappingType::ControlPointIndex:
					polygonVertexInfo._colorIndex = polygonVertexInfo._controlPointIndex;
					break;
				case IndexMappingType::IndexArrayAtControlPoint:
					polygonVertexInfo._colorIndex = layerColors->GetIndexArray().GetAt(polygonVertexInfo._controlPointIndex);
					break;
				case IndexMappingType::PolygonVertexIndex:
					polygonVertexInfo._colorIndex = polygonVertices.size();
					break;
				case IndexMappingType::IndexArrayAtPolygonVertex:
					polygonVertexInfo._colorIndex = layerColors->GetIndexArray().GetAt(polygonVertices.size());
					break;
				case IndexMappingType::Undefined:
					polygonVertexInfo._colorIndex = std::numeric_limits<uint16_t>::max();
					break;
				default:
					ThrowErrCode(ErrCode::UndefinedType, "Ÿ���� �߰��Ǿ���?");
					static_assert(static_cast<int>(IndexMappingType::Undefined) == 4);
			}

			polygonVertices.push_back(polygonVertexInfo);
		}
	}
}

void FbxLoader::loadFbxOptimizedMesh(const FbxMesh* mesh,
										const std::vector<FbxPolygonVertexInfo>& polygonVertices,
										const std::vector<FbxVertexSkinningInfo>& skinningInfos)
{
	check(mesh != nullptr, "�������Դϴ�.");
	check(!polygonVertices.empty(), "polygonVertex �����Ͱ� �����ϴ�.");

	const int controlPointCount = mesh->GetControlPointsCount();
	check(skinningInfos.empty() || controlPointCount == skinningInfos.size(), "skinning info�� controlPoint �����Ͱ� ��ġ���� �ʽ��ϴ�.");

	std::vector<DirectX::XMFLOAT3> controlPoints(controlPointCount);
	for (int i = 0; i < controlPointCount; ++i)
	{
		controlPoints[i] = fbxVector4ToXMFLOAT3(mesh->GetControlPointAt(i));
	}

	bool isSkinningMesh = !skinningInfos.empty();

	vector<vector<SkinnedVertex>> skinnedVertices;
	vector<vector<Vertex>> vertices;
	if (isSkinningMesh)
	{
		skinnedVertices.resize(_materialsInfos.size());
		for (int i = 0; i < skinnedVertices.size(); ++i)
		{
			skinnedVertices[i].reserve(controlPointCount);
		}
	}
	else
	{
		vertices.resize(_materialsInfos.size());
		for (int i = 0; i < vertices.size(); ++i)
		{
			vertices[i].reserve(controlPointCount);
		}
	}
	vector<vector<GeoIndex>> indices(_materialsInfos.size());
	for (int i = 0; i < indices.size(); ++i)
	{
		indices[i].reserve(polygonVertices.size());
	}

	const FbxLayerElementUV* layerUvs = mesh->GetLayer(0)->GetUVs(FbxLayerElement::eTextureDiffuse);
	if (layerUvs == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidFbxData, "uvLayer nullptr");
	}

	const FbxLayerElementNormal* layerNomrals = mesh->GetLayer(0)->GetNormals();
	if (layerNomrals == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidFbxData, "normalLayer nullptr");
	}

	struct VertexKey
	{
		uint16_t material;
		uint16_t controlPoint;
		uint16_t uv;
		bool operator==(const VertexKey& rhs) const
		{
			return (material == rhs.material)
				&& (controlPoint == rhs.controlPoint)
				&& (uv == rhs.uv);
		}
	};
	struct VertexKeyHasher
	{
		std::size_t operator()(const VertexKey& key) const
		{
			return ((key.material & 0xF) << 16)
				+ ((key.controlPoint & 0xF) << 8)
				+ (key.uv & 0xF);
		}
	};
	unordered_map<VertexKey, GeoIndex, VertexKeyHasher> indexMap;

	for (const auto& pv : polygonVertices)
	{
		if (pv._materialIndex >= _materialsInfos.size())
		{
			ThrowErrCode(ErrCode::MaterialNotFound, "pv matIndex: " + std::to_string(pv._materialIndex) +
						", matInfoSize: " + std::to_string(_materialsInfos.size()));
		}
		VertexKey key = { pv._materialIndex, pv._controlPointIndex, pv._uvIndex };
		auto it = indexMap.find(key);
		if (it != indexMap.end())
		{
			indices[pv._materialIndex].push_back(it->second);
		}
		else
		{
			if (isSkinningMesh)
			{
				SkinnedVertex v;
				v._position = controlPoints[pv._controlPointIndex];
				v._normal = fbxVector4ToXMFLOAT3(layerNomrals->GetDirectArray().GetAt(pv._normalIndex));
				v._textureCoord = fbxVector2ToXMFLOAT2(layerUvs->GetDirectArray().GetAt(pv._uvIndex));
				v._boneWeights = getWeight(skinningInfos[pv._controlPointIndex]._weight);
				v._boneIndices = skinningInfos[pv._controlPointIndex]._boneIndex;

				// uv��ǥ y�� ������������. Ȯ�� �ʿ� [1/15/2021 qwerw]
				v._textureCoord.y *= -1;

				vector<SkinnedVertex>& subMeshVertices = skinnedVertices[pv._materialIndex];
				const GeoIndex index = subMeshVertices.size();
				indices[pv._materialIndex].push_back(index);
				indexMap.emplace(make_pair(key, index));
				subMeshVertices.push_back(v);
			}
			else
			{
				Vertex v;
				v._position = controlPoints[pv._controlPointIndex];
				v._normal = fbxVector4ToXMFLOAT3(layerNomrals->GetDirectArray().GetAt(pv._normalIndex));
				v._textureCoord = fbxVector2ToXMFLOAT2(layerUvs->GetDirectArray().GetAt(pv._uvIndex));

				// uv��ǥ y�� ������������ [1/15/2021 qwerw]
				v._textureCoord.y *= -1;

				vector<Vertex>& subMeshVertices = vertices[pv._materialIndex];
				const GeoIndex index = subMeshVertices.size();
				indices[pv._materialIndex].push_back(index);
				indexMap.emplace(make_pair(key, index));
				subMeshVertices.push_back(v);
			}
		}
	}

	if (vertices.empty() && skinnedVertices.empty())
	{
		ThrowErrCode(ErrCode::InvalidVertexInfo);
	}
	if (indices.empty())
	{
		ThrowErrCode(ErrCode::InvalidIndexInfo);
	}

	FbxMeshInfo info(mesh->GetNode()->GetName(), std::move(vertices), std::move(skinnedVertices), std::move(indices));

	_meshInfos.emplace_back(std::move(info));
}

void FbxLoader::loadFbxSkin(FbxNode* node, std::vector<FbxVertexSkinningInfo>& skinningInfos)
{
	check(node != nullptr, "�������Դϴ�.");
	check(skinningInfos.empty(), "�������Դϴ�.");
	check(node->GetMesh()->GetDeformerCount() != 0, "�ִϸ��̼��� ���� ����Դϴ�.");
	check(!_boneIndexMap.empty(), "boneNode-index map�� ���� �����Ǿ�� �մϴ�.");
	check(!_boneHierarchy.empty(), "boneHierarchy�� ���� �����Ǿ�� �մϴ�.");
	check(!_boneNames.empty(), "boneNames�� ���� �ε�Ǿ�� �մϴ�.");
	check(_boneHierarchy.size() == _boneIndexMap.size(), "boneInfo�� ����� �ȸ���������ϴ�.");
	check(_boneHierarchy.size() == _boneNames.size(), "boneInfo�� ����� �ȸ���������ϴ�.");

	const int skinCount = node->GetMesh()->GetDeformerCount();
	const int controlPointCount = node->GetMesh()->GetControlPointsCount();
	skinningInfos.resize(controlPointCount);
	_boneOffsets.resize(_boneHierarchy.size(), MathHelper::Identity4x4);

	FbxAMatrix geometricTransform = getGeometryTransformation(node->GetMesh()->GetNode());
	for (int i = 0; i < skinCount; ++i)
	{
		FbxSkin* fbxSkin = FbxCast<FbxSkin>(node->GetMesh()->GetDeformer(i, FbxDeformer::eSkin));
		int clusterCount = fbxSkin->GetClusterCount();
		FbxCluster::ELinkMode linkMode = fbxSkin->GetCluster(0)->GetLinkMode();
		if (linkMode != FbxCluster::eNormalize)
		{
			ThrowErrCode(ErrCode::UndefinedType, "fbxClster link mode�� normalize�� �ƴ� ��Ȳ�� ������� �ʾҽ��ϴ�.");
		}
		for (int ci = 0; ci < clusterCount; ++ci)
		{
			FbxCluster* fbxCluster = fbxSkin->GetCluster(ci);

			if (fbxCluster == nullptr)
			{
				ThrowErrCode(ErrCode::InvalidFbxData, "�������Դϴ�.");
			}

			if (linkMode != fbxCluster->GetLinkMode())
			{
				ThrowErrCode(ErrCode::TypeIsDifferent, "link mode�� ���δٸ� ��Ȳ�� ������� �ʾҽ��ϴ�.");
			}

			FbxNode* linkNode = fbxCluster->GetLink();
			auto boneIndex = _boneIndexMap.find(linkNode);
			if (boneIndex == _boneIndexMap.end())
			{
				ThrowErrCode(ErrCode::InvalidFbxData, "boneIndex map�� ���� ����Դϴ�.");
			}

			FbxAMatrix transform, linkTransform;
			(void)fbxCluster->GetTransformMatrix(transform);
			(void)fbxCluster->GetTransformLinkMatrix(linkTransform);

			const DirectX::XMFLOAT4X4 matBoneOffsetMatrix = fbxMatrixToXMFLOAT4X4(linkTransform.Inverse() * transform * geometricTransform);
			check(MathHelper::equal(_boneOffsets[boneIndex->second], MathHelper::Identity4x4), "���� ��������ϴ�.");
			_boneOffsets[boneIndex->second] = matBoneOffsetMatrix;

			int* indices = fbxCluster->GetControlPointIndices();
			double* weights = fbxCluster->GetControlPointWeights();
			int weightControlPointCount = fbxCluster->GetControlPointIndicesCount();

			for (int i = 0; i < weightControlPointCount; ++i)
			{
				skinningInfos[indices[i]].insert(boneIndex->second, weights[i]);
			}
		}
	}
}

void FbxLoader::clearCachedFbxFileInfo(void) noexcept
{
	_materialsInfos.clear();

	_meshInfos.clear();

	_boneHierarchy.clear();
	_boneOffsets.clear();
	_boneNames.clear();

	_animations.clear();

	_boneIndexMap.clear();
	_animTimeList.clear();
}

void FbxLoader::loadFbxAnimations(FbxScene* fbxScene)
{
	if (fbxScene->GetSrcObjectCount<FbxAnimStack>() == 0)
	{
		return;
	}
	if (fbxScene->GetSrcObjectCount<FbxAnimStack>() != 1)
	{
		ThrowErrCode(ErrCode::InvalidAnimationData, "animation stack�� �������� ���� �ε����� ����. bake layer ���");
	}

	FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(0);
	if (animStack->GetMemberCount<FbxAnimLayer>() != 1)
	{
		ThrowErrCode(ErrCode::InvalidAnimationData, "animation stack�� �������� ���� �ε����� ����. bake layer ���");
	}
	FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(0);
	loadFbxAnimation(animLayer);
}

void FbxLoader::writeXmlFile(const string& path, const string& fileName) const
{
	if (SUCCEEDED(::CoInitialize(nullptr)))
	{
		if(!_meshInfos.empty())
		{
			for (int i = 0; i < _meshInfos.size(); ++i)
			{
				XMLWriter xmlMeshGeometry("MeshInfo");
				
				writeXmlMesh(xmlMeshGeometry, _meshInfos[i], fileName);

				const string filePath = path + "/Asset/Mesh/" + fileName + " " + _meshInfos[i]._name + ".xml";
				xmlMeshGeometry.writeXmlFile(filePath);;
			}
			
		}
		if(!_materialsInfos.empty())
		{
			XMLWriter xmlMaterial("MaterialInfo");

			writeXmlMaterial(xmlMaterial);

			const string filePath = path + "/Asset/Material/" + fileName + ".xml";
			xmlMaterial.writeXmlFile(filePath);
		}
		if(!_boneOffsets.empty())
		{
			XMLWriter xmlSkeleton("BoneInfo");

			writeXmlSkeleton(xmlSkeleton);

			const string filePath = path + "/Asset/Skeleton/" + fileName + ".xml";
			xmlSkeleton.writeXmlFile(filePath);
		}
		if(!_animations.empty())
		{
			XMLWriter xmlAnimation("AnimationInfo");

			writeXmlAnimation(xmlAnimation);

			const string filePath = path + "/Asset/Animation/" + fileName + ".xml";
			xmlAnimation.writeXmlFile(filePath);
		}
		::CoUninitialize();
	}
}

void FbxLoader::getKeyFrames(FbxAnimLayer* animLayer, FbxNode* linkNode, std::vector<std::set<uint32_t>>& keyFramesTimes) const noexcept
{
	check(animLayer != nullptr, "�������Դϴ�.");
	check(linkNode != nullptr, "�������Դϴ�.");
	check(keyFramesTimes.empty(), "outValue�� ����־�� �մϴ�.");
	check(!_animTimeList.empty(), "animationTimelist�� ����ֽ��ϴ�.");

	keyFramesTimes.resize(_animTimeList.size());

	FbxAnimCurve* animTranslationXCurve = linkNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* animTranslationYCurve = linkNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* animTranslationZCurve = linkNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	FbxAnimCurve* animScalingXCurve = linkNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* animScalingYCurve = linkNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* animScalingZCurve = linkNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	FbxAnimCurve* animRotationXCurve = linkNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* animRotationYCurve = linkNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* animRotationZCurve = linkNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	if (animTranslationXCurve == nullptr) return;
	if (animTranslationYCurve == nullptr) return;
	if (animTranslationZCurve == nullptr) return;
	if (animScalingXCurve == nullptr) return;
	if (animScalingYCurve == nullptr) return;
	if (animScalingZCurve == nullptr) return;
	if (animRotationXCurve == nullptr) return;
	if (animRotationYCurve == nullptr) return;
	if (animRotationZCurve == nullptr) return;

	getKeyFrameTimes(animTranslationXCurve, keyFramesTimes);
	getKeyFrameTimes(animTranslationYCurve, keyFramesTimes);
	getKeyFrameTimes(animTranslationZCurve, keyFramesTimes);
	getKeyFrameTimes(animScalingXCurve, keyFramesTimes);
	getKeyFrameTimes(animScalingYCurve, keyFramesTimes);
	getKeyFrameTimes(animScalingZCurve, keyFramesTimes);
	getKeyFrameTimes(animRotationXCurve, keyFramesTimes);
	getKeyFrameTimes(animRotationYCurve, keyFramesTimes);
	getKeyFrameTimes(animRotationZCurve, keyFramesTimes);
}
		
void FbxLoader::loadFbxAnimation(FbxAnimLayer* animLayer)
{
	check(animLayer != nullptr, "�������Դϴ�.");
	check(!_animTimeList.empty(), "animation Time List�� ����ֽ��ϴ�.");

	std::vector<std::vector<BoneAnimation>> boneAnimations(_animTimeList.size());
	for (int i = 0; i < _animTimeList.size(); ++i)
	{
		boneAnimations[i].resize(_boneIndexMap.size());
	}

	//uint32_t endFrame = 0;
	for (const auto& bone : _boneIndexMap)
	{
		std::vector<std::set<uint32_t>> keyFramesTimes;

		getKeyFrames(animLayer, bone.first, keyFramesTimes);
		
		if (keyFramesTimes.empty())
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "keyFrame�� �����ϴ�.");
		}

		for (int i = 0; i < keyFramesTimes.size(); ++i)
		{
			const auto& clip = keyFramesTimes[i];
			std::vector<KeyFrame> keyFrames;
			keyFrames.reserve(clip.size());
			for (const auto& key : clip)
			{
				KeyFrame keyFrame;
				keyFrame._frame = key - (*clip.begin());

				// animation�� �ϳ��϶��� ���ư��� �ڵ�. �� �ִϸ��̼ǿ����� global transform�� �ʿ��ϴ�... [2/6/2021 qwerwy]
				FbxTime frameTime;
				frameTime.SetFrame(key);

				FbxAMatrix worldMatrix = bone.first->EvaluateGlobalTransform(frameTime);
				FbxNode* parentNode = getParentLinkNode(bone.first);

				if (parentNode != nullptr)
				{
					FbxAMatrix parentWorldMatrix = parentNode->EvaluateGlobalTransform(frameTime);
					worldMatrix = parentWorldMatrix.Inverse() * worldMatrix;
				}
				keyFrame._translation = fbxVector4ToXMFLOAT3(worldMatrix.GetT());
				keyFrame._rotationQuat = fbxQuaternionToXMFLOAT4(worldMatrix.GetQ());
				keyFrame._scaling = fbxVector4ToXMFLOAT3(worldMatrix.GetS());

				keyFrames.emplace_back(std::move(keyFrame));
			}
			boneAnimations[i][bone.second] = BoneAnimation(std::move(keyFrames));
		}
	}

	for (int i = 0; i < _animTimeList.size(); ++i)
	{
		check(_animTimeList[i]._start <= _animTimeList[i]._end, "�ð��� �����մϴ�.");
		uint32_t animEndTime = _animTimeList[i]._end - _animTimeList[i]._start;

		auto it = _animations.emplace(_animTimeList[i]._name, AnimationClip(std::move(boneAnimations[i]), animEndTime));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, "animationName�� ��Ĩ�ϴ�. " + _animTimeList[i]._name);
		}
	}	
}

void FbxLoader::writeXmlMesh(XMLWriter& xmlMeshGeometry, const FbxMeshInfo& meshInfo, const string& fileName) const
{
	check(!fileName.empty(), "fileName�� �����ϴ�.");
	check(!meshInfo._indices.empty(), "mesh vertex index�� ������ϴ�.");
	check(!meshInfo._skinnedVertices.empty() || !meshInfo._vertices.empty(), "vertex ����� ��� ������ϴ�.");

	xmlMeshGeometry.addAttribute("Name", meshInfo._name.c_str());
	bool isSkinned = !meshInfo._skinnedVertices.empty();
	xmlMeshGeometry.addAttribute("IsSkinned", isSkinned);

	int totalVertexCount = 0;
	int totalIndexCount = 0;

	for (int i = 0; i < meshInfo._indices.size(); ++i)
	{
		xmlMeshGeometry.addNode("SubMesh");
		xmlMeshGeometry.addAttribute("Name", _materialsInfos[i]._name.c_str());

		int vertexCount = isSkinned ? meshInfo._skinnedVertices[i].size()
			: meshInfo._vertices[i].size();

		xmlMeshGeometry.addAttribute("VertexCount", vertexCount);
		xmlMeshGeometry.addAttribute("IndexCount", meshInfo._indices[i].size());

		totalVertexCount += vertexCount;
		totalIndexCount += meshInfo._indices[i].size();

		xmlMeshGeometry.addAttribute("MaterialFile", fileName.c_str());
		xmlMeshGeometry.addAttribute("MaterialName", _materialsInfos[i]._name.c_str());

			
		xmlMeshGeometry.openChildNode();
		{
			xmlMeshGeometry.addNode("Vertices");
			xmlMeshGeometry.openChildNode();
			{
				if (isSkinned)
				{
					for (int j = 0; j < meshInfo._skinnedVertices[i].size(); ++j)
					{
						xmlMeshGeometry.addNode("Vertex");
						xmlMeshGeometry.addAttribute("Position", meshInfo._skinnedVertices[i][j]._position);

						xmlMeshGeometry.addAttribute("Normal", meshInfo._skinnedVertices[i][j]._normal);

						xmlMeshGeometry.addAttribute("TexCoord", meshInfo._skinnedVertices[i][j]._textureCoord);

						xmlMeshGeometry.addAttribute("Weight", meshInfo._skinnedVertices[i][j]._boneWeights);

						xmlMeshGeometry.addAttribute("BoneIndex", meshInfo._skinnedVertices[i][j]._boneIndices);
					}
				}
				else
				{
					for (int j = 0; j < meshInfo._vertices[i].size(); ++j)
					{
						xmlMeshGeometry.addNode("Vertex");
						xmlMeshGeometry.addAttribute("Position", meshInfo._vertices[i][j]._position);
							
						xmlMeshGeometry.addAttribute("Normal", meshInfo._vertices[i][j]._normal);

						xmlMeshGeometry.addAttribute("TexCoord", meshInfo._vertices[i][j]._textureCoord);
					}
				}
			}
			xmlMeshGeometry.closeChildNode();

			xmlMeshGeometry.addNode("Indices");
			xmlMeshGeometry.openChildNode();
			{
				for (int j = 0; j < meshInfo._indices[i].size(); j += 3)
				{
					xmlMeshGeometry.addNode("Index");
					xmlMeshGeometry.addAttribute("_0", meshInfo._indices[i][j]);
					xmlMeshGeometry.addAttribute("_1", meshInfo._indices[i][j + 1]);
					xmlMeshGeometry.addAttribute("_2", meshInfo._indices[i][j + 2]);
				}
			}
			xmlMeshGeometry.closeChildNode();
		}
		xmlMeshGeometry.closeChildNode();
	}

	xmlMeshGeometry.closeChildNode();
	xmlMeshGeometry.addAttribute("TotalVertexCount", totalVertexCount);
	xmlMeshGeometry.addAttribute("TotalIndexCount", totalIndexCount);
}

void FbxLoader::writeXmlMaterial(XMLWriter& xmlMaterial) const
{
	check(!_materialsInfos.empty(), "materialInfo�� ���� �ε��ؾ� �մϴ�.");

	for (int i = 0; i < _materialsInfos.size(); ++i)
	{
		xmlMaterial.addNode("Material");
		xmlMaterial.addAttribute("Name", _materialsInfos[i]._name.c_str());

		for (int j = 0; j < _materialsInfos[i]._useTextureNames.size(); ++j)
		{
			string attrName = "DiffuseTexture";
			if (j != 0)
			{
				attrName += j;
			}
			xmlMaterial.addAttribute(attrName, _materialsInfos[i]._useTextureNames[j].c_str());
		}
		xmlMaterial.addAttribute("DiffuseAlbedo", _materialsInfos[i]._diffuseAlbedo);
		xmlMaterial.addAttribute("FresnelR0", _materialsInfos[i]._fresnelR0);
		xmlMaterial.addAttribute("Roughness", _materialsInfos[i]._roughness);
	}
}

void FbxLoader::writeXmlSkeleton(XMLWriter& xmlSkeleton) const
{
	check(!_boneHierarchy.empty(), "boneHierarcy�� �ε���� �ʾҽ��ϴ�.");
	check(_boneHierarchy.size() == _boneOffsets.size(), "boneInfo�� ����� ���� �ٸ��ϴ�.");
	check(_boneHierarchy.size() == _boneNames.size(), "boneInfo�� ����� ���� �ٸ��ϴ�.");

	xmlSkeleton.addAttribute("Hierarchy", _boneHierarchy);

	for (int i = 0; i < _boneOffsets.size(); ++i)
	{
		xmlSkeleton.addNode("Bone");
		xmlSkeleton.addAttribute("Index", i);
		xmlSkeleton.addAttribute("Name", _boneNames[i].c_str());
		xmlSkeleton.addAttribute("Offset", _boneOffsets[i]);
	}
}

void FbxLoader::writeXmlAnimation(XMLWriter& xmlAnimation) const
{
	check(!_animations.empty(), "animation info�� ���� �ε�Ǿ�� �մϴ�.");

	for (const auto& anim : _animations)
	{
		xmlAnimation.addNode("Animation");
		xmlAnimation.addAttribute("Name", anim.first.c_str());
		xmlAnimation.addAttribute("Start", 0);
		xmlAnimation.addAttribute("End", anim.second.getClipEndFrame());
		xmlAnimation.openChildNode();
		{
			const std::vector<BoneAnimation>& boneAnimations = anim.second.getBoneAnimationXXX();
			for (int i = 0; i < boneAnimations.size(); ++i)
			{
				xmlAnimation.addNode("BoneAnimation");
				xmlAnimation.addAttribute("BoneIndex", i);

				xmlAnimation.openChildNode();
				{
					const std::vector<KeyFrame>& keyFrames = boneAnimations[i].getKeyFrameReferenceXXX();

					for (int j = 0; j < keyFrames.size(); ++j)
					{
						xmlAnimation.addNode("KeyFrame");
						xmlAnimation.addAttribute("Frame", keyFrames[j]._frame);

						xmlAnimation.addAttribute("Translation", keyFrames[j]._translation);
						xmlAnimation.addAttribute("Scaling", keyFrames[j]._scaling);
						xmlAnimation.addAttribute("RotationQuat", keyFrames[j]._rotationQuat);
					}
				}
				xmlAnimation.closeChildNode();
			}
		}
		xmlAnimation.closeChildNode();
	}
}

void FbxLoader::loadFbxParseInfo(const std::string& fbxFilePath)
{
	check(!fbxFilePath.empty(), "�������Դϴ�.");

	std::string animParseInfoPath = fbxFilePath.substr(0, fbxFilePath.size() - 4) + "_AnimParseInfo.txt";
	
	std::ifstream animParseInfo(animParseInfoPath);

	if (animParseInfo.is_open()) 
	{
		std::string s;
		while (animParseInfo.peek() != EOF) 
		{
			getline(animParseInfo, s);
			std::vector<std::string> timeSpan = D3DUtil::tokenizeString(s, ' ');
			if (timeSpan.size() != 3 && timeSpan.size() != 4)
			{
				ThrowErrCode(ErrCode::InvalidAnimationData, "animParseInfoPath ������ �߸��Ǿ����ϴ�.");
			}
			AnimClipTimeInfo timeInfo;
			timeInfo._name = timeSpan[2];
			timeInfo._start = std::stoi(timeSpan[0]);
			timeInfo._end = std::stoi(timeSpan[1]);
			if (timeInfo._end < timeInfo._start)
			{
				ThrowErrCode(ErrCode::InvalidAnimationData, "animation �ð��� �����մϴ�.");
			}
			timeInfo._isReverse = false;

			if (timeSpan.size() == 4)
			{
				if (timeSpan[3] == "reverse")
				{
					timeInfo._isReverse = true;
				}
			}
			_animTimeList.emplace_back(timeInfo);
		}
	}
}

// bone ������ ���ٸ� �����Ǿ�� �ҵ� [2/7/2021 qwerwy]
FbxNode* FbxLoader::getParentLinkNode(FbxNode* linkNode) const
{
	auto it = _boneIndexMap.find(linkNode);
	if (it == _boneIndexMap.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "link node�� boneIndexMap�� �����ϴ�.");
	}

	const BoneIndex boneIndex = it->second;
	if (boneIndex == 0)
	{
		// root node�� parent�� ����.
		return nullptr;
	}

	for (const auto& b : _boneIndexMap)
	{
		if (b.second == _boneHierarchy[boneIndex])
		{
			FbxNodeAttribute* parentNodeAttribute = b.first->GetNodeAttribute();
			if (parentNodeAttribute != nullptr && parentNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				return b.first;
			}
			else
			{
				ThrowErrCode(ErrCode::TypeIsDifferent, "boneIndex :" + std::to_string(boneIndex) + "�� parent�� skeleton type�� �ƴմϴ�.");
			}
		}
	}
	ThrowErrCode(ErrCode::NodeNotFound, "boneIndex :"+ std::to_string(boneIndex) + "�� parent�� ã�� ���߽��ϴ�.");
}

void FbxLoader::getKeyFrameTimes(const FbxAnimCurve* curve, std::vector<std::set<uint32_t>>& keyFrameTimes) const noexcept
{
	check(!_animTimeList.empty(), "anim Timelist�� ����ֽ��ϴ�.");
	check(curve != nullptr, "�������Դϴ�.");
	// ���ǹ��� keyFrame�� �����ϴ� �۾� �ʿ� [1/29/2021 qwerw]
	int keyCount = curve->KeyGetCount();
	if (keyCount == 0)
	{
		return;
	}
	uint32_t firstKey = curve->KeyGet(0).GetTime().GetFrameCount();
	// ���̳ʸ���ġ�� �����ϸ� ���ڴ� [2/19/2021 qwerwy]
	int animationIndex = 0;
	for (int i = 0; i < _animTimeList.size(); ++i)
	{
		if (_animTimeList[i]._start <= firstKey && firstKey <= _animTimeList[i]._end)
		{
			animationIndex = i;
			break;
		}
	}
	
	if (animationIndex == _animTimeList.size())
	{
		return;
	}

	for (int i = 0; i < _animTimeList.size(); ++i)
	{
		(void)keyFrameTimes[i].insert(_animTimeList[i]._start);
		(void)keyFrameTimes[i].insert(_animTimeList[i]._end);
	}

	for (int k = 0; k < keyCount; ++k)
	{
		FbxAnimCurveKey key = curve->KeyGet(k);
		FbxLongLong keyFrame = key.GetTime().GetFrameCount();

		if (_animTimeList[animationIndex]._end < keyFrame)
		{
			for (int i = animationIndex; i < _animTimeList.size(); ++i)
			{
				if (_animTimeList[i]._start <= keyFrame && keyFrame <= _animTimeList[i]._end)
				{
					animationIndex = i;
					break;
				}
			}
			if (animationIndex == _animTimeList.size())
			{
				break;
			}
		}
		
		(void)keyFrameTimes[animationIndex].insert(keyFrame);
	}
}

FbxLoader::FbxLoader(void)
	: _fbxManager(nullptr)
{
}

FbxLoader::~FbxLoader(void)
{
	if (_fbxManager != nullptr)
	{
		_fbxManager->Destroy();
	}
}

void FbxLoader::ConvertFbxFiles(const string& filePath, const string& objectFolderPath)
{
	_fbxManager = FbxManager::Create();
	FbxIOSettings* ioSettings = FbxIOSettings::Create(_fbxManager, IOSROOT);
	_fbxManager->SetIOSettings(ioSettings);

	FbxImporter* importer = FbxImporter::Create(_fbxManager, "");
	FbxScene* fbxScene = FbxScene::Create(_fbxManager, "");
	ErrCode rv = ErrCode::Success;
	for (auto& p : std::filesystem::recursive_directory_iterator(fbxFolderPath + filePath))
	{
		if (p.is_directory()) continue;
		const std::string& pathString = p.path().string();
		if(pathString.substr(pathString.length() - 4) != ".fbx") continue;
		bool success = importer->Initialize(p.path().string().c_str(), -1, _fbxManager->GetIOSettings());
		if (!success)
		{
			ThrowErrCode(ErrCode::PathNotFound, "path not found : " + p.path().string());
		}

		success = importer->Import(fbxScene);
		if (!success)
		{
			ThrowErrCode(ErrCode::InvalidFbxData, "Import fail");
		}
		
		loadFbxParseInfo(p.path().string());

		FbxAxisSystem max(FbxAxisSystem::DirectX);
		max.ConvertScene(fbxScene);

		FbxGeometryConverter converter(_fbxManager);
		success = converter.Triangulate(fbxScene, true);
		if (!success)
		{
			ThrowErrCode(ErrCode::TriangulateFail, "Triangulate fail");
		}
		
		FbxNode* rootNode = fbxScene->GetRootNode();
		if (rootNode == nullptr)
		{
			ThrowErrCode(ErrCode::InvalidFbxData, "GetRootNode fail");
		}

		bool nodeFound = false;
		loadFbxSkeletonNode(rootNode, nodeFound);
		
		loadFbxMaterial(fbxScene);
		
		loadFbxMeshNode(rootNode);

		loadFbxAnimations(fbxScene);

		const string& fileNameOrigin = p.path().filename().string();
		const string& fileName = fileNameOrigin.substr(0, fileNameOrigin.length() - 4);
		writeXmlFile(objectFolderPath, fileName);

		clearCachedFbxFileInfo();
	}

	fbxScene->Destroy(true);
	importer->Destroy(true);
}

void FbxLoader::loadFbxMeshNode(FbxNode* node)
{
	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		 loadFbxMesh(node);
	}

	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		FbxNode* childNode = node->GetChild(i);
		if (childNode == nullptr)
		{
			ThrowErrCode(ErrCode::InvalidFbxData, "�������Դϴ�.");
		}
		
		loadFbxMeshNode(childNode);
	}
}

void FbxLoader::loadFbxMesh(FbxNode* node)
{
	check(node != nullptr, "node is nullptr");
	check(node->GetNodeAttribute() != nullptr, "node attribute is nullptr");
	check(node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh, "Mesh Node���� üũ�ϰ� ���Ծ�� �մϴ�.");


	FbxMesh* mesh = node->GetMesh();
	if (mesh == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidFbxData, "mesh node�ε� mesh�� �����ϴ�.");
	}

	check(mesh->IsTriangleMesh(), "triangulate���� �ʾ���.");

	ErrCode rv = ErrCode::Success;
	std::vector<FbxPolygonVertexInfo> polygonVertices;
	loadFbxPolygons(mesh, polygonVertices);

	check(!polygonVertices.empty(), "loadFbxPolygon�� �����Ѱ��ΰ�?");

	std::vector<FbxVertexSkinningInfo> skinningInfos;
	if (mesh->GetDeformerCount() > 0)
	{
		loadFbxSkin(node, skinningInfos);
	}
	
	loadFbxOptimizedMesh(mesh, polygonVertices, skinningInfos);
}



void FbxLoader::loadFbxSkeleton(FbxNode* node, BoneIndex parentIndex)
{
	check(node != nullptr, "node�� nullptr�Դϴ�.");
	check((parentIndex != UNDEFINED_BONE_INDEX) || _boneHierarchy.empty(), "rootNode�� ù��°�� ���;� �մϴ�.");
	check((parentIndex != UNDEFINED_BONE_INDEX) || _boneIndexMap.empty(), "rootNode�� ù��°�� ���;� �մϴ�.");
	check((parentIndex == UNDEFINED_BONE_INDEX) || (parentIndex < _boneIndexMap.size()), "parent�� child���� ���� ���;� �մϴ�.");

	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		if (BONE_INDEX_MAX <= _boneHierarchy.size())
		{
			ThrowErrCode(ErrCode::Overflow, "Bone�� �ʹ� �����ϴ�.");
		}
		const BoneIndex myIndex = static_cast<BoneIndex>(_boneHierarchy.size());

		_boneHierarchy.emplace_back(parentIndex);
		_boneNames.emplace_back(node->GetName());
		auto it = _boneIndexMap.emplace(node, myIndex);
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, "Bone�� Node�� �ߺ����� ����ֽ��ϴ�!");
		}
		parentIndex = myIndex;
	}

	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		auto child = node->GetChild(i);
		check(child != nullptr, "child is null");

		loadFbxSkeleton(child, parentIndex);
	}
}

DirectX::XMFLOAT4X4 FbxLoader::fbxMatrixToXMFLOAT4X4(const FbxAMatrix& matrix) noexcept
{
	return DirectX::XMFLOAT4X4(matrix.Get(0, 0), matrix.Get(0, 1), matrix.Get(0, 2), matrix.Get(0, 3),
							   matrix.Get(1, 0), matrix.Get(1, 1), matrix.Get(1, 2), matrix.Get(1, 3),
							   matrix.Get(2, 0), matrix.Get(2, 1), matrix.Get(2, 2), matrix.Get(2, 3),
							   matrix.Get(3, 0), matrix.Get(3, 1), matrix.Get(3, 2), matrix.Get(3, 3));
}

FbxLoader::IndexMappingType FbxLoader::getIndexMappingType(const FbxLayerElement::EReferenceMode referenceMode, const FbxLayerElement::EMappingMode mappingMode)
{
	if (FbxLayerElement::eByControlPoint == mappingMode)
	{
		if (FbxLayerElement::eDirect == referenceMode)
		{
			return IndexMappingType::ControlPointIndex;
		}
		else if (FbxLayerElement::eIndexToDirect == referenceMode)
		{
			return IndexMappingType::IndexArrayAtControlPoint;
		}
	}
	else if (FbxLayerElement::eByPolygonVertex == mappingMode)
	{
		if (FbxLayerElement::eDirect == referenceMode)
		{
			return IndexMappingType::PolygonVertexIndex;
		}
		if (FbxLayerElement::eIndexToDirect == referenceMode)
		{
			return IndexMappingType::IndexArrayAtPolygonVertex;
		}
	}

	ThrowErrCode(ErrCode::UndefinedType, "uvIndex�� undefined���˴ϴ�.");
}

std::array<float, BONE_WEIGHT_COUNT - 1> FbxLoader::getWeight(const std::array<float, BONE_WEIGHT_COUNT>& weightArray) noexcept
{
	float totalWeight = 0.f;
	for (int i = 0; i < BONE_WEIGHT_COUNT; ++i)
	{
		totalWeight += weightArray[i];
	}
	if (totalWeight == 0.f)
	{
		std::array<float, BONE_WEIGHT_COUNT - 1> rv = { 0.f };
		return rv;
	}
	
	std::array<float, BONE_WEIGHT_COUNT - 1> rv;
	for (int i = 0; i < BONE_WEIGHT_COUNT - 1; ++i)
	{
		rv[i] = (weightArray[i] / totalWeight);
	}
	return rv;
}

// skeleton�� ���ϴ� �ϳ���� ������ [1/20/2021 qwerw]
void FbxLoader::loadFbxSkeletonNode(FbxNode* node, bool& nodeFound)
{
	check(node != nullptr, "�������Դϴ�.");
	
	nodeFound = false;
	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		const uint16_t expectedBoneCount = node->GetChildCount(true) + 1;
		_boneHierarchy.reserve(expectedBoneCount);
		_boneIndexMap.reserve(expectedBoneCount);
		_boneOffsets.reserve(expectedBoneCount);
		_boneNames.reserve(expectedBoneCount);

		nodeFound = true;

		loadFbxSkeleton(node, UNDEFINED_BONE_INDEX);

		return;
	}

	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		FbxNode* childNode = node->GetChild(i);
		check(childNode != nullptr, "childeNode is null.");

		bool endRecursion = false;
		loadFbxSkeletonNode(childNode, endRecursion);
		if (endRecursion)
		{
			break;
		}
	}
}

void FbxLoader::loadFbxMaterial(FbxScene* node)
{
	check(_materialsInfos.empty(), "materialInfo�� load�Ǳ� ���� clear�Ǿ���� �մϴ�.");
	if (node->GetMaterialCount() == 0)
	{
		// material�� ������ mesh load���� material Index�� ��� �Ǵ°���?? [2/6/2021 qwerwy]
		return;
	}
	
	const int materialCount = node->GetMaterialCount();

	_materialsInfos.reserve(materialCount);

	for (int i = 0; i < materialCount; ++i)
	{
		const FbxSurfaceMaterial* material = node->GetMaterial(i);
		if (!material->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			ThrowErrCode(ErrCode::UndefinedType, "������� ���� Ÿ���Դϴ�.");
		}
		
		const FbxSurfacePhong* phong = static_cast<const FbxSurfacePhong*>(material);
		const FbxDouble3& diffuse = phong->Diffuse.Get();
		const FbxDouble3& fresnelR0 = phong->Reflection.Get(); // Ȯ�� �ʿ� [1/13/2021 qwerw]
		const float shininess = phong->Shininess.Get();
		
		_materialsInfos.emplace_back(phong->GetName(),
			DirectX::XMFLOAT4(diffuse.mData[0], diffuse.mData[1], diffuse.mData[2], 1.f),
			DirectX::XMFLOAT3(fresnelR0.mData[0], fresnelR0.mData[1], fresnelR0.mData[2]),
			1.f - shininess);

		const FbxProperty diffuseProperty = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (diffuseProperty.IsValid())
		{
			int textureCount = diffuseProperty.GetSrcObjectCount<FbxFileTexture>();
			_materialsInfos[i]._useTextureNames.resize(textureCount);
			for (int t = 0; t < textureCount; ++t)
			{
				const FbxFileTexture* texture = diffuseProperty.GetSrcObject<FbxFileTexture>(t);
				if (texture != nullptr)
				{
					const string textureName = texture->GetRelativeFileName();
					_materialsInfos[i]._useTextureNames[t] = textureName.substr(0, textureName.size() - 4);
				}
			}
		}
	}
}

FbxLoader::FbxVertexSkinningInfo::FbxVertexSkinningInfo() noexcept
	: _weight{ 0.f }
{
	for (auto& b : _boneIndex)
	{
		b = UNDEFINED_BONE_INDEX;
	}
}

void FbxLoader::FbxVertexSkinningInfo::insert(uint16_t boneIndex, float weight) noexcept
{
	if (_weight[BONE_WEIGHT_COUNT - 1] >= weight)
	{
		return;
	}
	_weight[BONE_WEIGHT_COUNT - 1] = weight;
	_boneIndex[BONE_WEIGHT_COUNT - 1] = boneIndex;

	for (int i = BONE_WEIGHT_COUNT - 2; i >= 0; --i)
	{
		if (_weight[i] < weight)
		{
			_weight[i + 1] = _weight[i];
			_boneIndex[i + 1] = _boneIndex[i];

			_weight[i] = weight;
			_boneIndex[i] = boneIndex;
		}
		else
		{
			break;
		}
	}
}

FbxLoader::FbxMeshInfo::FbxMeshInfo(string&& name,
	vector<vector<Vertex>>&& vertices,
	vector<vector<SkinnedVertex>>&& skinnedVertices,
	vector<vector<GeoIndex>>&& indices)
	: _name(name)
	, _vertices(vertices)
	, _skinnedVertices(skinnedVertices)
	, _indices(indices)
{

}
