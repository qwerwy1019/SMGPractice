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
		ThrowErrCode(ErrCode::InvalidVertexInfo, "mesh 노드에 vertex가 없을 수 있을까?");
	}
	polygonVertices.reserve(polygonCount * VERTEX_PER_POLYGON);

	// 멀티레이어를 고려하지 않기로 한다. [12/31/2020 qwerw]
	const FbxLayerElementMaterial* layerMaterials = mesh->GetLayer(0)->GetMaterials();
	if (layerMaterials == nullptr)
	{
		ThrowErrCode(ErrCode::MaterialNotFound, "material을 안쓰는 메쉬가 있나...?");
	}

	const FbxLayerElementUV* layerUvs = mesh->GetLayer(0)->GetUVs(FbxLayerElement::eTextureDiffuse);
	if (layerUvs == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidTextureInfo, "TextureUV 데이터가 이상합니다.");
	}
	const IndexMappingType uvIndexType = getIndexMappingType(layerUvs->GetReferenceMode(), layerUvs->GetMappingMode());

	const FbxLayerElementNormal* layerNomrals = mesh->GetLayer(0)->GetNormals();
	if (layerNomrals == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidNormalData, "Normal 데이터가 이상합니다.");
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
					ThrowErrCode(ErrCode::UndefinedType, "타입이 추가되었나?");
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
					ThrowErrCode(ErrCode::UndefinedType, "타입이 추가되었나?");
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
					polygonVertexInfo._colorIndex = UNDEFINED_COMMON_INDEX;
					break;
				default:
					ThrowErrCode(ErrCode::UndefinedType, "타입이 추가되었나?");
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
	check(mesh != nullptr, "비정상입니다.");
	check(!polygonVertices.empty(), "polygonVertex 데이터가 없습니다.");

	const int controlPointCount = mesh->GetControlPointsCount();
	check(skinningInfos.empty() || controlPointCount == skinningInfos.size(), "skinning info와 controlPoint 데이터가 일치하지 않습니다.");

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
		Index16 material;
		Index16 controlPoint;
		Index16 uv;
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

				// uv좌표 y가 뒤집어져있음. 확인 필요 [1/15/2021 qwerw]
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

				// uv좌표 y가 뒤집어져있음 [1/15/2021 qwerw]
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
	check(node != nullptr, "비정상입니다.");
	check(skinningInfos.empty(), "비정상입니다.");
	check(node->GetMesh()->GetDeformerCount() != 0, "애니메이션이 없는 노드입니다.");
	check(!_boneIndexMap.empty(), "boneNode-index map이 먼저 생성되어야 합니다.");
	check(!_boneHierarchy.empty(), "boneHierarchy가 먼저 생성되어야 합니다.");
	check(!_boneNames.empty(), "boneNames가 먼저 로드되어야 합니다.");
	check(_boneHierarchy.size() == _boneIndexMap.size(), "boneInfo가 제대로 안만들어졌습니다.");
	check(_boneHierarchy.size() == _boneNames.size(), "boneInfo가 제대로 안만들어졌습니다.");

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
			ThrowErrCode(ErrCode::UndefinedType, "fbxClster link mode가 normalize가 아닌 상황이 고려되지 않았습니다.");
		}
		for (int ci = 0; ci < clusterCount; ++ci)
		{
			FbxCluster* fbxCluster = fbxSkin->GetCluster(ci);

			if (fbxCluster == nullptr)
			{
				ThrowErrCode(ErrCode::InvalidFbxData, "비정상입니다.");
			}

			if (linkMode != fbxCluster->GetLinkMode())
			{
				ThrowErrCode(ErrCode::TypeIsDifferent, "link mode가 서로다른 상황이 고려되지 않았습니다.");
			}

			FbxNode* linkNode = fbxCluster->GetLink();
			auto boneIndex = _boneIndexMap.find(linkNode);
			if (boneIndex == _boneIndexMap.end())
			{
				ThrowErrCode(ErrCode::InvalidFbxData, "boneIndex map에 없는 노드입니다.");
			}

			FbxAMatrix transform, linkTransform;
			(void)fbxCluster->GetTransformMatrix(transform);
			(void)fbxCluster->GetTransformLinkMatrix(linkTransform);

			const DirectX::XMFLOAT4X4 matBoneOffsetMatrix = fbxMatrixToXMFLOAT4X4(linkTransform.Inverse() * transform * geometricTransform);
			check(MathHelper::equal(_boneOffsets[boneIndex->second], MathHelper::Identity4x4), "값이 덮어써집니다.");
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
	
}

