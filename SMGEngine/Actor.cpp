#include "stdafx.h"
#include "Actor.h"
#include "D3DUtil.h"
#include "Exception.h"
#include "D3DApp.h"
#include "ActionChart.h"
#include "MathHelper.h"
#include "CharacterInfoManager.h"
#include "StageInfo.h"
#include "SMGFramework.h"
#include "StageManager.h"
#include <algorithm>
#include "GameObject.h"

Actor::Actor(const SpawnInfo& spawnInfo)
	: _position(spawnInfo._position)
	, _direction(spawnInfo._direction)
	, _upVector(spawnInfo._upVector)
	, _size(spawnInfo._size)
	, _speed(0.f)
	, _verticalSpeed(0.f)
	, _moveType(MoveType::CharacterDirection)
	, _moveDirectionOffset(0.f)
	, _acceleration(0.f)
	, _targetSpeed(0.f)
	, _targetVerticalSpeed(10.f)
	, _additionalMoveVector(0, 0, 0)
	, _rotateType(RotateType::Fixed)
	, _rotateAngleOffset(0.f)
	, _rotateSpeed(0.f)
	, _gravityPoint(nullptr)
	, _localTickCount(0)
	, _onGround(false)
{
	_characterInfo = SMGFramework::getCharacterInfoManager()->getInfo(spawnInfo._key);
	// 시작할때 한번 검증하는 부분 만들면 옮겨야함. [5/20/2021 qwerw]
	if (nullptr == _characterInfo)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "캐릭터키가 비정상입니다. " + spawnInfo._key);
	}
	_actionChart = SMGFramework::getStageManager()->loadActionChartFromXML(_characterInfo->getActionChartFileName());
	_currentActionState = _actionChart->getActionState("IDLE");
	if (nullptr == _currentActionState)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, _characterInfo->getActionChartFileName() + " : IDLE 이 없습니다.");
	}
	
	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(_characterInfo->getObjectFileName());

	if (_currentActionState == nullptr)
	{
		ThrowErrCode(ErrCode::ActionChartLoadFail, "기본액션 IDLE이 없습니다. " + _characterInfo->getActionChartFileName());
	}

	_gravityPoint = SMGFramework::getStageManager()->getGravityPointAt(_position);
	_sectorCoord = SMGFramework::getStageManager()->getSectorCoord(_position);
	updateObjectWorldMatrix();
}

Actor::~Actor()
{
	SMGFramework::getD3DApp()->removeGameObject(_gameObject);
}

void Actor::rotateOnPlane(const float rotateAngle) noexcept
{
	check(!MathHelper::equal(rotateAngle, 0.f));
	XMVECTOR direction = XMLoadFloat3(&_direction);

	XMMATRIX rotateMatrix = XMMatrixRotationNormal(XMLoadFloat3(&_upVector), rotateAngle);
	direction = XMVector3Transform(direction, rotateMatrix);
	
	XMStoreFloat3(&_direction, XMVector3Normalize(direction));

	if (_rotateType == RotateType::Fixed || _rotateType == RotateType::ToCollidingTarget)
	{
		_rotateAngleOffset -= rotateAngle;
		static_assert(static_cast<int>(RotateType::Count) == 6, "타입 추가시 확인");
	}
}

void Actor::rotateUpVector(const DirectX::XMFLOAT3& toUpVector) noexcept
{
	XMVECTOR upVector = XMLoadFloat3(&toUpVector);
	XMVECTOR currentDirection = XMLoadFloat3(&_direction);
	double dot = clamp(XMVectorGetX(XMVector3Dot(upVector, currentDirection)), -1.f, 1.f);
	if (MathHelper::equal_d(dot, 0))
	{
		_upVector = toUpVector;
		return;
	}
	else if (MathHelper::equal_d(dot, 1) || MathHelper::equal_d(dot , -1))
	{
		check(false, "한 프레임에 90도 회전. 회전 속도를 조절해야함");
		upVector *= dot;

		_direction = _upVector;
		XMStoreFloat3(&_upVector, upVector);
		return;
	}

	XMMATRIX rotateMatrix = XMMatrixRotationNormal(XMVector3Cross(upVector, currentDirection), MathHelper::Pi_DIV2 - acos(dot));
	XMVECTOR newDirection = XMVector3Transform(currentDirection, rotateMatrix);

	XMStoreFloat3(&_upVector, XMVector3Normalize(upVector));
	XMStoreFloat3(&_direction, XMVector3Normalize(newDirection));
}

