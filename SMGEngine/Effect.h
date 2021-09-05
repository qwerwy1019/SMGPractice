#pragma once
#include "TypeGeometry.h"
#include "TypeCommon.h"
#include <deque>

class XMLReaderNode;
class FrameResource;
class MeshGeometry;

class EffectInstance
{
public:
	EffectInstance(const DirectX::XMFLOAT3& position, float size) noexcept;
	const DirectX::XMFLOAT3& getPosition() const noexcept { return _position; }
	const TickCount64& getStartTick() const noexcept { return _startTick; }
	void setStartTick(const TickCount64& startTick) noexcept { _startTick = startTick; }
	float getSize() const noexcept { return _size; }
protected:
	DirectX::XMFLOAT3 _position;
	TickCount64 _startTick;
	float _size;
};

class ConstantEffectInstance : public EffectInstance
{
public:
	ConstantEffectInstance(const DirectX::XMFLOAT3& position, float size) noexcept;
	ConstantEffectInstance(EffectInstance&& baseInstance) noexcept;
	float getAlpha(void) const noexcept;
	void setAlpha(float alpha, TickCount64 blendTick) noexcept;
	void setPosition(const DirectX::XMFLOAT3& position) noexcept;
private:
	float _alphaFrom;
	float _alphaTo;
	TickCount64 _alphaBlendTick;
	TickCount64 _alphaBlendStartTick;
};

class Effect
{
public:
	Effect(const XMLReaderNode& node);
	virtual ~Effect() = default;
	virtual void updateEffectInstanceData(FrameResource* frameResource,
										uint32_t& instanceCount) const noexcept = 0;
	bool getEffectInstanceData(const EffectInstance& instance, EffectInstanceData& instanceData) const noexcept;
protected:
	DirectX::XMFLOAT2 _size;
	uint32_t _totalFrame;
	TickCount64 _tickPerFrame;
	uint16_t _textureIndex;
};

class ConstantEffect : public Effect
{
public:
	ConstantEffect(const XMLReaderNode& node);
	virtual ~ConstantEffect() = default;
	ConstantEffectInstance* addInstance(EffectInstance&& instance) noexcept;
	void removeInstance(ConstantEffectInstance* instancePtr) noexcept;
	virtual void updateEffectInstanceData(FrameResource* frameResource,
										uint32_t& instanceCount) const noexcept override;
private:
	std::vector<std::unique_ptr<ConstantEffectInstance>> _instances;

};

class TemporaryEffect : public Effect
{
public:
	TemporaryEffect(const XMLReaderNode& node);
	virtual ~TemporaryEffect() = default;
	void addInstance(EffectInstance&& instance) noexcept;
	void update(void) noexcept;
	virtual void updateEffectInstanceData(FrameResource* frameResource,
										uint32_t& instanceCount) const noexcept override;

private:
	std::deque<EffectInstance> _instances;
	float _alphaDecrease;
	float _sizeDecrease;
	
};

class EffectManager
{
public:
	EffectManager();
	void update() noexcept;
	void createEffectMeshGeometry(void);
	void loadXML(const std::string& fileName);
	void loadXMLTemporaryEffects(const XMLReaderNode& node);
	void loadXMLConstantEffects(const XMLReaderNode& node);
	void addTemporaryEffectInstance(const std::string& effectName, EffectInstance&& instance) noexcept;
	ConstantEffectInstance* addConstantEffectInstance(const std::string& effectName, EffectInstance&& instance) noexcept;
	void removeConstantEffectInstance(const std::string& effectName, ConstantEffectInstance* instancePtr) noexcept;
	void updateEffectInstanceData(FrameResource* frameResource) noexcept;
	bool hasTemporaryEffect(const std::string& effectName) const noexcept;
	bool hasConstantEffect(const std::string& effectName) const noexcept;
	const MeshGeometry* getMeshGeometry(void) const noexcept;
	uint32_t getInstanceCount(void) const noexcept;
private:
	std::unordered_map<std::string, std::unique_ptr<TemporaryEffect>> _temporaryEffects;
	std::unordered_map<std::string, std::unique_ptr<ConstantEffect>> _constantEffects;

	uint32_t _instanceCount;
	const MeshGeometry* _mesh;
};