void FbxLoader::loadFbxAnimations(FbxScene* fbxScene)
{
	const int animationStackCount = fbxScene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < animationStackCount; ++i)
	{
		FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(i);
		const int animationLayerCount = animStack->GetMemberCount<FbxAnimLayer>();
		for (int j = 0; j < animationLayerCount; ++j)
		{
			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(j);
			const std::string& name = animLayer->GetName();
			if (name.empty())
			{
				ThrowErrCode(ErrCode::NodeNameNotFound, "애니메이션 이름이 비정상입니다.");
			}
			loadKeyFrame(animLayer, name);
		}
	}
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

void FbxLoader::getKeyFrames(FbxAnimLayer* animLayer, FbxNode* linkNode, std::set<FbxTime>& keyFramesTimes) const noexcept
{
	check(animLayer != nullptr, "비정상입니다.");
	check(linkNode != nullptr, "비정상입니다.");
	check(keyFramesTimes.empty(), "outValue는 비어있어야 합니다.");

	FbxAnimEvaluator* animEvaluator = linkNode->GetAnimationEvaluator();

	FbxVector4& localTranslate = linkNode->EvaluateLocalTranslation();
	FbxVector4& localRotation = linkNode->EvaluateLocalRotation();
	FbxVector4& localScale = linkNode->EvaluateLocalScaling();


	FbxAnimCurve* animTranslateXCurve = linkNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* animTranslateYCurve = linkNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* animTranslateZCurve = linkNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	FbxAnimCurve* animScaleXCurve = linkNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* animScaleYCurve = linkNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* animScaleZCurve = linkNode->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	FbxAnimCurve* animRotationXCurve = linkNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* animRotationYCurve = linkNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* animRotationZCurve = linkNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	if (animTranslateXCurve == nullptr) return;
	if (animTranslateYCurve == nullptr) return;
	if (animTranslateZCurve == nullptr) return;
	if (animScaleXCurve == nullptr) return;
	if (animScaleYCurve == nullptr) return;
	if (animScaleZCurve == nullptr) return;
	if (animRotationXCurve == nullptr) return;
	if (animRotationYCurve == nullptr) return;
	if (animRotationZCurve == nullptr) return;

	getKeyFrameTimes(animTranslateXCurve, keyFramesTimes);
	getKeyFrameTimes(animTranslateYCurve, keyFramesTimes);
	getKeyFrameTimes(animTranslateZCurve, keyFramesTimes);
	getKeyFrameTimes(animScaleXCurve, keyFramesTimes);
	getKeyFrameTimes(animScaleYCurve, keyFramesTimes);
	getKeyFrameTimes(animScaleZCurve, keyFramesTimes);
	getKeyFrameTimes(animRotationXCurve, keyFramesTimes);
	getKeyFrameTimes(animRotationYCurve, keyFramesTimes);
	getKeyFrameTimes(animRotationZCurve, keyFramesTimes);
}
		
void FbxLoader::loadKeyFrame(FbxAnimLayer* animLayer, const string& animationName)
{
	check(animLayer != nullptr, "비정상입니다.");
	check(!animationName.empty(), "이름이 없습니다.");

	std::vector<BoneAnimation> boneAnimations(_boneIndexMap.size());

	float endTime = 0.f;
	for (const auto& bone : _boneIndexMap)
	{
		std::set<FbxTime> keyFramesTimes;
		getKeyFrames(animLayer, bone.first, keyFramesTimes);
		
		if (keyFramesTimes.empty())
		{
			ThrowErrCode(ErrCode::InvalidAnimationData, "keyFrame이 없습니다.");
		}

		std::vector<KeyFrame> keyFrames;
		keyFrames.reserve(keyFramesTimes.size());
		for (const auto& t : keyFramesTimes)
		{
			KeyFrame keyFrame;
			keyFrame._timePos = t.GetFrameCount() / FPS_f;
			
			// animation이 하나일때만 돌아가는 코드. 각 애니메이션에서의 global transform이 필요하다... [2/6/2021 qwerwy]
			FbxAMatrix worldMatrix = bone.first->EvaluateGlobalTransform(t);
			FbxNode* parentNode = getParentLinkNode(bone.first);

			if (parentNode != nullptr)
			{
				FbxAMatrix parentWorldMatrix = parentNode->EvaluateGlobalTransform(t);
				worldMatrix = parentWorldMatrix.Inverse() * worldMatrix;
			}
			keyFrame._translation = fbxVector4ToXMFLOAT3(worldMatrix.GetT());
			keyFrame._rotationQuat = fbxQuaternionToXMFLOAT4(worldMatrix.GetQ());
			keyFrame._scale = fbxVector4ToXMFLOAT3(worldMatrix.GetS());

			keyFrames.emplace_back(std::move(keyFrame));
		}
		endTime = keyFrames.back()._timePos;

		boneAnimations[bone.second] = BoneAnimation(std::move(keyFrames));
	}

	auto it = _animations.emplace(animationName, AnimationClip(std::move(boneAnimations), endTime));
	if (it.second == false)
	{
		ThrowErrCode(ErrCode::KeyDuplicated, "animationName이 겹칩니다.");
	}
}