float Actor::getRotateAngleDelta(const TickCount64& deltaTick) const noexcept
{
	const float maxAngle = _rotateSpeed * deltaTick;
	float deltaAngle = 0.f;
	
	switch (_rotateType)
	{
		case RotateType::Fixed:
		case RotateType::ToCollidingTarget:
		{
			if (MathHelper::equal(_rotateAngleOffset, 0))
			{
				__noop;
			}
			else if (_rotateAngleOffset < 0)
			{
				deltaAngle = std::max(_rotateAngleOffset, -maxAngle);
			}
			else if (_rotateAngleOffset > 0)
			{
				deltaAngle = std::min(_rotateAngleOffset, maxAngle);
			}
		}
		break;
		case RotateType::ToPlayer:
		{
			check(SMGFramework::getStageManager()->getPlayerActor() != this);
			const PlayerActor* player = SMGFramework::getStageManager()->getPlayerActor();
			if (nullptr != player)
			{
				XMVECTOR playerPosition = XMLoadFloat3(&player->getPosition());
				// 미구현 [8/9/2021 qwerw]
			}
		}
		break;
		case RotateType::Path:
		{
			check(false, "미구현");
		}
		break;
		case RotateType::JoystickInput:
		{
			XMFLOAT2 stickInput = SMGFramework::Get().getStickInput(StickInputType::LStick);
			
			float stickInputLength = sqrt(stickInput.x * stickInput.x + stickInput.y * stickInput.y);
			if (MathHelper::equal(stickInputLength, 0))
			{
				return 0;
			}
			double stickAngleFromFront = acos(stickInput.y / stickInputLength);
			if (stickInput.x < 0.f)
			{
				stickAngleFromFront *= -1;
			}

			const XMFLOAT3& camDirectionFloat = SMGFramework::getD3DApp()->getCameraDirection();

			XMVECTOR camDirection = XMLoadFloat3(&camDirectionFloat);
			XMVECTOR actorDirection = XMLoadFloat3(&_direction);
			XMVECTOR actorUpVector = XMLoadFloat3(&_upVector);
			XMVECTOR frontRaw = camDirection - (XMVector3Dot(camDirection, actorUpVector) * actorUpVector);
			float frontRawLength = XMVectorGetX(XMVector3Length(frontRaw));
			if (MathHelper::equal(frontRawLength, 0))
			{
				check(false, "카메라 방향이 이상하지 않은지 확인해야함.");
				return 0;
			}
			XMVECTOR front = frontRaw / frontRawLength;
			XMVECTOR right = XMVector3Cross(actorUpVector, front);
			double currentAngleFromFront = acos(std::clamp(XMVectorGetX(XMVector3Dot(front, actorDirection)), -1.f, 1.f));
			if (XMVectorGetX(XMVector3Dot(right, actorDirection)) < 0.f)
			{
				currentAngleFromFront *= -1;
			}

			deltaAngle = stickAngleFromFront - currentAngleFromFront + _rotateAngleOffset;
			if (deltaAngle < -MathHelper::Pi)
			{
				deltaAngle += MathHelper::Pi;
			}
			else if (deltaAngle >= MathHelper::Pi)
			{
				deltaAngle -= MathHelper::Pi;
			}
		}
		break;
		case RotateType::ToWall:
		{
			check(false, "미구현");
		}
		break;
		case RotateType::Count:
		default:
		{
			static_assert(static_cast<int>(RotateType::Count) == 6, "타입 추가시 확인");
			check(false, "타입 추가시 확인");
		}
		break;
	}
	deltaAngle = std::clamp(deltaAngle, -maxAngle, maxAngle);
	return deltaAngle;
}

