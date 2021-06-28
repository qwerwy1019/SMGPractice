#include "stdafx.h"
#include "StageManager.h"
#include "StageInfo.h"
#include "D3DApp.h"
#include "SMGFramework.h"
#include "Actor.h"
#include "MathHelper.h"
#include "FileHelper.h"
#include "CharacterInfoManager.h"
#include "ActionChart.h"
#include "FrameEvent.h"
#include "ActionCondition.h"
#include <algorithm>
#include "MeshGeometry.h"

StageManager::StageManager()
	: _sectorSize(100, 100, 100)
	, _sectorUnitNumber(9, 9, 9)
	, _stageInfo(nullptr)
	, _isLoading(false)
	, _nextStageName("stage00")
	, _actorsBySector(9 * 9 * 9)
{
	//loadStage();
}

StageManager::~StageManager()
{
	_actorsBySector.clear();
	_terrainObjects.clear();
	_actors.clear();
}

void StageManager::loadStage(void)
{	
	//SMGFramework::getD3DApp()->releaseItemsForStageLoad();
	SMGFramework::getD3DApp()->prepareCommandQueue();
	loadStageInfo();
	createMap();
	spawnActors();
#if defined DEBUG | defined _DEBUG
	for (const auto& actor : _actors)
	{
		SMGFramework::getD3DApp()->createGameObjectDev(actor.get());
	}
#endif
	SMGFramework::getD3DApp()->executeCommandQueue();
	_isLoading = false;
}

void StageManager::update()
{
	TickCount64 deltaTick = SMGFramework::Get().getTimer().getDeltaTickCount();
	
	updateCamera();
	//updateStageScript();

	for (const auto& actor : _actors)
	{
		actor->updateActor(deltaTick);

		bool isChanged = false;
		isChanged |= rotateActor(actor.get(), deltaTick);
		isChanged |= moveActor(actor.get(), deltaTick);
		isChanged |= applyGravity(actor.get(), deltaTick);

		if (isChanged)
		{
			actor->updateObjectWorldMatrix();
		}
	}
}

void StageManager::moveActorXXX(Actor* actor, const DirectX::XMFLOAT3& moveVector) noexcept
{
	XMFLOAT3 position = actor->getPosition();
	XMFLOAT3 toPosition = MathHelper::add(position, moveVector);
	const int toSectorIndex = sectorCoordToIndex(getSectorCoord(toPosition));
	XMINT3 sector = getSectorCoord(actor->getPosition());
	const int sectorIndex = sectorCoordToIndex(sector);

	if (sectorIndex != toSectorIndex)
	{
		_actorsBySector[sectorIndex].erase(actor);
		_actorsBySector[toSectorIndex].insert(actor);
	}

	actor->setPosition(toPosition);
}

bool StageManager::rotateActor(Actor* actor, const TickCount64& deltaTick) const noexcept
{
	float rotateAngle = actor->getRotateAngleDelta(deltaTick);
	if (MathHelper::equal(rotateAngle, 0.f))
	{
		return false;
	}
	// 회전시 충돌을 고려하는 부분은 나중에 필요하면[5/12/2021 qwerwy]

	actor->rotateOnPlane(rotateAngle);
	return true;
}

bool StageManager::applyGravity(Actor* actor, const TickCount64& deltaTick) noexcept
{
	XMFLOAT3 gravityDirection;
	bool isGravityApplied = actor->getGravityDirection(gravityDirection);
	if (false == isGravityApplied)
	{
		return false;
	}

	actor->applyGravityRotation(deltaTick, gravityDirection);
	

	float speed = actor->getVerticalSpeed();
	if (MathHelper::equal(speed, 0))
	{
		return true;
	}
	XMFLOAT3 moveVector = MathHelper::mul(gravityDirection, speed);
	if (checkCollision(actor, moveVector))
	{
		return true;
	}
	
	if (checkGround(actor, moveVector))
	{
		return true;
	}

	moveActorXXX(actor, moveVector);

	return true;
}

void StageManager::setNextStage(std::string stageName) noexcept
{
	_nextStageName = stageName;
	_isLoading = true;
}

ActionChart* StageManager::loadActionChartFromXML(const std::string& actionChartName)
{
	auto findIt = _actionchartMap.find(actionChartName);
	if (findIt != _actionchartMap.end())
	{
		return findIt->second.get();
	}

	const std::string filePath = "../Resources/XmlFiles/ActionChart/" + actionChartName + ".xml";
	XMLReader xmlActionChart;
	xmlActionChart.loadXMLFile(filePath);

	auto it = _actionchartMap.emplace(actionChartName, new ActionChart(xmlActionChart.getRootNode()));
	check(it.second == true);
	return it.first->second.get();
}

const PlayerActor* StageManager::getPlayerActor(void) const noexcept
{
	return _playerActor;
}

