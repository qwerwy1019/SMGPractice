#include "stdafx.h"
#include "Effect.h"
#include "SMGFramework.h"
#include "D3DApp.h"
#include "MathHelper.h"
#include "FileHelper.h"
#include "MeshGeometry.h"

Effect::Effect(const XMLReaderNode& node)
{
	std::string textureName;
	node.loadAttribute("DiffuseTexture", textureName);
	_textureIndex = SMGFramework::getD3DApp()->loadTexture(textureName);
	node.loadAttribute("TotalFrame", _totalFrame);
	node.loadAttribute("TickPerFrame", _tickPerFrame);
	node.loadAttribute("Size", _size);

	if (_tickPerFrame * _totalFrame == 0)
	{
		ThrowErrCode(ErrCode::InvalidXmlData, "Effect 재생 시간이 0입니다.");
	}
}

bool Effect::getEffectInstanceData(const EffectInstance& instance, EffectInstanceData& instanceData) const noexcept
{
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();

	float radius = std::sqrt(_size.x * _size.x + _size.y + _size.y) * instance.getSize();
	XMMATRIX identityMat = XMLoadFloat4x4(&MathHelper::Identity4x4);

	instanceData._totalFrame = _totalFrame;
	instanceData._textureIndex = _textureIndex;
	TickCount64 effectTick = _totalFrame * _tickPerFrame;

	BoundingBox box(instance.getPosition(), XMFLOAT3(radius, radius, radius));
	if (SMGFramework::getD3DApp()->checkCulled(box, identityMat))
	{
		return false;
	}

	instanceData._position = instance.getPosition();

	TickCount64 effectLocalTick = (currentTick - instance.getStartTick()) % effectTick;
	instanceData._frame = effectLocalTick / _tickPerFrame;

	instanceData._size = XMFLOAT2(_size.x * instance.getSize(), _size.y * instance.getSize());

	instanceData._alpha = 1;
	return true;
}

TemporaryEffect::TemporaryEffect(const XMLReaderNode& node)
	: Effect(node)
{
	node.loadAttribute("AlphaDecrease", _alphaDecrease);
	node.loadAttribute("SizeDecrease", _sizeDecrease);
}

void TemporaryEffect::addInstance(EffectInstance&& instance) noexcept
{
	instance.setStartTick(SMGFramework::Get().getTimer().getCurrentTickCount());
	_instances.emplace_back(std::move(instance));
}

void TemporaryEffect::update(void) noexcept
{
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	currentTick -= _tickPerFrame * _totalFrame;
	while (!_instances.empty() && _instances.front().getStartTick() <= currentTick)
	{
		_instances.pop_front();
	}
}

EffectInstance::EffectInstance(const DirectX::XMFLOAT3& position, float size) noexcept
	: _position(position)
	, _startTick(0)
	, _size(size)
{

}

EffectManager::EffectManager()
	: _instanceCount(0)
	, _mesh(nullptr)
{
}

void EffectManager::update() noexcept
{
	for (const auto& t : _temporaryEffects)
	{
		t.second->update();
	}
}

void EffectManager::createEffectMeshGeometry(void)
{
	GeneratedMeshData pointMeshData;
	pointMeshData._vertices.push_back(Vertex(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT2(0, 0)));
	pointMeshData._indices.push_back(0);

	_mesh = SMGFramework::getD3DApp()->createMeshGeometry("onePoint", pointMeshData);
}

void EffectManager::loadXML(const std::string& fileName)
{
	const std::string filePath = "../Resources/XmlFiles/Effect/" + fileName + ".xml";

	XMLReader xmlEffect;
	xmlEffect.loadXMLFile(filePath);
	const auto& childNodes = xmlEffect.getRootNode().getChildNodesWithName();

	auto it = childNodes.find("ConstantEffects");
	loadXMLConstantEffects(it->second);

	it = childNodes.find("TemporaryEffects");
	loadXMLTemporaryEffects(it->second);
}

void EffectManager::loadXMLTemporaryEffects(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_temporaryEffects.reserve(childNodes.size());
	for (const auto& childNode : childNodes)
	{
		std::string name;
		childNode.loadAttribute("Name", name);
		auto it = _temporaryEffects.emplace(name, std::make_unique<TemporaryEffect>(childNode));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, name);
		}
	}
}

void EffectManager::loadXMLConstantEffects(const XMLReaderNode& node)
{
	const auto& childNodes = node.getChildNodes();
	_constantEffects.reserve(childNodes.size());
	for (const auto& childNode : childNodes)
	{
		std::string name;
		childNode.loadAttribute("Name", name);
		auto it = _constantEffects.emplace(name, std::make_unique<ConstantEffect>(childNode));
		if (it.second == false)
		{
			ThrowErrCode(ErrCode::KeyDuplicated, name);
		}
	}
}

void EffectManager::addTemporaryEffectInstance(const std::string& effectName, EffectInstance&& instance) noexcept
{
	auto it = _temporaryEffects.find(effectName);
	if (it == _temporaryEffects.end())
	{
		check(false, effectName + "가 이펙트 목록에 없습니다.");
		return;
	}

	it->second->addInstance(std::move(instance));
}

ConstantEffectInstance* EffectManager::addConstantEffectInstance(const std::string& effectName, EffectInstance&& instance) noexcept
{
	auto it = _constantEffects.find(effectName);
	if (it == _constantEffects.end())
	{
		check(false, effectName + "가 이펙트 목록에 없습니다.");
		return nullptr;
	}

	return it->second->addInstance(std::move(instance));
}