void Actor::applyGravityRotation(const TickCount64& deltaTick, const DirectX::XMFLOAT3& gravityDirection) noexcept
{
	check(_gravityPoint != nullptr);
	check(MathHelper::equal(MathHelper::length(gravityDirection), 1.f, 0.01f));

	XMVECTOR gravityDirectionV = XMLoadFloat3(&gravityDirection);
	XMVECTOR actorUpVector = XMLoadFloat3(&_upVector);

	double rotateAngle = acos(std::clamp(XMVectorGetX(XMVector3Dot(-gravityDirectionV, actorUpVector)), -1.f, 1.f));
	// gravity 회전 속도에 대한 다른 기준을 찾을때까진 일단 gravity에 비례하게 쓴다. [6/24/2021 qwerw]
	double maxRotateAngle = static_cast<double>(_gravityPoint->_gravity) * deltaTick;
	if (rotateAngle > maxRotateAngle)
	{
		XMVECTOR cross = XMVector3Cross(actorUpVector, -gravityDirectionV);
		if (MathHelper::equal(XMVectorGetX(XMVector3LengthSq(cross)), 0.f))
		{
			cross = XMLoadFloat3(&_direction);
		}
		actorUpVector = XMVector3Transform(actorUpVector, XMMatrixRotationAxis(cross, maxRotateAngle));
	}
	else
	{
		actorUpVector = -gravityDirectionV;
	}

	XMFLOAT3 newUpVector;
	XMStoreFloat3(&newUpVector, actorUpVector);

	rotateUpVector(newUpVector);
}

bool Actor::getGravityDirection(DirectX::XMFLOAT3& gravityDirection) const noexcept
{
	if (_gravityPoint == nullptr)
	{
		return false;
	}

	switch (_gravityPoint->_type)
	{
		case GravityPointType::Fixed:
		{
			gravityDirection = _gravityPoint->_position;
		}
		break;
		case GravityPointType::Point:
		{
			XMVECTOR actorPosition = XMLoadFloat3(&_position);
			XMVECTOR toGravityPoint = XMLoadFloat3(&_gravityPoint->_position) - actorPosition;
			float toGravityPointLength = XMVectorGetX(XMVector3Length(toGravityPoint));
			if (MathHelper::equal(toGravityPointLength, 0))
			{
				return false;
			}
			XMStoreFloat3(&gravityDirection, XMVector3Normalize(toGravityPoint));
		}
		break;
		case GravityPointType::GroundNormal:
		{
			check(false, "미구현");
		}
		break;
		case GravityPointType::Count:
		default:
		{
			static_assert(static_cast<int>(GravityPointType::Count) == 3, "타입 추가시 확인");
			check(false, "타입 추가시 확인");
		}
		break;
	}
	return true;
}

float Actor::getSpeed() const noexcept
{
	return _speed;
}

void Actor::updateActor(const TickCount64& deltaTick) noexcept
{
	updateActionChart(deltaTick);

	// 가속도 적용
	if (_speed < _targetSpeed)
	{
		_speed += _acceleration * deltaTick;
		_speed = std::min(_speed, _targetSpeed);

	}
	else
	{
		_speed -= _acceleration * deltaTick;
		_speed = std::max(_speed, _targetSpeed);
	}

	if (_gravityPoint != nullptr)
	{
		if (_verticalSpeed < _targetVerticalSpeed)
		{
			_verticalSpeed += _gravityPoint->_gravity * deltaTick;
			_verticalSpeed = std::min(_verticalSpeed, _targetVerticalSpeed);
		}
		else
		{
			_verticalSpeed -= _gravityPoint->_gravity * deltaTick;
			_verticalSpeed = std::max(_verticalSpeed, _targetVerticalSpeed);
		}
	}
}

