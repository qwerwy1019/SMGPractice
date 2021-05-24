#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

class CharacterInfo;
class ActionChart;
class ActionState;
struct SpawnInfo;
struct GameObject;

struct CollisionInfo
{
	float _collisionTime; // 0 ~ 1
	DirectX::XMFLOAT3 _collisionPoint; // {-1 ~ 1, -1 ~ 1, -1 ~ 1}
};

class Actor
{
public:
	Actor(const SpawnInfo& spawnInfo);
	virtual ~Actor();
	void rotateOnPlane(const float rotateAngle) noexcept;
	float getRotateAngleDelta(const TickCount64& deltaTick) const noexcept;
public:
	TickCount64 getLocalTickCount(void) const noexcept;
	DirectX::XMFLOAT3 getPosition(void) const noexcept;
	// 위치, 회전 정보를 따로 관리하고 계산하면 renderItem의 _worldMatrix랑 차이가 생길수도 있다. [3/1/2021 qwerwy]
	// 한쪽을 전체 계산해서 덮어씌우는쪽으로 생각해봐야겠다.
	bool XM_CALLCONV checkCollision(const Actor* otherActor, DirectX::FXMVECTOR moveVector, CollisionInfo& outCollisionInfo) const noexcept;
	float getRadius(void) const noexcept;
	float getSizeX(void) const noexcept;
	float getSizeY(void) const noexcept;
	float getSizeZ(void) const noexcept;
	static bool XM_CALLCONV checkCollideBoxWithBox(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	static bool XM_CALLCONV checkCollideBoxWithSphere(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	
	// 아직 미구현 [3/17/2021 qwerwy]
	static bool XM_CALLCONV checkCollideSphereWithPolygon(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	static bool XM_CALLCONV checkCollideBoxWithPolygon(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	static bool XM_CALLCONV checkCollidePolygonWithPolygon(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	DirectX::XMFLOAT3 getMoveVector(const TickCount64& deltaTick) const noexcept;
	void setPosition(const DirectX::XMFLOAT3& toPosition) noexcept;
	bool isActionEnd() const noexcept;
	void updateActor(const TickCount64& deltaTick) noexcept;

	void updateActionChart(const TickCount64& deltaTick) noexcept;

	ActionState* getCurrentActionState(void) const noexcept { return _currentActionState; }

	void setActionState(const ActionState* nextState) noexcept;
	void updateObjectWorldMatrix() noexcept;

	void setRotateType(const RotateType rotateType, const float rotateAngle, const float speed) noexcept;
	void setAcceleration(const float acceleration, const float maxSpeed) noexcept;
	void setDeceleration(const float deceleration, const float minSpeed) noexcept;
	void setVerticalSpeed(const float speed) noexcept;
private:
	DirectX::XMFLOAT3 _position;
	//DirectX::XMFLOAT4 _rotationQuat;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;

	float _size;

	// 이동 조작 관련
	float _speed;
	float _verticalSpeed;

	//MoveType _moveType;
	float _moveDirectionOffset;
	float _acceleration;
	float _maxSpeed;
	float _minSpeed;

	// 캐릭터 회전
	RotateType _rotateType;
	float _rotateAngleLeft;
	float _rotateSpeed;

	// 중력
	int _gravityPointIndex;

	GameObject* _gameObject;

	const CharacterInfo* _characterInfo;

	ActionChart* _actionChart;
	ActionState* _currentActionState;
	
	TickCount64 _localTickCount;
};

class PlayerActor : public Actor
{
};

class NonPlayerActor : public Actor
{
};