void EffectManager::removeConstantEffectInstance(const std::string& effectName, ConstantEffectInstance*instancePtr) noexcept
{
	auto it = _constantEffects.find(effectName);
	if (it == _constantEffects.end())
	{
		check(false, effectName + "가 이펙트 목록에 없습니다.");
		return;
	}
	
	it->second->removeInstance(instancePtr);
}

void EffectManager::updateEffectInstanceData(FrameResource* frameResource) noexcept
{
	check(frameResource != nullptr);

	_instanceCount = 0;

	for (const auto& c : _constantEffects)
	{
		c.second->updateEffectInstanceData(frameResource, _instanceCount);
	}

	for (const auto& t : _temporaryEffects)
	{
		t.second->updateEffectInstanceData(frameResource, _instanceCount);
	}
}

bool EffectManager::hasTemporaryEffect(const std::string& effectName) const noexcept
{
	auto it = _temporaryEffects.find(effectName);
	if (it == _temporaryEffects.end())
	{
		return false;
	}
	return true;
}

bool EffectManager::hasConstantEffect(const std::string& effectName) const noexcept
{
	auto it = _constantEffects.find(effectName);
	if (it == _constantEffects.end())
	{
		return false;
	}
	return true;
}

const MeshGeometry* EffectManager::getMeshGeometry(void) const noexcept
{
	return _mesh;
}

uint32_t EffectManager::getInstanceCount(void) const noexcept
{
	return _instanceCount;
}

ConstantEffect::ConstantEffect(const XMLReaderNode& node)
	: Effect(node)
{
	
}

ConstantEffectInstance* ConstantEffect::addInstance(EffectInstance&& instance) noexcept
{
	instance.setStartTick(SMGFramework::Get().getTimer().getCurrentTickCount());
	_instances.emplace_back(std::make_unique<ConstantEffectInstance>(std::move(instance)));
	return _instances.back().get();
}

void ConstantEffect::removeInstance(ConstantEffectInstance* instancePtr) noexcept
{
	for (int i = 0; i < _instances.size(); ++i)
	{
		if (_instances[i].get() == instancePtr)
		{
			_instances[i] = std::move(_instances.back());
			_instances.pop_back();
			return;
		}
	}
	check(false);
}

void ConstantEffect::updateEffectInstanceData(FrameResource* frameResource, uint32_t& instanceCount) const noexcept
{
	check(frameResource != nullptr);
	if (_instances.empty())
	{
		return;
	}
	for (const auto& instance : _instances)
	{
		float alpha = instance->getAlpha();
		if (MathHelper::equal(alpha, 0))
		{
			continue;
		}
		EffectInstanceData instanceData;
		if (!Effect::getEffectInstanceData(*instance, instanceData))
		{
			continue;
		}
		if (EFFECT_INSTANCE_MAX <= instanceCount)
		{
			check(false);
			continue;
		}
		instanceData._alpha = alpha;
		frameResource->setEffectBuffer(instanceCount++, instanceData);
	}
}

void TemporaryEffect::updateEffectInstanceData(FrameResource* frameResource, uint32_t& instanceCount) const noexcept
{
	check(frameResource != nullptr);
	if (_instances.empty())
	{
		return;
	}
	for (const auto& instance : _instances)
	{
		EffectInstanceData instanceData;
		if (!Effect::getEffectInstanceData(instance, instanceData))
		{
			continue;
		}
		if (EFFECT_INSTANCE_MAX <= instanceCount)
		{
			check(false);
			continue;
		}
		TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
		TickCount64 effectTick = _totalFrame * _tickPerFrame;
		TickCount64 effectLocalTick = (currentTick - instance.getStartTick()) % effectTick;

		float sizeDecreaseRate = _sizeDecrease * (static_cast<float>(effectLocalTick) / effectTick);
		float sizeRate = (1.f - sizeDecreaseRate);

		instanceData._size.x *= sizeRate;
		instanceData._size.y *= sizeRate;

		float alphaDecreaseRate = _alphaDecrease * (static_cast<float>(effectLocalTick) / effectTick);
		
		instanceData._alpha -= alphaDecreaseRate;

		frameResource->setEffectBuffer(instanceCount++, instanceData);
	}
}

ConstantEffectInstance::ConstantEffectInstance(const DirectX::XMFLOAT3& position, float size) noexcept
	: EffectInstance(position, size)
	, _alphaFrom(0.f)
	, _alphaTo(0.f)
	, _alphaBlendTick(0)
	, _alphaBlendStartTick(0)
{

}

ConstantEffectInstance::ConstantEffectInstance(EffectInstance&& baseInstance) noexcept
	: EffectInstance(baseInstance)
	, _alphaFrom(0.f)
	, _alphaTo(0.f)
	, _alphaBlendTick(0)
	, _alphaBlendStartTick(0)
{

}

float ConstantEffectInstance::getAlpha(void) const noexcept
{
	TickCount64 endTick = _alphaBlendStartTick + _alphaBlendTick;
	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	if (endTick < currentTick)
	{
		return _alphaTo;
	}
	check(_alphaBlendStartTick <= currentTick);
	float lerpPercent = static_cast<float>(currentTick - _alphaBlendStartTick) / _alphaBlendTick;
	
	return _alphaFrom * (1 - lerpPercent) + _alphaTo * lerpPercent;
}

void ConstantEffectInstance::setAlpha(float alpha, TickCount64 blendTick) noexcept
{
	float currentAlpha = getAlpha();

	TickCount64 currentTick = SMGFramework::Get().getTimer().getCurrentTickCount();
	_alphaBlendStartTick = currentTick;
	_alphaBlendTick = blendTick;
	_alphaFrom = currentAlpha;
	_alphaTo = alpha;
}