float Actor::getVerticalSpeed() const noexcept
{
	return _verticalSpeed;
}

void Actor::setActorOnGround(bool onGround) noexcept
{
	if (onGround == true)
	{
		_verticalSpeed = 0.f;
		_targetVerticalSpeed = 10.f;
	}

	_onGround = onGround;
}

bool Actor::isOnGround(void) const noexcept
{
	return _onGround;
}

TickCount64 Actor::getLocalTickCount(void) const noexcept
{
	return _localTickCount;
}

const DirectX::XMFLOAT3& Actor::getPosition(void) const noexcept
{
	return _position;
}

const DirectX::XMFLOAT3& Actor::getDirection(void) const noexcept
{
	return _direction;
}

const DirectX::XMFLOAT3& Actor::getUpVector(void) const noexcept
{
	return _upVector;
}

bool Actor::checkCollision(const Actor* lhs, const Actor* rhs) noexcept
{
	check(lhs != rhs);
	check(lhs != nullptr);
	check(rhs != nullptr);

	using namespace DirectX;
	XMFLOAT3 distanceVector = MathHelper::sub(lhs->getPosition(), rhs->getPosition());
		
	float radiusSum = lhs->getRadius() + rhs->getRadius();
	if(radiusSum * radiusSum < MathHelper::lengthSq(distanceVector))
	{
		return false;
	}

	switch (lhs->_characterInfo->getCollisionShape())
	{
		case CollisionShape::Sphere:
		{
			switch (rhs->_characterInfo->getCollisionShape())
			{
				case CollisionShape::Sphere:
				{
					return true;
				}
				break;
				case CollisionShape::Box:
				{
					return checkCollideBoxWithSphere(rhs, lhs);
				}
				break;
				case CollisionShape::Polygon:
				{
					return checkCollideSphereWithPolygon(lhs, rhs);
				}
				break;
				default:
				{
					check(false, "타입이 추가되었는지 확인해주세요.");
					static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입이 추가되었다면 작업해야합니다.");
					return false;
				}
			}
		}
		break;
		case CollisionShape::Box:
		{
			switch (lhs->_characterInfo->getCollisionShape())
			{
				case CollisionShape::Sphere:
				{
					return checkCollideBoxWithSphere(lhs, rhs);
				}
				break;
				case CollisionShape::Box:
				{
					return checkCollideBoxWithBox(lhs, rhs);
				}
				break;
				case CollisionShape::Polygon:
				{
					return checkCollideBoxWithPolygon(lhs, rhs);
				}
				break;
				default:
				{
					check(false, "타입이 추가되었는지 확인해주세요.");
					static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입이 추가되었다면 작업해야합니다.");
					return false;
				}
			}
		}
		break;
		case CollisionShape::Polygon:
		{
			switch (rhs->_characterInfo->getCollisionShape())
			{
				case CollisionShape::Sphere:
				{
					return checkCollideSphereWithPolygon(rhs, lhs);
				}
				break;
				case CollisionShape::Box:
				{
					return checkCollideBoxWithPolygon(rhs, lhs);
				}
				break;
				case CollisionShape::Polygon:
				{
					return checkCollidePolygonWithPolygon(lhs, rhs);
				}
				break;
				default:
				{
					check(false, "타입이 추가되었는지 확인해주세요.");
					static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입이 추가되었다면 작업해야합니다.");
					return false;
				}
			}
		}
		break;
		default:
		{
			check(false, "타입이 추가되었는지 확인해주세요.");
			static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입이 추가되었다면 작업해야합니다.");
			return false;
		}
	}
}

float Actor::getRadius(void) const noexcept
{
	return _size * _characterInfo->getRadiusXXX();
}

float Actor::getSizeX(void) const noexcept
{
	return _size * _characterInfo->getSizeXXXX();
}

