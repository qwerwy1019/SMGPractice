#include "stdafx.h"
#include "Terrain.h"
#include "Exception.h"
#include "StageInfo.h"
#include "SMGFramework.h"
#include "D3DApp.h"
#include "MathHelper.h"
#include "GameObject.h"
#include "Actor.h"
#include "CharacterInfoManager.h"
#include "ObjectInfo.h"
#include "StageManager.h"


bool Terrain::isGround(void) const noexcept
{
	return _isGround;
}

bool Terrain::isWall(void) const noexcept
{
	return _isWall;
}

Terrain::Terrain(const TerrainObjectInfo& terrainInfo)
	: _vertexBuffer(nullptr)
	, _indexBuffer(nullptr)
	, _max(0, 0, 0)
	, _min(0, 0, 0)
{
	_isGround = terrainInfo.isGround();
	_isWall = terrainInfo.isWall();

	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(terrainInfo.getObjectFileName());

	_gameObject->setWorldMatrix(terrainInfo.getPosition(),
		terrainInfo.getDirection(),
		terrainInfo.getUpVector(),
		terrainInfo.getSize());
	_size = terrainInfo.getSize();
#if defined DEBUG | defined _DEBUG
	//SMGFramework::getD3DApp()->createGameObjectDev(_gameObject);
#endif

	if (_isGround || _isWall)
	{
		makeAABBTree();
	}
}

Terrain::~Terrain()
{
	if (!SMGFramework::getStageManager()->isLoading())
	{
		SMGFramework::getD3DApp()->removeGameObject(_gameObject);
	}
}

void Terrain::makeAABBTree(void)
{
	check(_gameObject != nullptr);

	int totalIndexCount = 0;
	const auto& renderItems = _gameObject->getRenderItems();
	auto mesh = _gameObject->getMeshGeometry();
	check(mesh != nullptr);
	for (const auto& renderItem : renderItems)
	{
		totalIndexCount += renderItem->getSubMesh()._indexCount;
	}

	std::vector<TerrainAABBNode::DataType::Leaf> terrainLeafList(totalIndexCount / 3);
	int i = 0;

	for (const auto& renderItem : renderItems)
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

	const auto& subMesh = mesh->_subMeshList[terrainLeafList[0]._subMeshIndex];
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
	// reserve 크기 체크해야함 [6/24/2021 qwerw]
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
		ThrowErrCode(ErrCode::Overflow, "단위를 바꿔야함");
	}
	return (_aabbNodes.size() - 1);
}

const Vertex& Terrain::getVertexFromLeafNode(const TerrainAABBNode::DataType::Leaf& leafNode, const int offset) const noexcept
{
	const auto& subMesh = _gameObject->getMeshGeometry()->_subMeshList[leafNode._subMeshIndex];
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

	check(aabbRange._minX < aabbRange._maxX);
	check(aabbRange._minY < aabbRange._maxY);
	check(aabbRange._minZ < aabbRange._maxZ);

	return aabbRange;
}


