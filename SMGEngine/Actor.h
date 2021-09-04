#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

class CharacterInfo;
class ActionChart;
class ActionState;
class SpawnInfo;
struct GravityPoint;
class GameObject;
class Path;

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
	bool isQuaternionRotate(void) const noexcept;
	void rotateQuat(const DirectX::XMFLOAT4& rotationQuat) noexcept;
	DirectX::XMFLOAT4 getRotationQuat(const TickCount64& deltaTick) const noexcept;
	void setPath(int key) noexcept;
	const Path* getPath(void) const noexcept;
	bool isPathEnd(void) const noexcept;
	void setAnimationSpeed(float speed) noexcept;
	void setNextActionState(const std::string& actionStateName) noexcept;
public:
	TickCount64 getLocalTickCount(void) const noexcept;
	const DirectX::XMFLOAT3& getPosition(void) const noexcept;
	const DirectX::XMFLOAT3& getDirection(void) const noexcept;
	const DirectX::XMFLOAT3& getUpVector(void) const noexcept;

	static bool checkCollision(const Actor* lhs, const Actor* rhs) noexcept;
	float getRadius(void) const noexcept;
	float getSizeX(void) const noexcept;
	float getSizeY(void) const noexcept;
	float getSizeZ(void) const noexcept;
	float getHalfHeight(void) const noexcept;
	static bool checkCollideBoxWithBox(const Actor* lhs, const Actor* rhs) noexcept;
	static bool checkCollideBoxWithSphere(const Actor* lhs, const Actor* rhs) noexcept;
	static float getResistanceDistance(const Actor& selfActor, const Actor& targetActor) noexcept;

	// 아직 미구현 [3/17/2021 qwerwy]
	static bool checkCollideSphereWithPolygon(const Actor* lhs, const Actor* rhs) noexcept;
	static bool checkCollideBoxWithPolygon(const Actor* lhs, const Actor* rhs) noexcept;
	static bool checkCollidePolygonWithPolygon(const Actor* lhs, const Actor* rhs) noexcept;
	DirectX::XMFLOAT3 getMoveVector(const TickCount64& deltaTick) const noexcept;
	void setPosition(const DirectX::XMFLOAT3& toPosition) noexcept;
	void addMoveVector(const DirectX::XMFLOAT3& moveVector) noexcept;
	bool isActionEnd() const noexcept;
	void updateActor(const TickCount64& deltaTick) noexcept;

	void updateActionChart(const TickCount64& deltaTick) noexcept;

	const ActionState* getCurrentActionState(void) const noexcept { return _currentActionState; }
	const ActionChart* getActionChart(void) const noexcept { return _actionChart; }
	void setActionState(const std::string& actionStateName) noexcept;

	void updateObjectWorldMatrix() noexcept;

	void setRotateType(RotateType rotateType, float rotateAngleOffset, float speed) noexcept;
	void setAcceleration(float acceleration,
						float targetSpeed,
						float moveDirectionOffset,
						MoveType moveType) noexcept;
	void setVerticalSpeed(float speed) noexcept;
	void setTargetVerticalSpeed(float targetVerticalSpeed, float acceleration) noexcept;
	const CharacterInfo* getCharacterInfo(void) const noexcept;
	const GameObject* getGameObject(void) const noexcept;
	const DirectX::XMINT3& getSectorCoord(void) const noexcept;
	void processCollision(const Actor* collidingActor, CollisionCase collisionCase) noexcept;
	void setCulled(void) noexcept;
	void setActionChartVariable(const std::string& name, int value) noexcept;
	int getActionChartVariable(const std::string& name) const noexcept;
	int getActionIndex(void) const noexcept;
	void setGravityOn(bool on) noexcept;
	void setCollisionOn(bool on) noexcept;
	bool isCollisionOn(void) const noexcept;
	void setTargetPosition(const DirectX::XMFLOAT3& position) noexcept;
	const DirectX::XMFLOAT3& getTargetPosition(void) const noexcept;
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
	float _verticalAcceleration;
	float _targetVerticalSpeed;

	DirectX::XMFLOAT3 _additionalMoveVector;
	DirectX::XMINT3 _sectorCoord;

	const Path* _path;
	TickCount64 _pathTime;

	DirectX::XMFLOAT3 _targetPosition;

	// 캐릭터 회전
	RotateType _rotateType;
	float _rotateAngleOffset;
	float _rotateSpeed;

	const GravityPoint* _gravityPoint;
	bool _isGravityOn;

	GameObject* _gameObject;

	const CharacterInfo* _characterInfo;

	const ActionChart* _actionChart;
	const ActionState* _currentActionState;
	std::string _nextActionStateName;
	std::unordered_map<std::string, int> _actionChartVariables;
	int _actionIndex;
	
	TickCount64 _localTickCount;
	bool _isCollisionOn;

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