float Actor::getSizeY(void) const noexcept
{
	return _size * _characterInfo->getSizeYXXX();
}

float Actor::getSizeZ(void) const noexcept
{
	return _size * _characterInfo->getSizeZXXX();
}

float Actor::getHalfHeight(void) const noexcept
{
	switch (_characterInfo->getCollisionShape())
	{
		case CollisionShape::Sphere:
		{
			return getRadius();
		}
		break;
		case CollisionShape::Box:
		{
			return getSizeY();
		}
		break;
		case CollisionShape::Polygon:
		{
			check(false, "미구현");
			return 0.f;
		}
		break;
		case CollisionShape::Count:
		default:
		{
			static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입 추가시 확인");
			check(false);
			return 0.f;
		}
		break;

	}
}

bool Actor::checkCollideBoxWithBox(const Actor* lhs, const Actor* rhs) noexcept
{
	using namespace DirectX;
	XMVECTOR lhsCenter = XMLoadFloat3(&lhs->_position);
	XMVECTOR rhsCenter = XMLoadFloat3(&rhs->_position);

	XMVECTOR lhsDirection = XMLoadFloat3(&lhs->_direction);
	XMVECTOR lhsUpVector = XMLoadFloat3(&lhs->_upVector);
	XMVECTOR rhsDirection = XMLoadFloat3(&rhs->_direction);
	XMVECTOR rhsUpVector = XMLoadFloat3(&rhs->_upVector);

	XMVECTOR axis[6] = { lhsDirection, lhsUpVector, XMVector3Cross(lhsDirection, lhsUpVector),
						rhsDirection, rhsUpVector, XMVector3Cross(rhsDirection, rhsUpVector) };

	float size[6] = { lhs->getSizeX(), lhs->getSizeY(), lhs->getSizeZ(),
						rhs->getSizeX(), rhs->getSizeY(), rhs->getSizeZ() };

	for (int i = 0; i < 6; ++i)
	{
		float sum = 0.f;
		for (int j = 0; j < 6; ++j)
		{
			sum += abs(XMVectorGetX(XMVector3Dot(axis[i], axis[j])) * size[j]);
		}
		if (abs(XMVectorGetX(XMVector3Dot(axis[i], rhsCenter - lhsCenter))) > sum)
		{
			return false;
		}
	}
	return true;
}

bool Actor::checkCollideBoxWithSphere(const Actor* lhs, const Actor* rhs) noexcept
{
	check(lhs->_characterInfo->getCollisionShape() == CollisionShape::Box, "타입에러");
	check(rhs->_characterInfo->getCollisionShape() == CollisionShape::Sphere, "타입에러");
	using namespace DirectX;
	XMVECTOR boxPosition = XMLoadFloat3(&lhs->_position);
	XMVECTOR spherePosition = XMLoadFloat3(&rhs->_position);
	XMVECTOR lhsToRhs = spherePosition - boxPosition;

	XMVECTOR boxDirection = XMLoadFloat3(&lhs->_direction);
	XMVECTOR boxUpVector = XMLoadFloat3(&lhs->_upVector);
	XMVECTOR axis[3] = { boxDirection, boxUpVector, XMVector3Cross(boxDirection, boxUpVector) };

	float size[3] = { lhs->getSizeX(), lhs->getSizeY(), lhs->getSizeZ() };

	for (int i = 0; i < 3; ++i)
	{
		if (abs(XMVectorGetX(XMVector3Dot(axis[i], lhsToRhs))) > size[i] + rhs->getRadius())
		{
			return false;
		}
	}

	XMVECTOR apex = boxPosition;
	for (int i = 0; i < 3; ++i)
	{
		if (XMVectorGetX(XMVector3Dot(lhsToRhs, axis[i])) < 0)
		{
			axis[i] *= -1;
		}
	}
	XMVECTOR apexToSphere = spherePosition - (boxPosition + axis[0] + axis[1] + axis[2]);
	if (XMVectorGetX(XMVector3Dot(axis[0], apexToSphere)) > 0 &&
		XMVectorGetX(XMVector3Dot(axis[1], apexToSphere)) > 0 &&
		XMVectorGetX(XMVector3Dot(axis[2], apexToSphere)) > 0)
	{
		if (XMVectorGetX(XMVector3LengthSq(apex - spherePosition)) > rhs->getRadius() * rhs->getRadius())
		{
			return false;
		}
	}
	
	return true;
}