float XM_CALLCONV Terrain::checkCollisionXXX(int nodeIndex,
											DirectX::FXMVECTOR min, 
											DirectX::FXMVECTOR max,
											const TerrainCollisionInfoXXX& collisionInfo) const noexcept
{
	check(nodeIndex < _aabbNodes.size());
	check(0 <= nodeIndex);

	const auto& node = _aabbNodes[nodeIndex];
	if (node._children[0] == std::numeric_limits<uint16_t>::max() &&
		node._children[1] == std::numeric_limits<uint16_t>::max())
	{
		const auto& v0 = getVertexFromLeafNode(node._data._leaf, 0);
		const auto& v1 = getVertexFromLeafNode(node._data._leaf, 1);
		const auto& v2 = getVertexFromLeafNode(node._data._leaf, 2);

		const auto& t0 = XMLoadFloat3(&v0._position);
		const auto& t1 = XMLoadFloat3(&v1._position);
		const auto& t2 = XMLoadFloat3(&v2._position);

		switch (collisionInfo._shape)
		{
			case CollisionShape::Sphere:
			{
				return MathHelper::triangleIntersectSphere(t0, t1, t2,
													collisionInfo._position, 
													collisionInfo._velocity, 
													collisionInfo._radius);
			}
			break;
			case CollisionShape::Box:
			{
				return MathHelper::triangleIntersectBox(t0, t1, t2,
													collisionInfo._position,
													collisionInfo._boxX,
													collisionInfo._boxY,
													collisionInfo._boxZ,
													collisionInfo._velocity);
			}
			break;
			case CollisionShape::Line:
			{
				return MathHelper::triangleIntersectLine(t0, t1, t2, 
														collisionInfo._position,
														collisionInfo._velocity);
			}
			break;
			case CollisionShape::Polygon:
			case CollisionShape::Count:
			default:
			{
				check(false);
				static_assert(static_cast<int>(CollisionShape::Count) == 4);
				return MathHelper::NO_INTERSECTION;
			}
			break;
		}
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


		if (XMVector3Greater(collisionInfo._max, nodeMin) &&
			XMVector3Greater(nodeMax, collisionInfo._min))
		{
			return std::min(checkCollisionXXX(node._children[0], nodeMin, nodeMax, collisionInfo),
							checkCollisionXXX(node._children[1], nodeMin, nodeMax, collisionInfo));
		}
		else
		{
			return MathHelper::NO_INTERSECTION;
		}
	}
}

