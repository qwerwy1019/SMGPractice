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

Actor::Actor(const SpawnInfo& spawnInfo)
	: _position(spawnInfo._position)
	, _direction(spawnInfo._direction)
	, _upVector(spawnInfo._upVector)
	, _size(1.f)
	, _speed(0.f)
	, _verticalSpeed(0.f)
	, _moveDirectionOffset(0.f)
	, _acceleration(0.f)
	, _maxSpeed(0.f)
	, _minSpeed(0.f)
	, _rotateType(RotateType::Count)
	, _rotateAngleLeft(0.f)
	, _rotateSpeed(0.f)
	, _gravityPointIndex(-1)
	, _localTickCount(0)
{
	_characterInfo = SMGFramework::getCharacterInfoManager()->getInfo(spawnInfo._key);
	// 시작할때 한번 검증하는 부분 만들면 옮겨야함. [5/20/2021 qwerw]
	if (nullptr == _characterInfo)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "캐릭터키가 비정상입니다. " + spawnInfo._key);
	}
	_gameObject = SMGFramework::getD3DApp()->createObjectFromXML(_characterInfo->getObjectFileName());
	_actionChart = SMGFramework::getStageManager()->loadActionChartFromXML(_characterInfo->getActionChartFileName());

	_currentActionState = _actionChart->getActionState("IDLE");
	if (_currentActionState == nullptr)
	{
		ThrowErrCode(ErrCode::ActionChartLoadFail, "기본액션 IDLE이 없습니다. " + _characterInfo->getActionChartFileName());
	}
}

Actor::~Actor()
{

}

void Actor::rotateOnPlane(const float rotateAngle) noexcept
{
	check(!MathHelper::equal(rotateAngle, 0.f));

	_rotateAngleLeft -= rotateAngle;
	XMVECTOR direction = XMLoadFloat3(&_direction);

	XMMATRIX rotateMatrix = XMMatrixRotationNormal(XMLoadFloat3(&_upVector), rotateAngle);
	direction = XMVector3Transform(direction, rotateMatrix);
	
	XMStoreFloat3(&_direction, direction);
}

float Actor::getRotateAngleDelta(const TickCount64& deltaTick) const noexcept
{
	float maxAngle = _rotateSpeed * deltaTick;
	
	if (_rotateAngleLeft < 0)
	{
		return std::max(_rotateAngleLeft, -maxAngle);
	}
	else
	{
		return std::min(_rotateAngleLeft, maxAngle);
	}
}

void Actor::updateActor(const TickCount64& deltaTick) noexcept
{
	updateActionChart(deltaTick);

	// 가속도 적용
	_speed += _acceleration * deltaTick;
	_speed = std::max(_minSpeed, std::min(_speed, _maxSpeed));
	//_verticalSpeed += getGravity() * deltaTick;
}

TickCount64 Actor::getLocalTickCount(void) const noexcept
{
	return _localTickCount;
}

DirectX::XMFLOAT3 Actor::getPosition(void) const noexcept
{
	return _position;
}

