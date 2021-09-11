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
#include "Camera.h"
#include "Path.h"
#include "ObjectInfo.h"
#include "Effect.h"

Actor::Actor(const SpawnInfo& spawnInfo)
	: _position(spawnInfo.getPosition())
	, _direction(spawnInfo.getDirection())
	, _upVector(spawnInfo.getUpVector())
	, _size(spawnInfo.getSize())
	, _speed(0.f)
	, _verticalSpeed(0.f)
	, _moveType(MoveType::CharacterDirection)
	, _moveDirectionOffset(0.f)
	, _acceleration(0.f)
	, _targetSpeed(0.f)
	, _verticalAcceleration(0.f)
	, _targetVerticalSpeed(10.f)
	, _additionalMoveVector(0, 0, 0)
	, _sectorCoord(0, 0, 0)
	, _path(nullptr)
	, _pathTime(0)
	, _targetPosition(0, 0, 0)
	, _rotateType(RotateType::Fixed)
	, _rotateAngleOffset(0.f)
	, _rotateSpeed(0.f)
	, _gravityPoint(nullptr)
	, _isGravityOn(true)
	, _actionChart(nullptr)
	, _currentActionState(nullptr)
	, _actionIndex(0)
	, _localTickCount(0)
	, _onGround(false)
	, _isCollisionOn(true)
{
	_characterInfo = SMGFramework::getCharacterInfoManager()->getInfo(spawnInfo.getCharacterKey());
	// 시작할때 한번 검증하는 부분 만들면 옮겨야함. [5/20/2021 qwerw]
	if (nullptr == _characterInfo)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "캐릭터키가 비정상입니다. " + spawnInfo.getCharacterKey());
	}
	_actionChart = SMGFramework::getStageManager()->loadActionChartFromXML(_characterInfo->getActionChartFileName());
	const auto& idleAction = _actionChart->getActionState("IDLE");
	if (nullptr == idleAction)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, _characterInfo->getActionChartFileName() + " : IDLE 이 없습니다.");
	}
	_actionChartVariables = _actionChart->getVariables();
	_actionIndex = spawnInfo.getActionIndex();
	
	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(_characterInfo->getObjectFileName());

	_gravityPoint = SMGFramework::getStageManager()->getGravityPointAt(_position);
	_sectorCoord = SMGFramework::getStageManager()->getSectorCoord(_position);
	updateObjectWorldMatrix();
	setActionState("IDLE");
	if (_currentActionState == nullptr)
	{
		ThrowErrCode(ErrCode::ActionChartLoadFail, "기본액션 IDLE이 없습니다. " + _characterInfo->getActionChartFileName());
	}

}

Actor::~Actor()
{
	if (!SMGFramework::getStageManager()->isLoading())
	{
		releaseChildEffects();
#if defined DEBUG | defined _DEBUG
		for (const auto& devObject : _gameObject->_devObjects)
		{
			SMGFramework::getD3DApp()->removeGameObject(devObject);
		}
		_gameObject->_devObjects.clear();
#endif
		SMGFramework::getD3DApp()->removeGameObject(_gameObject);
	}
}

void Actor::rotateOnPlane(const float rotateAngle) noexcept
{
	check(!MathHelper::equal(rotateAngle, 0.f));
	check(!isnan(rotateAngle));

	XMVECTOR direction = XMLoadFloat3(&_direction);

	XMMATRIX rotateMatrix = XMMatrixRotationNormal(XMLoadFloat3(&_upVector), rotateAngle);
	direction = XMVector3Transform(direction, rotateMatrix);
	
	XMStoreFloat3(&_direction, XMVector3Normalize(direction));

	if (_rotateType == RotateType::Fixed || _rotateType == RotateType::ToCollidingTarget)
	{
		_rotateAngleOffset -= rotateAngle;
		static_assert(static_cast<int>(RotateType::Count) == 7, "타입 추가시 확인");
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
		upVector *= dot;

		_direction = _upVector;
		XMStoreFloat3(&_upVector, upVector);
		return;
	}

	XMMATRIX rotateMatrix = XMMatrixRotationAxis(XMVector3Cross(upVector, currentDirection), MathHelper::Pi_DIV2 - acos(dot));
	XMVECTOR newDirection = XMVector3Transform(currentDirection, rotateMatrix);
	check(!isnan(XMVectorGetX(newDirection)));
	XMStoreFloat3(&_upVector, XMVector3Normalize(upVector));
	XMStoreFloat3(&_direction, XMVector3Normalize(newDirection));
}