float XM_CALLCONV Terrain::checkCollisionLineXXX(int nodeIndex,
												DirectX::FXMVECTOR min,
												DirectX::FXMVECTOR max,
												const DirectX::XMFLOAT3& start,
												const DirectX::XMFLOAT3& velocity) const noexcept
{
	check(nodeIndex < _aabbNodes.size());
	check(0 <= nodeIndex);

	const auto& node = _aabbNodes[nodeIndex];
	if (node._children[0] == std::numeric_limits<uint16_t>::max() &&
		node._children[1] == std::numeric_limits<uint16_t>::max())
	{
		const auto& v0 = getVertexFromLeafNode(node._data._leaf, 0);
		const auto& v1 = getVertexFromLeafNode(node._data._leaf, 1);
		const auto& v2 = getVertexFromLeafNode(node._data._leaf, 2);

		const auto& t0 = XMLoadFloat3(&v0._position);
		const auto& t1 = XMLoadFloat3(&v1._position);
		const auto& t2 = XMLoadFloat3(&v2._position);

		return MathHelper::triangleIntersectLine(t0, t1, t2,
			XMLoadFloat3(&start),
			XMLoadFloat3(&velocity));
	}
	else
	{
		XMVECTOR nodeMinRate = XMVectorSet(static_cast<float>(node._data._node._minX) / std::numeric_limits<uint8_t>::max(),
			static_cast<float>(node._data._node._minY) / std::numeric_limits<uint8_t>::max(),
			static_cast<float>(node._data._node._minZ) / std::numeric_limits<uint8_t>::max(), 0);

		XMVECTOR nodeMaxRate = XMVectorSet(static_cast<float>(node._data._node._maxX) / std::numeric_limits<uint8_t>::max(),
			static_cast<float>(node._data._node._maxY) / std::numeric_limits<uint8_t>::max(),
			static_cast<float>(node._data._node._maxZ) / std::numeric_limits<uint8_t>::max(), 0);

		XMVECTOR nodeMinV = min + (max - min) * nodeMinRate;
		XMVECTOR nodeMaxV = min + (max - min) * nodeMaxRate;
		XMFLOAT3 nodeMin, nodeMax;
		XMStoreFloat3(&nodeMin, nodeMinV);
		XMStoreFloat3(&nodeMax, nodeMaxV);

		do 
		{
			// vector 원소를 index접근할 방법이 없는지... [9/11/2021 qwerw]
			float t;
			if (!MathHelper::equal(velocity.x, 0.f))
			{
				// x = nodeMax.x 평면과의 교점
				// (nodeMax.x, someY, someZ) 가 start + t * velocity이다
				// nodeMax.x == start.x + t * velocity.x이다.
				// t = (nodeMax.x - start.x) / velocity.x이다.
				t = (nodeMax.x - start.x) / velocity.x;
				if (0.f < t && t < 1.f)
				{
					float intersectY = start.y + t * velocity.y;
					float intersectZ = start.z + t * velocity.z;
					if (nodeMin.y < intersectY && intersectY < nodeMax.y &&
						nodeMin.z < intersectZ && intersectZ < nodeMax.z)
					{
						break;
					}
				}
			}
			
			if (!MathHelper::equal(velocity.y, 0.f))
			{
				t = (nodeMax.y - start.y) / velocity.y;
				if (0.f < t && t < 1.f)
				{
					float intersectX = start.x + t * velocity.x;
					float intersectZ = start.z + t * velocity.z;
					if (nodeMin.x < intersectX && intersectX < nodeMax.x &&
						nodeMin.z < intersectZ && intersectZ < nodeMax.z)
					{
						break;
					}
				}
			}

			if (!MathHelper::equal(velocity.z, 0.f))
			{
				t = (nodeMax.z - start.z) / velocity.z;

				float intersectX = start.x + t * velocity.x;
				float intersectY = start.y + t * velocity.y;
				if (0.f < t && t < 1.f)
				{
					if (nodeMin.x < intersectX && intersectX < nodeMax.x &&
						nodeMin.y < intersectY && intersectY < nodeMax.y)
					{
						break;
					}
				}
			}

			if (!MathHelper::equal(velocity.x, 0.f))
			{
				t = (nodeMin.x - start.x) / velocity.x;

				float intersectY = start.y + t * velocity.y;
				float intersectZ = start.z + t * velocity.z;
				if (0.f < t && t < 1.f)
				{
					if (nodeMin.y < intersectY && intersectY < nodeMax.y &&
						nodeMin.z < intersectZ && intersectZ < nodeMax.z)
					{
						break;
					}
				}
			}

			if (!MathHelper::equal(velocity.y, 0.f))
			{
				t = (nodeMin.y - start.y) / velocity.y;

				float intersectX = start.x + t * velocity.x;
				float intersectZ = start.z + t * velocity.z;
				if (0.f < t && t < 1.f)
				{
					if (nodeMin.x < intersectX && intersectX < nodeMax.x &&
						nodeMin.z < intersectZ && intersectZ < nodeMax.z)
					{
						break;
					}
				}
			}

			if (!MathHelper::equal(velocity.z, 0.f))
			{
				t = (nodeMin.z - start.z) / velocity.z;

				float intersectX = start.x + t * velocity.x;
				float intersectY = start.y + t * velocity.y;
				if (0.f < t && t < 1.f)
				{
					if (nodeMin.x < intersectX && intersectX < nodeMax.x &&
						nodeMin.y < intersectY && intersectY < nodeMax.y)
					{
						break;
					}
				}
			}

			return MathHelper::NO_INTERSECTION;
			
		} while (false);
		
		return std::min(checkCollisionLineXXX(node._children[0], nodeMinV, nodeMaxV, start, velocity),
			checkCollisionLineXXX(node._children[1], nodeMinV, nodeMaxV, start, velocity));
		
	}
}



void Terrain::setCulled(void) noexcept
{
	_gameObject->setCulled();
}

