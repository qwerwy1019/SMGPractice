#include "stdafx.h"
#include "Terrain.h"
#include "Exception.h"
#include "StageInfo.h"
#include "SMGFramework.h"
#include "D3DApp.h"
#include "MathHelper.h"


bool Terrain::isGround(void) const noexcept
{
	return _isGround;
}

bool Terrain::isWall(void) const noexcept
{
	return _isWall;
}

Terrain::Terrain(const TerrainObjectInfo& terrainInfo)
{
	_isGround = terrainInfo._isGround;
	_isWall = terrainInfo._isWall;

	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(terrainInfo._objectFileName);

	MathHelper::getWorldMatrix(terrainInfo._position,
		terrainInfo._direction,
		terrainInfo._upVector,
		terrainInfo._size,
		_gameObject->_worldMatrix);
	_gameObject->_dirtyFrames = FRAME_RESOURCE_COUNT;
#if defined DEBUG | defined _DEBUG
	SMGFramework::getD3DApp()->createGameObjectDev(_gameObject);
#endif

	if (_isGround || _isWall)
	{
		makeAABBTree();
	}
}

float Terrain::checkCollision(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& moveVector, float size) const noexcept
{
	check(MathHelper::equal(MathHelper::lengthSq(moveVector), 0) == false);
	check(size >= 0.f);
	
	XMVECTOR start = XMLoadFloat3(&position);
	XMVECTOR end = XMLoadFloat3(&moveVector);
	end += XMVector3Normalize(end) * size + start;

	XMMATRIX inverseMatrix = XMLoadFloat4x4(&_gameObject->_worldMatrix);
	inverseMatrix = XMMatrixInverse(nullptr, inverseMatrix);
	start = XMVector3Transform(start, inverseMatrix);
	end = XMVector3Transform(end, inverseMatrix);

	XMFLOAT3 startF, endF;
	XMStoreFloat3(&startF, start);
	XMStoreFloat3(&endF, end);

	XMVECTOR minV = XMLoadFloat3(&_min);
	XMVECTOR maxV = XMLoadFloat3(&_max);

	float collisionTime = checkCollisionXXX(_aabbNodes.size() - 1, startF, endF, minV, maxV);
	float moveVectorLength = MathHelper::length(moveVector);
	float adjustedCollisionTime = (collisionTime * (size + moveVectorLength) - size) / moveVectorLength;

	check(adjustedCollisionTime < 1.05);
	return adjustedCollisionTime;
}

void Terrain::makeAABBTree(void)
{
	check(_gameObject != nullptr);

	const MeshGeometry* mesh = nullptr;
	int totalIndexCount = 0;
	for (const auto& renderItem : _gameObject->_renderItems)
	{
		if (mesh != nullptr && mesh != renderItem->_geometry)
		{
			ThrowErrCode(ErrCode::InvalidXmlData, "�ٸ� mesh�� ���� terrain���� ����Ϸ��� ���� �۾� �ʿ���");
		}
		mesh = renderItem->_geometry;
		totalIndexCount += renderItem->getSubMesh()._indexCount;
	}
	_meshGeometry = mesh;

	std::vector<TerrainAABBNode::DataType::Leaf> terrainLeafList(totalIndexCount / 3);
	int i = 0;
	for (const auto& renderItem : _gameObject->_renderItems)
	{
		const auto& subMesh = renderItem->getSubMesh();
		auto subMeshIndex = renderItem->_subMeshIndex;
		for (int j = 0; j < subMesh._indexCount; j += 3)
		{
			terrainLeafList[i]._subMeshIndex = subMeshIndex;
			terrainLeafList[i]._index = j;
			++i;
		}
	}

	size_t vertexCount;
	_vertexBuffer = mesh->getVertexBufferXXX(vertexCount);
	_indexBuffer = mesh->getIndexBufferXXX();

	const auto& subMesh = _meshGeometry->_subMeshList[terrainLeafList[0]._subMeshIndex];
	_min = getVertexFromLeafNode(terrainLeafList[0], 0)._position;
	_max = _min;
	for (int i = 0; i < terrainLeafList.size(); ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			const Vertex& vertex = getVertexFromLeafNode(terrainLeafList[i], j);

			_min.x = std::min(_min.x, vertex._position.x);
			_min.y = std::min(_min.y, vertex._position.y);
			_min.z = std::min(_min.z, vertex._position.z);

			_max.x = std::max(_max.x, vertex._position.x);
			_max.y = std::max(_max.y, vertex._position.y);
			_max.z = std::max(_max.z, vertex._position.z);
		}
	}

	XMVECTOR min = XMLoadFloat3(&_min);
	XMVECTOR max = XMLoadFloat3(&_max);
	// reserve ũ�� üũ�ؾ��� [6/24/2021 qwerw]
	_aabbNodes.reserve(vertexCount);
	makeAABBTreeXXX(terrainLeafList, 0, terrainLeafList.size(), min, max);
}