void FbxLoader::writeXmlMesh(XMLWriter& xmlMeshGeometry, const FbxMeshInfo& meshInfo, const string& fileName) const
{
	check(!fileName.empty(), "fileName이 없습니다.");
	check(!meshInfo._indices.empty(), "mesh vertex index가 비었습니다.");
	check(!meshInfo._skinnedVertices.empty() || !meshInfo._vertices.empty(), "vertex 목록이 모두 비었습니다.");

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
	check(!_materialsInfos.empty(), "materialInfo를 먼저 로드해야 합니다.");

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
	check(!_boneHierarchy.empty(), "boneHierarcy가 로드되지 않았습니다.");
	check(_boneHierarchy.size() == _boneOffsets.size(), "boneInfo의 사이즈가 서로 다릅니다.");
	check(_boneHierarchy.size() == _boneNames.size(), "boneInfo의 사이즈가 서로 다릅니다.");

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
	check(!_animations.empty(), "animation info가 먼저 로드되어야 합니다.");

	for (const auto& anim : _animations)
	{
		xmlAnimation.addNode("Animation");
		xmlAnimation.addAttribute("Name", anim.first.c_str());
		xmlAnimation.addAttribute("StartTime", 0.f);
		xmlAnimation.addAttribute("EndTime", anim.second.getClipEndTime());
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
						xmlAnimation.addAttribute("Time", keyFrames[j]._timePos);

						xmlAnimation.addAttribute("Translation", keyFrames[j]._translation);
						xmlAnimation.addAttribute("Scale", keyFrames[j]._scale);
						xmlAnimation.addAttribute("RotationQuat", keyFrames[j]._rotationQuat);
					}
				}
				xmlAnimation.closeChildNode();
			}
		}
		xmlAnimation.closeChildNode();
	}
}

// bone 갯수가 많다면 개선되어야 할듯 [2/7/2021 qwerwy]
FbxNode* FbxLoader::getParentLinkNode(FbxNode* linkNode) const
{
	auto it = _boneIndexMap.find(linkNode);
	if (it == _boneIndexMap.end())
	{
		ThrowErrCode(ErrCode::NodeNotFound, "link node가 boneIndexMap에 없습니다.");
	}

	const BoneIndex boneIndex = it->second;
	if (boneIndex == 0)
	{
		// root node는 parent가 없다.
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
				ThrowErrCode(ErrCode::TypeIsDifferent, "boneIndex :" + std::to_string(boneIndex) + "의 parent가 skeleton type이 아닙니다.");
			}
		}
	}
	ThrowErrCode(ErrCode::NodeNotFound, "boneIndex :"+ std::to_string(boneIndex) + "의 parent를 찾지 못했습니다.");
}

void FbxLoader::getKeyFrameTimes(const FbxAnimCurve* curve, std::set<FbxTime>& keyFrameTimes) noexcept
{
	check(curve != nullptr, "비정상입니다.");
	// 무의미한 keyFrame은 제거하는 작업 필요 [1/29/2021 qwerw]
	int keyCount = curve->KeyGetCount();
	if (keyCount == 0)
	{
		return;
	}

	(void)keyFrameTimes.insert(curve->KeyGet(0).GetTime());
	float lastValue = curve->KeyGet(0).GetValue();
	for (int k = 1; k < keyCount; ++k)
	{
		FbxAnimCurveKey key = curve->KeyGet(k);
		float value = key.GetValue();
		if (std::abs(value - lastValue) < FBXSDK_FLOAT_EPSILON)
		{
			continue;
		}
		lastValue = value;
		FbxTime time = key.GetTime();
		(void)keyFrameTimes.insert(time);
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
		
// 		FbxAxisSystem axisSystem = fbxScene->GetGlobalSettings().GetAxisSystem();
// 
// 		if (axisSystem != FbxAxisSystem::DirectX)
// 		{
// 			FbxAxisSystem directX(FbxAxisSystem::DirectX);
// 			directX.ConvertScene(fbxScene);
// 			check(fbxScene->GetGlobalSettings().GetAxisSystem() == FbxAxisSystem::DirectX, "좌표계 변환이 안됨.");
// 		}
		int dir;
		FbxAxisSystem::EUpVector upvector;
		upvector = fbxScene->GetGlobalSettings().GetAxisSystem().GetUpVector(dir);

		FbxAxisSystem max(FbxAxisSystem::DirectX);
		max.ConvertScene(fbxScene);

		upvector = fbxScene->GetGlobalSettings().GetAxisSystem().GetUpVector(dir);

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
			ThrowErrCode(ErrCode::InvalidFbxData, "비정상입니다.");
		}
		
		loadFbxMeshNode(childNode);
	}
}

