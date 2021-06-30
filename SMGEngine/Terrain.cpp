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

	return checkCollisionXXX(_aabbNodes.size() - 1, startF, endF, minV, maxV);
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
			ThrowErrCode(ErrCode::InvalidXmlData, "다른 mesh를 같은 terrain으로 사용하려면 별도 작업 필요함");
		}
		mesh = renderItem->_geometry;
		totalIndexCount += renderItem->_indexCount;
	}

	std::vector<int> terrainIndexList;
	terrainIndexList.reserve(totalIndexCount / 3);
	for (const auto& renderItem : _gameObject->_renderItems)
	{
		for (int i = 0; i < renderItem->_indexCount; i += 3)
		{
			terrainIndexList.push_back(renderItem->_startIndexLocation + i);
		}
	}

	size_t vertexCount;
	_vertexBuffer = mesh->getVertexBufferXXX(vertexCount);
	_indexBuffer = mesh->getIndexBufferXXX();

	_min = _vertexBuffer[_indexBuffer[terrainIndexList[0]]]._position;
	_max = _min;
	for (int i = 0; i < terrainIndexList.size(); ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			const Vertex& vertex = _vertexBuffer[_indexBuffer[terrainIndexList[i] + j]];

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
	// reserve 크기 체크해야함 [6/24/2021 qwerw]
	_aabbNodes.reserve(vertexCount);
	makeAABBTreeXXX(terrainIndexList, 0, terrainIndexList.size(), min, max);
}

uint16_t XM_CALLCONV Terrain::makeAABBTreeXXX(std::vector<int>& terrainIndexList,
	int begin,
	int end,
	FXMVECTOR min,
	FXMVECTOR max)
{
	check(begin < end);
	check(_vertexBuffer != nullptr);
	check(_indexBuffer != nullptr);
	check(0 <= begin && begin < terrainIndexList.size());
	check(0 < end && end <= terrainIndexList.size());

	TerrainAABBNode node;
	if (end - begin == 1)
	{
		node._children[0] = std::numeric_limits<uint16_t>::max();
		node._children[1] = std::numeric_limits<uint16_t>::max();
		node._data._leaf._index = terrainIndexList[begin];
	}
	else
	{
		node._data._node = getAABBRange(terrainIndexList, begin, end, min, max);

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

		sortIndexList(terrainIndexList, begin, end, divideType);
		int mid = (begin + end) / 2;

		node._children[0] = makeAABBTreeXXX(terrainIndexList, begin, mid, nextMin, nextMax);
		node._children[1] = makeAABBTreeXXX(terrainIndexList, mid, end, nextMin, nextMax);
	}
	_aabbNodes.push_back(node);

	if (_aabbNodes.size() - 1 >= std::numeric_limits<uint16_t>::max())
	{
		ThrowErrCode(ErrCode::Overflow, "단위를 바꿔야함");
	}
	return (_aabbNodes.size() - 1);
}

void Terrain::sortIndexList(std::vector<int>& terrainIndexList, int begin, int end, DivideType divideType) const noexcept
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

	// indexList가 좌표상 거의 정렬된 상태여서 첫번째 인자가 pivot으로 적절하지 않음. [6/25/2021 qwerw]
	// merge sort로 바꾸는걸 고려해봐야 함. [6/25/2021 qwerw]
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

float Terrain::sortCompareValue(int index, DivideType type) const noexcept
{
	check(_vertexBuffer != nullptr);
	check(_indexBuffer != nullptr);

	switch (type)
	{
		case DivideType::X:
			return _vertexBuffer[_indexBuffer[index]]._position.x +
				_vertexBuffer[_indexBuffer[index + 1]]._position.x +
				_vertexBuffer[_indexBuffer[index + 2]]._position.x;
			break;
		case DivideType::Y:
			return _vertexBuffer[_indexBuffer[index]]._position.y +
				_vertexBuffer[_indexBuffer[index + 1]]._position.y +
				_vertexBuffer[_indexBuffer[index + 2]]._position.y;
			break;
		case DivideType::Z:
			return _vertexBuffer[_indexBuffer[index]]._position.z +
				_vertexBuffer[_indexBuffer[index + 1]]._position.z +
				_vertexBuffer[_indexBuffer[index + 2]]._position.z;
			break;
		default:
			check(false);
			break;
	}
	return 0.f;
}

TerrainAABBNode::DataType::Node XM_CALLCONV Terrain::getAABBRange(std::vector<int>& terrainIndexList,
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
			const Vertex& vertex = _vertexBuffer[_indexBuffer[terrainIndexList[i] + j]];
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
	// 이 노드에 속한 모든 점들의 x, y, z 좌표가 같은 경우
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
		auto index = node._data._leaf._index;

		const auto& v0 = _vertexBuffer[_indexBuffer[index]];
		const auto& v1 = _vertexBuffer[_indexBuffer[index + 1]];
		const auto& v2 = _vertexBuffer[_indexBuffer[index + 2]];

		const auto& p0 = XMLoadFloat3(&v0._position);
		const auto& p1 = XMLoadFloat3(&v1._position);
		const auto& p2 = XMLoadFloat3(&v2._position);

		const auto& fromV = XMLoadFloat3(&from);
		const auto& toV = XMLoadFloat3(&to);

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