void Actor::rotateQuat(const DirectX::XMFLOAT4& rotationQuat) noexcept
{
	XMVECTOR upVector, direction, rightVector;
	MathHelper::getRotatedAxis(XMLoadFloat4(&rotationQuat), upVector, direction, rightVector);

	XMStoreFloat3(&_upVector, upVector);
	XMStoreFloat3(&_direction, direction);
}

DirectX::XMFLOAT4 Actor::getRotationQuat(const TickCount64& deltaTick) const noexcept
{
	using namespace DirectX;
	check(isQuaternionRotate());
	XMVECTOR rotationQuat = XMVectorZero();
	switch (_rotateType)
	{
		case RotateType::Path:
		{
			check(_path != nullptr);
			XMFLOAT4 rotationQuatF;
			_path->getPathRotationAtTime(_pathTime, rotationQuatF, InterpolationType::Linear);
			rotationQuat = XMLoadFloat4(&rotationQuatF);
		}
		break;
		default:
		{
			static_assert(static_cast<int>(RotateType::Count) == 7, "타입 추가시 확인");
			check(false, "타입 추가시 확인");
		}
	}

	float maxAngle = _rotateSpeed * deltaTick;
	XMVECTOR upVector = XMLoadFloat3(&_upVector);
	XMVECTOR direction = XMLoadFloat3(&_direction);
	XMVECTOR rotationQuatOrigin = MathHelper::getQuaternion(upVector, direction);
	float deltaAngle = XMVectorGetX(XMQuaternionLength(rotationQuatOrigin - rotationQuat));
	
	XMFLOAT4 rotationQuatF;
	if (maxAngle < deltaAngle)
	{
		XMStoreFloat4(&rotationQuatF,
			XMQuaternionSlerp(rotationQuatOrigin, rotationQuat, deltaAngle / maxAngle));
	}
	else
	{
		XMStoreFloat4(&rotationQuatF, rotationQuat);
	}
	return rotationQuatF;
}

void Actor::setPath(int key) noexcept
{
	if (key < 0)
	{
		_path = nullptr;
		_pathTime = 0;
		return;
	}
	auto path = SMGFramework::getStageManager()->getStageInfo()->getPath(key);
	if (path == nullptr)
	{
		check(false, std::to_string(key) + " path가 없습니다.");
		return;
	}

	_path = path;
	_pathTime = 0;
}

const Path* Actor::getPath(void) const noexcept
{
	return _path;
}

bool Actor::isPathEnd(void) const noexcept
{
	if (_path == nullptr || _path->getMoveTick() < _pathTime)
	{
		return true;
	}
	return false;
}

void Actor::setAnimationSpeed(float speed) noexcept
{
	_gameObject->setAnimationSpeed(speed);
}

void Actor::setNextActionState(const std::string& actionStateName) noexcept
{
	check(!actionStateName.empty());
	_nextActionStateName = actionStateName;
}