void FbxLoader::loadFbxMesh(FbxNode* node)
{
	check(node != nullptr, "node is nullptr");
	check(node->GetNodeAttribute() != nullptr, "node attribute is nullptr");
	check(node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh, "Mesh Node인지 체크하고 들어왔어야 합니다.");


	FbxMesh* mesh = node->GetMesh();
	if (mesh == nullptr)
	{
		ThrowErrCode(ErrCode::InvalidFbxData, "mesh node인데 mesh가 없습니다.");
	}

	check(mesh->IsTriangleMesh(), "triangulate되지 않았음.");

	ErrCode rv = ErrCode::Success;
	std::vector<FbxPolygonVertexInfo> polygonVertices;
	loadFbxPolygons(mesh, polygonVertices);

	check(!polygonVertices.empty(), "loadFbxPolygon이 실패한것인가?");

	std::vector<FbxVertexSkinningInfo> skinningInfos;
	if (mesh->GetDeformerCount() > 0)
	{
		loadFbxSkin(node, skinningInfos);
	}
	
	loadFbxOptimizedMesh(mesh, polygonVertices, skinningInfos);
}



void FbxLoader::loadFbxSkeleton(FbxNode* node, BoneIndex parentIndex)
{
	check(node != nullptr, "node가 nullptr입니다.");
	check((parentIndex != UNDEFINED_BONE_INDEX) || _boneHierarchy.empty(), "rootNode가 첫번째로 들어와야 합니다.");
	check((parentIndex != UNDEFINED_BONE_INDEX) || _boneIndexMap.empty(), "rootNode가 첫번째로 들어와야 합니다.");
	check((parentIndex == UNDEFINED_BONE_INDEX) || (parentIndex < _boneIndexMap.size()), "parent가 child보다 먼저 들어와야 합니다.");

	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		if (BONE_INDEX_MAX <= _boneHierarchy.size())
		{
			ThrowErrCode(ErrCode::Overflow, "Bone이 너무 많습니다.");
		}
		const BoneIndex myIndex = static_cast<BoneIndex>(_boneHierarchy.size());

		_boneHierarchy.emplace_back(parentIndex);
		_boneNames.emplace_back(node->GetName());
		auto it = _boneIndexMap.emplace(node, myIndex);
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, "Bone에 Node가 중복으로 들어있습니다!");
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

	ThrowErrCode(ErrCode::UndefinedType, "uvIndex가 undefined가됩니다.");
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

// skeleton은 파일당 하나라고 가정함 [1/20/2021 qwerw]
void FbxLoader::loadFbxSkeletonNode(FbxNode* node, bool& nodeFound)
{
	check(node != nullptr, "비정상입니다.");
	
	nodeFound = false;
	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		const Index16 expectedBoneCount = node->GetChildCount(true) + 1;
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
	check(_materialsInfos.empty(), "materialInfo가 load되기 위해 clear되었어야 합니다.");
	if (node->GetMaterialCount() == 0)
	{
		// material이 없으면 mesh load에서 material Index는 어떻게 되는거지?? [2/6/2021 qwerwy]
		return;
	}
	
	const int materialCount = node->GetMaterialCount();

	_materialsInfos.reserve(materialCount);

	for (int i = 0; i < materialCount; ++i)
	{
		const FbxSurfaceMaterial* material = node->GetMaterial(i);
		if (!material->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			ThrowErrCode(ErrCode::UndefinedType, "고려되지 않은 타입입니다.");
		}
		
		const FbxSurfacePhong* phong = static_cast<const FbxSurfacePhong*>(material);
		const FbxDouble3& diffuse = phong->Diffuse.Get();
		const FbxDouble3& fresnelR0 = phong->Reflection.Get(); // 확인 필요 [1/13/2021 qwerw]
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

void FbxLoader::FbxVertexSkinningInfo::insert(Index16 boneIndex, float weight) noexcept
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
