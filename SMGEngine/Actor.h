#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

class CharacterInfo;
class ActionChart;
class ActionState;
struct SpawnInfo;
struct GravityPoint;
class GameObject;

class Actor
{
public:
	Actor(const SpawnInfo& spawnInfo);
	virtual ~Actor();
	void rotateOnPlane(const float rotateAngle) noexcept;
	void rotateUpVector(const DirectX::XMFLOAT3& toUpVector) noexcept;
	float getRotateAngleDelta(const TickCount64& deltaTick) const noexcept;
	void applyGravityRotation(const TickCount64& deltaTick, const DirectX::XMFLOAT3& gravityDirection) noexcept;
	bool getGravityDirection(DirectX::XMFLOAT3& gravityDirection) const noexcept;
	float getSpeed() const noexcept;
	float getVerticalSpeed() const noexcept;
	void setActorOnGround(bool onGround) noexcept;
	bool isOnGround(void) const noexcept;
public:
	TickCount64 getLocalTickCount(void) const noexcept;
	const DirectX::XMFLOAT3& getPosition(void) const noexcept;
	const DirectX::XMFLOAT3& getDirection(void) const noexcept;
	const DirectX::XMFLOAT3& getUpVector(void) const noexcept;
	// 위치, 회전 정보를 따로 관리하고 계산하면 renderItem의 _worldMatrix랑 차이가 생길수도 있다. [3/1/2021 qwerwy]
	// 한쪽을 전체 계산해서 덮어씌우는쪽으로 생각해봐야겠다.
	static bool XM_CALLCONV checkCollision(const Actor* lhs, const Actor* rhs) noexcept;
	float getRadius(void) const noexcept;
	float getSizeX(void) const noexcept;
	float getSizeY(void) const noexcept;
	float getSizeZ(void) const noexcept;
	static bool XM_CALLCONV checkCollideBoxWithBox(const Actor* lhs, const Actor* rhs) noexcept;
	static bool XM_CALLCONV checkCollideBoxWithSphere(const Actor* lhs, const Actor* rhs) noexcept;
	
	// 아직 미구현 [3/17/2021 qwerwy]
	static bool XM_CALLCONV checkCollideSphereWithPolygon(const Actor* lhs, const Actor* rhs) noexcept;
	static bool XM_CALLCONV checkCollideBoxWithPolygon(const Actor* lhs, const Actor* rhs) noexcept;
	static bool XM_CALLCONV checkCollidePolygonWithPolygon(const Actor* lhs, const Actor* rhs) noexcept;
	DirectX::XMFLOAT3 getMoveVector(const TickCount64& deltaTick) const noexcept;
	void setPosition(const DirectX::XMFLOAT3& toPosition) noexcept;
	bool isActionEnd() const noexcept;
	void updateActor(const TickCount64& deltaTick) noexcept;

	void updateActionChart(const TickCount64& deltaTick) noexcept;

	const ActionState* getCurrentActionState(void) const noexcept { return _currentActionState; }

	void setActionState(const ActionState* nextState) noexcept;
	void updateObjectWorldMatrix() noexcept;

	void setRotateType(const RotateType rotateType, const float rotateAngleOffset, const float speed) noexcept;
	void setAcceleration(const float acceleration, const float targetSpeed, MoveType moveType) noexcept;
	void setVerticalSpeed(const float speed) noexcept;
	void setTargetVerticalSpeed(const float targetVerticalSpeed) noexcept;
	const CharacterInfo* getCharacterInfo(void) const noexcept;
	const GameObject* getGameObject(void) const noexcept;
private:
	DirectX::XMFLOAT3 _position;
	//DirectX::XMFLOAT4 _rotationQuat;
	DirectX::XMFLOAT3 _direction;
	DirectX::XMFLOAT3 _upVector;

	float _size;

	// 이동 조작 관련
	float _speed;
	float _verticalSpeed;

	MoveType _moveType;
	float _moveDirectionOffset;
	float _acceleration;
	float _targetSpeed;
	float _targetVerticalSpeed;

	// 캐릭터 회전
	RotateType _rotateType;
	float _rotateAngleOffset;
	float _rotateSpeed;

	const GravityPoint* _gravityPoint;

	GameObject* _gameObject;

	const CharacterInfo* _characterInfo;

	const ActionChart* _actionChart;
	const ActionState* _currentActionState;
	
	TickCount64 _localTickCount;

	bool _onGround;
};

class PlayerActor : public Actor
{
public:
	PlayerActor(const SpawnInfo& spawnInfo);
	bool isMoving(void) const noexcept { return _isMoving; }
private:
	bool _isMoving;
};

class NonPlayerActor : public Actor
{
};