uint16_t XM_CALLCONV Terrain::makeAABBTreeXXX(std::vector<TerrainAABBNode::DataType::Leaf>& terrainLeafList,
	int begin,
	int end,
	FXMVECTOR min,
	FXMVECTOR max)
{
	check(begin < end);
	check(_vertexBuffer != nullptr);
	check(_indexBuffer != nullptr);
	check(0 <= begin && begin < terrainLeafList.size());
	check(0 < end && end <= terrainLeafList.size());

	TerrainAABBNode node;
	if (end - begin == 1)
	{
		node._children[0] = std::numeric_limits<uint16_t>::max();
		node._children[1] = std::numeric_limits<uint16_t>::max();
		node._data._leaf = terrainLeafList[begin];
	}
	else
	{
		node._data._node = getAABBRange(terrainLeafList, begin, end, min, max);

		XMVECTOR minXyz = DirectX::XMVectorSet(node._data._node._minX,
			node._data._node._minY,
			node._data._node._minZ,
			0);
		XMVECTOR maxXyz = DirectX::XMVectorSet(node._data._node._maxX,
			node._data._node._maxY,
			node._data._node._maxZ,
			0);

		XMVECTOR nextMin = min + (max - min) * minXyz / static_cast<float>(std::numeric_limits<uint8_t>::max());
		XMVECTOR nextMax = min + (max - min) * maxXyz / static_cast<float>(std::numeric_limits<uint8_t>::max());

		uint8_t xSize = node._data._node._maxX - node._data._node._minX;
		uint8_t ySize = node._data._node._maxY - node._data._node._minY;
		uint8_t zSize = node._data._node._maxZ - node._data._node._minZ;

		DivideType divideType;
		if (xSize > ySize)
		{
			if (xSize > zSize)
			{
				divideType = DivideType::X;
			}
			else
			{
				divideType = DivideType::Z;
			}
		}
		else
		{
			if (ySize > zSize)
			{
				divideType = DivideType::Y;
			}
			else
			{
				divideType = DivideType::Z;
			}
		}

		sortIndexList(terrainLeafList, begin, end, divideType);
		int mid = (begin + end) / 2;

		node._children[0] = makeAABBTreeXXX(terrainLeafList, begin, mid, nextMin, nextMax);
		node._children[1] = makeAABBTreeXXX(terrainLeafList, mid, end, nextMin, nextMax);
	}
	_aabbNodes.push_back(node);

	if (_aabbNodes.size() - 1 >= std::numeric_limits<uint16_t>::max())
	{
		ThrowErrCode(ErrCode::Overflow, "������ �ٲ����");
	}
	return (_aabbNodes.size() - 1);
}

const Vertex& Terrain::getVertexFromLeafNode(const TerrainAABBNode::DataType::Leaf& leafNode, const int offset) const noexcept
{
	const auto& subMesh = _meshGeometry->_subMeshList[leafNode._subMeshIndex];
	return _vertexBuffer[subMesh._baseVertexLoaction + _indexBuffer[subMesh._baseIndexLoacation + leafNode._index + offset]];
}

