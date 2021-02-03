#include "FbxLoader.h"
#include <filesystem>
#include <cassert>
#include "SMGEngine/SkinnedData.h"
#include "SMGEngine/MathHelper.h"
#include <fbxsdk/core/math/fbxquaternion.h>
#include <windows.h>
#include <msxml.h>
#include "SMGEngine/D3DUtil.h"
#include "SMGEngine/FileHelper.h"
#include <set>

HRESULT FbxLoader::loadFbxPolygons(FbxMesh* mesh, std::vector<FbxPolygonVertexInfo>& polygonVertices) const noexcept
{
	const int polygonCount = mesh->GetPolygonCount();
	polygonVertices.reserve(polygonCount * VERTEX_PER_POLYGON);

	// 멀티레이어를 고려하지 않기로 한다. [12/31/2020 qwerw]
	const FbxLayerElementMaterial* layerMaterials = mesh->GetLayer(0)->GetMaterials();
	assert(layerMaterials != nullptr && L"material을 안쓰는 메쉬가 있나...?");

	const FbxLayerElementUV* layerUvs = mesh->GetLayer(0)->GetUVs(FbxLayerElement::eTextureDiffuse);
	assert(nullptr != layerUvs);
	const IndexMappingType uvIndexType = getUVIndexType(layerUvs->GetReferenceMode(), layerUvs->GetMappingMode());

	const FbxLayerElementNormal* layerNomrals = mesh->GetLayer(0)->GetNormals();
	assert(layerNomrals != nullptr);
	const IndexMappingType normalIndexType = getUVIndexType(layerNomrals->GetReferenceMode(), layerNomrals->GetMappingMode());

	const FbxLayerElementVertexColor* layerColors = mesh->GetLayer(0)->GetVertexColors();
	const IndexMappingType colorIndexType =
		(layerColors != nullptr) ? getUVIndexType(layerColors->GetReferenceMode(), layerColors->GetMappingMode()) : IndexMappingType::Undefined;

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
					polygonVertexInfo._uvIndex = UNDEFINED_INDEX;
					break;
				default:
					assert(false && L"타입이 추가되었나?");
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
					polygonVertexInfo._normalIndex = UNDEFINED_INDEX;
					break;
				default:
					assert(false && L"타입이 추가되었나?");
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
					polygonVertexInfo._colorIndex = UNDEFINED_INDEX;
					break;
				default:
					assert(false && L"타입이 추가되었나?");
					static_assert(static_cast<int>(IndexMappingType::Undefined) == 4);
					break;
			}

			polygonVertices.push_back(polygonVertexInfo);
		}
	}
	return S_OK;
}

