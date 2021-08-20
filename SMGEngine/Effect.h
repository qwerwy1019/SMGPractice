#pragma once
#include "TypeGeometry.h"
#include "TypeCommon.h"
#include <deque>

class XMLReaderNode;
class FrameResource;
class MeshGeometry;

struct EffectInstance
{
	EffectInstance(const DirectX::XMFLOAT3& position, float size) noexcept;
	DirectX::XMFLOAT3 _position;
	TickCount64 _startTick;
	float _size;
};

class Effect
{
public:
	Effect(const XMLReaderNode& node);
	void addInstance(EffectInstance&& instance) noexcept;
	void updateEffectInstanceData(FrameResource* frameResource, uint32_t& instanceCount) const noexcept;
	void update(void) noexcept;
private:
	std::deque<EffectInstance> _instances;
	DirectX::XMFLOAT2 _size;
	uint32_t _totalFrame;
	TickCount64 _tickPerFrame;
	uint16_t _textureIndex;
	float _alphaDecrease;
	float _sizeDecrease;
	bool _isRepeat;
};

class EffectManager
{
public:
	EffectManager();
	void createEffectMeshGeometry(void);
	void loadXML(const std::string& fileName);
	void addEffectInstance(const std::string& effectName, EffectInstance&& instance) noexcept;
	void updateEffectInstanceData(FrameResource* frameResource) noexcept;
	void update(void) noexcept;
	bool hasEffect(const std::string& effectName) const noexcept;
	const MeshGeometry* getMeshGeometry(void) const noexcept;
	uint32_t getInstanceCount(void) const noexcept;
private:
	std::unordered_map<std::string, std::unique_ptr<Effect>> _effects;
	uint32_t _instanceCount;
	const MeshGeometry* _mesh;
};