void Terrain::sortIndexList(std::vector<TerrainAABBNode::DataType::Leaf>& terrainIndexList, int begin, int end, DivideType divideType) const noexcept
{
	check(begin <= end);
	check(_vertexBuffer != nullptr);
	check(_indexBuffer != nullptr);
	check(terrainIndexList.empty() == false);

	if (end - begin <= 1)
	{
		return;
	}

	int i = begin + 1;
	int j = end - 1;

	// indexList�� ��ǥ�� ���� ���ĵ� ���¿��� ù��° ���ڰ� pivot���� �������� ����. [6/25/2021 qwerw]
	// merge sort�� �ٲٴ°� ����غ��� ��. [6/25/2021 qwerw]
	std::swap(terrainIndexList[begin], terrainIndexList[(begin + end) / 2]);

	float pivotValue = sortCompareValue(terrainIndexList[begin], divideType);
	while (true)
	{
		while (i < end && sortCompareValue(terrainIndexList[i], divideType) <= pivotValue)
		{
			i++;
		}
		while (j > begin && sortCompareValue(terrainIndexList[j], divideType) >= pivotValue)
		{
			j--;
		}

		if (i <= j)
		{
			std::swap(terrainIndexList[i], terrainIndexList[j]);
		}
		else
		{
			std::swap(terrainIndexList[begin], terrainIndexList[j]);
			break;
		}
	}
	sortIndexList(terrainIndexList, begin, j, divideType);
	sortIndexList(terrainIndexList, j + 1, end, divideType);
}

float Terrain::sortCompareValue(const TerrainAABBNode::DataType::Leaf& index, DivideType type) const noexcept
{
	check(_vertexBuffer != nullptr);
	check(_indexBuffer != nullptr);

	switch (type)
	{
		case DivideType::X:
			return getVertexFromLeafNode(index, 0)._position.x +
				getVertexFromLeafNode(index, 1)._position.x +
				getVertexFromLeafNode(index, 2)._position.x;
			break;
		case DivideType::Y:
			return getVertexFromLeafNode(index, 0)._position.y +
				getVertexFromLeafNode(index, 1)._position.y +
				getVertexFromLeafNode(index, 2)._position.y;
			break;
		case DivideType::Z:
			return getVertexFromLeafNode(index, 0)._position.z +
				getVertexFromLeafNode(index, 1)._position.z +
				getVertexFromLeafNode(index, 2)._position.z;
			break;
		default:
			check(false);
			break;
	}
	return 0.f;
}

TerrainAABBNode::DataType::Node XM_CALLCONV Terrain::getAABBRange(std::vector<TerrainAABBNode::DataType::Leaf>& terrainIndexList,
	int begin,
	int end,
	FXMVECTOR min,
	FXMVECTOR max) const noexcept
{
	check(begin < end);
	check(_vertexBuffer != nullptr);
	check(_indexBuffer != nullptr);
	check(0 <= begin && begin < terrainIndexList.size());
	check(0 < end && end <= terrainIndexList.size());

	TerrainAABBNode::DataType::Node aabbRange;
	aabbRange._minX = std::numeric_limits<uint8_t>::max();
	aabbRange._minY = std::numeric_limits<uint8_t>::max();
	aabbRange._minZ = std::numeric_limits<uint8_t>::max();
	aabbRange._maxX = 0;
	aabbRange._maxY = 0;
	aabbRange._maxZ = 0;

	for (int i = begin; i < end; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			const Vertex& vertex = getVertexFromLeafNode(terrainIndexList[i], j);
			XMVECTOR position = XMLoadFloat3(&vertex._position);
			position = (position - min) * std::numeric_limits<uint8_t>::max() / (max - min);

			XMFLOAT3 positionF;
			XMStoreFloat3(&positionF, position);

			aabbRange._minX = std::min(aabbRange._minX, static_cast<uint8_t>(positionF.x));
			aabbRange._minY = std::min(aabbRange._minY, static_cast<uint8_t>(positionF.y));
			aabbRange._minZ = std::min(aabbRange._minZ, static_cast<uint8_t>(positionF.z));

			aabbRange._maxX = std::max(aabbRange._maxX, static_cast<uint8_t>(ceil(positionF.x)));
			aabbRange._maxY = std::max(aabbRange._maxY, static_cast<uint8_t>(ceil(positionF.y)));
			aabbRange._maxZ = std::max(aabbRange._maxZ, static_cast<uint8_t>(ceil(positionF.z)));
		}
	}
	// �� ��忡 ���� ��� ������ x, y, z ��ǥ�� ���� ���
	if (aabbRange._minX == aabbRange._maxX)
	{
		if (aabbRange._maxX == std::numeric_limits<uint8_t>::max())
		{
			++aabbRange._minX;
		}
		else
		{
			++aabbRange._maxX;
		}
	}
	if (aabbRange._minY == aabbRange._maxY)
	{
		if (aabbRange._maxY == std::numeric_limits<uint8_t>::max())
		{
			++aabbRange._minY;
		}
		else
		{
			++aabbRange._maxY;
		}
	}
	if (aabbRange._minZ == aabbRange._maxZ)
	{
		if (aabbRange._maxZ == std::numeric_limits<uint8_t>::max())
		{
			++aabbRange._minZ;
		}
		else
		{
			++aabbRange._maxZ;
		}
	}

	check(aabbRange._minX < aabbRange._maxX);
	check(aabbRange._minY < aabbRange._maxY);
	check(aabbRange._minZ < aabbRange._maxZ);

	return aabbRange;
}