HRESULT FbxLoader::loadFbxOptimizedMesh(const FbxMesh* mesh,
										const std::vector<FbxPolygonVertexInfo>& polygonVertices,
										const std::vector<FbxVertexSkinningInfo>& skinningInfos)
{
	const int controlPointCount = mesh->GetControlPointsCount();
	std::vector<DirectX::XMFLOAT3> controlPoints(controlPointCount);
	for (int i = 0; i < controlPointCount; ++i)
	{
		controlPoints[i] = fbxVector4ToXMFLOAT3(mesh->GetControlPointAt(i));
	}
	bool isSkinningMesh = !skinningInfos.empty();
	vector<vector<SkinnedVertex>> skinnedVertices(isSkinningMesh ? _materialsInfos.size() : 0);
	vector<vector<Vertex>> vertices(isSkinningMesh ? 0 : _materialsInfos.size());
	vector<vector<Index>> indices(_materialsInfos.size());

	const FbxLayerElementUV* layerUvs = mesh->GetLayer(0)->GetUVs(FbxLayerElement::eTextureDiffuse);
	assert(layerUvs != nullptr);

	const FbxLayerElementNormal* layerNomrals = mesh->GetLayer(0)->GetNormals();
	assert(layerNomrals != nullptr);

	struct VertexKey
	{
		CommonIndex material;
		CommonIndex controlPoint;
		CommonIndex uv;
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
	unordered_map<VertexKey, Index, VertexKeyHasher> indexMap;

	for (const auto& pv : polygonVertices)
	{
		if (pv._materialIndex >= _materialsInfos.size())
		{
			assert(false);
			return E_FAIL;
		}
		VertexKey key = { pv._materialIndex, pv._controlPointIndex, pv._uvIndex };
		auto it = indexMap.find(key);
		if (it != indexMap.end())
		{
			// vector reserve 이전에 해줘야할듯 [1/8/2021 qwerw]
			indices[pv._materialIndex].push_back(it->second);
		}
		else
		{
			if (isSkinningMesh)
			{
				SkinnedVertex v(controlPoints[pv._controlPointIndex],
					fbxVector4ToXMFLOAT3(layerNomrals->GetDirectArray().GetAt(pv._normalIndex)),
					fbxVector2ToXMFLOAT2(layerUvs->GetDirectArray().GetAt(pv._uvIndex)),
					getWeight(skinningInfos[pv._controlPointIndex]._weight),
					skinningInfos[pv._controlPointIndex]._boneIndex);
				// uv좌표 y가 뒤집어져있음. 확인 필요 [1/15/2021 qwerw]
				v._textureCoord.y *= -1;

				vector<SkinnedVertex>& subMeshVertices = skinnedVertices[pv._materialIndex];
				const Index index = subMeshVertices.size();
				indices[pv._materialIndex].push_back(index);
				indexMap.emplace(make_pair(key, index));
				subMeshVertices.push_back(v);
			}
			else
			{
				Vertex v(controlPoints[pv._controlPointIndex],
					fbxVector4ToXMFLOAT3(layerNomrals->GetDirectArray().GetAt(pv._normalIndex)),
					fbxVector2ToXMFLOAT2(layerUvs->GetDirectArray().GetAt(pv._uvIndex)));

				// uv좌표 y가 뒤집어져있음 [1/15/2021 qwerw]
				v._textureCoord.y *= -1;

				vector<Vertex>& subMeshVertices = vertices[pv._materialIndex];
				const Index index = subMeshVertices.size();
				indices[pv._materialIndex].push_back(index);
				indexMap.emplace(make_pair(key, index));
				subMeshVertices.push_back(v);
			}
		}
	}

	FbxMeshInfo info(mesh->GetNode()->GetName(), std::move(vertices), std::move(skinnedVertices), std::move(indices));

	_meshInfos.emplace_back(std::move(info));
	
	return S_OK;
}

HRESULT FbxLoader::loadFbxSkin(FbxNode* node, std::vector<FbxVertexSkinningInfo>& skinningInfos)
{
	assert(node != nullptr);
	assert(skinningInfos.empty());
	assert(node->GetMesh()->GetDeformerCount() != 0);
	assert(!_boneIndexMap.empty());
	assert(!_boneHierarchy.empty());
	assert(!_boneNames.empty());
	assert(_boneHierarchy.size() == _boneIndexMap.size());
	assert(_boneHierarchy.size() == _boneNames.size());

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
			assert(false);
			return E_FAIL;
		}
		for (int ci = 0; ci < clusterCount; ++ci)
		{
			FbxCluster* fbxCluster = fbxSkin->GetCluster(ci);

			if (fbxCluster == nullptr)
			{
				assert(false);
				return E_FAIL;
			}

			if (linkMode != fbxCluster->GetLinkMode())
			{
				assert(false);
				return E_FAIL;
			}

			FbxNode* linkNode = fbxCluster->GetLink();
			auto boneIndex = _boneIndexMap.find(linkNode);
			if (boneIndex == _boneIndexMap.end())
			{
				assert(false);
				return E_FAIL;
			}

			FbxAMatrix transform, linkTransform;
			(void)fbxCluster->GetTransformMatrix(transform);
			(void)fbxCluster->GetTransformLinkMatrix(linkTransform);

			const DirectX::XMFLOAT4X4 matBoneOffsetMatrix = fbxMatrixToXMFLOAT4X4(linkTransform.Inverse() * transform * geometricTransform);
			assert(MathHelper::equal(_boneOffsets[boneIndex->second], MathHelper::Identity4x4)
				|| MathHelper::equal(_boneOffsets[boneIndex->second], matBoneOffsetMatrix));
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
	return S_OK;
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

HRESULT FbxLoader::loadFbxAnimations(FbxScene* fbxScene) noexcept
{
	const int animationStackCount = fbxScene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < animationStackCount; ++i)
	{
		FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(i);
		const int animationLayerCount = animStack->GetMemberCount<FbxAnimLayer>();
		for (int j = 0; j < animationLayerCount; ++j)
		{
			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(j);

			HRESULT rv = loadKeyFrame(animLayer, animLayer->GetName());
			if (FAILED(rv))
			{
				return rv;
			}
		}
	}
	
	return S_OK;
}

HRESULT FbxLoader::writeXmlFile(const string& path, const string& fileName) const
{
	if (SUCCEEDED(::CoInitialize(nullptr)))
	{
		if(!_meshInfos.empty())
		{
			for (int i = 0; i < _meshInfos.size(); ++i)
			{
				XMLWriter xmlMeshGeometry;
				HRESULT rv = xmlMeshGeometry.createDocument("MeshInfo");
				if (FAILED(rv))
				{
					assert(false);
					return rv;
				}
				rv = writeXmlMesh(xmlMeshGeometry, i, fileName);
				if (FAILED(rv))
				{
					assert(false);
					return rv;
				}
				const string filePath = path + "/Asset/Mesh/" + fileName + " " + _meshInfos[i]._name + ".xml";
				rv = xmlMeshGeometry.writeXmlFile(filePath);
				if (FAILED(rv))
				{
					assert(false);
					return rv;
				}
			}
			
		}
		if(!_materialsInfos.empty())
		{
			XMLWriter xmlMaterial;
			HRESULT rv = xmlMaterial.createDocument("MaterialInfo");
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
			rv = writeXmlMaterial(xmlMaterial);
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
			const string filePath = path + "/Asset/Material/" + fileName + ".xml";
			rv = xmlMaterial.writeXmlFile(filePath);
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
		}
		if(!_boneOffsets.empty())
		{
			XMLWriter xmlSkeleton;
			HRESULT rv = xmlSkeleton.createDocument("BoneInfo");
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
			rv = writeXmlSkeleton(xmlSkeleton);
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
			const string filePath = path + "/Asset/Skeleton/" + fileName + ".xml";
			rv = xmlSkeleton.writeXmlFile(filePath);
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
		}
		if(!_animations.empty())
		{
			XMLWriter xmlAnimation;
			HRESULT rv = xmlAnimation.createDocument("AnimationInfo");
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
			rv = writeXmlAnimation(xmlAnimation);
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
			const string filePath = path + "/Asset/Animation/" + fileName + ".xml";
			rv = xmlAnimation.writeXmlFile(filePath);
			if (FAILED(rv))
			{
				assert(false);
				return rv;
			}
		}
		::CoUninitialize();
	}
	return S_OK;
}

HRESULT FbxLoader::getKeyFrames(FbxAnimLayer* animLayer, FbxNode* linkNode, std::set<FbxTime>& keyFramesTimes)
{
	assert(animLayer != nullptr);
	assert(linkNode != nullptr);
	assert(keyFramesTimes.empty());

	for (const auto& bone : _boneIndexMap)
	{
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

		if (animTranslateXCurve == nullptr) continue;
		if (animTranslateYCurve == nullptr) continue;
		if (animTranslateZCurve == nullptr) continue;
		if (animScaleXCurve == nullptr) continue;
		if (animScaleYCurve == nullptr) continue;
		if (animScaleZCurve == nullptr) continue;
		if (animRotationXCurve == nullptr) continue;
		if (animRotationYCurve == nullptr) continue;
		if (animRotationZCurve == nullptr) continue;

		//1. vector에 때려넣고 정렬한다. <- 900*9 개를 정렬해야함...
		//2. 같은 프레임이면 map에 넣는다. <- diff값은 구하기가 힘든 상황인데...-> 어차피 key를 생략하기 이전이라면 diff값을 넣을수 있지않나?->그럼 언제 어떻게 최적화할지?->diff는 안될듯...
		//3. vector 9개를 유지한다......
		// 결국 runtime에 interpolate를 하듯이, 그런 방식으로 keyFrame에서의 값을 구해야 할것같은...?
		// keyFrameTime만 순서대로 저장을 한 후에, 그때의 transformFormation을 찍어내야할듯
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
	return S_OK;
}
		
HRESULT FbxLoader::loadKeyFrame(FbxAnimLayer* animLayer, const string& animationName)
{
	auto it = _animations.emplace(animationName, AnimationClip(_boneIndexMap.size()));
	if (it.second == false)
	{
		assert(false && L"animationName이 겹칩니다.");
	}

	float endTime = 0.f;
	for (const auto& bone : _boneIndexMap)
	{
		std::set<FbxTime> keyFramesTimes;
		getKeyFrames(animLayer, bone.first, keyFramesTimes);
		
		if (keyFramesTimes.empty())
		{
			continue;
		}

		vector<KeyFrame> keyFrames;
		keyFrames.reserve(keyFramesTimes.size());
		for (const auto& t : keyFramesTimes)
		{
			KeyFrame keyFrame;
			keyFrame._timePos = t.GetFrameCount() / FPS_f;
			//FbxQuaternion quaternion;
			//quaternion.ComposeSphericalXYZ(eulerRotation);
			
			FbxAMatrix worldMatrix = bone.first->EvaluateGlobalTransform(t, FbxNode::eDestinationPivot);
			FbxNode* parentNode = getParentLinkNode(bone.first, bone.second);
			if (parentNode != nullptr)
			{
				FbxAMatrix parentWorldMatrix = parentNode->EvaluateGlobalTransform(t, FbxNode::eDestinationPivot);
				worldMatrix = parentWorldMatrix.Inverse() * worldMatrix;
			}
			keyFrame._translation = fbxVector4ToXMFLOAT3(worldMatrix.GetT());
			keyFrame._rotationQuat = fbxQuaternionToXMFLOAT4(worldMatrix.GetQ());
			keyFrame._scale = fbxVector4ToXMFLOAT3(worldMatrix.GetS());

			keyFrames.emplace_back(std::move(keyFrame));
		}
		endTime = keyFrames.back()._timePos;

		it.first->second.setBoneAnimationXXX(bone.second, BoneAnimation(std::move(keyFrames)));
	}
	it.first->second.setClipEndTimeXXX(endTime);
	return S_OK;
}

HRESULT FbxLoader::writeXmlMesh(XMLWriter& xmlMeshGeometry, int meshIndex, const string& fileName) const noexcept
{
	assert(!_meshInfos.empty());
	assert(meshIndex < _meshInfos.size());

	HRESULT rv;
	ReturnIfFailed(xmlMeshGeometry.addAttribute("Name", _meshInfos[meshIndex]._name.c_str()));
	bool isSkinned = !_meshInfos[meshIndex]._skinnedVertices.empty();
	ReturnIfFailed(xmlMeshGeometry.addAttribute("IsSkinned", isSkinned));

	int totalVertexCount = 0;
	int totalIndexCount = 0;

	for (int i = 0; i < _meshInfos[meshIndex]._indices.size(); ++i)
	{
		ReturnIfFailed(xmlMeshGeometry.addNode("SubMesh"));
		ReturnIfFailed(xmlMeshGeometry.addAttribute("Name", _materialsInfos[i]._name.c_str()));
		int vertexCount = isSkinned ? _meshInfos[meshIndex]._skinnedVertices[i].size()
			: _meshInfos[meshIndex]._vertices[i].size();
		ReturnIfFailed(xmlMeshGeometry.addAttribute("VertexCount", vertexCount));
		ReturnIfFailed(xmlMeshGeometry.addAttribute("IndexCount", _meshInfos[meshIndex]._indices[i].size()));
		totalVertexCount += vertexCount;
		totalIndexCount += _meshInfos[meshIndex]._indices[i].size();

		ReturnIfFailed(xmlMeshGeometry.addAttribute("MaterialFile", fileName.c_str()));
		ReturnIfFailed(xmlMeshGeometry.addAttribute("MaterialName", _materialsInfos[i]._name.c_str()));

			
		ReturnIfFailed(xmlMeshGeometry.openChildNode());
		{
			ReturnIfFailed(xmlMeshGeometry.addNode("Vertices"));
			ReturnIfFailed(xmlMeshGeometry.openChildNode());
			{
				if (isSkinned)
				{
					for (int j = 0; j < _meshInfos[meshIndex]._skinnedVertices[i].size(); ++j)
					{
						ReturnIfFailed(xmlMeshGeometry.addNode("Vertex"));
						ReturnIfFailed(xmlMeshGeometry.addAttribute("Position", _meshInfos[meshIndex]._skinnedVertices[i][j]._position));

						ReturnIfFailed(xmlMeshGeometry.addAttribute("Normal", _meshInfos[meshIndex]._skinnedVertices[i][j]._normal));

						ReturnIfFailed(xmlMeshGeometry.addAttribute("TexCoord", _meshInfos[meshIndex]._skinnedVertices[i][j]._textureCoord));

						ReturnIfFailed(xmlMeshGeometry.addAttribute("Weight", _meshInfos[meshIndex]._skinnedVertices[i][j]._boneWeights));

						ReturnIfFailed(xmlMeshGeometry.addAttribute("BoneIndex", _meshInfos[meshIndex]._skinnedVertices[i][j]._boneIndices));
					}
				}
				else
				{
					for (int j = 0; j < _meshInfos[meshIndex]._vertices[i].size(); ++j)
					{
						ReturnIfFailed(xmlMeshGeometry.addNode("Vertex"));
						ReturnIfFailed(xmlMeshGeometry.addAttribute("Position", _meshInfos[meshIndex]._vertices[i][j]._position));
							
						ReturnIfFailed(xmlMeshGeometry.addAttribute("Normal", _meshInfos[meshIndex]._vertices[i][j]._normal));

						ReturnIfFailed(xmlMeshGeometry.addAttribute("TexCoord", _meshInfos[meshIndex]._vertices[i][j]._textureCoord));
					}
				}
			}
			ReturnIfFailed(xmlMeshGeometry.closeChildNode());

			ReturnIfFailed(xmlMeshGeometry.addNode("Indices"));
			ReturnIfFailed(xmlMeshGeometry.openChildNode());
			{
				for (int j = 0; j < _meshInfos[meshIndex]._indices[i].size(); j += 3)
				{
					ReturnIfFailed(xmlMeshGeometry.addNode("Index"));
					ReturnIfFailed(xmlMeshGeometry.addAttribute("_0", _meshInfos[meshIndex]._indices[i][j]));
					ReturnIfFailed(xmlMeshGeometry.addAttribute("_1", _meshInfos[meshIndex]._indices[i][j + 1]));
					ReturnIfFailed(xmlMeshGeometry.addAttribute("_2", _meshInfos[meshIndex]._indices[i][j + 2]));
				}
			}
			ReturnIfFailed(xmlMeshGeometry.closeChildNode());
		}
		ReturnIfFailed(xmlMeshGeometry.closeChildNode());
	}

	ReturnIfFailed(xmlMeshGeometry.closeChildNode());
	ReturnIfFailed(xmlMeshGeometry.addAttribute("TotalVertexCount", totalVertexCount));
	ReturnIfFailed(xmlMeshGeometry.addAttribute("TotalIndexCount", totalIndexCount));

	return S_OK;
}

HRESULT FbxLoader::writeXmlMaterial(XMLWriter& xmlMaterial) const noexcept
{
	assert(!_materialsInfos.empty());

	for (int i = 0; i < _materialsInfos.size(); ++i)
	{
		ReturnIfFailed(xmlMaterial.addNode("Material"));
		ReturnIfFailed(xmlMaterial.addAttribute("Name", _materialsInfos[i]._name.c_str()));

		//ReturnIfFailed(xmlMaterial.addNode("Texture"));
		//ReturnIfFailed(xmlMaterial.openChildNode());
		{
			for (int j = 0; j < _materialsInfos[i]._useTextureNames.size(); ++j)
			{
				string attrName = "DiffuseTexture";
				if (j != 0)
				{
					attrName += j;
				}
				ReturnIfFailed(xmlMaterial.addAttribute(attrName, _materialsInfos[i]._useTextureNames[j].c_str()));
			}
		}
		//ReturnIfFailed(xmlMaterial.closeChildNode());
		ReturnIfFailed(xmlMaterial.addAttribute("DiffuseAlbedo", _materialsInfos[i]._diffuseAlbedo));
		ReturnIfFailed(xmlMaterial.addAttribute("FresnelR0", _materialsInfos[i]._fresnelR0));
		ReturnIfFailed(xmlMaterial.addAttribute("Roughness", _materialsInfos[i]._roughness));
	}

	return S_OK;
}

HRESULT FbxLoader::writeXmlSkeleton(XMLWriter& xmlSkeleton) const noexcept
{
	assert(!_boneHierarchy.empty());
	assert(_boneHierarchy.size() == _boneOffsets.size());
	assert(_boneHierarchy.size() == _boneNames.size());
	ReturnIfFailed(xmlSkeleton.addAttribute("Hierarchy", _boneHierarchy));

	for (int i = 0; i < _boneOffsets.size(); ++i)
	{
		ReturnIfFailed(xmlSkeleton.addNode("Bone"));
		ReturnIfFailed(xmlSkeleton.addAttribute("Index", i));
		ReturnIfFailed(xmlSkeleton.addAttribute("Name", _boneNames[i].c_str()));
		ReturnIfFailed(xmlSkeleton.addAttribute("Offset", _boneOffsets[i]));
	}
	return S_OK;
}

HRESULT FbxLoader::writeXmlAnimation(XMLWriter& xmlAnimation) const noexcept
{
	assert(!_animations.empty());
	for (const auto& anim : _animations)
	{
		ReturnIfFailed(xmlAnimation.addNode("Animation"));
		ReturnIfFailed(xmlAnimation.addAttribute("Name", anim.first.c_str()));
		ReturnIfFailed(xmlAnimation.addAttribute("StartTime", 0.f));
		ReturnIfFailed(xmlAnimation.addAttribute("EndTime", anim.second.getClipEndTime()));
		ReturnIfFailed(xmlAnimation.openChildNode());
		{
			const std::vector<BoneAnimation>& boneAnimations = anim.second.getBoneAnimationXXX();
			for (int i = 0; i < boneAnimations.size(); ++i)
			{
				ReturnIfFailed(xmlAnimation.addNode("BoneAnimation"));
				ReturnIfFailed(xmlAnimation.addAttribute("BoneIndex", i));

				ReturnIfFailed(xmlAnimation.openChildNode());
				{
					const std::vector<KeyFrame>& keyFrames = boneAnimations[i].getKeyFrameReferenceXXX();

					for (int j = 0; j < keyFrames.size(); ++j)
					{
						ReturnIfFailed(xmlAnimation.addNode("KeyFrame"));
						ReturnIfFailed(xmlAnimation.addAttribute("Time", keyFrames[j]._timePos));
						//ReturnIfFailed(xmlAnimation.addAttribute("Transform", keyFrames[j]._transform));
						ReturnIfFailed(xmlAnimation.addAttribute("Translation", keyFrames[j]._translation));
						ReturnIfFailed(xmlAnimation.addAttribute("Scale", keyFrames[j]._scale));
						ReturnIfFailed(xmlAnimation.addAttribute("RotationQuat", keyFrames[j]._rotationQuat));
					}
				}
				ReturnIfFailed(xmlAnimation.closeChildNode());
			}
		}
		ReturnIfFailed(xmlAnimation.closeChildNode());
	}
	return S_OK;
}

FbxNode* FbxLoader::getParentLinkNode(FbxNode* linkNode, int boneIndex) const noexcept
{
	if (boneIndex == 0)
	{
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
				return nullptr;
			}
		}
	}
	assert(false);
	return nullptr;
}