bool Actor::checkCollideSphereWithPolygon(const Actor* lhs, const Actor* rhs) noexcept
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool Actor::checkCollideBoxWithPolygon(const Actor* lhs, const Actor* rhs) noexcept
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool Actor::checkCollidePolygonWithPolygon(const Actor* lhs, const Actor* rhs) noexcept
{
	throw std::logic_error("The method or operation is not implemented.");
}

DirectX::XMFLOAT3 Actor::getMoveVector(const TickCount64& deltaTick) const noexcept
{
	using namespace DirectX;

	XMVECTOR direction = XMVectorSet(0, 0, 0, 0);

	switch (_moveType)
	{
		case MoveType::CharacterDirection:
		{
			direction = XMLoadFloat3(&_direction);
		}
		break;
		case MoveType::JoystickDirection:
		{
			XMFLOAT2 stickInput = SMGFramework::Get().getStickInput(StickInputType::LStick);
			if (MathHelper::equal(stickInput.x, 0) && MathHelper::equal(stickInput.y, 0))
			{
				return XMFLOAT3(0, 0, 0);
			}
			const XMFLOAT3& camDirectionFloat = SMGFramework::getD3DApp()->getCameraDirection();
			XMVECTOR camDirection = XMLoadFloat3(&camDirectionFloat);
			XMVECTOR actorUpVector = XMLoadFloat3(&_upVector);
			XMVECTOR front = camDirection - XMVector3Dot(camDirection, actorUpVector) * actorUpVector;
			XMVECTOR right = XMVector3Cross(actorUpVector, front);

			direction = XMVector3Normalize(right * stickInput.x + front * stickInput.y);
			
		}
		break;
		case MoveType::Path:
		{
			check(false, "미구현");
		}
		break;
		case MoveType::Count:
		default:
		{
			static_assert(static_cast<int>(MoveType::Count) == 3, "타입 추가시 확인");
			check(false, "타입이 비정상입니다.");
		}
		break;
	}
	if (_moveDirectionOffset != 0)
	{
		XMMATRIX rotateMatrix = XMMatrixRotationNormal(XMLoadFloat3(&_upVector), _moveDirectionOffset);
		direction = XMVector3Transform(direction, rotateMatrix);
	}
	XMFLOAT3 resultVector;
	XMStoreFloat3(&resultVector, direction * _speed + XMLoadFloat3(&_additionalMoveVector));

	return resultVector;
}

void Actor::setPosition(const DirectX::XMFLOAT3& toPosition) noexcept
{
	_position = toPosition;
	_additionalMoveVector = { 0, 0, 0 };
	_sectorCoord = SMGFramework::getStageManager()->getSectorCoord(toPosition);
	if (_gravityPoint == nullptr || MathHelper::length(MathHelper::sub(_gravityPoint->_position, toPosition)) > _gravityPoint->_radius)
	{
		_gravityPoint = SMGFramework::getStageManager()->getGravityPointAt(toPosition);
	}
}

void Actor::addMoveVector(const DirectX::XMFLOAT3& moveVector) noexcept
{
	_additionalMoveVector = MathHelper::add(_additionalMoveVector, moveVector);
}

bool Actor::isActionEnd() const noexcept
{
	if (_gameObject->isSkinnedAnimationObject() == false)
	{
		return false;
	}
	return _gameObject->isAnimationEnd();
}