float XM_CALLCONV Terrain::checkCollisionXXX(int nodeIndex, const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to, DirectX::FXMVECTOR min, DirectX::FXMVECTOR max) const noexcept
{
	check(nodeIndex < _aabbNodes.size());
	check(0 < nodeIndex);

	const auto& node = _aabbNodes[nodeIndex];
	if (node._children[0] == std::numeric_limits<uint16_t>::max() &&
		node._children[1] == std::numeric_limits<uint16_t>::max())
	{
		const auto& v0 = getVertexFromLeafNode(node._data._leaf, 0);
		const auto& v1 = getVertexFromLeafNode(node._data._leaf, 1);
		const auto& v2 = getVertexFromLeafNode(node._data._leaf, 2);

		const auto& p0 = XMLoadFloat3(&v0._position);
		const auto& p1 = XMLoadFloat3(&v1._position);
		const auto& p2 = XMLoadFloat3(&v2._position);

		const auto& fromV = XMLoadFloat3(&from);
		const auto& toV = XMLoadFloat3(&to);

		// _indexBuffer�� ����ִ� �ε����� submesh������ [7/1/2021 qwerw]
		const auto& intersect = MathHelper::triangleIntersectLine(p0, p1, p2, fromV, toV, true);

		return XMVectorGetX(XMVector3Length(intersect - fromV) / XMVector3Length(toV - fromV));
	}
	else
	{
		XMVECTOR nodeMinRate = XMVectorSet(static_cast<float>(node._data._node._minX) / std::numeric_limits<uint8_t>::max(),
									static_cast<float>(node._data._node._minY) / std::numeric_limits<uint8_t>::max(),
									static_cast<float>(node._data._node._minZ) / std::numeric_limits<uint8_t>::max(), 0);

		XMVECTOR nodeMaxRate = XMVectorSet(static_cast<float>(node._data._node._maxX) / std::numeric_limits<uint8_t>::max(),
									static_cast<float>(node._data._node._maxY) / std::numeric_limits<uint8_t>::max(),
									static_cast<float>(node._data._node._maxZ) / std::numeric_limits<uint8_t>::max(), 0);

		XMVECTOR nodeMin = min + (max - min) * nodeMinRate;
		XMVECTOR nodeMax = min + (max - min) * nodeMaxRate;

		XMFLOAT3 nodeMinF, nodeMaxF;
		XMStoreFloat3(&nodeMinF, nodeMin);
		XMStoreFloat3(&nodeMaxF, nodeMax);

		if (nodeMinF.x < std::max(from.x, to.x) &&
			std::min(from.x, to.x) < nodeMaxF.x &&
			nodeMinF.y < std::max(from.y, to.y) &&
			std::min(from.y, to.y) < nodeMaxF.y &&
			nodeMinF.z < std::max(from.z, to.z) &&
			std::min(from.z, to.z) < nodeMaxF.z)
		{
			return std::min(checkCollisionXXX(node._children[0], from, to, nodeMin, nodeMax),
							checkCollisionXXX(node._children[1], from, to, nodeMin, nodeMax));
		}
		else
		{
			return 1.f;
		}
	}
}