void FbxLoader::getKeyFrameTimes(const FbxAnimCurve* curve, std::set<FbxTime>& keyFrameTimes) noexcept
{
	assert(curve != nullptr);
	// 무의미한 keyFrame은 제거하는 작업 필요 [1/29/2021 qwerw]
	int keyCount = curve->KeyGetCount();
	if (keyCount == 0)
	{
		return;
	}
	// 임시방편 [2/3/2021 qwerw]
	float lastValue = curve->KeyGet(0).GetValue() + 100;
	for (int k = 0; k < keyCount; ++k)
	{
		FbxAnimCurveKey key = curve->KeyGet(k);
		float value = key.GetValue();
		if (std::abs(value - lastValue) < FBXSDK_FLOAT_EPSILON)
		{
			continue;
		}
		lastValue = value;
		FbxTime time = key.GetTime();
		keyFrameTimes.insert(time);
	}
}

DirectX::XMFLOAT4 FbxLoader::EulerVectorToQuaternion(float x, float y, float z) noexcept
{
	FbxVector4 euler(x, y, z);
	FbxQuaternion quaternion;
	quaternion.ComposeSphericalXYZ(euler);

	return DirectX::XMFLOAT4(quaternion.GetAt(0), quaternion.GetAt(1), quaternion.GetAt(2), quaternion.GetAt(3));

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

HRESULT FbxLoader::LoadFbxFiles(const string& filePath, const string& objectFolderPath)
{
	_fbxManager = FbxManager::Create();
	FbxIOSettings* ioSettings = FbxIOSettings::Create(_fbxManager, IOSROOT);
	_fbxManager->SetIOSettings(ioSettings);
	//(*(_fbxManager->GetIOSettings())).SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	FbxImporter* importer = FbxImporter::Create(_fbxManager, "");
	FbxScene* fbxScene = FbxScene::Create(_fbxManager, "");
	bool success = false;
	HRESULT rv = S_OK;
	for (auto& p : std::filesystem::recursive_directory_iterator(fbxFolderPath + filePath))
	{
		if (p.is_directory()) continue;

		success = importer->Initialize(p.path().string().c_str(), -1, _fbxManager->GetIOSettings());
		if (!success)
		{
			return E_FAIL;
		}

		success = importer->Import(fbxScene);
		if (!success)
		{
			return E_FAIL;
		}
		
		int dir;
		FbxAxisSystem::EUpVector upvector;
		upvector = fbxScene->GetGlobalSettings().GetAxisSystem().GetUpVector(dir);

		FbxAxisSystem max(FbxAxisSystem::DirectX);
		max.ConvertScene(fbxScene);

		upvector = fbxScene->GetGlobalSettings().GetAxisSystem().GetUpVector(dir);

		FbxGeometryConverter converter(_fbxManager);
		(void)converter.Triangulate(fbxScene, true);
		
		FbxNode* rootNode = fbxScene->GetRootNode();
		if (rootNode == nullptr)
		{
			return E_FAIL;
		}

		bool nodeFound = false;
		rv = loadFbxSkeletonNode(rootNode, nodeFound);
		if (FAILED(rv))
		{
			assert(false && L"fbx skeleton load fail");
			return E_FAIL;
		}
		
		rv = loadFbxMaterial(fbxScene);
		if (FAILED(rv))
		{
			assert(false && L"fbx material load fail");
			return rv;
		}
		rv = loadFbxMeshNode(rootNode);
		if (FAILED(rv))
		{
			assert(false && L"fbx mesh load fail");
			return E_FAIL;
		}

		rv = loadFbxAnimations(fbxScene);
		if (FAILED(rv))
		{
			assert(false && L"fbx animation load fail");
			return E_FAIL;
		}

		const string& fileNameOrigin = p.path().filename().string();
		const string& fileName = fileNameOrigin.substr(0, fileNameOrigin.length() - 4);
		rv = writeXmlFile(objectFolderPath, fileName);
		if (FAILED(rv))
		{
			assert(false && L"write xml failed");
			return E_FAIL;
		}
		clearCachedFbxFileInfo();
	}

	fbxScene->Destroy(true);
	importer->Destroy(true);
	return S_OK;
}

HRESULT FbxLoader::loadFbxMeshNode(FbxNode* node)
{
	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
	{
		HRESULT rv = loadFbxMesh(node);
		if (FAILED(rv))
		{
			return rv;
		}
	}

	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		FbxNode* childNode = node->GetChild(i);
		if (childNode == nullptr)
		{
			return E_FAIL;
		}
		HRESULT rv = loadFbxMeshNode(childNode);
		if (FAILED(rv))
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

HRESULT FbxLoader::loadFbxMesh(FbxNode* node)
{
	assert(node != nullptr);
	assert(node->GetNodeAttribute() != nullptr);
	assert(node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh);


	FbxMesh* mesh = node->GetMesh();
	assert(mesh != nullptr);
	assert(mesh->IsTriangleMesh() && L"triangulate되지 않았음.");

	HRESULT rv = S_OK;
	std::vector<FbxPolygonVertexInfo> polygonVertices;
	rv = loadFbxPolygons(mesh, polygonVertices);
	if (FAILED(rv))
	{
		return rv;
	}

	std::vector<FbxVertexSkinningInfo> skinningInfos;
	if (mesh->GetDeformerCount() > 0)
	{
		rv = loadFbxSkin(node, skinningInfos);
		if (FAILED(rv))
		{
			return rv;
		}
	}
	
	rv = loadFbxOptimizedMesh(mesh, polygonVertices, skinningInfos);
	if (FAILED(rv))
	{
		return rv;
	}
	
	return S_OK;
}



HRESULT FbxLoader::loadFbxSkeleton(FbxNode* node, CommonIndex parentIndex)
{
	assert(node != nullptr);
	assert(parentIndex != UNDEFINED_COMMON_INDEX || _boneHierarchy.empty());
	assert(parentIndex != UNDEFINED_COMMON_INDEX || _boneIndexMap.empty());
	assert(parentIndex == UNDEFINED_COMMON_INDEX || parentIndex < _boneIndexMap.size());

	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		const CommonIndex myIndex = _boneHierarchy.size();

		_boneHierarchy.push_back(parentIndex);
		_boneNames.emplace_back(node->GetName());
		auto it = _boneIndexMap.emplace(node, myIndex);
		if (it.second == false)
		{
			assert(false && L"Bone에 Node가 중복으로 들어있습니다!");
			return E_FAIL;
		}
		parentIndex = myIndex;
	}

	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		auto child = node->GetChild(i);
		if (child == nullptr)
		{
			return E_FAIL;
		}
		loadFbxSkeleton(child, parentIndex);
	}
	return S_OK;
}