float Actor::getRotateAngleDelta(const TickCount64& deltaTick) const noexcept
{
	check(!isQuaternionRotate());
	float deltaAngle = 0.f;
	
	switch (_rotateType)
	{
		case RotateType::Fixed:
		case RotateType::ToCollidingTarget:
		{
			deltaAngle = _rotateAngleOffset;
		}
		break;
		case RotateType::ToPlayer:
		{
			check(SMGFramework::getStageManager()->getPlayerActor() != this);
			const Actor* player = SMGFramework::getStageManager()->getPlayerActor();
			if (nullptr != player)
			{
				XMVECTOR toPlayer = XMLoadFloat3(&player->getPosition()) - XMLoadFloat3(&_position);
				XMVECTOR upVector = XMLoadFloat3(&_upVector);
				XMVECTOR direction = XMLoadFloat3(&_direction);

				deltaAngle = MathHelper::getDeltaAngleToVector(upVector, direction, toPlayer);
			}
		}
		break;
		case RotateType::JoystickInput:
		{
			XMFLOAT2 stickInput = SMGFramework::Get().getStickInput(StickInputType::LStick);

			XMFLOAT3 camDirectionF = SMGFramework::getCamera()->getDirection();
			XMVECTOR camDirection = XMLoadFloat3(&camDirectionF);
			XMVECTOR camUpVector = XMLoadFloat3(&SMGFramework::getCamera()->getUpVector());
			XMVECTOR camRight = XMVector3Cross(camUpVector, camDirection);

			XMVECTOR actorDirection = XMLoadFloat3(&_direction);
			XMVECTOR actorUpVector = XMLoadFloat3(&_upVector);

			XMVECTOR stickInputVector = camUpVector * stickInput.y + camRight * stickInput.x;
			deltaAngle = MathHelper::getDeltaAngleToVector(actorUpVector, actorDirection, stickInputVector);
		}
		break;
		case RotateType::ToWall:
		{
			check(false, "미구현");
		}
		break;
		case RotateType::FixedSpeed:
		{
			deltaAngle = _rotateSpeed * deltaTick;
		}
		break;
		case RotateType::Count:
		default:
		{
			static_assert(static_cast<int>(RotateType::Count) == 7, "타입 추가시 확인");
			check(false, "타입 추가시 확인");
		}
		break;
	}

	const float maxAngle = _rotateSpeed * deltaTick;
	return std::clamp(deltaAngle, -maxAngle, maxAngle);
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
			gravityDirection = _gravityPoint->_direction;
		}
		break;
		case GravityPointType::Point:
		{
			XMVECTOR actorPosition = XMLoadFloat3(&_position);
			XMVECTOR toGravityPoint = XMLoadFloat3(&_gravityPoint->_position) - actorPosition;
			float toGravityPointLength = XMVectorGetX(XMVector3Length(toGravityPoint));
			if (toGravityPointLength < _gravityPoint->_minRadius)
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
			_verticalSpeed += _gravityPoint->_gravity * _verticalAcceleration * deltaTick;
			_verticalSpeed = std::min(_verticalSpeed, _targetVerticalSpeed);
		}
		else
		{
			_verticalSpeed -= _gravityPoint->_gravity * _verticalAcceleration * deltaTick;
			_verticalSpeed = std::max(_verticalSpeed, _targetVerticalSpeed);
		}
	}
	if (_path != nullptr)
	{
		_pathTime += deltaTick;
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

void Actor::setActorOnWall(bool onWall) noexcept
{
	_onWall = onWall;
}

bool Actor::isOnWall(void) const noexcept
{
	return _onWall;
}

bool Actor::isQuaternionRotate(void) const noexcept
{
	static_assert(static_cast<int>(RotateType::Count) == 7, "타입 추가시 확인");
	return _rotateType == RotateType::Path;
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

	if (!checkCharacterTypeCollision(lhs, rhs))
	{
		return false;
	}
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
					static_assert(static_cast<int>(CollisionShape::Count) == 4, "타입이 추가되었다면 작업해야합니다.");
					return false;
				}
			}
		}
		break;
		case CollisionShape::Box:
		{
			switch (rhs->_characterInfo->getCollisionShape())
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
					static_assert(static_cast<int>(CollisionShape::Count) == 4, "타입이 추가되었다면 작업해야합니다.");
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
					static_assert(static_cast<int>(CollisionShape::Count) == 4, "타입이 추가되었다면 작업해야합니다.");
					return false;
				}
			}
		}
		break;
		default:
		{
			check(false, "타입이 추가되었는지 확인해주세요.");
			static_assert(static_cast<int>(CollisionShape::Count) == 4, "타입이 추가되었다면 작업해야합니다.");
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
			static_assert(static_cast<int>(CollisionShape::Count) == 4, "타입 추가시 확인");
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
	XMVECTOR axis[3] = { boxDirection, boxUpVector, XMVector3Cross(boxUpVector, boxDirection) };

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
		apex += axis[i] * size[i];
	}
	
	XMVECTOR apexToSphere = spherePosition - apex;
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
	check(false, "미구현");
	return false;
}