bool Actor::checkCollision(const Actor* otherActor, DirectX::FXMVECTOR moveVector, CollisionInfo& outCollisionInfo) const noexcept
{
	using namespace DirectX;
	// 아직 미구현 [3/17/2021 qwerwy]
// 	bool checkPathCollision = true;
// 	if (checkPathCollision && _characterInfo->getCollisionShape() == CollisionShape::Sphere)
// 	{
// 		switch (otherActor->_characterInfo->getCollisionShape())
// 		{
// 			case CollisionShape::Sphere:
// 			{
// 				const float moveLength = XMVectorGetX(XMVector3Length(moveVector));
// 				const XMVECTOR moveVectorNomalized = moveVector / moveLength;
// 				const XMVECTOR toCenter = otherActor->getPosition() - getPosition();
// 				const float dot = XMVectorGetX(XMVector3Dot(moveVectorNomalized, toCenter));
// 				if (dot < 0)
// 				{
// 					return false;
// 				}
// 
// 				const float radiusSum = getRadius() + otherActor->getRadius();
// 				const float toCenterLengthSq = XMVectorGetX(XMVector3LengthSq(toCenter));
// 				if ((radiusSum + moveLength) * (radiusSum + moveLength) < toCenterLengthSq)
// 				{
// 					return false;
// 				}
// 				const float centerToMoveLengthSq = toCenterLengthSq - dot * dot;
// 				
// 				if (centerToMoveLengthSq > radiusSum * radiusSum)
// 				{
// 					return false;
// 				}
// 
// 				const float resultMoveVectorLength = dot - sqrt(radiusSum * radiusSum - centerToMoveLengthSq);
// 				if(moveLength < resultMoveVectorLength)
// 				{
// 					return false;
// 				}
// 
// 				resultMoveVector = moveVectorNomalized * resultMoveVectorLength;
// 				return true;
// 			}
// 			break;
// 			case CollisionShape::Box:
// 			{
// 				int i = 0, y = 0;
// 				const float moveLength = XMVectorGetX(XMVector3Length(moveVector));
// 				const XMVECTOR moveVectorNomalized = moveVector / moveLength;
// 				const XMVECTOR position = otherActor->getPosition();
// 				const XMVECTOR direction = XMLoadFloat3(&otherActor->_direction);
// 				const XMVECTOR upVector = XMLoadFloat3(&otherActor->_upVector);
// 				const XMVECTOR sideVector = XMVector3Cross(direction, upVector);
// 
// 				const XMVECTOR normal[6] = { direction, upVector, sideVector,
// 						-direction, -upVector, -sideVector };
// 				
// 				const float size[6] = { getSizeX(), getSizeY(), getSizeZ(),
// 									otherActor->getSizeX(), otherActor->getSizeY(), otherActor->getSizeZ() };
// 
// 				for (int i = 0; i < 6; ++i)
// 				{
// 					float dot = XMVectorGetX(XMVector3Dot(normal[i], moveVectorNomalized));
// 					if (dot <= 0)
// 					{
// 						// 평행할때 충돌 체크를 할지 말지 고민해봐야함 [3/13/2021 qwerwy]
// 						return false;
// 					}
// 					
// 					const float planeConstant = XMVectorGetX(XMVector3Dot(normal[i] * size[i] + position, normal[i]));
// 					
// 					float moveLength = planeConstant + _characterInfo->getRadius() - XMVectorGetX(XMVector3Dot(normal[i], getPosition()));
// 					moveLength /= XMVectorGetX(XMVector3Dot(normal[i], moveVectorNomalized));
// 
// 					
// 				}
// 				XMVECTOR intersectPoint = XMPlaneIntersectLine();
// 				XMVECTOR toIntersectPoint = getPosition() - intersectPoint;
// 
// 
// 			}
// 			break;
// 			case CollisionShape::Polygon:
// 			{
// 				for (auto p : polygons)
// 				{
// 					XMVECTOR plane = XMPlaneFromPoints(p[0], p[1], p[2]);
// 					
// 				}
// 			}
// 			break;
// 			default:
// 			{
// 				ThrowErrCode(ErrCode::UndefinedType, "타입이 추가되었는지 확인해주세요.");
// 				static_assert(static_cast<int>(CollisionShape::Count) == 3, "타입이 추가되었다면 작업해야합니다.");
// 			}
// 		}
// 		// path check
// 		currentPosition;
// 		moveVector;
// 		planesOfotherActor;
// 
// 		for all PLANES
// 		{
// 			
// 		}
// 		POINT planePoint = _position + direction * sizeZ * _size;
// 		checkCollision
// 		XMPlaneFromPointNormal()
// 		plane dot moveVector
// 		XMPlaneIntersectLine(plane, currentPosition, moveVector)
// 	}
// 	else
	{
		XMFLOAT3 distanceVector = MathHelper::sub(otherActor->getPosition(), getPosition());
		
		float radiusSum = getRadius() + otherActor->getRadius();
		if(radiusSum * radiusSum < MathHelper::distanceSq(distanceVector))
		{
			return false;
		}

		switch (_characterInfo->getCollisionShape())
		{
			case CollisionShape::Sphere:
			{
				switch (otherActor->_characterInfo->getCollisionShape())
				{
					case CollisionShape::Sphere:
						return true;
					break;
					case CollisionShape::Box:
					{
						return checkCollideBoxWithSphere(otherActor, this, -moveVector, outCollisionInfo);
					}
					break;
					case CollisionShape::Polygon:
					{
						return checkCollideSphereWithPolygon(this, otherActor, moveVector, outCollisionInfo);
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
				switch (otherActor->_characterInfo->getCollisionShape())
				{
					case CollisionShape::Sphere:
					{
						return checkCollideBoxWithSphere(this, otherActor, moveVector, outCollisionInfo);
					}
					break;
					case CollisionShape::Box:
					{
						return checkCollideBoxWithBox(this, otherActor, moveVector, outCollisionInfo);
					}
					break;
					case CollisionShape::Polygon:
					{
						return checkCollideBoxWithPolygon(this, otherActor, moveVector, outCollisionInfo);
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
				switch (otherActor->_characterInfo->getCollisionShape())
				{
					case CollisionShape::Sphere:
					{
						return checkCollideSphereWithPolygon(otherActor, this, -moveVector, outCollisionInfo);
					}
					break;
					case CollisionShape::Box:
					{
						return checkCollideBoxWithPolygon(otherActor, this, -moveVector, outCollisionInfo);
					}
					break;
					case CollisionShape::Polygon:
					{
						return checkCollidePolygonWithPolygon(this, otherActor, moveVector, outCollisionInfo);
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
		// obb check
	}
}

float Actor::getRadius(void) const noexcept
{
	return _size * _characterInfo->getRadius();
}

float Actor::getSizeX(void) const noexcept
{
	return _size * _characterInfo->getSizeX();
}

float Actor::getSizeY(void) const noexcept
{
	return _size * _characterInfo->getSizeY();
}

float Actor::getSizeZ(void) const noexcept
{
	return _size * _characterInfo->getSizeZ();
}

bool XM_CALLCONV Actor::checkCollideBoxWithBox(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept
{
	using namespace DirectX;
	XMVECTOR lhsCenter = XMLoadFloat3(&lhs->_position) + lhsMoveVector;
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

bool XM_CALLCONV Actor::checkCollideBoxWithSphere(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept
{
	check(lhs->_characterInfo->getCollisionShape() == CollisionShape::Box, "타입에러");
	check(rhs->_characterInfo->getCollisionShape() == CollisionShape::Sphere, "타입에러");
	using namespace DirectX;
	XMVECTOR boxPosition = XMLoadFloat3(&lhs->_position);
	XMVECTOR spherePosition = XMLoadFloat3(&rhs->_position) + lhsMoveVector;
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

bool XM_CALLCONV Actor::checkCollideSphereWithPolygon(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool XM_CALLCONV Actor::checkCollideBoxWithPolygon(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept
{
	throw std::logic_error("The method or operation is not implemented.");
}

bool XM_CALLCONV Actor::checkCollidePolygonWithPolygon(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept
{
	throw std::logic_error("The method or operation is not implemented.");
}

DirectX::XMFLOAT3 Actor::getMoveVector(const TickCount64& deltaTick) const noexcept
{
	using namespace DirectX;

	XMVECTOR direction = XMLoadFloat3(&_direction);
	if (_moveDirectionOffset != 0)
	{
		XMMATRIX rotateMatrix = XMMatrixRotationNormal(XMLoadFloat3(&_upVector), _moveDirectionOffset);
		direction = XMVector3Transform(direction, rotateMatrix);
	}
	XMFLOAT3 resultVector;
	XMStoreFloat3(&resultVector, direction * _speed);
	return resultVector;
}

void Actor::setPosition(const DirectX::XMFLOAT3& toPosition) noexcept
{
	_position = toPosition;
}

bool Actor::isActionEnd() const noexcept
{
	if (nullptr == _gameObject->_skinnedModelInstance)
	{
		return false;
	}
	return _gameObject->_skinnedModelInstance->isAnimationEnd();
}

void Actor::updateActionChart(const TickCount64& deltaTick) noexcept
{
	TickCount64 progressedTick = _localTickCount + deltaTick;
	_currentActionState->processFrameEvents(*this, _localTickCount, progressedTick);
	_localTickCount = progressedTick;
	
	std::string nextStateName;
	if (_currentActionState->checkBranch(*this, nextStateName))
	{
		ActionState* nextState = _actionChart->getActionState(nextStateName);
		setActionState(nextState);
	}
}

void Actor::setActionState(const ActionState* nextState) noexcept
{
	_localTickCount = 0;
	if (_gameObject->_skinnedModelInstance != nullptr)
	{
		_gameObject->_skinnedModelInstance->setAnimation(nextState->getAnimationName(), nextState->getBlendTick());
	}

	//0tick의 frameevent 진행
	//updateRenderWorldMatrix();
}

void Actor::updateObjectWorldMatrix() noexcept
{
	using namespace DirectX;

	XMMATRIX position = XMMatrixTranslation(_position.x, _position.y, _position.z);
	
	XMMATRIX rotation = XMMatrixSet(_direction.x, _direction.y, _direction.z, 0,
									_upVector.x, _upVector.y, _upVector.z, 0,
									_direction.y * _upVector.z - _direction.z * _upVector.y,
									_direction.z * _upVector.x - _direction.x * _upVector.z,
									_direction.x * _upVector.y - _direction.y * _upVector.x, 0,
									0, 0, 0, 1);
	XMMATRIX scaling = XMMatrixScaling(_characterInfo->getSizeX(), _characterInfo->getSizeY(), _characterInfo->getSizeZ()) * _size;
	
	XMStoreFloat4x4(&_gameObject->_worldMatrix, scaling * rotation * position);
	_gameObject->_dirtyFrames = FRAME_RESOURCE_COUNT;
}

void Actor::setRotateType(const RotateType rotateType, const float rotateAngle, const float speed) noexcept
{
	check(-MathHelper::Pi <= rotateAngle && rotateAngle < MathHelper::Pi);
	check(0 < speed);

	_rotateType = rotateType;
	_rotateAngleLeft = rotateAngle;
	_rotateSpeed = speed;
}

void Actor::setAcceleration(const float acceleration, const float maxSpeed) noexcept
{
	check(acceleration > 0.f);
	check(maxSpeed > 0.f);
	_acceleration = acceleration;
	_maxSpeed = maxSpeed;
}

void Actor::setDeceleration(const float deceleration, const float minSpeed) noexcept
{
	check(deceleration > 0.f);
	check(minSpeed >= 0.f);
	_acceleration = -deceleration;
	_minSpeed = minSpeed;
}

void Actor::setVerticalSpeed(const float speed) noexcept
{
	_verticalSpeed = speed;
}
