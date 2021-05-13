#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"

struct RenderItem;
class CharacterInfo;
class ActionChart;
class ActionState;

struct CollisionInfo
{
	float _collisionTime; // 0 ~ 1
	DirectX::XMFLOAT3 _collisionPoint; // {-1 ~ 1, -1 ~ 1, -1 ~ 1}
};
class CharacterInfo
{
public:
	CollisionShape getCollisionShape(void) const noexcept { return _collisionShape; }
	CollisionType getCollisionType(void) const noexcept { return _collisionType; }
	float getRadius(void) const noexcept { return _radius; }
	float getSizeX(void) const noexcept { return _radius; }
	float getSizeY(void) const noexcept { return _radius; }
	float getSizeZ(void) const noexcept { return _radius; }
private:
	CollisionType _collisionType;
	std::string _actionChartName;
	CollisionShape _collisionShape;

	float _radius;

	// box type�϶��� ���Ǵ� �� [3/10/2021 qwerwy]
	float _sizeX;
	float _sizeY;
	float _sizeZ;

};

class Actor
{
public:
	Actor();
	~Actor();
	void rotateOnPlane(const float rotateAngle) noexcept;
	float getRotateAngleDelta(const TickCount64& deltaTick) const noexcept;
public:
	TickCount64 getLocalTickCount(void) const noexcept;
	DirectX::XMFLOAT3 getPosition(void) const noexcept;
	// ��ġ, ȸ�� ������ ���� �����ϰ� ����ϸ� renderItem�� _worldMatrix�� ���̰� ������� �ִ�. [3/1/2021 qwerwy]
	// ������ ��ü ����ؼ� ������������ �����غ��߰ڴ�.
	bool XM_CALLCONV checkCollision(const Actor* otherActor, DirectX::FXMVECTOR moveVector, CollisionInfo& outCollisionInfo) const noexcept;
	float getRadius(void) const noexcept { return _size * _characterInfo->getRadius(); }
	float getSizeX(void) const noexcept { return _size * _characterInfo->getSizeX(); }
	float getSizeY(void) const noexcept { return _size * _characterInfo->getSizeY(); }
	float getSizeZ(void) const noexcept { return _size * _characterInfo->getSizeZ(); }
	static bool XM_CALLCONV checkCollideBoxWithBox(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	static bool XM_CALLCONV checkCollideBoxWithSphere(const Actor* lhs, const Actor* rhs, DirectX::FXMVECTOR lhsMoveVector, CollisionInfo& outCollisionInfo) noexcept;
	
	// ���� �̱��� [3/17/2021 qwerwy]
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
	void updateRenderWorldMatrix() noexcept;

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

	// �̵� ���� ����
	float _speed;
	float _verticalSpeed;

	//MoveType _moveType;
	float _moveDirectionOffset;
	float _acceleration;
	float _maxSpeed;
	float _minSpeed;

	// ĳ���� ȸ��
	RotateType _rotateType;
	float _rotateAngleLeft;
	float _rotateSpeed;

	// �߷�
	int _gravityPointIndex;

	RenderItem* _renderItem;

	CharacterInfo* _characterInfo;

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