bool Actor::checkCollideBoxWithPolygon(const Actor* lhs, const Actor* rhs) noexcept
{
	check(false, "미구현");
	return false;
}

bool Actor::checkCollidePolygonWithPolygon(const Actor* lhs, const Actor* rhs) noexcept
{
	check(false, "미구현");
	return false;
}

DirectX::XMFLOAT3 Actor::getMoveVector(const TickCount64& deltaTick) const noexcept
{
	using namespace DirectX;

	XMVECTOR direction = XMVectorSet(0, 0, 0, 0);
	bool applySpeed = true;

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
			const XMFLOAT3& camDirectionFloat = SMGFramework::getCamera()->getDirection();
			XMVECTOR camDirection = XMLoadFloat3(&camDirectionFloat);
			XMVECTOR actorUpVector = XMLoadFloat3(&_upVector);
			XMVECTOR front = camDirection - XMVector3Dot(camDirection, actorUpVector) * actorUpVector;
			XMVECTOR right = XMVector3Cross(actorUpVector, front);

			direction = XMVector3Normalize(right * stickInput.x + front * stickInput.y);
			
		}
		break;
		case MoveType::Path:
		{
			check(_path != nullptr);
			applySpeed = false;

			XMFLOAT3 position;
			_path->getPathPositionAtTime(_pathTime, position, InterpolationType::Linear);

			direction = XMLoadFloat3(&position) - XMLoadFloat3(&_position);
		}
		break;
		case MoveType::ToPoint:
		{
			direction = XMLoadFloat3(&_targetPosition) - XMLoadFloat3(&_position);
			float distance = XMVectorGetX(XMVector3Length(direction));
			if (distance < _speed/**deltaTick */)
			{
				applySpeed = false;
			}
			else
			{
				direction /= distance;
			}
		}
		break;
		case MoveType::Count:
		default:
		{
			static_assert(static_cast<int>(MoveType::Count) == 4, "타입 추가시 확인");
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
	if (applySpeed)
	{
		direction *= _speed /** deltaTick*/;
	}
	XMStoreFloat3(&resultVector, direction + XMLoadFloat3(&_additionalMoveVector));

	return resultVector;
}