void Actor::updateActionChart(const TickCount64& deltaTick) noexcept
{
	TickCount64 progressedTick = _localTickCount + deltaTick;
	_currentActionState->processFrameEvents(*this, _localTickCount, progressedTick);
	_localTickCount = progressedTick;
	
	std::string nextStateName;
	if (_currentActionState->checkBranch(*this, nextStateName))
	{
		setActionState(nextStateName);
	}
}

void Actor::setActionState(const std::string& nextStateName) noexcept
{
	ActionState* nextState = _actionChart->getActionState(nextStateName);
	check(nullptr != nextState);

	_localTickCount = 0;
	if (_gameObject->isSkinnedAnimationObject())
	{
		_gameObject->setAnimation(nextState->getAnimationName(), nextState->getBlendTick());
	}
	_currentActionState = nextState;

	_currentActionState->processFrameEvents(*this, -1, 0);
	//0tick의 frameevent 진행
	updateObjectWorldMatrix();
}

void Actor::updateObjectWorldMatrix() noexcept
{
	_gameObject->setWorldMatrix(_position, _direction, _upVector, _size);
}

void Actor::setRotateType(const RotateType rotateType, const float rotateAngleOffset, const float speed) noexcept
{
	check(-MathHelper::Pi <= rotateAngleOffset && rotateAngleOffset < MathHelper::Pi);
	check(0 < speed);

	_rotateType = rotateType;
	_rotateAngleOffset = rotateAngleOffset;
	_rotateSpeed = speed;
}

void Actor::setAcceleration(const float acceleration, const float targetSpeed, MoveType moveType) noexcept
{
	check(acceleration > 0.f);
	check(targetSpeed >= 0.f);
	check(moveType != MoveType::Count);

	_acceleration = acceleration;
	_targetSpeed = targetSpeed;
	_moveType = moveType;
}

void Actor::setVerticalSpeed(const float speed) noexcept
{
	_verticalSpeed = speed;
}

void Actor::setTargetVerticalSpeed(const float targetVerticalSpeed) noexcept
{
	_targetVerticalSpeed = targetVerticalSpeed;
}

const CharacterInfo* Actor::getCharacterInfo(void) const noexcept
{
	return _characterInfo;
}

const GameObject* Actor::getGameObject(void) const noexcept
{
	return _gameObject;
}

const DirectX::XMINT3& Actor::getSectorCoord(void) const noexcept
{
	return _sectorCoord;
}

void Actor::processCollision(const Actor* collidingActor, CollisionCase collisionCase) noexcept
{
	check(this != nullptr);
	check(collidingActor != nullptr);

	_actionChart->processCollisionHandlers(*this, *collidingActor, collisionCase);
}

void Actor::setCulled(void) noexcept
{
	_gameObject->setCulled();
}

float Actor::getResistanceDistance(const Actor& selfActor, const Actor& targetActor) noexcept
{
	switch (selfActor.getCharacterInfo()->getCollisionType())
	{
		case CollisionType::SolidObject:
		{
			return 0.f;
		}
		break;
		case CollisionType::Character:
		{
			switch (targetActor.getCharacterInfo()->getCollisionType())
			{
				case CollisionType::SolidObject:
				{
					return 0.05f;
				}
				break;
				case CollisionType::Character:
				{
					return 0.025f;
				}
				break;
				case CollisionType::Item:
				{
					return 0.f;
				}
				break;
				case CollisionType::Count:
				default:
				{
					static_assert(static_cast<int>(CollisionType::Count) == 3, "타입 추가시 확인");
					check(false);
					return 0.f;
				}
				break;
			}
		}
		break;
		case CollisionType::Item:
		{
			return 0.f;
		}
		break;
		case CollisionType::Count:
		default:
		{
			static_assert(static_cast<int>(CollisionType::Count) == 3, "타입 추가시 확인");
			check(false);
			return 0.f;
		}
		break;
	}
}

PlayerActor::PlayerActor(const SpawnInfo& spawnInfo)
	: Actor(spawnInfo)
	, _isMoving(false)
{
}