DirectX::XMFLOAT4X4 FbxLoader::fbxMatrixToXMFLOAT4X4(const FbxAMatrix& matrix) noexcept
{
	return DirectX::XMFLOAT4X4(matrix.Get(0, 0), matrix.Get(0, 1), matrix.Get(0, 2), matrix.Get(0, 3),
							   matrix.Get(1, 0), matrix.Get(1, 1), matrix.Get(1, 2), matrix.Get(1, 3),
							   matrix.Get(2, 0), matrix.Get(2, 1), matrix.Get(2, 2), matrix.Get(2, 3),
							   matrix.Get(3, 0), matrix.Get(3, 1), matrix.Get(3, 2), matrix.Get(3, 3));
}

FbxLoader::IndexMappingType FbxLoader::getUVIndexType(const FbxLayerElement::EReferenceMode referenceMode, const FbxLayerElement::EMappingMode mappingMode) noexcept
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

	assert(false && L"uvIndex가 0이됩니다.");
	return IndexMappingType::Undefined;
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
HRESULT FbxLoader::loadFbxSkeletonNode(FbxNode* node, bool& nodeFound)
{
	nodeFound = false;
	if (node->GetNodeAttribute() != nullptr &&
		node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		const CommonIndex boneCount = node->GetChildCount(true) + 1;
		_boneHierarchy.reserve(boneCount);
		_boneIndexMap.reserve(boneCount);
		_boneOffsets.reserve(boneCount);
		_boneNames.reserve(boneCount);
		HRESULT rv = loadFbxSkeleton(node, UNDEFINED_COMMON_INDEX);
		nodeFound = true;
		return rv;
	}

	const int childCount = node->GetChildCount();
	for (int i = 0; i < childCount; ++i)
	{
		FbxNode* childNode = node->GetChild(i);
		if (childNode == nullptr)
		{
			return E_FAIL;
		}
		bool endRecursion = false;
		HRESULT rv = loadFbxSkeletonNode(childNode, endRecursion);
		if (FAILED(rv))
		{
			return E_FAIL;
		}
		if (endRecursion)
		{
			break;
		}
	}

	return S_OK;
}

HRESULT FbxLoader::loadFbxMaterial(FbxScene* node)
{
	if (node->GetMaterialCount() == 0)
	{
		return S_OK;
	}
	
	assert(_materialsInfos.empty());

	const int materialCount = node->GetMaterialCount();

	_materialsInfos.reserve(materialCount);

	for (int i = 0; i < materialCount; ++i)
	{
		const FbxSurfaceMaterial* material = node->GetMaterial(i);
		if (!material->GetClassId().Is(FbxSurfacePhong::ClassId))
		{
			assert(false);
			continue;
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

	return S_OK;
}

FbxLoader::FbxVertexSkinningInfo::FbxVertexSkinningInfo() noexcept
	: _weight{ 0.f }
{
	for (auto& b : _boneIndex)
	{
		b = UNDEFINED_BONE_INDEX;
	}
}

void FbxLoader::FbxVertexSkinningInfo::insert(CommonIndex boneIndex, float weight) noexcept
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
	vector<vector<Index>>&& indices)
	: _name(name)
	, _vertices(vertices)
	, _skinnedVertices(skinnedVertices)
	, _indices(indices)
{

}