void Actor::setPosition(const DirectX::XMFLOAT3& toPosition) noexcept
{
	check(!isnan(toPosition.x));
	check(!isnan(toPosition.y));
	check(!isnan(toPosition.z));

	_position = toPosition;
	_additionalMoveVector = { 0, 0, 0 };
	_sectorCoord = SMGFramework::getStageManager()->getSectorCoord(toPosition);
	if (_gravityPoint == nullptr || MathHelper::length(MathHelper::sub(_gravityPoint->_position, toPosition)) > _gravityPoint->_radius)
	{
		if (_isGravityOn)
		{
			_gravityPoint = SMGFramework::getStageManager()->getGravityPointAt(toPosition);
			_verticalSpeed = 0.f;
		}
	}

	for (const auto& childEffect : _childEffects)
	{
		XMFLOAT3 position = MathHelper::add(_actionChart->getChildEffectOffset(childEffect.first), _position);

		childEffect.second->setPosition(position);
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
	
	if (!_nextActionStateName.empty())
	{
		setActionState(_nextActionStateName);
		_nextActionStateName.clear();
	}
	else
	{
		std::string nextStateName;
		if (_currentActionState->checkBranch(*this, nextStateName))
		{
			setActionState(nextStateName);
		}
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

void Actor::setAcceleration(float acceleration, float targetSpeed, float moveDirectionOffset, MoveType moveType) noexcept
{
	check(acceleration > 0.f);
	check(targetSpeed >= 0.f);
	check(moveType != MoveType::Count);

	_acceleration = acceleration;
	_targetSpeed = targetSpeed;
	_moveDirectionOffset = moveDirectionOffset;
	_moveType = moveType;
}

void Actor::setVerticalSpeed(float speed) noexcept
{
	_verticalSpeed = speed;
}

void Actor::setTargetVerticalSpeed(float targetVerticalSpeed, float acceleration) noexcept
{
	_targetVerticalSpeed = targetVerticalSpeed;
	_verticalAcceleration = acceleration;
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

void Actor::setActionChartVariable(const std::string& name, int value) noexcept
{
	auto it = _actionChartVariables.find(name);
	if (it == _actionChartVariables.end())
	{
		check(false, name);
		return;
	}
	it->second = value;
}

int Actor::getActionChartVariable(const std::string& name) const noexcept
{
	auto it = _actionChartVariables.find(name);
	if (it == _actionChartVariables.end())
	{
		check(false, name);
		return 0;
	}
	return it->second;
}

int Actor::getActionIndex(void) const noexcept
{
	return _actionIndex;
}

void Actor::setGravityOn(bool on) noexcept
{
	check(SMGFramework::getStageManager() != nullptr);

	_isGravityOn = on;
	if (on)
	{
		_gravityPoint = SMGFramework::getStageManager()->getGravityPointAt(_position);
		_verticalSpeed = 0.f;
	}
	else
	{
		_gravityPoint = nullptr;
	}
}

void Actor::setCollisionOn(bool on) noexcept
{
	_isCollisionOn = on;
}

bool Actor::isCollisionOn(void) const noexcept
{
	return _isCollisionOn;
}

void Actor::setTargetPosition(const DirectX::XMFLOAT3& position) noexcept
{
	_targetPosition = position;
}

const DirectX::XMFLOAT3& Actor::getTargetPosition(void) const noexcept
{
	return _targetPosition;
}

void Actor::enableChildEffect(int effectKey) noexcept
{
	auto it = _childEffects.find(effectKey);
	if (it != _childEffects.end())
	{
		return;
	}

	ChildEffectInfo childEffectInfo;
	bool success = _actionChart->getChildEffectInfo(effectKey, childEffectInfo);
	if (!success)
	{
		return;
	}
	
	XMFLOAT3 position = MathHelper::add(childEffectInfo._positionOffset, _position);
	EffectInstance instance(position, _size * childEffectInfo._size);
	auto instancePtr = SMGFramework::getEffectManager()->addConstantEffectInstance(childEffectInfo._effectName, std::move(instance));
	if (instancePtr == nullptr)
	{
		return;
	}

	_childEffects.emplace(effectKey, instancePtr);
}
void Actor::releaseChildEffects(void) noexcept
{
	for (const auto& effect : _childEffects)
	{
		ChildEffectInfo childEffectInfo;
		bool success = _actionChart->getChildEffectInfo(effect.first, childEffectInfo);
		if (!success)
		{
			continue;
		}
		SMGFramework::getEffectManager()->removeConstantEffectInstance(childEffectInfo._effectName, effect.second);

	}
}
void Actor::disableChildEffect(int effectKey) noexcept
{
	auto it = _childEffects.find(effectKey);
	if (it == _childEffects.end())
	{
		return;
	}

	ChildEffectInfo childEffectInfo;
	bool success = _actionChart->getChildEffectInfo(effectKey, childEffectInfo);
	if (!success)
	{
		return;
	}

	SMGFramework::getEffectManager()->removeConstantEffectInstance(childEffectInfo._effectName, it->second);
	_childEffects.erase(it);
}

void Actor::changeMaterial(uint8_t renderItemIndex, const std::string& fileName, const std::string& name) noexcept
{
	_gameObject->changeMaterial(renderItemIndex, fileName, name);
}

bool Actor::checkCharacterTypeCollision(const Actor* lhs, const Actor* rhs) noexcept
{
	static_assert(static_cast<int>(CharacterType::Count) == 5, "타입 추가시 함수 전체 확인해야함");
	check(lhs->_characterInfo->getCharacterType() != CharacterType::Count);
	check(rhs->_characterInfo->getCharacterType() != CharacterType::Count);

	switch (lhs->_characterInfo->getCharacterType())
	{
		case CharacterType::Player:
		{
			switch (rhs->_characterInfo->getCharacterType())
			{
				case CharacterType::Player:
				case CharacterType::Monster:
				case CharacterType::Object:
				case CharacterType::Item:
				{
					return true;
				}
				break;
				case CharacterType::PlayerAttackObject:
				{
					return false;
				}
				break;
			}
		}
		break;
		case CharacterType::Monster:
		{
			switch (rhs->_characterInfo->getCharacterType())
			{
				case CharacterType::Player:
				case CharacterType::Monster:
				case CharacterType::PlayerAttackObject:
				case CharacterType::Object:
				{
					return true;
				}
				break;
				case CharacterType::Item:
				{
					return false;
				}
				break;
			}
		}
		break;
		case CharacterType::Object:
		{
			switch (rhs->_characterInfo->getCharacterType())
			{
				case CharacterType::Player:
				case CharacterType::Monster:
				case CharacterType::Object:
				{
					return true;
				}
				break;
				case CharacterType::PlayerAttackObject:
				case CharacterType::Item:
				{
					return false;
				}
				break;
			}
		}
		break;
		case CharacterType::PlayerAttackObject:
		{
			switch (rhs->_characterInfo->getCharacterType())
			{
				case CharacterType::Monster:
				{
					return true;
				}
				break;
				case CharacterType::Player:
				case CharacterType::PlayerAttackObject:
				case CharacterType::Item:
				case CharacterType::Object:
				{
					return false;
				}
				break;
			}
		}
		break;
		case CharacterType::Item:
		{
			switch (rhs->_characterInfo->getCharacterType())
			{
				case CharacterType::Player:
				{
					return true;
				}
				break;
				case CharacterType::Monster:
				case CharacterType::PlayerAttackObject:
				case CharacterType::Item:
				case CharacterType::Object:
				{
					return false;
				}
				break;
			}
		}
		break;
	}
	check(false);
	return false;
}

void Actor::setChildEffectAlpha(int effectKey, float alpha, TickCount64 blendTick)
{
	auto it = _childEffects.find(effectKey);
	if (it == _childEffects.end())
	{
		check(false);
		return;
	}
	it->second->setAlpha(alpha, blendTick);
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
					return 0.1f;
				}
				break;
				case CollisionType::Character:
				{
					return 0.05f;
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

bool XM_CALLCONV Actor::checkCollisionWithLine(FXMVECTOR start, FXMVECTOR velocity, float& collisionTime) noexcept
{
	// 우선은 적당히 위치만 나오면 되니까 구체로만 검사한다. [9/10/2021 qwerw]
	XMVECTOR sphereCenter = XMLoadFloat3(&_position);
	float speed = XMVectorGetX(XMVector3Length(velocity));
	check(!MathHelper::equal(speed, 0));
	float distance = XMVectorGetX(XMVector3Length(XMVector3Cross(sphereCenter - start, velocity / speed)));
	float radius = getRadius();
	if (radius < distance)
	{
		return false;
	}

	float centerDistance = XMVectorGetX(XMVector3Length(start - XMLoadFloat3(&_position)));

	float t = sqrt(centerDistance * centerDistance - distance * distance)
				- sqrt(radius * radius - distance * distance);
	if (speed < t)
	{
		return false;
	}
	collisionTime = t / speed;
	return true;
}

bool Actor::checkPointerPicked(void) const noexcept
{
	return (_characterInfo->getCharacterType() == CharacterType::Item);
}

bool Actor::isPlaneMove(void) const noexcept
{
	return _moveType != MoveType::ToPoint &&
		_moveType != MoveType::Path;

	static_assert(static_cast<int>(MoveType::Count) == 4);
}