const GravityPoint* StageManager::getGravityPointAt(const DirectX::XMFLOAT3& position) const noexcept
{
	if (_stageInfo == nullptr)
	{
		check(false);
		return nullptr;
	}

	return _stageInfo->getGravityPointAt(position);
}
bool StageManager::checkCollision(Actor* actor, const DirectX::XMFLOAT3& moveVector) const noexcept
{
	check(MathHelper::equal(MathHelper::lengthSq(moveVector), 0) == false);
	check(actor != nullptr);

	XMFLOAT3 position = actor->getPosition();
	XMINT3 sector = getSectorCoord(actor->getPosition());
	const int sectorIndex = sectorCoordToIndex(sector);
	check(sectorIndex < _actorsBySector.size(), "sector 범위를 벗어납니다.");

	XMVECTOR moveVectorLoaded = XMLoadFloat3(&moveVector);
	for (int i = std::max(0, sector.x - 1); i < std::min(_sectorUnitNumber.x, sector.x + 1); ++i)
	{
		for (int j = std::max(0, sector.y - 1); j < std::min(_sectorUnitNumber.y, sector.y + 1); ++j)
		{
			for (int k = std::max(0, sector.z - 1); k < std::min(_sectorUnitNumber.z, sector.z + 1); ++k)
			{
				const auto& actorsInSector = _actorsBySector[sectorCoordToIndex(XMINT3(i, j, k))];
				for (const auto& otherActor : actorsInSector)
				{
					if (otherActor == actor)
					{
						continue;
					}

					CollisionInfo outCollisionInfo;
					if (actor->checkCollision(otherActor, moveVectorLoaded, outCollisionInfo))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool StageManager::checkGround(Actor* actor, DirectX::XMFLOAT3& moveVector) const noexcept
{
	for (const auto& terrain : _terrains)
	{
		if (terrain.isGround() == false)
		{
			continue;
		}
		
	}
	return false;
}

bool StageManager::moveActor(Actor* actor, const TickCount64& deltaTick) noexcept
{
	const XMFLOAT3 zeroVector(0, 0, 0);
	XMFLOAT3 moveVector = actor->getMoveVector(deltaTick);
	if (MathHelper::equal(moveVector, zeroVector))
	{
		return false;
	}
	
	if (checkCollision(actor, moveVector))
	{
		return false;
	}

// 	if (checkWall(actor, moveVector))
// 	{
// 		return false;
// 	}

	moveActorXXX(actor, moveVector);
	return true;
}

int StageManager::sectorCoordToIndex(const DirectX::XMINT3& sectorCoord) const noexcept
{
	return sectorCoord.x * _sectorUnitNumber.x + sectorCoord.y * _sectorUnitNumber.y + sectorCoord.z * _sectorUnitNumber.z;
}

DirectX::XMINT3 StageManager::getSectorCoord(const DirectX::XMFLOAT3& position) const noexcept
{
	check(_sectorSize.x != 0);
	check(_sectorSize.y != 0);
	check(_sectorSize.z != 0);

	DirectX::XMFLOAT3 baseCoord = { static_cast<float>((_sectorUnitNumber.x / 2) * _sectorSize.x),
									static_cast<float>((_sectorUnitNumber.y / 2) * _sectorSize.y),
									static_cast<float>((_sectorUnitNumber.z / 2) * _sectorSize.z) };

	DirectX::XMINT3 rv = { static_cast<int>(position.x + baseCoord.x) / _sectorSize.x,
						   static_cast<int>(position.y + baseCoord.y) / _sectorSize.y,
						   static_cast<int>(position.z + baseCoord.z) / _sectorSize.z };
	// todo [5/22/2021 qwerwy]
	check(0 <= rv.x && rv.x < _sectorUnitNumber.x);
	check(0 <= rv.y && rv.y < _sectorUnitNumber.y);
	check(0 <= rv.z && rv.z < _sectorUnitNumber.z);

	return rv;
}

void StageManager::spawnActors()
{
	check(_stageInfo != nullptr);
	const auto& spawnInfos = _stageInfo->getSpawnInfos();
	for (const auto& spawnInfo : spawnInfos)
	{
		std::unique_ptr<Actor> actor;
		auto characterInfo = SMGFramework::getCharacterInfoManager()->getInfo(spawnInfo._key);
		check(characterInfo != nullptr);
		switch (characterInfo->getCharacterType())
		{
			case CharacterType::Player:
			{
				actor = std::make_unique<PlayerActor>(spawnInfo);
				_playerActor = static_cast<PlayerActor*>(actor.get());
			}
			break;
			case CharacterType::Monster:
			case CharacterType::Object:
			{
				actor = std::make_unique<Actor>(spawnInfo);
			}
			break;
			case CharacterType::Count:
			default:
			{
				static_assert(static_cast<int>(CharacterType::Count) == 3);
				check(false, "spawnInfo" + std::to_string(spawnInfo._key) + " characterType이 이상합니다.");
			}
		}
		
		int sectorIndex = sectorCoordToIndex(getSectorCoord(spawnInfo._position));
		check(sectorIndex < _actorsBySector.size());
		_actorsBySector[sectorIndex].emplace(actor.get());
		if (spawnInfo._isTerrain)
		{
			_terrainObjects.push_back(actor->getGameObject());
		}
		_actors.emplace_back(std::move(actor));
	}
}

void StageManager::loadStageInfo()
{
	_stageInfo = std::make_unique<StageInfo>();

	const std::string stageInfoFilePath =
		"../Resources/XmlFiles/StageInfo/" + _nextStageName + ".xml";
	XMLReader xmlStageInfo;

	xmlStageInfo.loadXMLFile(stageInfoFilePath);

	_stageInfo->loadXml(xmlStageInfo.getRootNode());
}

void StageManager::updateCamera() noexcept
{
	check(_stageInfo != nullptr);
	check(_playerActor != nullptr);

	if (!_fixedCameraName.empty())
	{
		setCameraCount(0);
		const auto& camera = _stageInfo->getFixedCameraPoint(_fixedCameraName);
		SMGFramework::getD3DApp()->setCameraInput(camera._position, camera._focusPosition, camera._upVector, camera._cameraSpeed, camera._cameraFocusSpeed);
	}
	else
	{
		const auto& nearCameraList = _stageInfo->getNearCameraPoints(_playerActor->getPosition());
		setCameraCount(nearCameraList.size());

		if (nearCameraList.empty())
		{
			using namespace MathHelper;
			auto playerPosition = _playerActor->getPosition();
			auto playerDirection = _playerActor->getDirection();
			auto playerUpVector = _playerActor->getUpVector();

			auto cameraFocusPosition = add(playerPosition, mul(playerUpVector, 100));
			auto cameraPosition = add(playerPosition, mul(sub(mul(playerUpVector, 2.f), playerDirection), 600));

			SMGFramework::getD3DApp()->setCameraInput(cameraPosition, cameraFocusPosition, playerUpVector, 10.f, 10.f);
		}
		else
		{
			int cameraIndex = getCameraIndex();
			check(cameraIndex < nearCameraList.size());
			if (cameraIndex != -1)
			{
				using namespace MathHelper;
				float distanceSum = 0.f;
				DirectX::XMFLOAT3 positionSum(0, 0, 0);
				DirectX::XMFLOAT3 upVectorSum(0, 0, 0);
				for (const auto& cam : nearCameraList)
				{
					float distance = length(sub(cam->_position, _playerActor->getPosition()));
					distanceSum += distance;

					positionSum = add(positionSum, mul(cam->_position, distance));
					upVectorSum = add(upVectorSum, mul(cam->_upVector, distance));
				}
				SMGFramework::getD3DApp()->setCameraInput(
					div(positionSum, distanceSum),
					_playerActor->getPosition(),
					div(upVectorSum, distanceSum), 1.f, 1.f);
			}
			else
			{
				SMGFramework::getD3DApp()->setCameraInput(
					nearCameraList[cameraIndex]->_position,
					_playerActor->getPosition(),
					nearCameraList[cameraIndex]->_upVector, 1.f, 1.f);
			}
		}
	}
}

void StageManager::setCameraCount(const int count) noexcept
{
	_cameraCount = count;
	if (_cameraCount <= _cameraIndex)
	{
		_cameraIndex = -1;
	}
}

int StageManager::getCameraCount() const noexcept
{
	return _cameraCount;
}

int StageManager::getCameraIndex() const noexcept
{
	return _cameraIndex;
}

void StageManager::createMap(void)
{
	_sectorSize = _stageInfo->getSectorSize();
	_sectorUnitNumber = _stageInfo->getSectorUnitNumber();
	const auto& terrainObjectInfos = _stageInfo->getTerrainObjectInfos();
	_terrains.reserve(terrainObjectInfos.size());
	for (const auto& terrainObjectInfo : terrainObjectInfos)
	{
		_terrains.emplace_back(terrainObjectInfo);
	}
}

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
	makeAABBTreeImpl(terrainIndexList, 0, terrainIndexList.size(), min, max);
}

uint16_t XM_CALLCONV Terrain::makeAABBTreeImpl(std::vector<int>& terrainIndexList,
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
		
		node._children[0] = makeAABBTreeImpl(terrainIndexList, begin, mid, nextMin, nextMax);
		node._children[1] = makeAABBTreeImpl(terrainIndexList, mid, end, nextMin, nextMax);
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