bool Terrain::checkCollision(const Actor& actor, const DirectX::XMFLOAT3& velocity, float& collisionTime) const noexcept
{
	XMMATRIX inverseMatrix = XMLoadFloat4x4(&_gameObject->getWorldMatrix());
	inverseMatrix = XMMatrixInverse(nullptr, inverseMatrix);

	TerrainCollisionInfoXXX collisionInfo;
	// sliding을 구현하지 않았기 때문에, 겹쳐지는 경우에는 뒤로 보내야해서 미리 반지름만큼 뒤로 보내서 충돌 체크를 할 것임. [7/12/2021 qwerw]
	float adjustingDistance = actor.getRadius() * 0.5;
	XMVECTOR velocityWorld = XMVectorSetW(XMLoadFloat3(&velocity), 0.f);
	float speed = XMVectorGetX(XMVector3Length(velocityWorld));
	XMVECTOR adjustingVelocityWorld = (velocityWorld * adjustingDistance / speed);

	collisionInfo._velocity = XMVector4Transform(velocityWorld + adjustingVelocityWorld, inverseMatrix);
	collisionInfo._position = XMVector3Transform(XMLoadFloat3(&actor.getPosition()) - adjustingVelocityWorld, inverseMatrix);

	switch (actor.getCharacterInfo()->getCollisionShape())
	{
		case CollisionShape::Polygon:
		case CollisionShape::Sphere:
		{
			collisionInfo._shape = CollisionShape::Sphere;
			
			collisionInfo._radius = actor.getRadius() / _size;
			collisionInfo._min = XMVectorMin(collisionInfo._position, collisionInfo._position + collisionInfo._velocity);
			collisionInfo._max = XMVectorMax(collisionInfo._position, collisionInfo._position + collisionInfo._velocity);

			XMVECTOR radiusVector = XMVectorSet(collisionInfo._radius, collisionInfo._radius, collisionInfo._radius, 0.f);
			collisionInfo._min -= radiusVector;
			collisionInfo._max += radiusVector;
		}
		break;
		case CollisionShape::Box:
		{
			collisionInfo._shape = CollisionShape::Box;

			XMVECTOR direction = XMVectorSetW(XMLoadFloat3(&actor.getDirection()), 0.f);
			XMVECTOR upVector = XMVectorSetW(XMLoadFloat3(&actor.getUpVector()), 0.f);
			XMVECTOR rightVector = XMVectorSetW(XMVector3Cross(upVector, direction), 0.f);
			collisionInfo._boxZ = XMVector4Transform(-direction, inverseMatrix);
			collisionInfo._boxY = XMVector4Transform(upVector, inverseMatrix);
			collisionInfo._boxX = XMVector4Transform(rightVector, inverseMatrix);

			collisionInfo._boxX *= actor.getSizeX();
			collisionInfo._boxY *= actor.getSizeY();
			collisionInfo._boxZ *= actor.getSizeZ();

			collisionInfo._min = XMVectorMin(collisionInfo._position, collisionInfo._position + collisionInfo._velocity);
			collisionInfo._max = XMVectorMax(collisionInfo._position, collisionInfo._position + collisionInfo._velocity);

			XMVECTOR absVector = XMVectorAbs(collisionInfo._boxX) + XMVectorAbs(collisionInfo._boxY) + XMVectorAbs(collisionInfo._boxZ);

			collisionInfo._min -= absVector;
			collisionInfo._max += absVector;
		}
		break;
		case CollisionShape::Count:
		default:
		{
			check(false);
			static_assert(static_cast<int>(CollisionShape::Count) == 4);
		}
	}

	XMVECTOR minV = XMLoadFloat3(&_min);
	XMVECTOR maxV = XMLoadFloat3(&_max);
	
	float collisionTimeRaw = checkCollisionXXX(_aabbNodes.size() - 1, minV, maxV, collisionInfo);
	if (1.f < collisionTimeRaw)
	{
		return false;
	}
	collisionTime = (collisionTimeRaw * (adjustingDistance + speed) - adjustingDistance) / speed;

	check(collisionTime < 1.05);

	return true;
}

bool Terrain::checkCollisionLine(DirectX::FXMVECTOR start, DirectX::FXMVECTOR velocity, float& collisionTime) const noexcept
{
	XMMATRIX inverseMatrix = XMLoadFloat4x4(&_gameObject->getWorldMatrix());
	inverseMatrix = XMMatrixInverse(nullptr, inverseMatrix);

	DirectX::XMFLOAT3 startF, velocityF;
	TerrainCollisionInfoXXX collisionInfo;
	collisionInfo._shape = CollisionShape::Line;
	XMStoreFloat3(&startF, XMVector3Transform(start, inverseMatrix));
	XMStoreFloat3(&velocityF, XMVector4Transform(velocity, inverseMatrix));

	XMVECTOR minV = XMLoadFloat3(&_min);
	XMVECTOR maxV = XMLoadFloat3(&_max);

	collisionTime = checkCollisionLineXXX(_aabbNodes.size() - 1, minV, maxV, startF, velocityF);
	return collisionTime < 1.f;
}
