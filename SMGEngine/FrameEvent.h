#pragma once
#include "TypeCommon.h"
#include "TypeAction.h"
#include "TypeUI.h"
class Actor;
class ActionCondition;
class XMLReaderNode;

class FrameEvent
{
public:
	FrameEvent(const XMLReaderNode& node);
	virtual ~FrameEvent() = default;
	virtual void process(Actor& actor) const noexcept = 0;
	virtual FrameEventType getType() const noexcept = 0;
	TickCount64 getProcessTick(void) const noexcept { return _processTick; }
	bool checkConditions(Actor& actor) const noexcept;
	static std::unique_ptr<FrameEvent> loadXMLFrameEvent(const XMLReaderNode& node);
private:
	TickCount64 _processTick;
	std::vector<std::unique_ptr<ActionCondition>> _conditions;
};

class FrameEvent_Rotate : public FrameEvent
{
public:
	FrameEvent_Rotate(const XMLReaderNode& node);
	virtual ~FrameEvent_Rotate() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Rotate; }
private:
	RotateType _rotateType;
	float _offset;
	float _rotateSpeed;
};

class FrameEvent_Speed : public FrameEvent
{
public:
	FrameEvent_Speed(const XMLReaderNode& node);
	virtual ~FrameEvent_Speed() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Speed; }
private:
	float _targetSpeed;
	float _acceleration;
	float _moveDirectionOffset;
	MoveType _moveType;
};

class FrameEvent_Jump : public FrameEvent
{
public:
	FrameEvent_Jump(const XMLReaderNode& node);
	virtual ~FrameEvent_Jump() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Jump; }
private:
	float _speed;
	float _targetFallSpeed;
	float _acceleration;
};

class FrameEvent_Die : public FrameEvent
{
public:
	FrameEvent_Die(const XMLReaderNode& node);
	virtual ~FrameEvent_Die() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Die; }
private:
};

class FrameEvent_SpawnCharacter : public FrameEvent
{
public:
	FrameEvent_SpawnCharacter(const XMLReaderNode& node);
	virtual ~FrameEvent_SpawnCharacter() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::SpawnCharacter; }
private:
	CharacterKey _characterKey;
	DirectX::XMFLOAT3 _position;
	float _size;
	int _actionIndex;
};

class FrameEvent_Effect : public FrameEvent
{
public:
	FrameEvent_Effect(const XMLReaderNode& node);
	virtual ~FrameEvent_Effect() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Effect; }
private:
	std::string _effectName;
	DirectX::XMFLOAT3 _positionOffset;
	float _size;
};

class FrameEvent_SetVariable : public FrameEvent
{
public:
	FrameEvent_SetVariable(const XMLReaderNode& node);
	virtual ~FrameEvent_SetVariable() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::SetVariable; }
private:
	std::string _name;
	int _value;
};

class FrameEvent_FallSpeed : public FrameEvent
{
public:
	FrameEvent_FallSpeed(const XMLReaderNode& node);
	virtual ~FrameEvent_FallSpeed() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::FallSpeed; }
private:
	float _targetFallSpeed;
	float _acceleration;
};


class FrameEvent_SetPath : public FrameEvent
{
public:
	FrameEvent_SetPath(const XMLReaderNode& node);
	virtual ~FrameEvent_SetPath() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::SetPath; }
private:
	int _pathKey;
};

class FrameEvent_Gravity : public FrameEvent
{
public:
	FrameEvent_Gravity(const XMLReaderNode& node);
	virtual ~FrameEvent_Gravity() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Gravity; }
private:
	bool _on;
};

class FrameEvent_SetCamera : public FrameEvent
{
public:
	FrameEvent_SetCamera(const XMLReaderNode& node);
	virtual ~FrameEvent_SetCamera() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::SetCamera; }
private:
	int _cameraKey;
};

class FrameEvent_Collision : public FrameEvent
{
public:
	FrameEvent_Collision(const XMLReaderNode& node);
	virtual ~FrameEvent_Collision() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::Collision; }
private:
	bool _on;
};

class FrameEvent_TargetPosition : public FrameEvent
{
public:
	FrameEvent_TargetPosition(const XMLReaderNode& node);
	virtual ~FrameEvent_TargetPosition() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::TargetPosition; }
private:
	enum class TargetPositionType
	{
		PathStart,
		PointerPicked,
		
		Count,
	};
	TargetPositionType _type;
	int _key;
};

class FrameEvent_AnimationSpeed : public FrameEvent
{
public:
	FrameEvent_AnimationSpeed(const XMLReaderNode& node);
	virtual ~FrameEvent_AnimationSpeed() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::AnimationSpeed; }
private:
	float _speed;
};

class FrameEvent_AddStageVariable : public FrameEvent
{
public:
	FrameEvent_AddStageVariable(const XMLReaderNode& node);
	virtual ~FrameEvent_AddStageVariable() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::AddStageVariable; }
private:
	std::string _variableName;
	int _value;
};

class FrameEvent_EnableEffect : public FrameEvent
{
public:
	FrameEvent_EnableEffect(const XMLReaderNode& node);
	virtual ~FrameEvent_EnableEffect() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::EnableEffect; }
private:
	int _effectKey;
};

class FrameEvent_DisableEffect : public FrameEvent
{
public:
	FrameEvent_DisableEffect(const XMLReaderNode& node);
	virtual ~FrameEvent_DisableEffect() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::DisableEffect; }
private:
	int _effectKey;
};

class FrameEvent_SetEffectAlpha : public FrameEvent
{
public:
	FrameEvent_SetEffectAlpha(const XMLReaderNode& node);
	virtual ~FrameEvent_SetEffectAlpha() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::SetEffectAlpha; }
private:
	TickCount64 _blendTick;
	int _effectKey;
	float _alpha;
};

class FrameEvent_CallUIFunction : public FrameEvent
{
public:
	FrameEvent_CallUIFunction(const XMLReaderNode& node);
	virtual ~FrameEvent_CallUIFunction() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::CallUIFunction; }
private:
	std::string _uiGroupName;
	UIFunctionType _uiFunctionType;
};

class FrameEvent_ChangeMaterial : public FrameEvent
{
public:
	FrameEvent_ChangeMaterial(const XMLReaderNode& node);
	virtual ~FrameEvent_ChangeMaterial() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::ChangeMaterial; }
private:
	std::string _fileName;
	std::string _name;
	uint8_t _renderItemIndex;
};

class FrameEvent_StarShoot : public FrameEvent
{
public:
	FrameEvent_StarShoot(const XMLReaderNode& node);
	virtual ~FrameEvent_StarShoot() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::StarShoot; }
private:
};

class FrameEvent_MoveCameraImmediately : public FrameEvent
{
public:
	FrameEvent_MoveCameraImmediately(const XMLReaderNode& node);
	virtual ~FrameEvent_MoveCameraImmediately() = default;
	virtual void process(Actor& actor) const noexcept override;
	virtual FrameEventType getType() const noexcept override { return FrameEventType::MoveCameraImmediately; }
private:
};